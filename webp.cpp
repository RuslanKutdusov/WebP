#include <iostream>
#include "webp/webp.h"


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

 void print_help(){
	 std::cout << "WebP Decoded/Encoder\n";
	 std::cout << "\t-h - this help\n";
	 std::cout << "\t-d|-e input_file_name output_file_name - decode|encode input file to output file\n";
 }


int main(int argc, char * argv[])
{
	webp::vp8l::huffman_io::init_array();
	std::string input;
	std::string output;
	bool encode = false;
	bool decode = false;
	for(++argv; argv[0]; ++argv){
		if (argv[0] == std::string("-d"))
			decode = true;
		else
		if (argv[0] == std::string("-e"))
			encode = true;
		else
		if (argv[0] == std::string("-h")){
			print_help();
			return 0;
		}
		else{
			if (input.size() == 0)
				input = argv[0];
			else if (output.size() == 0)
				output = argv[0];
			else{
				printf("What you mean? %s\n", argv[0]);
				print_help();
				return 1;
			}
		}
	}

	if ((encode && decode) || (!encode && !decode)){
		printf("Specify key -d or -e\n");
		print_help();
		return 1;
	}
	if (input.size() == 0){
		printf("Specify input file name\n");
		print_help();
		return 1;
	}
	if (output.size() == 0){
		printf("Specify output file name\n");
		print_help();
		return 1;
	}

	try{
		if (decode){
			webp::WebP_DECODER webp(input);
			webp.save2png(output);
			return 0;
		}
		if (encode){
			image_t image;
			read_png(input, image);
			webp::WebP_ENCODER encoder(image.image, image.width, image.height, output);
			return 0;
		}
	}
	catch(webp::exception::Exception & e){
		std::cout << e.message << std::endl;
	}
	return 0;
}
