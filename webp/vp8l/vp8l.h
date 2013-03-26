#ifndef VP8_H_
#define VP8_H_
#include "../platform.h"
#include "../exception/exception.h"
#include "../utils/bit_readed.h"
#include "color_cache.h"
#include "transform.h"
#include "huffman_io.h"
#include "../utils/bit_writer.h"
#include "../lz77/lz77.h"
//#include <openssl/sha.h>
#include <png.h>
#include <set>
#include <math.h>


namespace webp
{
namespace vp8l
{


#define PALLETE_MAX_COLORS 256
#define LZ77_MAX_DISTANCE 1024
#define LZ77_MAX_LENGTH 128




class VP8_LOSSLESS_DECODER
{
private:
	struct MetaHuffmanInfo
	{
		//VP8_LOSSLESS_HUFFMAN ** meta_huffmans;
		std::vector<huffman_io::dec::VP8_LOSSLESS_HUFFMAN> meta_huffmans;
		uint32_t huffman_bits;
		uint32_t huffman_xsize;
		uint32_t meta_huffman_codes_num;
		utils::pixel_array entropy_image;
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


		VP8_LOSSLESS_TRANSFORM transform(transform_type);

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
	void ReadEntropyCodedImage(const uint32_t & xsize, const uint32_t & ysize, utils::pixel_array & data)
	{
		uint32_t color_cache_bits =	ReadColorCacheBits();
		VP8_LOSSLESS_COLOR_CACHE color_cache(color_cache_bits);
		uint32_t color_cache_size = color_cache_bits == 0 ? 0 :1 << color_cache_bits;

		MetaHuffmanInfo meta_huffman_info;

		//восстанавливаем дерево Хаффмана
		meta_huffman_info.meta_huffmans.push_back(huffman_io::dec::VP8_LOSSLESS_HUFFMAN(&m_bit_reader, color_cache_size));
		//декодируем entropy-coded image
		ReadLZ77CodedImage(meta_huffman_info, xsize, ysize, data, color_cache);
	}
	/*
	 * ReadSpatiallyCodedImage()
	 * Бросает исключения: InvalidVP8, InvalidHuffman
	 * Назначение:
	 * Читает и декодирует spatially coded image
	 */
	void ReadSpatiallyCodedImage(utils::pixel_array & argb_image)
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
			meta_huffman_info.meta_huffmans.push_back(huffman_io::dec::VP8_LOSSLESS_HUFFMAN(&m_bit_reader, color_cache_size));

