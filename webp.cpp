#include <iostream>
#include "webp/webp.h"
#include "webp/utils/bit_writer.h"
#include "webp/vp8l/vp8l.h"
#include "webp/lz77/lz77.h"


void PNGAPI error_function(png_structp png, png_const_charp dummy) {
  (void)dummy;  // remove variable-unused warning
  longjmp(png_jmpbuf(png), 1);
}

struct image_t{
	webp::utils::pixel_array image;
	uint16_t width;
	uint16_t height;
};

 int read_png(const std::string & file_name, image_t & image) {
  png_structp png;
  png_infop info;
  int color_type, bit_depth, interlaced;
  int has_alpha;
  int num_passes;
  int p;
  int ok = 0;
  png_uint_32 width, height, y;
  int stride;
  uint8_t* rgb = NULL;

  FILE * fp;

#ifdef LINUX
	  fp = fopen(file_name.c_str(), "rb");
	#endif
	#ifdef WINDOWS
	  fopen_s(&fp, file_name.c_str(), "rb");
	#endif
	if (fp == NULL)
		throw  webp::exception::FileOperationException();

  png = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
  if (png == NULL) {
    goto End;
  }

  png_set_error_fn(png, 0, error_function, NULL);
  if (setjmp(png_jmpbuf(png))) {
 Error:
    png_destroy_read_struct(&png, NULL, NULL);
    free(rgb);
    goto End;
  }

  info = png_create_info_struct(png);
  if (info == NULL) goto Error;

  png_init_io(png, fp);
  png_read_info(png, info);
  if (!png_get_IHDR(png, info,
                    &width, &height, &bit_depth, &color_type, &interlaced,
                    NULL, NULL)) goto Error;

  png_set_strip_16(png);
  png_set_packing(png);
  if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
  if (color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
    if (bit_depth < 8) {
      png_set_expand_gray_1_2_4_to_8(png);
    }
    png_set_gray_to_rgb(png);
  }
  if (png_get_valid(png, info, PNG_INFO_tRNS)) {
    png_set_tRNS_to_alpha(png);
    has_alpha = 1;
  } else {
    has_alpha = !!(color_type & PNG_COLOR_MASK_ALPHA);
  }

 /* if (!keep_alpha) {
    png_set_strip_alpha(png);
    has_alpha = 0;
  }*/

  num_passes = png_set_interlace_handling(png);
  png_read_update_info(png, info);
  stride = (has_alpha ? 4 : 3) * width * sizeof(*rgb);
  rgb = (uint8_t*)malloc(stride * height);
  if (rgb == NULL) goto Error;
  for (p = 0; p < num_passes; ++p) {
    for (y = 0; y < height; ++y) {
      png_bytep row = rgb + y * stride;
      png_read_rows(png, &row, NULL, 1);
    }
  }
  png_read_end(png, info);
  png_destroy_read_struct(&png, &info, NULL);

  image.width = width;
  image.height = height;
  image.image.realloc(height * width);
  for(size_t y = 0; y < height; y++)
	  for(size_t x = 0; x < width; x++){
		  size_t i = y * width + x;
		  if (has_alpha) {
			  *webp::utils::ALPHA(image.image[i]) = rgb[stride * y + x * 4];
			  *webp::utils::RED(image.image[i])   = rgb[stride * y + x * 4 + 1];
			  *webp::utils::GREEN(image.image[i]) = rgb[stride * y + x * 4 + 2];
			  *webp::utils::BLUE(image.image[i])  = rgb[stride * y + x * 4 + 3];
		  }
		  else{
			  *webp::utils::ALPHA(image.image[i]) = 0xFF;
			  *webp::utils::RED(image.image[i])   = rgb[stride * y + x * 3];
			  *webp::utils::GREEN(image.image[i]) = rgb[stride * y + x * 3 + 1];
			  *webp::utils::BLUE(image.image[i])  = rgb[stride * y + x * 3 + 2];
		  }
	  }
  free(rgb);

 End:
  fclose(fp);
  return ok;
}

