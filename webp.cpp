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

void encode(char * fn){
	
}

int main(int argc, char * argv[])
{
	webp::vp8l::huffman_io::init_array();
	try{
		if (argc != 3)
			return 1;
		if (strncmp(argv[1], "-d", 2) == 0){
			webp::WebP_DECODER webp(argv[2]);
			std::string output = argv[2];
			output += ".png";
			webp.save2png(output);
			return 0;
		}
		if (strncmp(argv[1], "-e", 2) == 0){
			image_t image;
			read_png(argv[2], image);
			webp::WebP_ENCODER encoder(image.image, image.width, image.height, "output.webp");
			return 0;
		}
	}
	catch(webp::exception::Exception & e){
		std::cout << e.message << std::endl;
	}
	return 0;
}
