#ifndef WEBP_H_
#define WEBP_H_
#include "platform.h"
#include "exception/exception.h"
#include "utils/utils.h"
#include "vp8l/vp8l.h"
#include <png.h>

#define WEBP_FILE_HEADER_LENGTH 12

namespace webp
{

enum FILE_FORMAT
{
	FILE_FORMAT_LOSSY,
	FILE_FORMAT_LOSSLESS
};

class WebP
{
private:
	uint32_t m_file_size;
	FILE_FORMAT m_file_format;
	utils::array<uint32_t> m_argb_image;
	uint32_t					m_image_width;
	uint32_t					m_image_height;
	WebP()
	{

	}
	void read_webp_file_header(const uint8_t ** iterable_pointer)
	{
		if (memcmp(*iterable_pointer, "RIFF", 4) != 0)
			throw exception::InvalidWebPFileFormat();
		*iterable_pointer += 4;

		memcpy(&m_file_size, *iterable_pointer, sizeof(uint32_t));
		*iterable_pointer += sizeof(uint32_t);

		if (memcmp(*iterable_pointer, "WEBP", 4) != 0)
			throw exception::InvalidWebPFileFormat();
		*iterable_pointer += 4;
	}
	void init(utils::array<uint8_t> & encoded_data)
	{
		//чтобы бегать по данным и не потерять указатель на начало буфера
		const uint8_t * iterable_pointer = &encoded_data[0];
		read_webp_file_header(&iterable_pointer);

		if (memcmp(iterable_pointer, "VP8 ", 4) == 0)
			m_file_format = FILE_FORMAT_LOSSY;
		else if (memcmp(iterable_pointer, "VP8L", 4) == 0)
			m_file_format = FILE_FORMAT_LOSSLESS;
		else
			throw exception::UnsupportedVP8();
		iterable_pointer += 4;

		//m_file_size это размер файла - 8(из заголовка RIFF(4 байта) и file_size(4 байта)),
		//data_length это m_file_size - 8(из заголовка WEBP(4 байта) и VP8_(4 байта))
		uint32_t vp8_data_length = m_file_size - 8;
		utils::array<uint8_t> vp8_data(vp8_data_length);
		memcpy(&vp8_data[0], iterable_pointer, vp8_data_length);
		if (m_file_format == FILE_FORMAT_LOSSLESS)
		{
			vp8l::VP8_LOSSLESS_DECODER decoder(&vp8_data[0], vp8_data_length, m_argb_image);
			m_image_width = decoder.image_width();
			m_image_height = decoder.image_height();
		}
		else
			throw exception::UnsupportedVP8();
	}
public:
	WebP(const std::string & file_name)
	{
		uint32_t file_length;
		utils::array<uint8_t> buf;
		utils::read_file(file_name, file_length, buf);
		if (file_length < WEBP_FILE_HEADER_LENGTH)
			throw exception::InvalidWebPFileFormat();
		init(buf);
		printf("%lu\n", m_argb_image.size());
	}
	void save2png(const std::string & file_name)
	{
		int i =0;
		//uint8_t * rgb = new uint8_t[m_image_height * m_image_width * 3];
		utils::array<uint8_t> rgb(m_image_height * m_image_width * 3);
		for(size_t y = 0; y < m_image_height; y++)
			for(size_t x = 0; x < m_image_width; x++)
			{
				size_t j = y * m_image_width + x;
				rgb[i++] = (m_argb_image[j] >> 16) & 0x000000ff;
				rgb[i++] = (m_argb_image[j] >> 8) & 0x000000ff;
				rgb[i++] = (m_argb_image[j] >> 0) & 0x000000ff;
			}

		png_structp png;
		png_infop info;
		png_uint_32 y;

		png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (png == NULL)
			throw exception::PNGError();

		info = png_create_info_struct(png);
		if (info == NULL)
		{
			png_destroy_write_struct(&png, NULL);
			throw exception::PNGError();
		}

		if (setjmp(png_jmpbuf(png)))
		{
			png_destroy_write_struct(&png, &info);
			throw exception::PNGError();
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
			throw exception::FileOperationException();
		}
		png_init_io(png, fp);
		png_set_IHDR(png, info, m_image_width, m_image_height, 8,
			   PNG_COLOR_TYPE_RGB,
			   PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
			   PNG_FILTER_TYPE_DEFAULT);
		png_write_info(png, info);
		for (y = 0; y < m_image_height; ++y)
		{
			png_bytep row = rgb + y * m_image_width * 3;
			png_write_rows(png, &row, 1);
		}
		png_write_end(png, info);
		png_destroy_write_struct(&png, &info);
		fclose(fp);
	}
	virtual ~WebP()
	{

	}
};


}

#endif /* WEBP_H_ */
