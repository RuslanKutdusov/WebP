#ifndef VP8_H_
#define VP8_H_
#include "../platform.h"
#include "../exception/exception.h"
#include "../utils/bit_readed.h"
#include "../huffman/huffman.h"
#include "color_cache.h"
#include "huffman.h"
#include <openssl/sha.h>
#include <png.h>

#define DIV_ROUND_UP(num, den) ((num) + (den) - 1) / (den)

namespace webp
{
namespace vp8l
{

struct point
{
	int8_t x;
	int8_t y;
	point(){}
	point(int8_t x, int8_t y)
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


static uint32_t Average2(const uint32_t & a, const uint32_t & b)
{
	uint32_t ret;
	uint8_t * byte_a = (uint8_t*)&a;
	uint8_t * byte_b = (uint8_t*)&b;
	uint8_t * byte_r = (uint8_t*)&ret;
	for(size_t i = 0; i < 4; i++)
		byte_r[i] = (byte_a[i] + byte_b[i]) / 2;
	return ret;
}

static uint32_t Select(uint32_t L, uint32_t T, uint32_t TL)
{
  // L = left pixel, T = top pixel, TL = top left pixel.

  // ARGB component estimates for prediction.
  int32_t pAlpha = *utils::ALPHA(L) + *utils::ALPHA(T) - *utils::ALPHA(TL);
  int32_t pRed = *utils::RED(L) + *utils::RED(T) - *utils::RED(TL);
  int32_t pGreen = *utils::GREEN(L) + *utils::GREEN(T) - *utils::GREEN(TL);
  int32_t pBlue = *utils::BLUE(L) + *utils::BLUE(T) - *utils::BLUE(TL);

  // Manhattan distances to estimates for left and top pixels.
  int32_t pL = abs(pAlpha - *utils::ALPHA(L)) + abs(pRed - *utils::RED(L)) +
           abs(pGreen - *utils::GREEN(L)) + abs(pBlue - *utils::BLUE(L));
  int32_t pT = abs(pAlpha - *utils::ALPHA(T)) + abs(pRed - *utils::RED(T)) +
           abs(pGreen - *utils::GREEN(T)) + abs(pBlue - *utils::BLUE(T));

  // Return either left or top, the one closer to the prediction.
  if (pL <= pT)
    return L;
  else
    return T;
}

static int32_t Clamp(const int32_t & a)
{
  return (a < 0) ? 0 : (a > 255) ?  255 : a;
}

static uint32_t ClampAddSubtractFull(const uint32_t & a, const uint32_t & b, const uint32_t & c)
{
	uint32_t ret;
	for(size_t i = 0; i < 4; i++)
	{
		uint8_t * byte_a = utils::COLOR[i](a);
		uint8_t * byte_b = utils::COLOR[i](b);
		uint8_t * byte_c = utils::COLOR[i](c);
		uint8_t * byte_ret = utils::COLOR[i](ret);
		*byte_ret = Clamp(*byte_a + *byte_b - *byte_c);
	}
	return ret;
}

static uint32_t ClampAddSubtractHalf(const uint32_t & a, const uint32_t & b)
{
	uint32_t ret;
	for(size_t i = 0; i < 4; i++)
	{
		uint8_t * byte_a = utils::COLOR[i](a);
		uint8_t * byte_b = utils::COLOR[i](b);
		uint8_t * byte_ret = utils::COLOR[i](ret);
		*byte_ret = Clamp(*byte_a + (*byte_a - *byte_b) / 2);
	}
	return ret;
}

static void PNGAPI error_function(png_structp png, png_const_charp dummy) {
  (void)dummy;  // remove variable-unused warning
  longjmp(png_jmpbuf(png), 1);
}

struct ColorTransformElement
{
	uint8_t green_to_red;
	uint8_t green_to_blue;
	uint8_t red_to_blue;
	ColorTransformElement(const uint32_t & argb)
	{
		red_to_blue = *utils::RED(argb);
		green_to_blue = *utils::GREEN(argb);
		green_to_red = *utils::BLUE(argb);
	}
};

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
	Type					m_type;
	uint32_t				m_xsize;
	uint32_t				m_ysize;
	uint32_t				m_bits;
	utils::array<uint32_t>	m_data;
public:
	VP8_LOSSLESS_TRANSFORM()
		: m_type(TRANSFORM_NUMBER)
	{

	}
	virtual ~VP8_LOSSLESS_TRANSFORM()
	{

	}
	void init_data(uint32_t xsize, uint32_t ysize, uint32_t bits)
	{
		m_xsize = xsize;
		m_ysize = ysize;
		m_bits = bits;
		m_data.realloc(xsize * ysize);
	}
	const Type & type() const
	{
		return m_type;
	}
	const uint32_t & bits() const
	{
		return m_bits;
	}
	const uint32_t & xsize() const
	{
		return m_xsize;
	}
	const uint32_t & ysize() const
	{
		return m_ysize;
	}
	utils::array<uint32_t>& data()
	{
		return m_data;
	}
	uint32_t data_length()
	{
		return m_data.size();
	}
};



class VP8_LOSSLESS_DECODER
{
private:
	struct MetaHuffmanInfo
	{
		//VP8_LOSSLESS_HUFFMAN ** meta_huffmans;
		std::vector<VP8_LOSSLESS_HUFFMAN> meta_huffmans;
		uint32_t huffman_bits;
		uint32_t huffman_xsize;
		uint32_t meta_huffman_codes_num;
		utils::array<uint32_t> entropy_image;
		MetaHuffmanInfo()
			: huffman_bits(0), huffman_xsize(0), meta_huffman_codes_num(1)
		{

		}
	};
private:
	//прочитано из заголовка vp8l
	uint32_t				m_lossless_stream_length;
	uint32_t					m_image_width;
	uint32_t					m_image_height;
	int32_t					m_alpha_is_used;
	int32_t					m_version_number;
	//часто приходится читать m_data по битам
	utils::BitReader		m_bit_reader;

