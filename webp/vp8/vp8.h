#ifndef VP8_H_
#define VP8_H_
#include "../platform.h"
#include "../exception/exception.h"
#include "../utils/bit_readed.h"

namespace webp
{
namespace vp8
{

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
private:
	const uint8_t* const	m_data;
	uint32_t				m_data_length;
	uint32_t				m_lossless_stream_length;
	int32_t					m_image_width;
	int32_t					m_image_height;
	int32_t					m_alpha_is_used;
	int32_t					m_version_number;
	int32_t					m_applied_transforms;
	utils::BitReader		m_bit_reader;
	VP8_LOSSLESSS_TRANSFORM	m_transform;

	VP8_LOSSLESS_DECODER()
		: m_data(NULL), m_data_length(0), m_transform(m_bit_reader)
	{

	}
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
public:
	VP8_LOSSLESS_DECODER(const uint8_t * const data, uint32_t data_length)
		: m_data(data), m_data_length(data_length), m_bit_reader(data, data_length), m_transform(m_bit_reader)
	{
		ReadInfo();

		while(m_bit_reader.ReadBit())
		{
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
