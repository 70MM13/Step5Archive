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

void ExitError(int err, String text) {
	Cout() << GetFileName(GetExeFilePath()) << ": " << "Error: " << text << "\n";
	Exit(err);
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
	
	ArchOpts opts=none;
	Mode mode=unset;
	bool custom_archive_extension = false;
	
	String source="", destination="";
	
	Banner();
	
	const Vector<String>& cmdline = CommandLine();
	for(int i = 0; i < cmdline.GetCount(); i++) {
		if ( (cmdline[i] == "/?") | (cmdline[i] == "-h") ) {
			Help();
			Exit(0);
		}
	}

	int iSrc=0, iDst=0;
	if (cmdline.GetCount() >= 2) {
		if (cmdline[0].StartsWith("-")) {
			if      (cmdline[0] == "-e") { mode = unpack; }
			else if (cmdline[0]	== "-p") { mode = pack; }
			else if (cmdline[0] == "-l") { mode = unpack; opts = ArchOpts::list; }
			else if (cmdline[0] == "-t") { mode = unpack; opts = ArchOpts::test; }
			else ExitError(1, Format("Unknown option: %s", cmdline[0]));
			
			iSrc=1;
			iDst=2;
		}
	}
	if (	(mode==unset) && ( (cmdline.GetCount() < 1) || (cmdline.GetCount() > 2) )
		||	(mode!=unset) && ( (cmdline.GetCount() < 2) || (cmdline.GetCount() > 3) ) )
	{
		ExitError(1, "Error: invalid number of parameters -- use /? or -h for help");
	}
		
	source = NormalizePath(cmdline.Get(iSrc, ""));
	destination = cmdline.Get(iDst, "");
	
	if (HasWildcards(source) || HasWildcards(destination)) {
		ExitError(1, "Error: No wildcards allowed");
	}
	if (!FileExists(source)) {
		ExitError(1, Format("File not found \"%s\"", source));
	}
	
	// no mode given -- autodetect source
	if (mode == unset) {
		if (source.EndsWith("PX.INI")) {
			mode = pack;
		}
		else if (source.EndsWith(".ACS")) {
			mode = unpack;
		}
		else {
			ExitError(1, Format("Error: Don't know how to handle \"%s\"", source));
		}
	}
	// (packing) no destination given -- set to current directory
	if( (mode == pack) && (destination=="") ) {
		destination=GetCurrentDirectory();
	}
	// when destination is existing directory, make sure it ends with '/' or '\\'
	if (destination != "") {
		if (DirectoryExists(destination)) {
			if (destination.Last() != DirSeparator()) {
				destination += DirSeparator();
			}
		}
		else {
			if (mode == unpack) ExitError(1, Format("Error: Destination directory does not exist (%s)", destination));
		}
	}
	// (packing) no filename given -- construct from project filename
	if ((mode == pack) && GetFileName(destination) == "") {
		destination << ReplaceSuffix(GetFileName(source), "PX.ACS");
	}
		
	if (mode == pack) Pack(source, destination, opts);
	else if (mode == unpack) Unpack(source, destination, opts);
	else {
		ExitError(3, "Nothing to do -- use /? or -h for help");
	}

}