	//применные трансформации
	std::map<VP8_LOSSLESS_TRANSFORM::Type, VP8_LOSSLESS_TRANSFORM> m_transforms;
	std::list<VP8_LOSSLESS_TRANSFORM::Type> m_transforms_order;

	utils::array<uint32_t>	m_decoded_data;

	//ширина ARGB-изображения в последнем lz77-coded image, в случае если есть Color indexing trasformation, т.е "палитра",
	//может отличаться от ширины изображения, которые мы декодируем, если кол-во цветом в палитре <=16.
	//тогда один байт в строке может содержать индексы для 2,4,8 пикселей.
	//если m_color_indexing_xsize == 0, тогда "палитры" нет, и ширина ARGB-изображения в последнем lz77-coded image равна
	//ширине изображения, которые мы декодируем,
	//иначе m_color_indexing_xsize и есть эта ширина
	uint32_t					m_color_indexing_xsize;

	VP8_LOSSLESS_DECODER()
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

		std::map<VP8_LOSSLESS_TRANSFORM::Type, VP8_LOSSLESS_TRANSFORM>::iterator iter = m_transforms.find(transform_type);
		if (iter != m_transforms.end())
			throw exception::InvalidVP8L();

		m_transforms_order.push_front(transform_type);


		VP8_LOSSLESS_TRANSFORM transform;

