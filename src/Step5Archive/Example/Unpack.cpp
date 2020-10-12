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

void Unpack(String source, String destination)
{
	FileIn archive_stream(source);
	Step5Archive archive(archive_stream);
	
	if (archive.IsError()) {
		Cout() << Format("Invalid file: %s\n", source);
		return;
	}

	Cout() << "Archive: " << source << "\n";
	if (destination != "") {
		Cout() << "Destination: " << destination << "\n";
	}
	Cout() << "\n";
	
	Cout() << "   Date      Time   Attr    Size    Compressed  Name \n";
	Cout() << "------------------- ---- ---------- ----------  --------------------\n";
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
	Cout() << "------------------- ---- ---------- ----------  --------------------\n";
	Cout() << Format("                         %10>d %10>d  %i files\n\n", utotal, ctotal, archive.GetCount());
}