		//см описание m_color_indexing_xsize
		uint32_t xsize =  m_color_indexing_xsize == 0 ? m_image_width : m_color_indexing_xsize;
		ReadLZ77CodedImage(meta_huffman_info, xsize, m_image_height, argb_image, color_cache);
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
	void ReadLZ77CodedImage(const MetaHuffmanInfo & meta_huffman_info, const uint32_t & xsize, const uint32_t & ysize, utils::pixel_array & data,
								VP8_LOSSLESS_COLOR_CACHE & color_cache)
	{
		uint32_t data_fills = 0;
		uint32_t last_cached = data_fills;
		uint32_t x = 0, y = 0;
		while(data_fills != xsize * ysize)
		{
			const huffman_io::dec::VP8_LOSSLESS_HUFFMAN & huffman = meta_huffman_info.meta_huffmans[SelectMetaHuffman(meta_huffman_info, x, y)];
			int32_t S = huffman.read_symbol(huffman_io::GREEN);
			//если прочитанное значение меньше 256, значит это значение зеленой компоненты цвета пикселя, а дальше идут красная,
			//синяя компонента и альфа, все эти значения пакуем в data
			if (S < 256)
			{
				int32_t red   = huffman.read_symbol(huffman_io::RED);
				int32_t blue  = huffman.read_symbol(huffman_io::BLUE);
				int32_t alpha = huffman.read_symbol(huffman_io::ALPHA);
				printf("%08X\n",  (alpha << 24) + (red << 16) + (S << 8) + blue);
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
				uint32_t lz77_length = lz77::prefix_coding_decode(prefix_code, m_bit_reader);
				prefix_code = huffman.read_symbol(huffman_io::DIST_PREFIX);
				uint32_t lz77_distance_code = lz77::prefix_coding_decode(prefix_code, m_bit_reader);
				uint32_t lz77_distance = lz77::distance_code2distance(xsize, lz77_distance_code);
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
public:
	VP8_LOSSLESS_DECODER(const uint8_t * const data, uint32_t data_length, size_t x, size_t y, utils::pixel_array & p)
		: m_bit_reader(data, data_length)
	{
		ReadEntropyCodedImage(x, y, p);
	}
	VP8_LOSSLESS_DECODER(const uint8_t * const data, uint32_t data_length, utils::pixel_array & argb_image)
		: m_bit_reader(data, data_length)
	{
		ReadInfo();

		argb_image.realloc(m_image_width * m_image_height);
		argb_image.fill(0);
		m_color_indexing_xsize = 0;

		while(m_bit_reader.ReadBits(1))
			ReadTransform();
		ReadSpatiallyCodedImage(argb_image);

		for(std::list<VP8_LOSSLESS_TRANSFORM::Type>::iterator iter = m_transforms_order.begin(); iter != m_transforms_order.end(); ++iter)
			m_transforms[*iter].inverse(argb_image, m_image_width, m_image_height);
	}
	const uint32_t image_width(){
		return m_image_width;
	}
	const uint32_t image_height(){
		return m_image_height;
	}
	virtual ~VP8_LOSSLESS_DECODER()	{
	}
};

class VP8_LOSSLESS_ENCODER
{
public:
	typedef std::set<uint32_t> pallete_t;
	struct entropy_t{
		float 	red_p[256];
		float	blue_p[256];
		float	green_p[256];
		float	alpha_p[256];
		float	entropy;
	};
	utils::BitWriter m_bit_writer;
	VP8_LOSSLESS_ENCODER()
	{

	}
	void CreatePallete(const utils::pixel_array & argb_image, pallete_t & pallete) const{
		for(size_t i = 0; i < argb_image.size(); i++){
			pallete.insert(argb_image[i]);
			if (pallete.size() > PALLETE_MAX_COLORS){
				pallete.clear();
				return;
			}
		}
	}
	void AnalyzeEntropy(const utils::pixel_array & argb_image, entropy_t & entropy) const {
		memset(&entropy, 0, sizeof(entropy));
		for(size_t i = 0; i < argb_image.size(); i++){
			++entropy.red_p[*utils::RED(argb_image[i])];
			++entropy.green_p[*utils::GREEN(argb_image[i])];
			++entropy.blue_p[*utils::BLUE(argb_image[i])];
			++entropy.alpha_p[*utils::ALPHA(argb_image[i])];
		}
		for(size_t i = 0; i < 256; i++){
			if (entropy.red_p[i] != 0){
				entropy.red_p[i]   /= argb_image.size();
				entropy.entropy -= entropy.red_p[i] * log2f(entropy.red_p[i]);
			}

			if (entropy.green_p[i] != 0){
				entropy.green_p[i] /= argb_image.size();
				entropy.entropy -= entropy.green_p[i] * log2f(entropy.green_p[i]);
			}

			if (entropy.blue_p[i] != 0){
				entropy.blue_p[i]  /= argb_image.size();
				entropy.entropy -= entropy.blue_p[i] * log2f(entropy.blue_p[i]);
			}

			if (entropy.alpha_p[i] != 0){
				entropy.alpha_p[i] /= argb_image.size();
				entropy.entropy -= entropy.alpha_p[i] * log2f(entropy.alpha_p[i]);
			}
		}
	}
	void ApplySubtractGreenTransform(utils::pixel_array & argb_image){
		for(size_t i = 0; i < argb_image.size(); i++){
			*utils::RED(argb_image[i])  = (*utils::RED(argb_image[i])  - *utils::GREEN(argb_image[i])) & 0xff;
			*utils::BLUE(argb_image[i]) = (*utils::BLUE(argb_image[i]) - *utils::GREEN(argb_image[i])) & 0xff;
		}
		m_bit_writer.WriteBit(1);//transform present
		m_bit_writer.WriteBits(VP8_LOSSLESS_TRANSFORM::SUBTRACT_GREEN, 2);
	}
	void ApplyColorIndexingTransform(const pallete_t & pallete){
		m_bit_writer.WriteBit(1);//transform present
		m_bit_writer.WriteBits(VP8_LOSSLESS_TRANSFORM::COLOR_INDEXING_TRANSFORM, 2);
	}
	void write_info(const uint32_t & width, const uint32_t & height){
		m_bit_writer.WriteBits(0, 32);
		m_bit_writer.WriteBits('\x2F', 8);
		m_bit_writer.WriteBits(width - 1, 14);
		m_bit_writer.WriteBits(height - 1, 14);
		m_bit_writer.WriteBits(0, 1);
		m_bit_writer.WriteBits(0, 3);
	}
	struct HuffmanTree{
		histoarray histo;
		huffman_coding::enc::HuffmanTree * tree;
		HuffmanTree(const size_t & num_symbols)
			: histo(num_symbols)
		{
			histo.fill(0);
		}
	};
	void WriteEntropyCodedImage(const size_t & xsize, const size_t & ysize, const utils::pixel_array & data){
		m_bit_writer.WriteBit(0);//no color cache

		lz77::LZ77<uint32_t> lz77(LZ77_MAX_DISTANCE, LZ77_MAX_LENGTH, data);

		histoarray histos[HUFFMAN_CODES_COUNT_IN_HUFFMAN_META_CODE] = { histoarray(256 + 24), histoarray(256), histoarray(256), histoarray(256), histoarray(40)};
		for(size_t i = 0; i < lz77.output().size(); i++){
			if (lz77.output()[i].length == 0 && lz77.output()[i].distance == 0){
				++histos[huffman_io::GREEN][*utils::GREEN(lz77.output()[i].symbol)];
				++histos[huffman_io::RED][*utils::RED(lz77.output()[i].symbol)];
				++histos[huffman_io::BLUE][*utils::BLUE(lz77.output()[i].symbol)];
				++histos[huffman_io::ALPHA][*utils::ALPHA(lz77.output()[i].symbol)];
			}
			else{
				symbol_t symbol;
				size_t t1,t2;
				lz77::prefix_coding_encode(lz77.output()[i].length, symbol, t1, t2);
				++histos[huffman_io::GREEN][symbol + 256];


				uint32_t dist_code = lz77::distance2dist_code(xsize, lz77.output()[i].distance);
				lz77::prefix_coding_encode(dist_code, symbol, t1, t2);
				++histos[huffman_io::DIST_PREFIX][symbol];
			}
		}
		huffman_coding::enc::HuffmanTree* trees[HUFFMAN_CODES_COUNT_IN_HUFFMAN_META_CODE];
		for(size_t i = 0; i < HUFFMAN_CODES_COUNT_IN_HUFFMAN_META_CODE; i++){
			trees[i] = new huffman_coding::enc::HuffmanTree(histos[i], MAX_ALLOWED_CODE_LENGTH);
			huffman_io::enc::VP8_LOSSLESS_HUFFMAN hio(&m_bit_writer, *trees[i]);
		}
		WriteLZ77CodedImage(xsize, ysize, trees, lz77);
		for(size_t i = 0; i < HUFFMAN_CODES_COUNT_IN_HUFFMAN_META_CODE; i++)
			delete trees[i];
	}
	void WriteSpatiallyCodedImage(const size_t & xsize, const size_t & ysize, const utils::pixel_array & data){
		m_bit_writer.WriteBit(0);//no color cache
		m_bit_writer.WriteBit(0);//no huffman image
		lz77::LZ77<uint32_t> lz77(LZ77_MAX_DISTANCE, LZ77_MAX_LENGTH, data);

		histoarray histos[HUFFMAN_CODES_COUNT_IN_HUFFMAN_META_CODE] = { histoarray(256 + 24), histoarray(256), histoarray(256), histoarray(256), histoarray(40)};
		for(size_t i = 0; i < lz77.output().size(); i++){
			if (lz77.output()[i].length == 0 && lz77.output()[i].distance == 0){
				++histos[huffman_io::GREEN][*utils::GREEN(lz77.output()[i].symbol)];
				++histos[huffman_io::RED][*utils::RED(lz77.output()[i].symbol)];
				++histos[huffman_io::BLUE][*utils::BLUE(lz77.output()[i].symbol)];
				++histos[huffman_io::ALPHA][*utils::ALPHA(lz77.output()[i].symbol)];
			}
			else{
				symbol_t symbol;
				size_t t1,t2;
				lz77::prefix_coding_encode(lz77.output()[i].length, symbol, t1, t2);
				++histos[huffman_io::GREEN][symbol + 256];


				uint32_t dist_code = lz77::distance2dist_code(xsize, lz77.output()[i].distance);
				lz77::prefix_coding_encode(dist_code, symbol, t1, t2);
				++histos[huffman_io::DIST_PREFIX][symbol];
			}
		}
		huffman_coding::enc::HuffmanTree* trees[HUFFMAN_CODES_COUNT_IN_HUFFMAN_META_CODE];
		for(size_t i = 0; i < HUFFMAN_CODES_COUNT_IN_HUFFMAN_META_CODE; i++){
			trees[i] = new huffman_coding::enc::HuffmanTree(histos[i], MAX_ALLOWED_CODE_LENGTH);
			huffman_io::enc::VP8_LOSSLESS_HUFFMAN hio(&m_bit_writer, *trees[i]);
		}
		WriteLZ77CodedImage(xsize, ysize, trees, lz77);
		for(size_t i = 0; i < HUFFMAN_CODES_COUNT_IN_HUFFMAN_META_CODE; i++)
			delete trees[i];
	}
	void WriteLZ77CodedImage(const size_t & xsize, const size_t & ysize, huffman_coding::enc::HuffmanTree ** trees,
								const lz77::LZ77<uint32_t> & lz77){
		for(size_t i = 0; i < lz77.output().size(); i++){
			if (lz77.output()[i].length == 0 && lz77.output()[i].distance == 0){
				symbol_t g = *utils::GREEN(lz77.output()[i].symbol);
				symbol_t r = *utils::RED(lz77.output()[i].symbol);
				symbol_t b = *utils::BLUE(lz77.output()[i].symbol);
				symbol_t a = *utils::ALPHA(lz77.output()[i].symbol);

				if (trees[huffman_io::GREEN]->get_num_nodes() > 1)
					m_bit_writer.WriteBits(trees[huffman_io::GREEN]->get_codes()[g], trees[huffman_io::GREEN]->get_lengths()[g]);

				if (trees[huffman_io::RED]->get_num_nodes() > 1)
					m_bit_writer.WriteBits(trees[huffman_io::RED]->get_codes()[r], trees[huffman_io::RED]->get_lengths()[r]);

				if (trees[huffman_io::BLUE]->get_num_nodes() > 1)
					m_bit_writer.WriteBits(trees[huffman_io::BLUE]->get_codes()[b], trees[huffman_io::BLUE]->get_lengths()[b]);

				if (trees[huffman_io::ALPHA]->get_num_nodes() > 1)
					m_bit_writer.WriteBits(trees[huffman_io::ALPHA]->get_codes()[a], trees[huffman_io::ALPHA]->get_lengths()[a]);

				printf("%s\n", huffman_io::bit2str(trees[huffman_io::GREEN]->get_codes()[g], trees[huffman_io::GREEN]->get_lengths()[g]));
				printf("%s\n", huffman_io::bit2str(trees[huffman_io::RED]->get_codes()[r], trees[huffman_io::RED]->get_lengths()[r]));
				printf("%s\n", huffman_io::bit2str(trees[huffman_io::BLUE]->get_codes()[b], trees[huffman_io::BLUE]->get_lengths()[b]));
				printf("%s\n", huffman_io::bit2str(trees[huffman_io::ALPHA]->get_codes()[a], trees[huffman_io::ALPHA]->get_lengths()[a]));
				printf("ARGB (%08X)\n", lz77.output()[i].symbol);
			}
			else{
				printf("LZ77 (%u %u) ",lz77.output()[i].length, lz77.output()[i].distance);
				symbol_t g;
				size_t extra_bits_count, extra_bits;
				lz77::prefix_coding_encode(lz77.output()[i].length, g, extra_bits_count, extra_bits);
				//printf("length=( prfx(%u, %u, %u), %s) ", g, extra_bits, extra_bits_count, huffman_io::bit2str(trees[huffman_io::GREEN]->get_codes()[g + 256], trees[huffman_io::GREEN]->get_lengths()[g + 256]));
				m_bit_writer.WriteBits(trees[huffman_io::GREEN]->get_codes()[g + 256], trees[huffman_io::GREEN]->get_lengths()[g + 256]);
				if (extra_bits_count > 0)
					m_bit_writer.WriteBits(extra_bits, extra_bits_count);

				symbol_t dist;
				uint32_t dist_code = lz77::distance2dist_code(xsize, lz77.output()[i].distance);
				lz77::prefix_coding_encode(dist_code, dist, extra_bits_count, extra_bits);
				//printf("distance=(%u prfx(%u, %u, %u), %s)\n", dist_code, dist, extra_bits, extra_bits_count,
					//				huffman_io::bit2str(trees[huffman_io::DIST_PREFIX]->get_codes()[dist], trees[huffman_io::DIST_PREFIX]->get_lengths()[dist]));
				if (trees[huffman_io::DIST_PREFIX]->get_num_nodes() > 1)
					m_bit_writer.WriteBits(trees[huffman_io::DIST_PREFIX]->get_codes()[dist], trees[huffman_io::DIST_PREFIX]->get_lengths()[dist]);
				if (extra_bits_count > 0)
					m_bit_writer.WriteBits(extra_bits, extra_bits_count);
			}
		}
	}
public:
	VP8_LOSSLESS_ENCODER(const utils::pixel_array & argb_image, const uint32_t & width, const uint32_t & height)
	{
		write_info(width, height);

		pallete_t pallete;
		utils::pixel_array image(argb_image);
		/*CreatePallete(image, pallete);
		if (pallete.size() == 0){//палитры нет
			entropy_t entropy;
			entropy_t entropy2;
			AnalyzeEntropy(image, entropy);
			ApplySubtractGreenTransform(image);
			AnalyzeEntropy(image, entropy2);
		}
		else{//палитра есть
			ApplyColorIndexingTransform(pallete);
		}*/
		m_bit_writer.WriteBit(0);//no transform
		WriteSpatiallyCodedImage(width, height, image);
	}
	const utils::BitWriter & get_bit_writer(){
		return m_bit_writer;
	}
};
}
}

#endif /* VP8_H_ */
