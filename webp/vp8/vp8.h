#ifndef VP8_H_
#define VP8_H_
#include "../platform.h"
#include "../exception/exception.h"
#include "../utils/bit_readed.h"
#include "../huffman/huffman.h"

namespace webp
{
namespace vp8
{

#define MAX_COLOR_CACHE_BITS 11

/*	meta huffman code состоит из пяти кодов Хаффмана
*	-первый для зеленого + длин чего там + цветового кэша, алфавит для этого кода состоит из 256 символово для зеленого, 24 для длин и еще несколько,
*		в зависимости от размера цветового кэша
*	-второй для красного, алфавит из 256 символов
*	-третий для синего, алфавит из 256 символов
*	-четвертый для альфы, алфавит из 256 символов
*	-пятый для distance prefix, алфавит из 40 символов
*/
#define HUFFMAN_CODES_COUNT_IN_HUFFMAN_META_CODE 5
static const uint32_t AlphabetSize[HUFFMAN_CODES_COUNT_IN_HUFFMAN_META_CODE] = { 256 + 24, 256, 256, 256, 40 };
enum MetaHuffmanCode
{
	GREEN       = 0,
	RED         = 1,
	BLUE        = 2,
	ALPHA       = 3,
	DIST_PREFIX = 4
};

static const int kCodeLengthCodes = 19;
static const int kCodeLengthCodeOrder[kCodeLengthCodes] = {
  17, 18, 0, 1, 2, 3, 4, 5, 16, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
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
	Type			type;
	//int				bits;
	//int				xsize;
	//int				ysize;
	uint32_t*		data;
public:
	VP8_LOSSLESS_TRANSFORM()
		: type(TRANSFORM_NUMBER), data(NULL)
	{

	}
	virtual ~VP8_LOSSLESS_TRANSFORM()
	{

	}
	friend class VP8_LOSSLESS_DECODER;
};

/*
 * пусть при чтении данных мы знаем, что дальше идут коды Хаффмана, этот класс принимает ссылку на BitReader и размер
 * цветового кэша, считывает коды Хаффмана и строит деревья, их 5 штук
 */
class VP8_LOSSLESS_HUFFMAN
{
private:
	utils::BitReader		m_unused;
	utils::BitReader&		m_bit_reader;
	huffman::HuffmanTree*	m_huffman_trees[HUFFMAN_CODES_COUNT_IN_HUFFMAN_META_CODE];
	VP8_LOSSLESS_HUFFMAN()
		: m_bit_reader(m_unused)
	{

	}
	VP8_LOSSLESS_HUFFMAN & operator=(const VP8_LOSSLESS_HUFFMAN&)
	{
		return *this;
	}
	int read_symbol(const huffman::HuffmanTree& tree)
	{
		huffman::HuffmanTree::iterator iter = tree.root();
		while(!(*iter).is_leaf())
			iter.next(m_bit_reader.ReadBits(1));
		return (*iter).symbol();
	}
	void read_code_length(const int* const code_length_code_lengths, int num_symbols, int* const code_lengths)
	{
		int symbol;
		int max_symbol;
		huffman::HuffmanTree tree(code_length_code_lengths,  kCodeLengthCodes);

		////////////////////////////////////////////////////
		//Незадокументированный кусок кода, копипаст из libwebp
		////////////////////////////////////////////////////
		if (m_bit_reader.ReadBits(1))// use length
		{
			const int length_nbits = 2 + 2 * m_bit_reader.ReadBits(3);
			max_symbol = 2 + m_bit_reader.ReadBits(length_nbits);
			if (max_symbol > num_symbols)
				throw 1;
		}
		else
			max_symbol = num_symbols;
		////////////////////////////////////////////////////
		///////////////////////////////////////////////////


		symbol = 0;
		int prev_code_len = 8;
		while (symbol < num_symbols)
		{
			int code_len;
			if (max_symbol-- == 0)
				break;
			code_len = read_symbol(tree);
			if (code_len < 16)
			{
				code_lengths[symbol++] = code_len;
				if (code_len != 0)
					prev_code_len = code_len;
			}
			else
			{
				int repeat_count = 0;
				bool repeat_last_code_len = false;
				if (code_len == 16)
				{
					repeat_count = m_bit_reader.ReadBits(2) + 3;
					repeat_last_code_len = true;
				}
				if (code_len == 17)
					repeat_count = m_bit_reader.ReadBits(3) + 3;
				if (code_len == 18)
					repeat_count = m_bit_reader.ReadBits(7) + 11;
				if (symbol + repeat_count > num_symbols)
					throw 1;
				for(int i = 0; i < repeat_count; i++)
					code_lengths[symbol++] = repeat_last_code_len ? prev_code_len : 0;
			}
		}
	}
	int read_code(const uint32_t & alphabet_size, const uint32_t & huffman_tree_index)
	{
		//Simple code length или Normal code length
		uint32_t is_simple_code = m_bit_reader.ReadBits(1);

		if (is_simple_code == 1)
		{
			//в случае простой длины кода, символов может 1 или 2
			uint32_t num_symbols = m_bit_reader.ReadBits(1) + 1;
			//длины первого кода может быть 1 и 8 бит,
			//если first_symbol_len_code == 1, тогда длина первого кода 8 бит, иначе 1
			uint32_t first_symbol_len_code = m_bit_reader.ReadBits(1);
			//массив под символы
			int32_t symbols[2];
			//массив под коды символов
			int32_t codes[2];
			//длины кодов символов
			int32_t code_lengths[2];
			//читаем первый символ
			symbols[0] = m_bit_reader.ReadBits(1 + 7 * first_symbol_len_code);
			codes[0] = 0;
			code_lengths[0] = num_symbols - 1;
			//и если есть, читаем второй символ
			if (num_symbols == 2)
			{
				symbols[1] = m_bit_reader.ReadBits(8);
				codes[1] = 1;
				code_lengths[1] = num_symbols - 1;
			}
			//строим дерево Хаффмана
			m_huffman_trees[huffman_tree_index] = new huffman::HuffmanTree(code_lengths, codes, symbols, alphabet_size, num_symbols);
		}
		else
		{
			int* code_lengths = NULL;
			int i;

			int code_length_code_lengths[kCodeLengthCodes] = { 0 };
			const int num_codes = m_bit_reader.ReadBits(4) + 4;
			if (num_codes > kCodeLengthCodes)
				throw 1;

			code_lengths = new(std::nothrow) int[alphabet_size];
			if (code_lengths == NULL)
				throw 1;

			for (i = 0; i < num_codes; ++i)
			  code_length_code_lengths[kCodeLengthCodeOrder[i]] = m_bit_reader.ReadBits(3);

			read_code_length(code_length_code_lengths, alphabet_size,
										code_lengths);
			m_huffman_trees[huffman_tree_index] = new huffman::HuffmanTree(code_lengths, alphabet_size);
			delete[] code_lengths;
		}
		return 1;
	}
public:
	VP8_LOSSLESS_HUFFMAN(utils::BitReader & bit_reader, const uint32_t & color_cache_size)
		: m_bit_reader(bit_reader)
	{
		for(uint32_t i = 0; i < HUFFMAN_CODES_COUNT_IN_HUFFMAN_META_CODE; i++)
		{
			uint32_t alphabet_size = AlphabetSize[i];
			if (i == 0)
				alphabet_size += color_cache_size;
			if (!read_code(alphabet_size, i))
				throw 1;
		}
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
					ReadEntropyCodedImage(block_xsize, block_ysize, transform.data);
				}
				break;
			default:
				throw exception::InvalidVP8();
		}
	}
	void ReadEntropyCodedImage(int32_t xsize, int32_t ysize, uint32_t * data)
	{
		uint32_t color_cache_size = ReadColorCacheSize();
		VP8_LOSSLESS_HUFFMAN huffman(m_bit_reader, color_cache_size);
		ReadLZ77CodedImage();
	}
	int32_t ReadColorCacheSize()
	{
		if (m_bit_reader.ReadBits(1) != 0)
		{
			int32_t color_cache_code_bits = m_bit_reader.ReadBits(4);
			if (color_cache_code_bits > MAX_COLOR_CACHE_BITS)
				throw exception::InvalidVP8();
			return 1 << color_cache_code_bits;
		}
		return 0;
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
		: m_data(data), m_data_length(data_length), m_bit_reader(data, data_length),
		  m_applied_transforms(0), m_next_transform(0)
	{
		ReadInfo();

		while(m_bit_reader.ReadBits(1))
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
	friend class VP8_LOSSLESS_TRANSFORM;
};

}
}

#endif /* VP8_H_ */
