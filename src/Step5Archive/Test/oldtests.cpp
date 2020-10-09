#include "../Step5Archive.h"

#include "Test.brc"


namespace Upp {

StringStream uncompressed_stream;
StringStream compressed_stream;


void TestCodecBrc()
{
	MemStream brc_stream(s5demopx_ini, s5demopx_ini_length);
	
	LOG("*** UnCompressing...");
	DecodeStream (brc_stream, uncompressed_stream);
	if (codec_error) LOG(Format("*** AcsClass ERROR= 0x%X", codec_error));

	LOG("*** UnCompressed: ***");
	LOG(uncompressed_stream.GetResult());

	LOG("*** Compressing...");
	uncompressed_stream.Seek(0);
	compressed_stream.Seek(0);
	EncodeStream (uncompressed_stream, compressed_stream);
	if (codec_error) LOG(Format("*** AcsClass ERROR= 0x%X", codec_error));

	String s(s5demopx_ini, s5demopx_ini_length);
	if (s == compressed_stream.GetResult()) LOG("*** verification ok");
	else LOG("*** verification FAILED");
}


// From Autotest/StringStream
String RandomString(int n)
{
	String h;
	while(n-- > 0)
		h.Cat((byte)Random());
	return h;
}

void TestCodecRandom()
{

// From Autotest/StringStream
	StringStream ss;
	String es;
	int time0 = msecs();
	int i = 0;
	float insize=0, outsize=0;
	float ratio;
	
/**/while(msecs(time0) < 150000) {
		if(++i % 10 == 0)
		{
			ratio = (outsize/insize)*100;
			Cout() << i << " : " << insize << "->" << outsize << ", " << Format("%.2f", ratio) << "\%\r\n";
			insize = 0;
			outsize = 0;
		}
		Cout() << ".";
		Cout().Flush();

		int p = Random(1000);
		String h = RandomString(p);
		ss.Put(h);
		es.Cat(h);

		ss.Seek(0);
		compressed_stream.Create();
		EncodeStream (ss, compressed_stream);
		ASSERT(codec_error==0);
		
		compressed_stream.Seek(0);
		uncompressed_stream.Create();
		DecodeStream(compressed_stream, uncompressed_stream);
		ASSERT(codec_error==0);
		
		insize+= uncompressed_stream.GetSize();
		outsize+= compressed_stream.GetSize();
		
		h = uncompressed_stream.GetResult();
		ASSERT(h == es);
/**/	}

		if(Random(15) == 0) {
		//	ss.Create();
		//	es.Clear();
		}
//		LOGHEXDUMP(es, es.GetCount());
	
	
	LOG("========= OK");
}


}