#include <iostream>
#include "webp/webp.h"
#include "webp/utils/bit_writer.h"

int main(int argc, char * argv[])
{
	webp::WebP webp(argv[1]);
	webp.save2png(argv[2]);
	//webp::utils::BitWriterTest bwt;
	//bwt.test();
	return 0;
}
