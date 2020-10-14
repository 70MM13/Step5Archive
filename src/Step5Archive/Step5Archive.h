#ifndef _Step5Archive_Step5Archive_h_
#define _Step5Archive_Step5Archive_h_

#include <Core/Core.h>

extern int codec_error;
int EncodeStream (Upp::Stream &in, Upp::Stream &out, int size_in=INT_MAX);
//int EncodeStream (Upp::Stream &in, Upp::Stream &out);
int DecodeStream (Upp::Stream &in, Upp::Stream &out, int size_in=INT_MAX, int size_out=INT_MAX);
//int DecodeStream (Upp::Stream &in, Upp::Stream &out);


namespace Upp {

enum file_attribute {
	readonly            = 0x00000001,
	hidden              = 0x00000002,
	system              = 0x00000004,
	directory           = 0x00000010,
	archive             = 0x00000020,
	device              = 0x00000040,
	normal              = 0x00000080,
	temporary           = 0x00000100,
	sparse_file         = 0x00000200,
	reparse_point       = 0x00000400,
	compressed          = 0x00000800,
	offline             = 0x00001000,
	not_content_indexed = 0x00002000,
	encrypted           = 0x00004000
};


// Trying to be compatible with UnZip
	
class Step5Archive
{
protected:
	static const int sizeof_header = 54;
	static const int sizeof_filerecord = 287;
	static const int sizeof_path = 260;
public:
	static const int path_max = sizeof_path - 1;
	static const int filecount_max = 40;
	static const int minsize_compression = 1024; // Originally smaller files are stored. But why? those small files compress fine...
	
protected:
	struct Header {
		char id[6] = "STEP5"; //0-terminated
		// 1 bytes '\0'
		uint16 nfiles =0;
		uint32 asize =0;
		//reserved 41 bytes
	};
	
	struct File : Moveable<File> {
		char path[sizeof_path] = "";
		uint16 date = 0;
		uint16 time = 0;
		uint16 attr = 0;
		uint8 compr = 0;
		uint32 usize = 0;
		uint32 csize = 0;
		//8 bytes reserved
		uint32 offset = 0;
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
	bool   IsFolder(int i) const   { return (files[i].attr & file_attribute::directory) > 0 ; }
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
};


class WriteStep5Archive : protected Step5Archive
{
	bool processing;
	
	void UpdateHeader();
	void UpdateFileRecord();
	void InsertFileRecord();
	
	static word GetDosDate(Time time);
	static word GetDosTime(Time time);

public:
	using Step5Archive::IsError;
	using Step5Archive::GetCLength;
	
	bool   IsFileOpened()          { return processing; }
	void   WriteFile(Stream& in, String path, Time tm = GetSysTime(), dword attr=0, bool compressed = true);
	
	void   Create(Stream& out);

	WriteStep5Archive(Stream& out);
	WriteStep5Archive();
};

}
#endif
