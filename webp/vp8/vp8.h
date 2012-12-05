#ifndef VP8_H_
#define VP8_H_
#include "../platform.h"
#include "../exception/exception.h"
#include "../utils/bit_readed.h"

namespace webp
{
namespace vp8
{

class VP8_LOSSLESS_DECODER_INTERNAL_INTERFACE
{
private:

};

class VP8_LOSSLESSS_TRANSFORM
{
public:
	enum TransformType {
		  PREDICTOR_TRANSFORM			= 0,
		  COLOR_TRANSFORM				= 1,
		  SUBTRACT_GREEN				= 2,
		  COLOR_INDEXING_TRANSFORM		= 3,
		  TRANSFORM_NUMBER				= 4
	};
private:
	TransformType	type;
	int				bits;
	int				xsize;
	int				ysize;
	uint32_t*		data;
	const utils::BitReader & m_bit_reader;
	VP8_LOSSLESSS_TRANSFORM()
		: m_bit_reader(utils::BitReader())
	{

	}
public:
	VP8_LOSSLESSS_TRANSFORM(utils::BitReader & bit_reader)
		: m_bit_reader(bit_reader)
	{

	}
	virtual ~VP8_LOSSLESSS_TRANSFORM()
	{

	}
};



class VP8_LOSSLESS_DECODER
{
private:
	enum TransformType {
			  PREDICTOR_TRANSFORM			= 0,
			  COLOR_TRANSFORM				= 1,
			  SUBTRACT_GREEN				= 2,
			  COLOR_INDEXING_TRANSFORM		= 3,
			  TRANSFORM_NUMBER				= 4
		};
private:
	const uint8_t* const	m_data;
	//байт в m_data
	uint32_t				m_data_length;
	//прочитано из заголовка vp8l
	uint32_t				m_lossless_stream_length;
	int32_t					m_image_width;
	int32_t					m_image_height;
	int32_t					m_alpha_is_used;
	int32_t					m_version_number;
	//часто приходится читать m_data по битам
	utils::BitReader		m_bit_reader;

	VP8_LOSSLESSS_TRANSFORM	m_transform;

	//размер color cache
	uint32_t				m_color_cache_size;
	//применные трансформации,
	//мл.бит в 1 - применен PREDICTOR_TRANSFORM, иначе  - еще не применялся
	//след бит в 1 - применен COLOR_TRANSFORM, иначе нет - еще не применялся
	//след бит в 1 - применен SUBTRACT_GREEN, иначе нет - еще не применялся
	//след бит в 1 - применен COLOR_INDEXING_TRANSFORM, иначе - еще не применялся
	int32_t					m_applied_transforms;

	VP8_LOSSLESS_DECODER()
		: m_data(NULL), m_data_length(0), m_transform(m_bit_reader)
	{

	}
	/*
	 * ReadInfo()
	 * Бросает исключения: UnsupportedVP8
	 * Назначение:
	 * Читает заголовок VP8L
	 */
	void ReadInfo()
	{
		m_lossless_stream_length = m_bit_reader.ReadBits(32);
		char signature = m_bit_reader.ReadBits(8);
		if (signature != '\x2F')
			throw exception::UnsupportedVP8();

		m_image_width    = m_bit_reader.ReadBits(14) + 1;
		m_image_height   = m_bit_reader.ReadBits(14) + 1;
		m_alpha_is_used  = m_bit_reader.ReadBit();
		m_version_number = m_bit_reader.ReadBits(3);
	}
	/*
	 * ReadTransform()
	 * Бросает исключения: InvalidVP8
	 * Назначение:
	 * Читает трансформации, которые необходимо применить
	 */
	void ReadTransform()
	{
		//Читаем тип трансформации, которую надо применить
		TransformType transform_type = (TransformType)m_bit_reader.ReadBits(2);

		//проверяем, что мы не применяли эту трансформацию
		if (m_applied_transforms & (1U << transform_type))
			throw exception::InvalidVP8();
		m_applied_transforms |= (1U << transform_type);
		switch (transform_type)
		{
			case(PREDICTOR_TRANSFORM):
				{
					int32_t size_bits = m_bit_reader.ReadBits(3) + 2;
					#define DIV_ROUND_UP(num, den) ((num) + (den) - 1) / (den)
					int32_t block_xsize = DIV_ROUND_UP(m_image_width, 1 << size_bits);
					int32_t block_ysize = DIV_ROUND_UP(m_image_height, 1 << size_bits);
					ReadEntropyCodedImage();
				}
				break;
			default:
				throw exception::InvalidVP8();
		}
	}
	void ReadEntropyCodedImage()
	{
		ReadColorCacheInfo();
		ReadHuffmanCodes();
		ReadLZ77CodedImage();
	}
	void ReadColorCacheInfo()
	{
		uint32_t color_cache_code_bits = m_bit_reader.ReadBits(4);
		if (color_cache_code_bits != 0)
			m_color_cache_size = 1 << color_cache_code_bits;
	}
	void ReadHuffmanCodes()
	{

	}
	void ReadHuffmanCode()
	{

	}
	void ReadLZ77CodedImage()
	{

	}
public:
	VP8_LOSSLESS_DECODER(const uint8_t * const data, uint32_t data_length)
		: m_data(data), m_data_length(data_length), m_bit_reader(data, data_length), m_transform(m_bit_reader),
		  m_color_cache_size(0), m_applied_transforms(0)
	{
		ReadInfo();

		while(m_bit_reader.ReadBit())
		{
			ReadTransform();
			/*TransformType transform_type = (TransformType)m_bit_reader.ReadBits(2);
			int size_bits = m_bit_reader.ReadBits(3) + 2;
			int block_width = (1 << size_bits);
			int block_height = (1 << size_bits);
			#define DIV_ROUND_UP(num, den) ((num) + (den) - 1) / (den)
			int block_xsize = DIV_ROUND_UP(m_image_width, 1 << size_bits);
			int block_xsize2= (m_image_width + (1 << size_bits) - 1) >> size_bits;
			printf("3\n");*/
		}
	}
	virtual ~VP8_LOSSLESS_DECODER()
	{

	}
	friend class VP8_LOSSLESSS_TRANSFORM;
};

}
}

#endif /* VP8_H_ */
