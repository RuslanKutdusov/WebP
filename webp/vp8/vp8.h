#ifndef VP8_H_
#define VP8_H_
#include "../platform.h"
#include "../exception/exception.h"
#include "../utils/bit_readed.h"
#include "../huffman/huffman.h"
#include "color_cache.h"
#include "huffman.h"

namespace webp
{
namespace vp8l
{

struct point
{
	uint8_t x;
	uint8_t y;
	point(){}
	point(uint8_t x, uint8_t y)
		: x(x), y(y)
	{

	}
};

static const uint32_t BORDER_DISTANCE_CODE = 120;
static const point dist_codes_diff[BORDER_DISTANCE_CODE] = {
		point(0, 1), point(1, 0),  point(1, 1),  point(-1, 1), point(0, 2),  point(2, 0),  point(1, 2),  point(-1, 2),
		point(2, 1),  point(-2, 1), point(2, 2),  point(-2, 2), point(0, 3),  point(3, 0),  point(1, 3),  point(-1, 3),
		point(3, 1),  point(-3, 1), point(2, 3),  point(-2, 3), point(3, 2),  point(-3, 2), point(0, 4),  point(4, 0),
		point(1, 4),  point(-1, 4), point(4, 1),  point(-4, 1), point(3, 3),  point(-3, 3), point(2, 4),  point(-2, 4),
		point(4, 2),  point(-4, 2), point(0, 5),  point(3, 4),  point(-3, 4), point(4, 3),  point(-4, 3), point(5, 0),
		point(1, 5),  point(-1, 5), point(5, 1),  point(-5, 1), point(2, 5),  point(-2, 5), point(5, 2),  point(-5, 2),
		point(4, 4),  point(-4, 4), point(3, 5),  point(-3, 5), point(5, 3),  point(-5, 3), point(0, 6),  point(6, 0),
		point(1, 6),  point(-1, 6), point(6, 1),  point(-6, 1), point(2, 6),  point(-2, 6), point(6, 2),  point(-6, 2),
		point(4, 5),  point(-4, 5), point(5, 4),  point(-5, 4), point(3, 6),  point(-3, 6), point(6, 3),  point(-6, 3),
		point(0, 7),  point(7, 0),  point(1, 7),  point(-1, 7), point(5, 5),  point(-5, 5), point(7, 1),  point(-7, 1),
		point(4, 6),  point(-4, 6), point(6, 4),  point(-6, 4), point(2, 7),  point(-2, 7), point(7, 2),  point(-7, 2),
		point(3, 7),  point(-3, 7), point(7, 3),  point(-7, 3), point(5, 6),  point(-5, 6), point(6, 5),  point(-6, 5),
		point(8, 0),  point(4, 7),  point(-4, 7), point(7, 4),  point(-7, 4), point(8, 1),  point(8, 2),  point(6, 6),
		point(-6, 6), point(8, 3),  point(5, 7),  point(-5, 7), point(7, 5),  point(-7, 5), point(8, 4),  point(6, 7),
		point(-6, 7), point(7, 6),  point(-7, 6), point(8, 5),  point(7, 7),  point(-7, 7), point(8, 6),  point(8, 7)};

class VP8_LOSSLESS_TRANSFORM
{
public:
	enum Type {
		  PREDICTOR_TRANSFORM			= 0,
		  COLOR_TRANSFORM				= 1,
		  SUBTRACT_GREEN				= 2,
		  COLOR_INDEXING_TRANSFORM		= 3,
		  TRANSFORM_NUMBER				= 4
	};
private:
	Type			m_type;
	//int				bits;
	int32_t				m_xsize;
	int32_t				m_ysize;
	uint32_t		m_data_length;
	uint32_t*		m_data;
public:
	VP8_LOSSLESS_TRANSFORM()
		: m_type(TRANSFORM_NUMBER), m_data(NULL)
	{

	}
	virtual ~VP8_LOSSLESS_TRANSFORM()
	{

	}
	void init_data(int32_t xsize, int32_t ysize)
	{
		m_xsize = xsize;
		m_ysize = ysize;
		m_data_length = xsize * ysize;
		m_data = new uint32_t[m_data_length];
	}
	const Type & type() const
	{
		return m_type;
	}
	uint32_t * data()
	{
		return m_data;
	}
	uint32_t data_length()
	{
		return m_data_length;
	}
};



class VP8_LOSSLESS_DECODER
{
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

