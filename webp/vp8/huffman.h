#ifndef _HUFFMAN_H_
#define _HUFFMAN_H_
#include "../platform.h"
#include "../exception/exception.h"
#include "../utils/bit_readed.h"

namespace webp
{
namespace vp8l
{

/*	Мета код Хаффмана состоит из 5 кодов Хаффмана
*	-первый для зеленого + префикс длины совпадения LZ77 + цветового кэша, алфавит для этого кода состоит из 256 символово для зеленого, 24 для префикса и еще несколько,
*		в зависимости от размера цветового кэша
*	-второй для красного, алфавит из 256 символов
*	-третий для синего, алфавит из 256 символов
*	-четвертый для альфы, алфавит из 256 символов
*	-пятый для префикса смещения LZ77, алфавит из 40 символов
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
		for(uint32_t i = 0; i < HUFFMAN_CODES_COUNT_IN_HUFFMAN_META_CODE; i++)
			m_huffman_trees[i] = NULL;
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
				//printf("(%d %d %d)", repeat_count, repeat_last_code_len, prev_code_len);
				for(int i = 0; i < repeat_count; i++)
				{
					code_lengths[symbol++] = repeat_last_code_len ? prev_code_len : 0;
				}
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
			int32_t* code_lengths = NULL;

			int32_t code_length_code_lengths[kCodeLengthCodes] = { 0 };
			int32_t num_codes = m_bit_reader.ReadBits(4) + 4;
			if (num_codes > kCodeLengthCodes)
				throw 1;

			code_lengths = new(std::nothrow) int32_t[alphabet_size];
			memset(code_lengths, 0, alphabet_size * sizeof(int32_t));
			if (code_lengths == NULL)
				throw 1;

			for (int32_t i = 0; i < num_codes; ++i)
				code_length_code_lengths[kCodeLengthCodeOrder[i]] = m_bit_reader.ReadBits(3);

			read_code_length(code_length_code_lengths, alphabet_size, code_lengths);
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
			m_huffman_trees[i] = NULL;
			uint32_t alphabet_size = AlphabetSize[i];
			if (i == 0)
				alphabet_size += color_cache_size;
			if (!read_code(alphabet_size, i))
				throw 1;
		}
	}
	int32_t read_symbol(const MetaHuffmanCode & mhc)
	{
		return read_symbol(*m_huffman_trees[mhc]);
	}
	virtual ~VP8_LOSSLESS_HUFFMAN()
	{
		for(uint32_t i = 0; i < HUFFMAN_CODES_COUNT_IN_HUFFMAN_META_CODE; i++)
			if (m_huffman_trees[i] != NULL)
				delete m_huffman_trees[i];
	}
};

class VP8_LOSSLESS_HUFFMAN_META
{
private:
	utils::BitReader		m_unused;
	utils::BitReader&		m_bit_reader;
	VP8_LOSSLESS_HUFFMAN_META()
		: m_bit_reader(m_unused)
	{

	}
public:
	VP8_LOSSLESS_HUFFMAN_META(utils::BitReader & bit_reader, const uint32_t & color_cache_size)
		: m_bit_reader(bit_reader)
	{
		uint32_t use_meta_huffman_codes = bit_reader.ReadBits(1);
		if (use_meta_huffman_codes)
		{

		}
	}
};
}
}

#endif /* HUFFMAN_H_ */
