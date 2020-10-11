#include "Example.h"

void Unpack(String source, String destination)
{
	Cout() << "Unpacking archive files from: \"" << source << "\"\n";
	if (destination != "") {
		Cout() << "                          to: \"" << destination << "\"\n";
	}
}