		switch (transform_type)
		{
			case(VP8_LOSSLESS_TRANSFORM::PREDICTOR_TRANSFORM):
			case(VP8_LOSSLESS_TRANSFORM::COLOR_TRANSFORM):
				{
					uint32_t size_bits = m_bit_reader.ReadBits(3) + 2;
					uint32_t block_xsize = DIV_ROUND_UP(m_image_width, 1 << size_bits);
					uint32_t block_ysize = DIV_ROUND_UP(m_image_height, 1 << size_bits);
					transform.init_data(block_xsize, block_ysize, size_bits);
					ReadEntropyCodedImage(block_xsize, block_ysize, transform.data());
				}
				break;
			case(VP8_LOSSLESS_TRANSFORM::COLOR_INDEXING_TRANSFORM):
				{
					//кол-во цветов в палитре
					uint32_t num_colors = m_bit_reader.ReadBits(8) + 1;
					//Показывает, сколько пикселей объединены, см. описание m_color_indexing_xsize
					uint32_t bits = (num_colors > 16) ? 0 //пиксели не объединены
								  : (num_colors > 4) ? 1 //2 пикселя объединены, индексы в пределах [0..15]
								  : (num_colors > 2) ? 2 //4 пикселя объединены, индексы в пределах [0..3]
								  : 3;//8 пикселей объединены, индексы в пределах [0..1]
					m_color_indexing_xsize = DIV_ROUND_UP(m_image_width, 1 << bits);
					//"палитра" - одномерный массив
					transform.init_data(num_colors, 1, bits);
					//в transform.data() будет находится "палитра"
					ReadEntropyCodedImage(num_colors, 1, transform.data());
					//"палитра" закодирована с помощью subraction-coding для уменьшения энтропии.
					//чтобы ее восстановить, надо каждый цвет покомпонентно сложить с предыдущим
					uint8_t * color_map_bytes = (uint8_t*)&transform.data()[0];
					for(size_t i = 4; i < 4 * num_colors; i++)
						color_map_bytes[i] = (color_map_bytes[i] + color_map_bytes[i - 4]) & 0xff;
				}
				break;
			case(VP8_LOSSLESS_TRANSFORM::SUBTRACT_GREEN):
				break;
			default:
				throw exception::InvalidVP8L();
		}
		m_transforms.insert(std::make_pair(transform_type, transform));
	}
	/*
	 * ReadEntropyCodedImage()
	 * Бросает исключения: InvalidVP8, InvalidHuffman
	 * Назначение:
	 * Читает и декодирует entropy-coded image
	 */
	void ReadEntropyCodedImage(const uint32_t & xsize, const uint32_t & ysize, utils::array<uint32_t> & data)
	{
		uint32_t color_cache_bits =	ReadColorCacheBits();
		VP8_LOSSLESS_COLOR_CACHE color_cache(color_cache_bits);
		uint32_t color_cache_size = color_cache_bits == 0 ? 0 :1 << color_cache_bits;

		MetaHuffmanInfo meta_huffman_info;

		//восстанавливаем дерево Хаффмана
		meta_huffman_info.meta_huffmans.push_back(VP8_LOSSLESS_HUFFMAN(&m_bit_reader, color_cache_size));
		//декодируем entropy-coded image
		ReadLZ77CodedImage(meta_huffman_info, xsize, ysize, data, color_cache);
	}
	/*
	 * ReadSpatiallyCodedImage()
	 * Бросает исключения: InvalidVP8, InvalidHuffman
	 * Назначение:
	 * Читает и декодирует spatially coded image
	 */
	void ReadSpatiallyCodedImage()
	{
		uint32_t color_cache_bits =	ReadColorCacheBits();
		VP8_LOSSLESS_COLOR_CACHE color_cache(color_cache_bits);
		uint32_t color_cache_size = color_cache_bits == 0 ? 0 :1 << color_cache_bits;

		uint32_t use_meta_huffman_codes = m_bit_reader.ReadBits(1);
		uint32_t entropy_image_size = 0;
		//Кол-во мета кодов Хаффмана в наборе,
		//если имеется meta-huffman, то кол-во мета кодов отлично от 1
		MetaHuffmanInfo meta_huffman_info;
		if (use_meta_huffman_codes)
		{
			//декодируем meta huffman
			uint32_t huffman_bits = m_bit_reader.ReadBits(3) + 2;
			uint32_t huffman_xsize = DIV_ROUND_UP(m_image_width, 1 << huffman_bits);
			uint32_t huffman_ysize = DIV_ROUND_UP(m_image_height, 1 << huffman_bits);
			entropy_image_size  = huffman_xsize * huffman_ysize;
			meta_huffman_info.entropy_image.realloc(entropy_image_size);
			ReadEntropyCodedImage(huffman_xsize, huffman_ysize, meta_huffman_info.entropy_image);

			meta_huffman_info.huffman_xsize = huffman_xsize;
			meta_huffman_info.huffman_bits = huffman_bits;

			//определяем кол-во мета кодов Хаффмана
			for (uint32_t i = 0; i < entropy_image_size; ++i)
			{
				//Индекс мета кода Хаффмана содержится в красной и зеленой компоненте
				//вытаскиваем его их красной и зеленой компоненты
				uint32_t meta_huffman_code_index = (meta_huffman_info.entropy_image[i] >> 8) & 0xffff;
				//и кладем как есть обратно, чтобы в след раз нам не пришлось делать сдвиги
				meta_huffman_info.entropy_image[i] = meta_huffman_code_index;
				if (meta_huffman_code_index >= meta_huffman_info.meta_huffman_codes_num)
					meta_huffman_info.meta_huffman_codes_num = meta_huffman_code_index + 1;
			}
		}

		for(uint32_t i = 0; i < meta_huffman_info.meta_huffman_codes_num; i++)
			meta_huffman_info.meta_huffmans.push_back(VP8_LOSSLESS_HUFFMAN(&m_bit_reader, color_cache_size));

		//см описание m_color_indexing_xsize
		uint32_t xsize =  m_color_indexing_xsize == 0 ? m_image_width : m_color_indexing_xsize;
		ReadLZ77CodedImage(meta_huffman_info, xsize, m_image_height, m_decoded_data, color_cache);
		SHA_CTX c;
		SHA1_Init(&c);
		SHA1_Update(&c, &m_decoded_data[0], m_decoded_data.size());
		unsigned char sha1[20];
		SHA1_Final(sha1, &c);
		for(int i = 0; i < 20; i++)
			printf("%02X", sha1[i]);
		printf("\n");
	}
	/*
	 * ReadColorCacheInfo()
	 * Бросает исключения: InvalidVP8
	 * Назначение:
	 * Читает размер color cache и при необходимости инициализирует его
	 */
	uint32_t ReadColorCacheBits()
	{
		//если бит установлен, то имеется color cache и читаем его размер
		if (m_bit_reader.ReadBits(1) != 0)
		{
			uint32_t color_cache_code_bits = m_bit_reader.ReadBits(4);
			if (color_cache_code_bits > MAX_COLOR_CACHE_BITS)
				throw exception::InvalidVP8L();
			return color_cache_code_bits;
		}
		return 0;
	}
	/*
	 * LZ77_prefix_coding_encode
	 * Бросает исключения: нет
	 * Назначение:
	 * длины совпадения и смещения LZ77 закодированы, эта функция декодирует их значения
	 */
	uint32_t LZ77_prefix_coding_encode(const uint32_t & prefix_code)
	{
		if (prefix_code < 4)
			return prefix_code + 1;
		uint32_t extra_bits = (prefix_code - 2) >> 1;
		uint32_t offset = (2 + (prefix_code & 1)) << extra_bits;
		return offset + m_bit_reader.ReadBits(extra_bits) + 1;
	}
	/*
	 * LZ77_distance_code2distance
	 * Бросает исключения: нет
	 * Назначение:
	 * длины совпадения LZ77 меньшие 120 закодированы, эта функция декодирует их значения
	 */
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
			return lz77_distance_code - BORDER_DISTANCE_CODE;
	}
	/*
	 * SelectMetaHuffman
	 * Бросает исключения: нет
	 * Назначение:
	 * если имеется meta huffman, то этот метод для заданного пикселя ARGB изображения определяет каким мета кодом Хаффмана из набора
	 * он закодирован
	 * если meta huffaman нет, то все пиксели ARGB изображения закодированы одним мета кодом Хаффмана
	 */
	uint32_t SelectMetaHuffman(const MetaHuffmanInfo & meta_huffman_info, uint32_t x, uint32_t y)
	{
		if (meta_huffman_info.meta_huffman_codes_num == 1)
			return 0;
		uint32_t index = (y >> meta_huffman_info.huffman_bits) * meta_huffman_info.huffman_xsize + (x >> meta_huffman_info.huffman_bits);
		return meta_huffman_info.entropy_image[index];
	}
	/*
	 * ReadLZ77CodedImage
	 * Бросает исключения: нет
	 * Назначение:
	 * читает и декодирует lz77 coded image
	 */
	void ReadLZ77CodedImage(const MetaHuffmanInfo & meta_huffman_info, const uint32_t & xsize, const uint32_t & ysize, utils::array<uint32_t> & data,
								VP8_LOSSLESS_COLOR_CACHE & color_cache)
	{
		uint32_t data_fills = 0;
		uint32_t last_cached = data_fills;
		uint32_t x = 0, y = 0;
		while(data_fills != xsize * ysize)//data.size())
		{
			const VP8_LOSSLESS_HUFFMAN & huffman = meta_huffman_info.meta_huffmans[SelectMetaHuffman(meta_huffman_info, x, y)];
			int32_t S = huffman.read_symbol(GREEN);
			//если прочитанное значение меньше 256, значит это значение зеленой компоненты цвета пикселя, а дальше идут красная,
			//синяя компонента и альфа, все эти значения пакуем в data
			if (S < 256)
			{
				int32_t red   = huffman.read_symbol(RED);
				int32_t blue  = huffman.read_symbol(BLUE);
				int32_t alpha = huffman.read_symbol(ALPHA);
				data[data_fills++] = (alpha << 24) + (red << 16) + (S << 8) + blue;
				x++;
				if (x >= xsize)
				{
					x = 0;
					y++;
					if (color_cache.is_presented())
						while(last_cached < data_fills)
							color_cache.insert(data[last_cached++]);
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
				if (color_cache.is_presented())
					while(last_cached < data_fills)
						color_cache.insert(data[last_cached++]);
			}
			else
			{
				uint32_t color_cache_key = S - 256 - 24;
				while(last_cached < data_fills)
					color_cache.insert(data[last_cached++]);
				data[data_fills++] = color_cache.get(color_cache_key);
				x++;
				if (x >= xsize)
				{
					x = 0;
					y++;
					while(last_cached < data_fills)
						color_cache.insert(data[last_cached++]);
				}
			}
		}
	}
	/*
	 * InverseColorIndexingTransform
	 * Бросает исключения: нет
	 * Назначение:
	 * инвертирует color indexing tranform если имеется
	 */
	void InverseColorIndexingTransform()
	{
		//инвертируем трансформацию если она есть
		if (m_color_indexing_xsize != 0)
		{
			//в m_decoded_data находятся индексы, скопируем их, т.к в m_decoded_data будем формировать конечное изображение
			utils::array<uint32_t> indices = m_decoded_data;
			VP8_LOSSLESS_TRANSFORM & transform = m_transforms[VP8_LOSSLESS_TRANSFORM::COLOR_INDEXING_TRANSFORM];
			//кол-во пикселей(по сути индексов) в одном байте indices
			size_t pixels_per_byte = 1 << transform.bits();
			//сколько бит приходится на один пиксель(индекс)
			size_t bits_per_pixel = 8 >> transform.bits();
			size_t mask = (1 << bits_per_pixel) - 1;
			//пробегаемся по всем строкам
			for(size_t y = 0; y < m_image_height; y++)
			{
				//берем указатель на строку в конечном изображении
				uint32_t * row_iter = &m_decoded_data[y * m_image_width];
				//пробегаемся по всем байтам indices
				for(size_t x = 0; x < m_color_indexing_xsize; x++)
					//пробегаемся по всем индексам в байте
					for(size_t i = 0; i < pixels_per_byte; i++)
					{
						//вытаскиваем индекс
						uint32_t color_table_index = (*utils::GREEN(indices[y * m_color_indexing_xsize + x]) >> (i * bits_per_pixel)) & mask;
						//вытаскиваем цвет из палитры по индексу
						*row_iter = transform.data()[color_table_index];
						//сдвигаемся к следущему пикселю в строке
						row_iter++;
					}
			}
		}
	}
	/*
	 * InverseSubstractGreenTransform
	 * Бросает исключения: нет
	 * Назначение:
	 * инвертирует subtract green tranform если имеется
	 */
	void InverseSubstractGreenTransform()
	{
		if (m_transforms.find(VP8_LOSSLESS_TRANSFORM::SUBTRACT_GREEN) == m_transforms.end())
			return;
		for(size_t y = 0; y < m_image_height; y++)
			for(size_t x = 0; x < m_image_width; x++)
			{
				uint32_t i = y * m_image_width + x;
				*utils::RED(m_decoded_data[i])  = (*utils::RED(m_decoded_data[i])  + *utils::GREEN(m_decoded_data[i])) & 0xff;
				*utils::BLUE(m_decoded_data[i]) = (*utils::BLUE(m_decoded_data[i]) + *utils::GREEN(m_decoded_data[i])) & 0xff;
			}
	}
	/*
	 * InversePredictorTransform
	 * Бросает исключения: нет
	 * Назначение:
	 * инвертирует subtract green tranform если имеется
	 */
	void AddPixelsEq(uint32_t* a, uint32_t b) {
	  const uint32_t alpha_and_green = (*a & 0xff00ff00u) + (b & 0xff00ff00u);
	  const uint32_t red_and_blue = (*a & 0x00ff00ffu) + (b & 0x00ff00ffu);
	  *a = (alpha_and_green & 0xff00ff00u) | (red_and_blue & 0x00ff00ffu);
	}
	void InversePredictorTransform()
	{
		if (m_transforms.find(VP8_LOSSLESS_TRANSFORM::PREDICTOR_TRANSFORM) == m_transforms.end())
			return;
		VP8_LOSSLESS_TRANSFORM & tranform =  m_transforms[VP8_LOSSLESS_TRANSFORM::PREDICTOR_TRANSFORM];
		for(size_t y = 0; y < m_image_height; y++)
			for(size_t x = 0; x < m_image_width; x++)
			{
				size_t i = y * m_image_width + x;
				uint32_t P;//&m_decoded_data[i];
				if (x == 0 && y == 0)
				{
					P = 0xff000000;
					AddPixelsEq(&m_decoded_data[i], P);
					continue;
				}
				if (x == 0)
				{
					P = m_decoded_data[i - m_image_width];
					AddPixelsEq(&m_decoded_data[i], P);
					continue;
				}
				if (y == 0)
				{
					P = m_decoded_data[i - 1];
					AddPixelsEq(&m_decoded_data[i], P);
					continue;
				}
				uint32_t L = m_decoded_data[i - 1];
				uint32_t T = m_decoded_data[i - m_image_width];
				uint32_t TR = (x == m_image_width - 1) ? L : m_decoded_data[i - m_image_width + 1];
				uint32_t TL = m_decoded_data[i - m_image_width - 1];
				int block_index = (y >> tranform.bits()) * tranform.xsize() + (x >> tranform.bits());
				uint32_t mode = *utils::GREEN(tranform.data()[block_index]);
				switch (mode) {
					case 0:
						P = 0xff000000;
						break;
					case 1:
						P = L;
						break;
					case 2:
						P = T;
						break;
					case 3:
						P = TR;
						break;
					case 4:
						P = TL;
						break;
					case 5:
						P = Average2(Average2(L, TR), T);
						break;
					case 6:
						P = Average2(L, TL);
						break;
					case 7:
						P = Average2(L, T);
						break;
					case 8:
						P = Average2(TL, T);
						break;
					case 9:
						P = Average2(T, TR);
						break;
					case 10:
						P = Average2(Average2(L, TL), Average2(T, TR));
						break;
					case 11:
						//Select(top[0], left, top[-1]);
						P = Select(T, L, TL);//Select(L, T, TL); - косяк документации?
						break;
					case 12:
						P = ClampAddSubtractFull(L, T, TL);
						break;
					case 13:
						P = ClampAddSubtractHalf(Average2(L, T), TL);
						break;
					default:
						throw exception::InvalidVP8L();
						break;
				}
				AddPixelsEq(&m_decoded_data[i], P);
			}
	}
	int8_t ColorTransformDelta(int8_t t, int8_t c)
	{
		return (t * c) >> 5;
	}
	void InverseColorTransform()
	{
		if (m_transforms.find(VP8_LOSSLESS_TRANSFORM::COLOR_TRANSFORM) == m_transforms.end())
			return;
		VP8_LOSSLESS_TRANSFORM & tranform =  m_transforms[VP8_LOSSLESS_TRANSFORM::COLOR_TRANSFORM];
		for(size_t y = 0; y < m_image_height; y++)
			for(size_t x = 0; x < m_image_width; x++)
			{
				size_t i = y * m_image_width + x;
				int block_index = (y >> tranform.bits()) * tranform.xsize() + (x >> tranform.bits());
				uint32_t data = tranform.data()[block_index];
				ColorTransformElement cte(data);

				uint8_t red = *utils::RED(m_decoded_data[i]);
				uint8_t green = *utils::GREEN(m_decoded_data[i]);
				uint8_t blue = *utils::BLUE(m_decoded_data[i]);

				//еще один косяк документации, в написано, что надо вычитать
				red  += ColorTransformDelta(cte.green_to_red,  green);
				blue += ColorTransformDelta(cte.green_to_blue, green);
				blue += ColorTransformDelta(cte.red_to_blue, red & 0xff);

				*utils::RED(m_decoded_data[i]) = red & 0xff;
				*utils::BLUE(m_decoded_data[i]) = blue & 0xff;
			}
	}
public:
	VP8_LOSSLESS_DECODER(const uint8_t * const data, uint32_t data_length)
		: m_bit_reader(data, data_length)
	{
		ReadInfo();

		m_decoded_data.realloc(m_image_width * m_image_height);
		m_decoded_data.fill(0);
		m_color_indexing_xsize = 0;

		while(m_bit_reader.ReadBits(1))
		{
			ReadTransform();
		}
		//color predictor subtract-green
		ReadSpatiallyCodedImage();

		for(std::list<VP8_LOSSLESS_TRANSFORM::Type>::iterator iter = m_transforms_order.begin(); iter != m_transforms_order.end(); ++iter)
		{
			if (*iter == VP8_LOSSLESS_TRANSFORM::PREDICTOR_TRANSFORM)
				InversePredictorTransform();
			if (*iter == VP8_LOSSLESS_TRANSFORM::COLOR_TRANSFORM)
				InverseColorTransform();
			if (*iter == VP8_LOSSLESS_TRANSFORM::COLOR_INDEXING_TRANSFORM)
				InverseColorIndexingTransform();
			if (*iter == VP8_LOSSLESS_TRANSFORM::SUBTRACT_GREEN)
				InverseSubstractGreenTransform();
		}

		int i =0;
		uint8_t * rgb = new uint8_t[m_image_height * m_image_width * 3];
				for(size_t y = 0; y < m_image_height; y++)
					for(size_t x = 0; x < m_image_width; x++)
					{
						size_t j = y * m_image_width + x;
						rgb[i++] = (m_decoded_data[j] >> 16) & 0x000000ff;
						rgb[i++] = (m_decoded_data[j] >> 8) & 0x000000ff;
						rgb[i++] = (m_decoded_data[j] >> 0) & 0x000000ff;
					}

				  png_structp png;
				  png_infop info;
				  png_uint_32 y;

				  png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
				                                NULL, error_function, NULL);

				  info = png_create_info_struct(png);

				  setjmp(png_jmpbuf(png));
				  FILE * out_file = fopen("1.png", "wb");
				  png_init_io(png, out_file);
				  png_set_IHDR(png, info, m_image_width, m_image_height, 8,
				               PNG_COLOR_TYPE_RGB,
				               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
				               PNG_FILTER_TYPE_DEFAULT);
				  png_write_info(png, info);
				  for (y = 0; y < m_image_height; ++y) {
				    png_bytep row = rgb + y * m_image_width * 3;
				    png_write_rows(png, &row, 1);
				  }
				  png_write_end(png, info);
				  png_destroy_write_struct(&png, &info);
				  delete[] rgb;
	}
	virtual ~VP8_LOSSLESS_DECODER()
	{

	}
	friend class VP8_LOSSLESS_TRANSFORM;
};

}
}

#endif /* VP8_H_ */
