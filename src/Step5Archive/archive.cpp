#include "Step5Archive.h"

#define LLOG(x) RLOG(x)

namespace Upp {


void Step5Archive::ReadDir()
{
	error = true;
	files.Clear();
	
	if (archive->GetSize() <= sizeof_header) {
		LLOG("Error: File too small for header");
		return;
	}
	// Reading header
	archive->ClearError();
	archive->Seek(0);
	archive->Get(header.id, 6);
	archive->Get8();
	header.nfiles = archive->Get16le();
	header.asize = archive->Get32le();
	archive->SeekCur(41);
	ASSERT(archive->GetPos() == sizeof_header);
	
	if (archive->IsError()) {
		LLOG("Error: Header: Stream error");
		return;
	}
	if (archive->GetPos() != sizeof_header){
		LLOG("Error: Header: Internal positioning");
		return;
	}
	if (strcmp(header.id, "STEP5") != 0) {
		LLOG("Error: Header: ID not found");
		return;
	}
	if ( (header.nfiles < 0) || (header.nfiles > 40) ) {
		LLOG(Format("Error: Header: number of files (%i) out of range [0..40]", header.nfiles));
		return;
	}
	if (header.asize != archive->GetSize()) {
		LLOG(Format("Error: Header: size (%i) does not match actual file size (%i)", (int)header.asize, archive->GetSize()));
		return;
	}
	
	// setting start of data block (end of header+directory)
	offset = sizeof_header + (header.nfiles * sizeof_filerecord);
	if (archive->GetSize() < offset) {
		LLOG("Error: can't read records (file too small)");
		return;
	}

	// Reading directory
	for (int i=0; i<header.nfiles; i++) {
		File &f = files.Add();
		archive->Get(f.path, 260); f.path[259] = '\0';
		f.date = archive->Get16();
		f.time = archive->Get16();
		f.attr = archive->Get16();
		f.compr = archive->Get8();
		f.usize = archive->Get32();
		f.csize = archive->Get32();
		archive->SeekCur(8);
		f.offset = archive->Get32();
	}
	ASSERT(archive->GetPos() == offset);
	ASSERT(files.GetCount() == header.nfiles);
	
	// Running a few checks
	int total=0;
	for (int i=0; i<files.GetCount(); i++) {
		for(int j=0; j<files.GetCount(); j++) {
			if (i != j) {
				if (files[i].offset >= (files[j].offset+files[j].csize) ) continue;
				if (files[j].offset >= (files[i].offset+files[i].csize) ) continue;
				LLOG(Format("Error: Overlap detected of records [%i](%i..%i) and [%i](%i..%i)",
									i, (int)files[i].offset, (int)(files[i].offset+files[i].csize-1),
									j, (int)files[j].offset, (int)(files[j].offset+files[j].csize-1)) );
				return;
			}
		}
		total += files[i].csize;
	}
	if (offset+total != archive->GetSize()) {
		LLOG(Format("Error: Calculated file size (%i) does not match actual file size (%i)", offset+total, archive->GetSize()));
		return;
	}
	
	//ok seems a valid file
	error = false;
}

Time Step5Archive::GetDosDateTime(word dosdate, word dostime)
{
	Time time;
	time.year =   ((dosdate >> 9) & 0x7f) + 1980;
	time.month =  ((dosdate >> 5) & 0x0f);
	time.day =    ((dosdate)      & 0x1f);
	time.hour =   ((dostime >> 11)& 0x1f);
	time.minute = ((dostime >> 5) & 0x3f);
	time.second = ((dostime << 1) & 0x3e);
	return time;
}

bool Step5Archive::ReadFile(Stream& out, Gate<int, int> progress)
{
	if(error)
		return false;
	if(IsFolder()) {
		current++;
		return true;
	}
	error = true;
	if(current >= files.GetCount())
		return false;
	const File& f = files[current];
	
	archive->Seek(offset+f.offset);
	
	int r = 0;
	if (f.compr ==0) {			/* stored */
		r = CopyStream(out, *archive, f.csize, AsGate64(progress));
	}
	else if (f.compr == 1) {	/* compressed */
		int d = out.GetSize();
		DecodeStream(*archive, out, f.csize);
		r = out.GetSize() -d;
	}
	else {
		LLOG(Format("Unknown compression type (%i) on file %i (%s)", (int)f.compr, current, f.path));
		return false;
	}
	if (r != f.usize) {
		LLOG(Format("Size mismatch restoring file - got %i bytes (expecting %i)", r, (int)f.usize));
		return false;
	}
	
	error = false;
	return true;
}

String Step5Archive::ReadFile(Gate<int, int> progress)
{
	StringStream ss;
	return ReadFile(ss, progress) ? ss.GetResult() : String::GetVoid();
}

String Step5Archive::ReadFile(const char *path, Gate<int, int> progress)
{
    for(int i = 0; i < files.GetCount(); i++)
        if(files[i].path == path) {
            Seek(i);
            return ReadFile(progress);
        }

    return String::GetVoid();
}

void Step5Archive::Create(Stream& in)
{
	archive = &in;
	ReadDir();
}

Step5Archive::Step5Archive(Stream& in)
{
	Create(in);
}

Step5Archive::Step5Archive()
{
	error = true;
	archive = NULL;
}

Step5Archive::~Step5Archive()
{
	
}



}