#ifndef _Step5Archive_Step5Archive_h_
#define _Step5Archive_Step5Archive_h_

#include <Core/Core.h>

using namespace Upp;


extern int codec_error;

int EncodeStream (Stream &in, Stream &out);

int DecodeStream (Stream &in, Stream &out);



#endif
