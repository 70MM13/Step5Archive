#ifndef _Step5Archive_Step5Archive_h_
#define _Step5Archive_Step5Archive_h_

#include <Core/Core.h>

extern int codec_error;


namespace Upp {

int EncodeStream (Stream &in, Stream &out);
int DecodeStream (Stream &in, Stream &out);



enum FILE_ATTRIBUTE {
	READONLY            = 0x00000001,
	HIDDEN              = 0x00000002,
	SYSTEM              = 0x00000004,
	DIRECTORY           = 0x00000010,
	ARCHIVE             = 0x00000020,
	DEVICE              = 0x00000040,
	NORMAL              = 0x00000080,
	TEMPORARY           = 0x00000100,
	SPARSE_FILE         = 0x00000200,
	REPARSE_POINT       = 0x00000400,
	COMPRESSED          = 0x00000800,
	OFFLINE             = 0x00001000,
	NOT_CONTENT_INDEXED = 0x00002000,
	ENCRYPTED           = 0x00004000
};



class Step5Archive
{
	
	struct Header {
		char id[6] = "STEP5"; //0-terminated
		// 1 bytes '\0'
		uint16 nfiles;
		uint32 asize;
		//reserved 41 bytes
	};
	
	struct File : Moveable<File> {
		char path[260];
		uint16 date;
		uint16 time;
		uint16 attribute;
		uint8 compression;
		uint32 usize;
		uint32 csize;
		//8 bytes reserved
		uint32 offset;
	};
	
	Stream *archive;
	bool         error;
	Vector<File> files;
	int          current;
	
	void ReadDir();
	
	static Time GetDosDateTime(word date, word time);
	
	
	
public:
	typedef Step5Archive CLASSNAME;
	Step5Archive();
};


}
#endif
