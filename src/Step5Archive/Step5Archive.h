#ifndef _Step5Archive_Step5Archive_h_
#define _Step5Archive_Step5Archive_h_

#include <Core/Core.h>

extern int codec_error;
int EncodeStream (Upp::Stream &in, Upp::Stream &out);
int DecodeStream (Upp::Stream &in, Upp::Stream &out);


namespace Upp {

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


// Trying to be compatible with UnZip
	
class Step5Archive
{
	
	static const int sizeof_header = 54;
	struct Header {
		char id[6] = "STEP5"; //0-terminated
		// 1 bytes '\0'
		uint16 nfiles;
		uint32 asize;
		//reserved 41 bytes
	};
	
	static const int sizeof_filerecord = 287;
	struct File : Moveable<File> {
		char path[260];
		uint16 date;
		uint16 time;
		uint16 attr;
		uint8 compr;
		uint32 usize;
		uint32 csize;
		//8 bytes reserved
		uint32 offset;
	};
	
	Stream *archive;
	bool         error;
	Header       header;
	int          offset;
	Vector<File> files;
	int          current;
	
	void ReadDir();
	
	static Time GetDosDateTime(word dosdate, word dostime);
	
public:
	bool   IsEof() const           { return current >= files.GetCount(); }
	operator bool() const          { return !IsEof() && !IsError(); }
	
	bool   IsError() const         { return error; }
	void   ClearError()            { error = false; }

	int    GetCount() const        { return files.GetCount(); }
	String GetPath(int i) const    { return files[i].path; }
	bool   IsFolder(int i) const   { return (files[i].attr & FILE_ATTRIBUTE::DIRECTORY) > 0 ; }
	bool   IsFile(int i) const     { return !IsFolder(i); }
	word   GetAttr(int i) const    { return files[i].attr; }
	int    GetCLength(int i) const { return files[i].csize; }
	int    GetLength(int i) const  { return files[i].usize; }
	Time   GetTime(int i) const    { return GetDosDateTime(files[i].date, files[i].time); }

	void   Seek(int i)             { ASSERT(i >= 0 && i < files.GetCount()); current = i; }

	String GetPath() const         { return GetPath(current); }
	bool   IsFolder() const        { return IsFolder(current); }
	bool   IsFile() const          { return !IsFolder(); }
	word   GetAttr() const         { return GetAttr(current); }
	int    GetCLength() const      { return GetCLength(current); }
	int    GetLength() const       { return GetLength(current); }
	Time   GetTime() const         { return GetTime(current); }

	void   Skip()                  { current++; }
	void   SkipFile()              { current++; }
	bool   ReadFile(Stream& out, Gate<int, int> progress = Null);
	String ReadFile(Gate<int, int> progress = Null);

	String ReadFile(const char *path, Gate<int, int> progress = Null);
	
	dword  GetPos() const; // what's this ?

	void   Create(Stream& in);

	Step5Archive(Stream& in);
	Step5Archive();
	~Step5Archive();
};


}
#endif