void save_png(const std::string & file_name, const image_t & image){
		webp::utils::array<uint8_t> rgb(image.height * image.width * 3);
		int i = 0;
 		for(size_t y = 0; y < image.height; y++)
 			for(size_t x = 0; x < image.width; x++)
 			{
 				size_t j = y * image.width + x;
 				rgb[i++] = (image.image[j] >> 16) & 0x000000ff;
 				rgb[i++] = (image.image[j] >> 8) & 0x000000ff;
 				rgb[i++] = (image.image[j] >> 0) & 0x000000ff;
 			}

 		png_structp png;
 		png_infop info;
 		png_uint_32 y;

 		png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
 		if (png == NULL)
 			throw webp::exception::PNGError();

 		info = png_create_info_struct(png);
 		if (info == NULL)
 		{
 			png_destroy_write_struct(&png, NULL);
 			throw  webp::exception::PNGError();
 		}

 		if (setjmp(png_jmpbuf(png)))
 		{
 			png_destroy_write_struct(&png, &info);
 			throw  webp::exception::PNGError();
 		}
 		FILE * fp = NULL;

 		#ifdef LINUX
 		  fp = fopen(file_name.c_str(), "wb");
 		#endif
 		#ifdef WINDOWS
 		  fopen_s(&fp, file_name.c_str(), "wb");
 		#endif
 		if (fp == NULL)
 		{
 			png_destroy_write_struct(&png, &info);
 			throw  webp::exception::FileOperationException();
 		}
 		png_init_io(png, fp);
 		png_set_IHDR(png, info, image.width, image.height, 8,
 			   PNG_COLOR_TYPE_RGB,
 			   PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
 			   PNG_FILTER_TYPE_DEFAULT);
 		png_write_info(png, info);
 		for (y = 0; y < image.height; ++y)
 		{
 			png_bytep row = rgb + y * image.width * 3;
 			png_write_rows(png, &row, 1);
 		}
 		png_write_end(png, info);
 		png_destroy_write_struct(&png, &info);
 		fclose(fp);
}

void huffman_test(const std::string & fn)
{
	//using namespace webp;
	webp::utils::BitWriter bw;
	image_t image;
	read_png(fn, image);
	int histo_[256] = {0};
	webp::histoarray histo(256);
	histo.fill(0);
	for(size_t i = 0; i < image.image.size(); i++){
		++histo[*webp::utils::GREEN(image.image[i])];
		++histo_[*webp::utils::GREEN(image.image[i])];
	}

	webp::huffman_coding::enc::HuffmanTree tree(histo, 15);

	/*HuffmanTreeCode tree1;
	tree1.codes = new uint16_t[256];
	tree1.code_lengths = new uint8_t[256];
	tree1.num_symbols = 256;
	VP8LCreateHuffmanTree(histo_, 15, &tree1);
	for(size_t i = 0; i < 256; i++)
		printf("%u ", tree1.code_lengths[i]);
	printf("\n");

	const int CODE_LENGTH_CODES = 19;
	uint8_t code_length_bitdepth[CODE_LENGTH_CODES] = { 0 };
	  uint16_t code_length_bitdepth_symbols[CODE_LENGTH_CODES] = { 0 };
	  const int max_tokens = tree1.num_symbols;
	  int num_tokens;
	  HuffmanTreeCode huffman_code;
	  HuffmanTreeToken* const tokens =
	      (HuffmanTreeToken*)malloc(sizeof(*tokens) * max_tokens);

	  huffman_code.num_symbols = CODE_LENGTH_CODES;
	  huffman_code.code_lengths = code_length_bitdepth;
	  huffman_code.codes = code_length_bitdepth_symbols;

	  num_tokens = VP8LCreateCompressedHuffmanTree(&tree1, tokens, max_tokens);

	    int histogram[CODE_LENGTH_CODES] = { 0 };
	    int i;
	    for (i = 0; i < num_tokens; ++i) {
	    	printf("(%u %u) ", tokens[i].code, tokens[i].extra_bits);
	      ++histogram[tokens[i].code];
	    }
	    printf("\n");
	    VP8LCreateHuffmanTree(histogram, 7, &huffman_code);

*/

	webp::vp8l::huffman_io::enc::VP8_LOSSLESS_HUFFMAN io(&bw, tree);
	bw.save2file("huffman");

	uint32_t len;
	webp::utils::array<uint8_t> buf;
	webp::utils::read_file("huffman", len, buf);
	webp::utils::BitReader br(&buf[0], buf.size());
	webp::vp8l::huffman_io::dec::VP8_LOSSLESS_HUFFMAN io_dec(&br);
	io_dec.read();
}

