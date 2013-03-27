#ifndef _HUFFMAN_H_
#define _HUFFMAN_H_
#include "../platform.h"
#include "../exception/exception.h"
#include "../utils/bit_readed.h"
#include "../utils/bit_writer.h"
#include "../huffman_coding/huffman_coding.h"
#include <math.h>

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

#define MAX_ALLOWED_CODE_LENGTH      64

#define NON_ZERO_REPS_CODE		MAX_ALLOWED_CODE_LENGTH + 1
#define ZERO_11_REPS_CODE 		MAX_ALLOWED_CODE_LENGTH + 2
#define ZERO_138_REPS_CODE 	MAX_ALLOWED_CODE_LENGTH + 3

#define MAX_ALLOWED_CODE_LENGTH_OF_RLE_TREE 	32
static const size_t BITS_COUNT_FOR_RLE_CODE_LENGTHS = (int)log2f(MAX_ALLOWED_CODE_LENGTH_OF_RLE_TREE) + 1;//3//log2 MAX_ALLOWED_CODE_LENGTH_OF_RLE_TREE

//static const size_t kCodeLengthCodes = 19;
#define RLE_CODES_COUNT 		MAX_ALLOWED_CODE_LENGTH + 4
static const size_t  BITS_COUNTS_FOR_RLE_CODES_COUNT = (int)log2f(RLE_CODES_COUNT);//4//log2 RLE_CODES_COUNT - 1
static int kCodeLengthCodeOrder[RLE_CODES_COUNT] = {
		ZERO_11_REPS_CODE, ZERO_138_REPS_CODE, 0, 1, 2, 3, 4, 5, NON_ZERO_REPS_CODE};//, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
//};

