#include "Example.h"

String DosAttributeCharacters(word attr) {
	String s;
	s << (((attr & file_attribute::archive)>0)	? "A" : ".");
//	s << (((attr & file_attribute::directory)>0)? "D" : ".");
	s << (((attr & file_attribute::system)>0)	? "S" : ".");
	s << (((attr & file_attribute::hidden)>0)	? "H" : ".");
	s << (((attr & file_attribute::readonly)>0) ? "R" : ".");
	return s;
}

void Unpack(String source, String destination, ArchOpts opts)
{
	Confirmation confirm;
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
			confirm = AskConfirmation("* File already exists -- overwrite?");
			if (confirm < 0) ExitError(255, "user abort");
			if (confirm != 1) continue;
		}
		
		if (opts == test) {
			String outs = archive.ReadFile();
		}
		else {
			String basedir = GetFileDirectory(fn);
			if (!FileExists(basedir)) {
				switch (AskConfirmation(Format("* Directory %s does not exist -- create?", basedir)))
				{
					case no : continue;
					case yes :
						if (!RealizeDirectory(fn)) ExitError(20, "Cannot create directory");
						break;
					default : ExitError (255, "user abort");
				}
			}
			
			FileOut out(fn);
			if (out.IsError()) ExitError(20, "Creating output file");
			archive.ReadFile(out);
		}

		if (archive.IsError()) {
			ExitError(20, "Extracting file");
			// remove?
		}

		FileSetTime(fn, archive.GetTime());
	
		/* TODO: write attributes ? */

		Cout() <<" Ok. \n";
	}

}

