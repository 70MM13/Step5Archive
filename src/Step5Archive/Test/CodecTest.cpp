#include "Test.h"


// helper for DUMPing raw data without messing up the logging
String PrintableAscii(const char *text)
{
	StringBuffer x;
	while(*text) {
		if (iscntrl(*text)) //if((byte)*text < 31)
			x.Cat('\\' + IntStr((byte)*text));
		else x.Cat(*text);
		text++;
	}
	return String(x);
}




void CodecTest::SetUp()
{
}


void CodecTest::DoEncodeDecode()
{
	ss.Seek(0);
	compressed_stream.Create();
	EncodeStream (ss, compressed_stream);
	ASSERT_EQ(codec_error, 0);
	
	compressed_stream.Seek(0);
	uncompressed_stream.Create();
	DecodeStream(compressed_stream, uncompressed_stream);
	ASSERT_EQ(codec_error, 0);
	
	insize+= uncompressed_stream.GetSize();
	outsize+= compressed_stream.GetSize();
}


void CodecTest::DoTestRun(int max, Function<String (int)> generator, bool dups)
{
	int time0 = msecs();
	int p=0, i=0;

	Vector <String> duplicates;
	duplicates.SetCount(100);
	String h;
	
	insize = 0;
	outsize = 0;
	
	do {
		if (dups) {
			h = "";
			for (int j=0; j<duplicates.GetCount(); j++)
			{
				p = Random(7);
				duplicates[ j ] = generator(p);
				h += duplicates [ Random( duplicates.GetCount() ) ];
			}
		}
		else {
			p = Random(1000);
			h = generator(p);
		}
		ss.Put(h);
		es.Cat(h);
		DoEncodeDecode();
		h = uncompressed_stream.GetResult();
		ASSERT_EQ(h, es);

		if(Random(15) == 0) {
			ss.Create();
			es.Clear();
		}
		i++;
	}
	while (msecs(time0) < max);

//	if(++i % 10 == 0) /* originally in loop */
	{
		ratio = (outsize/insize)*100;
		LOG(Format(" - in:%i -> out:%i (%.2f%%)", insize, outsize, ratio));
		insize = 0;
		outsize = 0;
	}
	
//	DUMP(PrintableAscii(h));
}




/*
	helper functions
*/

// From Autotest/StringStream
String RandomString(int n)
{
	String h;
	while(n-- > 0)
		h.Cat((byte)Random());
	return h;
}

String RandomStringAscii (int n)
{
	String PrintableAscii(" !\"#$%&\'()*+,-./0123456789:;<=>\?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~");
	String h;
	while (n-- > 0)
		h.Cat(PrintableAscii[Random(PrintableAscii.GetCount())]);
	return h;
}




/*
	Testing the encoder against the decoder, with random data.
	The encoder acts a bit different on ASCII data, hence the extra test.
	
	Extra data is written to log file, to show the compression ratio
*/

TEST_F(CodecTest, Random)
{
	LOG("CodecTest.Random");
	DoTestRun(2500, [](int p)->String { return RandomString(p); }, false);
}

TEST_F(CodecTest, RandomAscii)
{
	LOG("CodecTest.RandomAscii");
	DoTestRun(2500, [](int p)->String { return RandomStringAscii(p); }, false);
}

TEST_F(CodecTest, RandomWithDuplicates)
{
	LOG("CodecTest.RandomWithDuplicates");
	DoTestRun(2500, [](int p)->String { return RandomString(p); }, true);
}

TEST_F(CodecTest, RandomAsciiWithDuplicates)
{
	LOG("CodecTest.RandomAsciiWithDuplicates");
	DoTestRun(25000000, [](int p)->String { return RandomStringAscii(p); }, true);
}



/*
	Testing with a small blob extracted from an original archive
	original file is "S5DEMOPX.INI" - dump the .GetResult to read it
*/

TEST_F(CodecTest, BRC_INI)
{
	MemStream brc_stream(s5demopx_ini, s5demopx_ini_length);
	
	brc_stream.Seek(0);
	uncompressed_stream.Create();
	DecodeStream (brc_stream, uncompressed_stream);
	ASSERT_EQ (codec_error, 0);
	
//	DUMP(uncompressed_stream.GetResult());

	uncompressed_stream.Seek(0);
	compressed_stream.Create();
	EncodeStream (uncompressed_stream, compressed_stream);
	ASSERT_EQ (codec_error, 0);

	String s(s5demopx_ini, s5demopx_ini_length);
	ASSERT_EQ (s, compressed_stream.GetResult());
}