static void init_array(){
	/*kCodeLengthCodeOrder[0] = ZERO_11_REPS_CODE;
	kCodeLengthCodeOrder[1] = ZERO_138_REPS_CODE;
	kCodeLengthCodeOrder[2] = 0;
	kCodeLengthCodeOrder[3] = 1;
	kCodeLengthCodeOrder[4] = 2;
	kCodeLengthCodeOrder[5] = 3;
	kCodeLengthCodeOrder[6] = 4;
	kCodeLengthCodeOrder[7] = 5;
	kCodeLengthCodeOrder[8] = 16;*/
	for(size_t i = 9; i < RLE_CODES_COUNT; i++)
		kCodeLengthCodeOrder[i] = i - 3;
}

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
	void read_code_length(const utils::array<code_length_t> & code_length_code_lengths, const size_t & num_symbols, utils::array<code_length_t> & code_lengths)
	{
		symbol_t symbol;
		symbol_t max_symbol;
		webp::huffman_coding::dec::HuffmanTree tree(code_length_code_lengths);

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
		code_length_t prev_code_len = 8;
		while (symbol < num_symbols)
		{
			code_length_t code_len;
			if (max_symbol-- == 0)
				break;
			code_len = read_symbol(tree);
			if (code_len < NON_ZERO_REPS_CODE)
			{
				code_lengths[symbol++] = code_len;
				if (code_len != 0)
					prev_code_len = code_len;
			}
			else
			{
				size_t repeat_count = 0;
				bool repeat_last_code_len = false;
				if (code_len == NON_ZERO_REPS_CODE)
				{
					repeat_count = m_bit_reader->ReadBits(2) + 3;
					repeat_last_code_len = true;
				}
				if (code_len == ZERO_11_REPS_CODE)
					repeat_count = m_bit_reader->ReadBits(3) + 3;
				if (code_len == ZERO_138_REPS_CODE)
					repeat_count = m_bit_reader->ReadBits(7) + 11;
				if (symbol + repeat_count > num_symbols)
					throw exception::InvalidHuffman();
				for(size_t i = 0; i < repeat_count; i++)
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
			symbol_t symbols[2];
			//массив под коды символов
			code_t codes[2];
			//длины кодов символов
			code_length_t code_lengths[2];
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
			utils::array<code_length_t> code_length_code_lengths(RLE_CODES_COUNT);
			code_length_code_lengths.fill(0);
			size_t num_codes = m_bit_reader->ReadBits(BITS_COUNTS_FOR_RLE_CODES_COUNT) + 4;
			if (num_codes > RLE_CODES_COUNT)
				throw exception::InvalidHuffman();

			utils::array<code_length_t> code_lengths(alphabet_size);
			code_lengths.fill(0);
			for (size_t i = 0; i < num_codes; ++i)
				code_length_code_lengths[kCodeLengthCodeOrder[i]] = m_bit_reader->ReadBits(BITS_COUNT_FOR_RLE_CODE_LENGTHS);

			read_code_length(code_length_code_lengths, alphabet_size, code_lengths);
			m_huffman_trees.push_back(webp::huffman_coding::dec::HuffmanTree(code_lengths));
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

class RLESequence{
private:
	struct RLESequenceElement{
		uint16_t	m_code_length;
		uint8_t		m_extra_bits;
		RLESequenceElement(const uint16_t & code_length, const uint8_t & extra_bits)
				: m_code_length(code_length), m_extra_bits(extra_bits)
		{

		}
	};
	std::vector<RLESequenceElement> m_code_lengths;
	void zero_reps(size_t reps) {
		while(reps >= 1){
			//если длина серии <3, то просто записываем ее
			if (reps < 3){
				for(size_t j = 0; j < reps; j++)
					add(0, 0);
				break;
			}
			//если длина серии <11, то пишем код ZERO_11_REPS_CODE, и экстра биты = длина серии - 3(чтобы влезть в 3 бита)
			else if (reps < 11){
				add(ZERO_11_REPS_CODE, reps - 3);
				break;
			}
			//если длина серии <139, то пишем код ZERO_138_REPS_CODE, и экстра биты = длина серии - 11(чтобы влезть в 7 бит)
			else if (reps < 139){
				add(ZERO_138_REPS_CODE, reps - 11);
				break;
			}
			//иначе пишем код 18, и экстра биты = ZERO_138_REPS_CODE, уменьшаем длину серии и в цикл
			else{
				add(ZERO_138_REPS_CODE, 138 - 11);
				reps -= 138;
			}
		}
	}
	void code_length_reps(size_t reps, const code_length_t & prev_code_length,
						const code_length_t & code_length) {
		if (prev_code_length != code_length){
			add(code_length, 0);
			reps--;
		}
		while(reps >= 1){
			if (reps < 3){
				for(size_t j = 0; j < reps; j++)
					add(code_length, 0);
				break;
			}
			else if (reps < 7){
				add(NON_ZERO_REPS_CODE, reps - 3);
				break;
			}
			else{
				add(NON_ZERO_REPS_CODE, 6 - 3);
				reps -= 6;
			}
		}
	}
	void add(const uint16_t & code_length, const uint8_t & extra_bits){
		m_code_lengths.push_back(RLESequenceElement(code_length, extra_bits));
	}
	RLESequence(){

	}
public:
	RLESequence(const huffman_coding::enc::HuffmanTree & tree){
		code_length_t prev_code_length = 8;
		//RLE
		for(size_t i = 0; i < tree.get_num_symbols(); ){
			//текущее значение
			code_length_t code_length = tree.get_lengths()[i];
			//индекс на след значение
			size_t k = i + 1;
			//кол-во повторений
			size_t runs;
			//определяем до какого индекса, значения повторяются непрерывно
			while (k < tree.get_num_symbols() && tree.get_lengths()[k] == code_length) ++k;
			//определяем кол-во повторяющихся значений
			runs = k - i;
			//серии повторяющихся 0ей сжимаем одним способом, другие серии - другим способом
			if (code_length == 0) {
				zero_reps(runs);
			}
			else {
				code_length_reps(runs, prev_code_length, code_length);
				prev_code_length = code_length;
			}
			i += runs;
		}
	}
	const uint16_t & code_length(size_t i) const{
		return m_code_lengths[i].m_code_length;
	}
	const uint8_t & extra_bits(size_t i) const{
		return m_code_lengths[i].m_extra_bits;
	}
	const size_t size() const{
		return m_code_lengths.size();
	}
};

class VP8_LOSSLESS_HUFFMAN{
private:
	utils::BitWriter * m_bit_writer;
	void write_compressed_code(const huffman_coding::enc::HuffmanTree & codes) const{
		//сжимаем по RLE исходные длины кодов
		RLESequence rle_sequence(codes);
		histoarray histo(RLE_CODES_COUNT);
		histo.fill(0);
		for(size_t i = 0; i < rle_sequence.size(); i++)
			++histo[rle_sequence.code_length(i)];

		huffman_coding::enc::HuffmanTree tree_of_rle_sequence(histo, MAX_ALLOWED_CODE_LENGTH_OF_RLE_TREE);


		//определяем кол-во длин кодов для записи, нулевые длины кодов не пишем
		//мин. 4 длин кодов
		size_t code_lengths2write = RLE_CODES_COUNT;
		for(; code_lengths2write > 4; --code_lengths2write)
		{
			size_t i = kCodeLengthCodeOrder[code_lengths2write - 1];
			if (tree_of_rle_sequence.get_lengths()[i] != 0)
				break;
		}
		m_bit_writer->WriteBits(code_lengths2write - 4, BITS_COUNTS_FOR_RLE_CODES_COUNT);//пишем кол-во длин кодов
		for(size_t i = 0; i < code_lengths2write; i++)
		{
			size_t j = kCodeLengthCodeOrder[i];
			m_bit_writer->WriteBits(tree_of_rle_sequence.get_lengths()[j], BITS_COUNT_FOR_RLE_CODE_LENGTHS);
		}

		m_bit_writer->WriteBit(0);//незадокументированный бит!!!!!!!!!!!!!!!!!!!!!!!!!!
		//записываем RLE посл-ть
		for(size_t i = 0; i < rle_sequence.size(); i++){
			uint16_t sequence_element = rle_sequence.code_length(i);
			uint8_t extra_bits = rle_sequence.extra_bits(i);
			m_bit_writer->WriteBits(tree_of_rle_sequence.get_codes()[sequence_element], tree_of_rle_sequence.get_lengths()[sequence_element]);
			if (sequence_element == NON_ZERO_REPS_CODE)
				m_bit_writer->WriteBits(extra_bits, 2);
			if (sequence_element == ZERO_11_REPS_CODE)
				m_bit_writer->WriteBits(extra_bits, 3);
			if (sequence_element == ZERO_138_REPS_CODE)
				m_bit_writer->WriteBits(extra_bits, 7);
		}
	}
	void write_codes(const huffman_coding::enc::HuffmanTree & codes) const{
		int count = 0;
		uint32_t symbols[2] = { 0, 0 };

		// считаем кол-во символов, у которых длина кода != 0
		for (size_t i = 0; i < codes.get_num_symbols(); ++i){
			if (codes.get_lengths()[i] != 0){
				if (count < 2)
					symbols[count] = i;
				++count;
			}
			if (count > 2)
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
	VP8_LOSSLESS_HUFFMAN(utils::BitWriter * bw, const huffman_coding::enc::HuffmanTree & codes)
		: m_bit_writer(bw)
	{
		write_codes(codes);
	}
};

}
}
}
}

#endif /* HUFFMAN_H_ */
