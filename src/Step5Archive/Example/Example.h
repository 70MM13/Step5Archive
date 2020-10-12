#ifndef _Step5Archive_Example_Example_h_
#define _Step5Archive_Example_Example_h_

#include <Core/Core.h>

#include <Step5Archive/Step5Archive.h>

using namespace Upp;

typedef enum ARCHOPTS {
	none     = 0x00000000,
	list,
	test
} ArchOpts;

typedef enum CONFIRM {
	cancel = -1,
	no = 0,
	yes = 1
} Confirmation;

Confirmation AskConfirmation(String question);
void ExitError(int err, String text);

String ReplaceSuffix (String filename, String new_suffix);

void Pack (String source, String Destination, ArchOpts opts=none);
void Unpack (String source, String Destination, ArchOpts opts=none);

#endif
