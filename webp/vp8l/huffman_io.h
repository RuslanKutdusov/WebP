#ifndef _HUFFMAN_H_
#define _HUFFMAN_H_
#include "../platform.h"
#include "../exception/exception.h"
#include "../utils/bit_readed.h"
#include "../utils/bit_writer.h"
#include "../huffman_coding/huffman_coding.h"

namespace webp
{
namespace vp8l
{
namespace huffman_io
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

namespace dec
{

/*
 * пусть при чтении данных мы знаем, что дальше идут коды Хаффмана, этот класс принимает ссылку на BitReader и размер
 * цветового кэша, считывает коды Хаффмана и строит деревья, их 5 штук
 */
class VP8_LOSSLESS_HUFFMAN
{
private:
	utils::BitReader*		m_bit_reader;
	//webp::huffman::dec::HuffmanTree*	m_huffman_trees[HUFFMAN_CODES_COUNT_IN_HUFFMAN_META_CODE];
	std::vector<webp::huffman_coding::dec::HuffmanTree>	m_huffman_trees;
	VP8_LOSSLESS_HUFFMAN()
		: m_bit_reader(NULL)
	{

	}
	int read_symbol(const webp::huffman_coding::dec::HuffmanTree& tree) const
	{
		webp::huffman_coding::dec::HuffmanTree::iterator iter = tree.root();
		while(!(*iter).is_leaf())
			iter.next(m_bit_reader->ReadBits(1));
		return (*iter).symbol();
	}
	void read_code_length(const int* const code_length_code_lengths, int32_t num_symbols, utils::array<int32_t> & code_lengths)
	{
		int symbol;
		int max_symbol;
		webp::huffman_coding::dec::HuffmanTree tree(code_length_code_lengths,  kCodeLengthCodes);

		////////////////////////////////////////////////////
		//Незадокументированный кусок кода, копипаст из libwebp
		////////////////////////////////////////////////////
		if (m_bit_reader->ReadBits(1))// use length
		{
			const int length_nbits = 2 + 2 * m_bit_reader->ReadBits(3);
			max_symbol = 2 + m_bit_reader->ReadBits(length_nbits);
			if (max_symbol > num_symbols)
				throw exception::InvalidHuffman();
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
					repeat_count = m_bit_reader->ReadBits(2) + 3;
					repeat_last_code_len = true;
				}
				if (code_len == 17)
					repeat_count = m_bit_reader->ReadBits(3) + 3;
				if (code_len == 18)
					repeat_count = m_bit_reader->ReadBits(7) + 11;
				if (symbol + repeat_count > num_symbols)
					throw exception::InvalidHuffman();
				for(int i = 0; i < repeat_count; i++)
					code_lengths[symbol++] = repeat_last_code_len ? prev_code_len : 0;
			}
		}
	}
	void read_code(const uint32_t & alphabet_size)
	{
		//Simple code length или Normal code length
		uint32_t is_simple_code = m_bit_reader->ReadBits(1);

		if (is_simple_code == 1)
		{
			//в случае простой длины кода, символов может 1 или 2
			uint32_t num_symbols = m_bit_reader->ReadBits(1) + 1;
			//длины первого кода может быть 1 и 8 бит,
			//если first_symbol_len_code == 1, тогда длина первого кода 8 бит, иначе 1
			uint32_t first_symbol_len_code = m_bit_reader->ReadBits(1);
			//массив под символы
			int32_t symbols[2];
			//массив под коды символов
			int32_t codes[2];
			//длины кодов символов
			int32_t code_lengths[2];
			//читаем первый символ
			symbols[0] = m_bit_reader->ReadBits(1 + 7 * first_symbol_len_code);
			codes[0] = 0;
			code_lengths[0] = num_symbols - 1;
			//и если есть, читаем второй символ
			if (num_symbols == 2)
			{
				symbols[1] = m_bit_reader->ReadBits(8);
				codes[1] = 1;
				code_lengths[1] = num_symbols - 1;
			}
			//строим дерево Хаффмана
			m_huffman_trees.push_back(webp::huffman_coding::dec::HuffmanTree(code_lengths, codes, symbols, alphabet_size, num_symbols));
		}
		else
		{
			int32_t code_length_code_lengths[kCodeLengthCodes] = { 0 };
			int32_t num_codes = m_bit_reader->ReadBits(4) + 4;
			if (num_codes > kCodeLengthCodes)
				throw exception::InvalidHuffman();

			utils::array<int32_t> code_lengths(alphabet_size);
			code_lengths.fill(0);

			for (int32_t i = 0; i < num_codes; ++i)
				code_length_code_lengths[kCodeLengthCodeOrder[i]] = m_bit_reader->ReadBits(3);

			read_code_length(code_length_code_lengths, alphabet_size, code_lengths);
			m_huffman_trees.push_back(webp::huffman_coding::dec::HuffmanTree(&code_lengths[0], alphabet_size));
		}
	}
public:
	/*
	 * Исключение: InvalidHuffman
	 */
	VP8_LOSSLESS_HUFFMAN(utils::BitReader * bit_reader, const uint32_t & color_cache_size)
		: m_bit_reader(bit_reader)
	{
		for(uint32_t i = 0; i < HUFFMAN_CODES_COUNT_IN_HUFFMAN_META_CODE; i++)
		{
			uint32_t alphabet_size = AlphabetSize[i];
			if (i == 0)
				alphabet_size += color_cache_size;
			read_code(alphabet_size);
		}
	}
	int32_t read_symbol(const MetaHuffmanCode & mhc) const
	{
		return read_symbol(m_huffman_trees[mhc]);
	}
	virtual ~VP8_LOSSLESS_HUFFMAN()
	{

	}
};

}

