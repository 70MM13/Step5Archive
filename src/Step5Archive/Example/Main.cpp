#include "Example.h"

String DirSeparator() {
#if defined(PLATFORM_POSIX)
	return "/";
#elif defined(PLATFORM_WIN32) || defined(PLATFORM_WIN64)
	return "\\";
#endif
}

void Banner() {
	Cout() << GetExeFilePath() << "\n";
	Cout() << "Simple archiver for *PX.ACS files\n";
}

void Help()
{
	Cout() << "Usage: " << GetFileName(GetExeFilePath()) << " <source> [destination] \n\n";
	Cout() << R"t(where:
  * Options when archiving:
           <source> : ??????PX.INI - compress project files to archive
      [destination] : optional - path to new archive
                      when omitted, creates archive in current directory
                      without filename, uses the project name

  * Options when dearchiving:
           <source> : ??????PX.ACS - extract files from archive\n
      [destination] : optional - path to extract file to
                      when omitted, the full pathnames in the archive are used

	)t";
}

void ExitError(int erno, String text) {
	Cout() << GetFileName(GetExeFilePath()) << ": " << "Error: " << text << "\n";
}

Confirmation AskConfirmation (String question)
{
	int retval;
	String s;
	
	for (;;) {
		Cout() << "" << question << " (Yes/No/Cancel) :";
		s = ReadStdIn();
		s = ToLower(s);
		if      ( (s=="y") || (s=="yes") ) return yes;
		else if ( (s=="n") || (s=="no") ) return no;
		else if ( (s=="c") || (s=="cancel") ) return cancel;
		else {
			Cout() << " **BAD INPUT\n";
		}
	}
	return cancel;
}




CONSOLE_APP_MAIN
{
	typedef enum MODE {
		unset = 0,
		pack,
		unpack
	} Mode;
	
	Mode mode=unset;
	bool custom_archive_extension = false;
	
	String source, destination;
	
	
	Banner();
	
	const Vector<String>& cmdline = CommandLine();
	for(int i = 0; i < cmdline.GetCount(); i++) {
		if ( (cmdline[i] == "/?") | (cmdline[i] == "/h") | (cmdline[i] == "--help") ) {
			Help();
			Exit(0);
		}
	}
	if (mode == unset) {
		if (cmdline.GetCount() >= 1) {
			source = NormalizePath(cmdline[0]);
			if (source.EndsWith("PX.INI")) {
				if (HasWildcards(source)) ExitError(1, "No wildcards allowed in project name");
				if (!FileExists(source)) ExitError(1, Format("File not found \"%s\"", source));
				mode = pack;
			}
			else {
				if (custom_archive_extension || source.EndsWith(".ACS")) {
					mode = unpack;
				}
				else {
					ExitError(1, Format("Don't know how to handle \"%s\"", source));
				}
			}
		}
		if (cmdline.GetCount() >= 2) {
			destination = NormalizePath(cmdline[1]);
			if (HasWildcards(destination)) ExitError(2, "No wildcards allowed in [destination]");
		}
		else {
			destination = GetCurrentDirectory();
		}
		if (DirectoryExists(destination)) {
			if (destination.Last() != DirSeparator()) {
				destination += DirSeparator();
			}
		}
		if ((mode == pack) && GetFileName(destination) == "") {
			destination << ReplaceSuffix(GetFileName(source), "PX.ACS");
		}


		
		if (mode == pack) Pack(source, destination);
		else if (mode == unpack) Unpack(source, destination);
		else {
			ExitError(3, "Nothing to do -- use /? or -h for help");
		}
	}


}
