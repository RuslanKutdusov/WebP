#ifndef VP8_H_
#define VP8_H_
#include "../platform.h"
#include "../exception/exception.h"

namespace webp
{
namespace vp8
{

class VP8_LOSSLESS_DECODER
{
private:
	enum TransformType {
		  PREDICTOR_TRANSFORM             = 0,
		  COLOR_TRANSFORM                 = 1,
		  SUBTRACT_GREEN                  = 2,
		  COLOR_INDEXING_TRANSFORM        = 3,
	};
private:
	VP8_LOSSLESS_DECODER()
	{

	}
	char * data;
	uint32_t data_length;
	uint64_t bits_readed;
	uint32_t m_lossless_stream_length;
	int m_image_width;
	int m_image_height;
	int m_alpha_is_used;
	int m_version_number;
	char ReadBit()
	{
		uint32_t byte_index = bits_readed / 8;
		char byte = data[byte_index];
		uint32_t bit_index = bits_readed - byte_index * 8;
		byte >>= bit_index;
		bits_readed++;
		return byte & '\1';
	}
	template<class T>
	T ReadBits(const uint32_t & bits_count)
	{
		T ret;
		memset(&ret, 0, sizeof(T));
		for(uint32_t i = 0; i < bits_count; i++)
		{
			ret |= ReadBit() << i;
		}
		return ret;
	}
public:
	VP8_LOSSLESS_DECODER(const char * data, uint32_t data_length)
	{
		this->data = data;
		this->data_length = data_length;

		m_lossless_stream_length = ReadBits<uint32_t>(32);
		char signature = ReadBits<char>(8);
		if (signature != '\x2F')
			throw exception::UnsupportedVP8();

		m_image_width = ReadBits<int>(14) + 1;
		m_image_height = ReadBits<int>(14) + 1;
		m_alpha_is_used = ReadBits<int>(1);
		m_version_number = ReadBits<int>(3);

		while(ReadBit())
		{
			TransformType transform_type = (TransformType)ReadBits<int>(2);
			int size_bits = ReadBits<int>(3) + 2;
			int block_width = (1 << size_bits);
			int block_height = (1 << size_bits);
			#define DIV_ROUND_UP(num, den) ((num) + (den) - 1) / (den)
			int block_xsize = DIV_ROUND_UP(m_image_width, 1 << size_bits);
			printf("3\n");
		}
	}
	/*int DIV_ROUND_UP(int num, int den)
	{
		return ((num) + (den) - 1) / den;
	}*/
};

}
}

#endif /* VP8_H_ */
