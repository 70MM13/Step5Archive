#ifndef _Step5Archive_Test_h_
#define _Step5Archive_Test_h_

#include <Core/Core.h>
#include <plugin/gmock/gmock.h>

#include "../Step5Archive.h"

#include "Test.brc"


using namespace Upp;

/* old tests */
void TestCodecBrc();
void TestCodecRandom();



/* Google Test Fixture */

class CodecTest : public testing::Test {
protected:
	virtual void SetUp() override;
	
protected:
	StringStream uncompressed_stream;
	StringStream compressed_stream;
	StringStream ss;
	String es;
	float insize=0, outsize=0, ratio=0;
	
	void DoEncodeDecode();
	void DoTestRun(int max, Function<String (int)> generator, bool dups=false);
};



#endif
