#include "Example.h"


String ReplaceSuffix (String filename, String new_suffix)
{
	String fn(filename);
	if (fn.GetLength() >= 6) {
		fn.Trim(fn.GetLength()-6);
		fn << new_suffix;
	}
	return fn;
}


void Pack(String source, String destination)
{
	Vector <String> filenames;
	TextSettings px_ini;
	int i;
	String fn;
	
	Cout() << "Project: " << source << "\n";
	Cout() << "Archive: " << destination << "\n";

	filenames.Add(source);

	px_ini.Load(source);
	for(i=0; i<=9; i++) {
		switch (i) {
			case 0: fn = px_ini.Get("OnlineSettings", "PathFile");         break; // PX.INI
			case 1: fn = px_ini.Get("BlocksSettings", "ProgFile");         break; // ST.S5D, D0.S5D
			case 2: fn = px_ini.Get("SymbSettings", "SymbIniFile");        break; // Z2.INI, Z1.INI, Z0.INI
			case 3: fn = px_ini.Get("SymbSettings", "SymbSeqFile");        break; // Z0.SEQ
			case 4: fn = px_ini.Get("DocSettings", "FooterFile");          break; // F1.INI, F2.INI
			case 5: fn = px_ini.Get("DocSettings", "DocCommFile");         break; // SU.INI
			case 6: fn = px_ini.Get("DocSettings", "PrinterFile");         break; // DR.INI
			case 7: fn = px_ini.Get("DocSettings", "OutFile");             break; // LS.INI
			case 8: fn = px_ini.Get("EpromSettings", "SysidFile");         break; // SD.INI
			case 9: fn = px_ini.Get("STLBatchSettings", "STLSourceFile");  break; // A0.SEQ, A1.SEQ
			default: NEVER(); fn = ""; break;
		}
//		if (!FileExists(fn)) {
//			if (yes != AskConfirmation(Format("* \"%s\" : File not found. Continue?", fn))) {
//				Cout() << "* Aborted\n";
//				return;
//			}
//		}
//		else
		if (fn != "") {
			switch (i) {
				case 1:
					filenames.Add(ReplaceSuffix(fn, "ST.S5D"));
					filenames.Add(ReplaceSuffix(fn, "D0.S5D"));
					break;
				case 2:
					filenames.Add(ReplaceSuffix(fn, "Z2.INI"));
					filenames.Add(ReplaceSuffix(fn, "Z1.INI"));
					filenames.Add(ReplaceSuffix(fn, "Z0.INI"));
					break;
				case 4:
					filenames.Add(ReplaceSuffix(fn, "F1.INI"));
					filenames.Add(ReplaceSuffix(fn, "F2.INI"));
					break;
				case 9:
					filenames.Add(ReplaceSuffix(fn, "A0.SEQ"));
					filenames.Add(ReplaceSuffix(fn, "A1.SEQ"));
					break;
				default:
					filenames.Add(fn);
			}
		}
	}
	
	for(i=0; i<filenames.GetCount(); i++) {
		Cout() << i << " " << filenames[i] << "\n";
	}
}