namespace enc
{

class VP8_LOSSLESS_HUFFMAN{
private:
	utils::BitWriter * m_bit_writer;

	void write_compressed_code(const huffman_coding::enc::HuffmanTreeCodes & codes) const{
		//сжимаем по RLE исходные длины кодов
		huffman_coding::enc::RLESequence rle_sequence;
		codes.compress(rle_sequence);
		utils::array<uint16_t> histo(kCodeLengthCodes);
		histo.fill(0);
		for(size_t i = 0; i < rle_sequence.size(); i++)
			++histo[rle_sequence.code_length(i)];

		huffman_coding::enc::HuffmanTreeCodes tree_of_rle_sequence(histo);

		//определяем кол-во длин кодов для записи, нулевые длины кодов не пишем
		//мин. 4 длин кодов
		size_t code_lengths2write = kCodeLengthCodes;
		for(; code_lengths2write > 4; --code_lengths2write)
		{
			size_t i = kCodeLengthCodeOrder[code_lengths2write - 1];
			if (tree_of_rle_sequence.get_lengths()[i] != 0)
				break;
		}
		m_bit_writer->WriteBits(code_lengths2write, 4);//пишем кол-во длин кодов
		for(size_t i = 0; i < code_lengths2write; i++)
		{
			size_t j = kCodeLengthCodeOrder[i];
			m_bit_writer->WriteBits(tree_of_rle_sequence.get_lengths()[j], 3);
		}

		m_bit_writer->WriteBit(0);//незадокументированный бит!!!!!!!!!!!!!!!!!!!!!!!!!!

		//записываем RLE посл-ть
		for(size_t i = 0; i < rle_sequence.size(); i++){
			uint16_t sequence_element = rle_sequence.code_length(i);
			uint8_t extra_bits = rle_sequence.extra_bits(i);
			m_bit_writer->WriteBits(tree_of_rle_sequence.get_codes()[sequence_element], tree_of_rle_sequence.get_lengths()[sequence_element]);
			if (sequence_element == 16)
				m_bit_writer->WriteBits(extra_bits, 2);
			if (sequence_element == 17)
				m_bit_writer->WriteBits(extra_bits, 3);
			if (sequence_element == 18)
				m_bit_writer->WriteBits(extra_bits, 7);
		}

	}
	void write_codes(const huffman_coding::enc::HuffmanTreeCodes & codes) const{
		int count = 0;
		uint32_t symbols[2] = { 0, 0 };

		// считаем кол-во символов, у которых длина кода != 0
		for (size_t i = 0; i < codes.get_num_symbols(); ++i){
			if (codes.get_lengths()[i] != 0)
				if (count < 2) symbols[count] = i;
					++count;
			if (count == 2)
				break;
		}

		if (count == 0) {   //символов нет, сл-но может об этом сказать с помощью simple length code
			m_bit_writer->WriteBit(1);//говорим, что это simple length code
			m_bit_writer->WriteBit(0);//кол-во символов 0
			m_bit_writer->WriteBit(0);//длина первого символа 0(проигнорируется)
			m_bit_writer->WriteBit(0);//первый символ - \0 (проигнорируется)
		}
		else if (count <= 2 && symbols[0] < 256 && symbols[1] < 256) {
			m_bit_writer->WriteBit(1);//говорим, что это simple code length
			m_bit_writer->WriteBit(count - 1);//записываем кол-во символов - 1
			if (symbols[0] <= 1) {//если первый символ это \0 или \1, можем записать его в 2 бита
				m_bit_writer->WriteBit(0);//говорим, что длина первого simple length code равна 1
				m_bit_writer->WriteBit(symbols[0]);//записываем символ
			} else {//иначе если первый символ не \0 и не \1, записываем все его 8 бит
				m_bit_writer->WriteBit(1);//говорим, что длина первого символа равна 8 битам
				m_bit_writer->WriteBits(symbols[0], 8);//записываем символ
			}
			if (count == 2) {//если символов 2, то записываем все 8 бит второго символа
				m_bit_writer->WriteBits(symbols[1], 8);
			}
		} else {
			m_bit_writer->WriteBit(0);//не simple length code
			write_compressed_code(codes);
		}
	}
	VP8_LOSSLESS_HUFFMAN()
		: m_bit_writer(NULL)
	{

	}
public:
	VP8_LOSSLESS_HUFFMAN(utils::BitWriter * bw)
		: m_bit_writer(bw)
	{

	}
};
}
}
}
}

#endif /* HUFFMAN_H_ */