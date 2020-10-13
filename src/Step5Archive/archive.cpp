#include "Step5Archive.h"

#define LLOG(x) RLOG(x)

namespace Upp {


/*
	Archive Reader
*/

void Step5Archive::ReadDir()
{
	error = true;
	files.Clear();
	offset=0;
	
	if (archive->GetSize() < sizeof_header) {
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
		archive->Get(f.path, sizeof_path);   f.path[sizeof_path-1] = '\0';
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
	if (r != (int)f.usize) {
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

//Step5Archive::~Step5Archive()
//{
//	
//}




/*
	Archive Writer
*/

void WriteStep5Archive::UpdateHeader()
{
	error = true;
	header.nfiles = files.GetCount();
	header.asize = archive->GetSize();
	
	archive->Seek(0);
	archive->Put(header.id, 5);
	archive->Put0(2);
	archive->Put16le(header.nfiles);
	archive->Put32le(header.asize);
	archive->SeekCur(41);
	ASSERT(archive->GetPos() == sizeof_header);
	
	if (archive->IsError()) {
		LLOG("Error: UpdateHeader: Stream error");
		return;
	}
	if (archive->GetPos() != sizeof_header){
		LLOG("Error: UpdateHeader: Internal positioning");
		return;
	}
	
	error = false;
}

void WriteStep5Archive::UpdateFileRecord()
{
	error = true;

	File& f = files[current];
	//LOG(Format("UpdateFileRecord %i usize= "));
	archive->Seek(sizeof_header + ((header.nfiles-1) * sizeof_filerecord));
	archive->Put(f.path, sizeof_path);
	archive->Put16(f.date);
	archive->Put16(f.time);
	archive->Put16(f.attr);
	archive->Put((int)f.compr);
	archive->Put32(f.usize);
	archive->Put32(f.csize);
	archive->Put0(8);
	archive->Put32(f.offset);
	
	error = false;
}

void WriteStep5Archive::InsertFileRecord()
{
	int new_offset;
	
	error = true;
	String buf;
	archive->Seek(offset);
	buf = archive->Get(archive->GetSize() - offset);
	ASSERT(buf.GetLength() == archive->GetSize()-offset);
	archive->Seek(offset);
	archive->Put0(sizeof_filerecord);
	new_offset = archive->GetPos();
	archive->Put(buf);
	
	if (new_offset != (sizeof_header + (header.nfiles * sizeof_filerecord)) ) {
		LLOG("Invalid offset position after inserting filerecord");
		NEVER();
		return;
	}
	offset = new_offset;
	files[current].offset = archive->GetSize() - offset;

	UpdateHeader();
	UpdateFileRecord();
}


word WriteStep5Archive::GetDosDate(Time time)
{
	word attr;
	attr = (0x7f & (time.year - 1980)) << 9; // =   ((dosdate >> 9) & 0x7f) + 1980;
	attr |= (0x0f & time.month) << 5; // =  ((dosdate >> 5) & 0x0f);
	attr |= (0x1f & time.day); // =    ((dosdate)      & 0x1f);
	return attr;
}

word WriteStep5Archive::GetDosTime(Time time)
{
	word attr;
	attr = (0x1f & time.hour) << 11; // =   ((dostime >> 11)& 0x1f);
	attr |= (0x3f & time.minute) << 5; // = ((dostime >> 5) & 0x3f);
	attr |= (0x3e & time.second); // = ((dostime << 1) & 0x3e);
	return attr;
}

void WriteStep5Archive::WriteFile(Stream& in, String path, Time tm, dword attr, bool compressed)
{
	error = true;

	int i = sizeof_path-1;
	if (path.GetLength() > i) {
		LLOG(Format("Path length too long (%i>%i)", path.GetLength(), i));
		return;
	}
	
	File& f = files.Add();
	current = files.GetCount()-1;
	header.nfiles += 1;
	
	strcpy(f.path, path.Begin());
	f.path[sizeof_path-1] = 0;
	f.date = GetDosDate(tm);
	f.time = GetDosTime(tm);
	f.attr = attr;
	f.compr = compressed;
	f.usize = in.GetSize();
	f.csize = 0;
	f.offset = 0;
	InsertFileRecord();
	if (error || archive->IsError()) return;
	
	archive->SeekEnd();
	int r = 0;
	if (!compressed) {
		r = CopyStream(*archive, in);
	}
	else {
		int d = archive->GetSize();
		EncodeStream(in, *archive);
		r = archive->GetSize() - d;
	}
	files[current].csize = r;
	
	UpdateHeader();
	UpdateFileRecord();
}

void WriteStep5Archive::Create(Stream& out)
{
	error = true;
	processing = false;
	files.Clear();
	
	archive = &out;

	if (archive->GetSize() > 0) {
		// read existing header
		ReadDir();
		if (error) {
			LLOG("Non-empty stream is not a valid archive");
			return;
		}
	}
	else {
		// write initial header
		archive->Seek(0);
		archive->Put0(sizeof_header);
		offset=sizeof_header;
		Header nh;
		header = nh;
		UpdateHeader();
		if (error) {
			LLOG("Error initializing header");
			return;
		}
	}
	
}

WriteStep5Archive::WriteStep5Archive(Stream& out)
{
	Create (out);
}

WriteStep5Archive::WriteStep5Archive()
{
	error = true;
	processing = false;
	files.Clear();
	archive = NULL;
}



}