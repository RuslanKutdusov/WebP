#include <iostream>
#include "webp/webp.h"

int main(int argc, char * argv[])
{
	webp::WebP webp(argv[1]);
	//webp.save2png(argv[2]);
	return 0;
}