	VP8_LOSSLESS_TRANSFORM	m_transform;

	//применные трансформации,
	//мл.бит в 1 - применен PREDICTOR_TRANSFORM, иначе  - еще не применялся
	//след бит в 1 - применен COLOR_TRANSFORM, иначе нет - еще не применялся
	//след бит в 1 - применен SUBTRACT_GREEN, иначе нет - еще не применялся
	//след бит в 1 - применен COLOR_INDEXING_TRANSFORM, иначе - еще не применялся
	int32_t					m_applied_transforms;
	VP8_LOSSLESS_TRANSFORM m_transforms[VP8_LOSSLESS_TRANSFORM::TRANSFORM_NUMBER];
	int32_t					m_next_transform;

	VP8_LOSSLESS_DECODER()
		: m_data(NULL), m_data_length(0)
	{

	}
	VP8_LOSSLESS_DECODER & operator=(const VP8_LOSSLESS_DECODER&)
	{
		return *this;
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
		char signature = (char)m_bit_reader.ReadBits(8);
		if (signature != '\x2F')
			throw exception::UnsupportedVP8();

		m_image_width    = m_bit_reader.ReadBits(14) + 1;
		m_image_height   = m_bit_reader.ReadBits(14) + 1;
		m_alpha_is_used  = m_bit_reader.ReadBits(1);
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
		VP8_LOSSLESS_TRANSFORM::Type transform_type = (VP8_LOSSLESS_TRANSFORM::Type)m_bit_reader.ReadBits(2);

		//проверяем, что мы не применяли эту трансформацию
		if (m_applied_transforms & (1U << transform_type))
			throw exception::InvalidVP8();
		m_applied_transforms |= (1U << transform_type);
		if (m_next_transform == VP8_LOSSLESS_TRANSFORM::TRANSFORM_NUMBER)
			throw exception::InvalidVP8();

		VP8_LOSSLESS_TRANSFORM & transform = m_transforms[m_next_transform++];


		switch (transform_type)
		{
			case(VP8_LOSSLESS_TRANSFORM::PREDICTOR_TRANSFORM):
				{
					int32_t size_bits = m_bit_reader.ReadBits(3) + 2;
					#define DIV_ROUND_UP(num, den) ((num) + (den) - 1) / (den)
					int32_t block_xsize = DIV_ROUND_UP(m_image_width, 1 << size_bits);
					int32_t block_ysize = DIV_ROUND_UP(m_image_height, 1 << size_bits);
					transform.init_data(block_xsize, block_ysize);
					ReadEntropyCodedImage(block_xsize, block_ysize, transform.data(), transform.data_length());
				}
				break;
			default:
				throw exception::InvalidVP8();
		}
	}
	void ReadEntropyCodedImage(const int32_t & xsize, const int32_t & ysize, uint32_t * data, const uint32_t & data_len)
	{
		uint32_t color_cache_bits = ReadColorCacheBits();
		VP8_LOSSLESS_COLOR_CACHE * color_cache = color_cache_bits == 0 ? NULL : new VP8_LOSSLESS_COLOR_CACHE(color_cache_bits);
		uint32_t color_cache_size = color_cache_bits == 0 ? 0 : 1 << color_cache_bits;

		VP8_LOSSLESS_HUFFMAN huffman(m_bit_reader, color_cache_size);
		ReadLZ77CodedImage(huffman, xsize, ysize, data, data_len, color_cache);

		if (color_cache != NULL)
			delete color_cache;
	}
	int32_t ReadColorCacheBits()
	{
		if (m_bit_reader.ReadBits(1) != 0)
		{
			int32_t color_cache_code_bits = m_bit_reader.ReadBits(4);
			if (color_cache_code_bits > MAX_COLOR_CACHE_BITS)
				throw exception::InvalidVP8();
			//return 1 << color_cache_code_bits;
			return color_cache_code_bits;
		}
		return 0;
	}
	uint32_t LZ77_prefix_coding_encode(const uint32_t & prefix_code)
	{
		if (prefix_code < 4)
			return prefix_code + 1;
		uint32_t extra_bits = (prefix_code - 2) >> 1;
		uint32_t offset = (2 + (prefix_code & 1)) << extra_bits;
		return offset + m_bit_reader.ReadBits(extra_bits) + 1;
	}
	uint32_t LZ77_distance_code2distance(const uint32_t & xsize, const uint32_t & lz77_distance_code)
	{
		if (lz77_distance_code <= BORDER_DISTANCE_CODE)
		{
			const point & diff = dist_codes_diff[lz77_distance_code - 1];
			uint32_t lz77_distance = diff.x + diff.y * xsize;
			lz77_distance = lz77_distance >= 1 ? lz77_distance : 1;
			return lz77_distance;
		}
		else
			return lz77_distance_code;
	}
	void ReadLZ77CodedImage(VP8_LOSSLESS_HUFFMAN & huffman, const uint32_t & xsize, const uint32_t & ysize, uint32_t * data,
								const uint32_t & data_len, VP8_LOSSLESS_COLOR_CACHE * color_cache)
	{
		uint32_t data_fills = 0;
		uint32_t last_cached = data_fills;
		uint32_t x = 0, y = 0;

		while(data_fills != data_len)
		{
			uint32_t S = huffman.read_symbol(GREEN);
			if (S < 256)
			{
				uint32_t red   = huffman.read_symbol(RED);
				uint32_t blue  = huffman.read_symbol(BLUE);
				uint32_t alpha = huffman.read_symbol(ALPHA);
				data[data_fills++] = (alpha << 24) + (red << 16) + (S << 8) + blue;
				x++;
				if (x >= xsize)
				{
					x = 0;
					y++;
					if (color_cache != NULL)
						while(last_cached < data_fills)
							color_cache->insert(data[last_cached++]);
				}
			}
			else if (S < 256 + 24)
			{
				uint32_t prefix_code = S - 256;
				uint32_t lz77_length = LZ77_prefix_coding_encode(prefix_code);
				prefix_code = huffman.read_symbol(DIST_PREFIX);
				uint32_t lz77_distance_code = LZ77_prefix_coding_encode(prefix_code);
				uint32_t lz77_distance = LZ77_distance_code2distance(xsize, lz77_distance_code);
				for(uint32_t i = 0; i < lz77_length; i++, data_fills++)
				{
					data[data_fills] = data[data_fills - lz77_distance];
					x++;
					if (x >= xsize)
					{
						x = 0;
						y++;
					}
				}
				/*x += lz77_length;
				while (x >= xsize)
				{
					x -= xsize;
					y++;
				}*/
				if (color_cache != NULL)
					while(last_cached < data_fills)
						color_cache->insert(data[last_cached++]);
			}
			else
			{
				uint32_t color_cache_key = S - 256 - 24;
				while(last_cached < data_fills)
					color_cache->insert(data[last_cached++]);
				data[data_fills++] = color_cache->get(color_cache_key);
				x++;
				if (x >= xsize)
				{
					x = 0;
					y++;
					if (color_cache != NULL)
						while(last_cached < data_fills)
							color_cache->insert(data[last_cached++]);
				}
			}
		}
	}
public:
	VP8_LOSSLESS_DECODER(const uint8_t * const data, uint32_t data_length)
		: m_data(data), m_data_length(data_length), m_bit_reader(data, data_length),
		  m_applied_transforms(0), m_next_transform(0)
	{
		ReadInfo();

		while(m_bit_reader.ReadBits(1))
		{
			ReadTransform();
		}
	}
	virtual ~VP8_LOSSLESS_DECODER()
	{

	}
	friend class VP8_LOSSLESS_TRANSFORM;
};

}
}

#endif /* VP8_H_ */
