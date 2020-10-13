#include "Example.h"

String DosAttributeCharacters(word attr) {
	String s;
	s << (((attr & FILE_ATTRIBUTE::ARCHIVE)>0)	? "A" : ".");
//	s << (((attr & FILE_ATTRIBUTE::DIRECTORY)>0)? "D" : ".");
	s << (((attr & FILE_ATTRIBUTE::SYSTEM)>0)	? "S" : ".");
	s << (((attr & FILE_ATTRIBUTE::HIDDEN)>0)	? "H" : ".");
	s << (((attr & FILE_ATTRIBUTE::READONLY)>0) ? "R" : ".");
	return s;
}

void Unpack(String source, String destination, ArchOpts opts)
{
	FileIn archive_stream(source);
	Step5Archive archive(archive_stream);
	
	if (archive.IsError()) {
		Cout() << Format("Not an archive: %s\n", source);
		return;
	}
	
	if (opts==list) Cout() << "Listing";
	else if (opts==test) Cout() << "Testing";
	else Cout() << "Extracting";
	
	Cout() << " archive: " << source << "\n";
	if ( (opts!=list) && (destination != "") ) {
		Cout() << "Destination: " << NormalizePath(destination) << "\n";
	}
	Cout() << "\n";

	// list archive contents
	if (opts==list) {
		Cout() << "   Date      Time   Attr    Size    Compressed  Name \n";
		Cout() << "------------------- ---- ---------- ----------  -------------------- - -  -   -\n";
		int ctotal=0, utotal=0;
		for (int i=0; i<archive.GetCount(); i++) {
			archive.Seek(i);
			Cout() << Format("%` %s %10>d %10>d  %s\n",
								archive.GetTime(),
								DosAttributeCharacters(archive.GetAttr()),
								archive.GetLength(),
								archive.GetCLength(),
								archive.GetPath()
							);
			ctotal += archive.GetCLength();
			utotal += archive.GetLength();
		}
		Cout() << "------------------- ---- ---------- ----------  -------------------- - -  -   -\n";
		Cout() << Format("                         %10>d %10>d  %i files\n\n", utotal, ctotal, archive.GetCount());
		return;
	}

	
	String fn;
	for (int i=0; i<archive.GetCount(); i++) {
		archive.Seek(i);

		fn = archive.GetPath();
		if (destination != "") {
			fn = destination + GetFileName(NativePath(fn));
		}
		Cout() << fn << " ";
		if (FileExists(fn)) {
			int a = AskConfirmation("* File already exists - overwrite?");
			if (a < 0) ExitError(20, "user abort");
			if (a != 1) continue;
		}
		
		if (opts == test) {
			String outs = archive.ReadFile();
		}
		else {
			FileOut out(fn);
			if (out.IsError()) ExitError(20, "Creating outstream");
			archive.ReadFile(out);
		}

		if (archive.IsError()) {
			ExitError(20, "Writing outstream");
		}
		
		/* TODO: write time and attributes */
		else Cout() <<" Ok. \n";
		
	}

}