void test(){
	/*webp::utils::array<uint16_t> histo(5);
	histo[0] = 24;
	histo[1] = 6;
	histo[2] = 56;
	histo[3] = 12;
	histo[4] = 45;*/
	image_t image;
	read_png("basn3p08.webp.png", image);
	int histo_[256] = {0};
	webp::histoarray histo(256);
	histo.fill(0);
	for(size_t i = 0; i < image.image.size(); i++){
		++histo[*webp::utils::RED(image.image[i])];
		++histo_[*webp::utils::GREEN(image.image[i])];
	}
	webp::huffman_coding::enc::HuffmanTree tree(histo, 15);
}

void lz77test(char * fn){

	image_t image;
	read_png(fn, image);

	printf("%lu\n", image.image.size());

	/*webp::histoarray histo(256);
	histo.fill(0);
	for(size_t i = 0; i < image.image.size(); i++)
		++histo[*webp::utils::GREEN(image.image[i])];

	for(size_t i = 0; i < 256; i++)
		if (histo[i] != 0)
			printf("%u %u\n",i, histo[i]);

	FILE * f = fopen("data", "wb");
	fwrite(&image.image[0], 4, image.image.size(), f);
	fclose(f);*/


	webp::lz77::LZ77<uint32_t> LZ77_pixel(1024, 128, image.image);
	printf("%lu\n", LZ77_pixel.output().size());
	LZ77_pixel.test_unpack(image.image.size());
	webp::histoarray histo2(256);
	histo2.fill(0);
	for(size_t i = 0; i < LZ77_pixel.output().size(); i++)
		if (LZ77_pixel.output()[i].length == 0 && LZ77_pixel.output()[i].distance == 0)
			++histo2[*webp::utils::GREEN(LZ77_pixel.output()[i].symbol)];

	/*for(size_t i = 0; i < 256; i++)
		if (histo2[i] != 0)
			printf("%u %u\n",i, histo2[i]);*/

	webp::huffman_coding::enc::HuffmanTree tree2(histo2, 15);
	webp::utils::BitWriter bw;
	webp::vp8l::huffman_io::enc::VP8_LOSSLESS_HUFFMAN hio(&bw, tree2);
	bw.save2file("hio");

	uint32_t len;
	webp::utils::array<uint8_t> buf;
	webp::utils::read_file("hio", len, buf);
	webp::utils::BitReader br(&buf[0], buf.size());
	webp::vp8l::huffman_io::dec::VP8_LOSSLESS_HUFFMAN io_dec(&br);
	io_dec.read();

	/*for(size_t i = 0; i < 256; i++)
	{
		printf("%u %u\n", histo[i], histo2[i]);
	}*/
}

void entropy_image(char * fn){
	webp::vp8l::VP8_LOSSLESS_ENCODER enc;
	image_t image;
	read_png(fn, image);
	enc.WriteEntropyCodedImage(image.width, image.height, image.image);

	enc.m_bit_writer.save2file("entropy_image");
	webp::utils::array<uint8_t> buf;
	uint32_t len;
	webp::utils::read_file("entropy_image", len, buf);
	image.image.fill(0);
	webp::vp8l::VP8_LOSSLESS_DECODER dec(&buf[0], len, image.width, image.height, image.image);
	save_png("out.png", image);
}

void encode(char * fn){
	image_t image;
	read_png(fn, image);
	webp::WebP_ENCODER encoder(image.image, image.width, image.height, "output.webp");
}

int main(int argc, char * argv[])
{
	webp::vp8l::huffman_io::init_array();
	if (strncmp(argv[1], "-d", 2) == 0){
		webp::WebP_DECODER webp(argv[2]);
		std::string output = argv[2];
		output += ".png";
		webp.save2png(output);
		return 0;
	}
	if (strncmp(argv[1], "-e", 2) == 0){
		encode(argv[2]);
		return 0;
	}
	return 0;
}
