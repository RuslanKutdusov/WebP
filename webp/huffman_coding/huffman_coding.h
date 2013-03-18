#ifndef HUFFMAN_H_
#define HUFFMAN_H_

#include <list>
#include "../exception/exception.h"
#include "../utils/utils.h"

namespace webp
{
namespace huffman_coding
{

#define NON_EXISTENT_SYMBOL (-1)
#define MAX_ALLOWED_CODE_LENGTH      15

namespace dec
{

class HuffmanTree;

class HuffmanTreeNode
{
private:
	int32_t m_symbol;
	int32_t m_children;
public:
	HuffmanTreeNode()
		: m_children(-1)
	{

	}
	HuffmanTreeNode(const HuffmanTreeNode & node)
		: m_symbol(node.m_symbol), m_children(node.m_children)
	{

	}
	HuffmanTreeNode & operator=(const HuffmanTreeNode & node)
	{
		if (&node != this)
		{
			m_symbol = node.m_symbol;
			m_children = node.m_children;
		}
		return *this;
	}
	virtual ~HuffmanTreeNode()
	{

	}
	bool is_leaf() const
	{
		return m_children == 0;
	}
	bool is_empty() const
	{
		return m_children < 0;
	}
	int symbol() const
	{
		return m_symbol;
	}
	friend class HuffmanTree;
};

class HuffmanTree
{
public:
	class iterator
	{
	private:
		HuffmanTreeNode* m_node;
		iterator()
			: m_node(NULL){}
	public:
		iterator(const HuffmanTree & tree)
			: m_node(&tree.m_root[0])
		{

		}
		const HuffmanTreeNode& operator*() const
		{
			return *m_node;
		}
		const HuffmanTreeNode& next(int right)
		{
			m_node += m_node->m_children + right;
			return *m_node;
		}
	};
	friend class iterator;
private:
	utils::array<HuffmanTreeNode>	m_root;   // all the nodes, starting at root.
	int32_t						m_max_nodes;           // max number of nodes
	int32_t						m_num_nodes;           // number of currently occupied nodes
	HuffmanTree()

	{
		iterator iter(*this);
		HuffmanTreeNode node = *iter;
	}
	int init(const int & num_leaves)
	{
		if (num_leaves == 0)
			return 0;

		m_max_nodes = 2 * num_leaves - 1;
		m_root.realloc(m_max_nodes);

		m_num_nodes = 1;
		return 1;
	}
	void release()
	{

	}
	void AssignChildren(HuffmanTreeNode * node)
	{
		HuffmanTreeNode* const children = m_root + m_num_nodes;
		node->m_children = (int)(children - node);
		m_num_nodes += 2;
	}
	int add_symbol(int32_t symbol, int32_t code, int32_t code_length)
	{
		HuffmanTreeNode* node = &m_root[0];
		const HuffmanTreeNode* const max_node = m_root + m_max_nodes;
		while (code_length-- > 0)
		{
			if (node >= max_node)
				return 0;
			if (node->is_empty())
			{
				if (is_full())
					return 0;
				AssignChildren(node);
			}
			else if (node->is_leaf())
				return 0;
			node += node->m_children + ((code >> code_length) & 1);
		}
		if (node->is_empty())
		{
			node->m_children = 0;
		}
		else
		if (!node->is_leaf())
			return 0;
		node->m_symbol = symbol;
		return 1;
	}
	int code_length_to_codes(const int* const code_lengths,
	                              int code_lengths_size, utils::array<int32_t> & huff_codes)
	{
	  int symbol;
	  int code_len;
	  int code_length_hist[MAX_ALLOWED_CODE_LENGTH + 1] = { 0 };
	  int curr_code;
	  int next_codes[MAX_ALLOWED_CODE_LENGTH + 1] = { 0 };
	  int max_code_length = 0;

	  // вычисляем макс длину кода
	  for (symbol = 0; symbol < code_lengths_size; ++symbol)
	  {
		  if (code_lengths[symbol] > max_code_length)
			  max_code_length = code_lengths[symbol];
	  }
	  if (max_code_length > MAX_ALLOWED_CODE_LENGTH)
		  return 0;

	  // вычисляем гистрограмму длин кодов
	  for (symbol = 0; symbol < code_lengths_size; ++symbol)
		  ++code_length_hist[code_lengths[symbol]];

	  code_length_hist[0] = 0; //кода с длиной 0 не может быть

	  // next_codes[code_len] означает след код с длиной code_len, первоначально это первый код с такой длиной
	  curr_code = 0;
	  next_codes[0] = -1;   //кода с длиной 0 не может быть
	  for (code_len = 1; code_len <= max_code_length; ++code_len) {
	    curr_code = (curr_code + code_length_hist[code_len - 1]) << 1;
	    next_codes[code_len] = curr_code;
	  }

	  for (symbol = 0; symbol < code_lengths_size; ++symbol)
	  {
		if (code_lengths[symbol] > 0)
			huff_codes[symbol] = next_codes[code_lengths[symbol]]++;
		else
			huff_codes[symbol] = NON_EXISTENT_SYMBOL;
	  }
	  return 1;
	}
	void cnstr_error()
	{
		release();
		throw exception::InvalidHuffman();
	}
public:
	//явная инициализация дерева, задаются длины кодов, сами коды хаффмана, символы и кол-во символов
	HuffmanTree(const int* const code_lengths,
				 const int* const codes,
				 const int* const symbols, int max_symbol,
				 int num_symbols)
	{
		if (!init(num_symbols))
			cnstr_error();

		for (int i = 0; i < num_symbols; ++i)
		{
			if (codes[i] != NON_EXISTENT_SYMBOL)
			{
				if (symbols[i] < 0 || symbols[i] >= max_symbol)
					cnstr_error();
				if (!add_symbol(symbols[i], codes[i], code_lengths[i]))
					cnstr_error();
			}
		}
		if (!is_full())
			cnstr_error();
	}
	//неявная инициализация дерева, задаются только длины кодов, и их кол-во
	HuffmanTree(const int* const code_lengths, int code_lengths_size)
	{
		int symbol;
		int num_symbols = 0;//кол-во символов
		int root_symbol = 0;//в случае если символ только один, то его можно легко найти сразу в след. цикле

		for (symbol = 0; symbol < code_lengths_size; ++symbol)
		{
			if (code_lengths[symbol] > 0)//симовола, длина кода которого равна 0, не существует
			{
				++num_symbols;
				root_symbol = symbol;
			}
		}

		if (!init(num_symbols))
			cnstr_error();

		// Build tree.
		if (num_symbols == 1)
		{
			const int max_symbol = code_lengths_size;
			if (root_symbol < 0 || root_symbol >= max_symbol)
				cnstr_error();
			if (!add_symbol(root_symbol, 0, 0))
				cnstr_error();
		}
		else
		{
			utils::array<int32_t> codes(code_lengths_size);


			if (!code_length_to_codes(code_lengths, code_lengths_size, codes))
				cnstr_error();

			for (symbol = 0; symbol < code_lengths_size; ++symbol)
			{
				if (code_lengths[symbol] > 0)
					if (!add_symbol(symbol, codes[symbol], code_lengths[symbol]))
						cnstr_error();
			}
		}
		if (!is_full())
			cnstr_error();
	}
	HuffmanTree(const HuffmanTree & tree)
		: m_max_nodes(tree.m_max_nodes), m_num_nodes(tree.m_num_nodes)
	{
		m_root = tree.m_root;
	}
	virtual ~HuffmanTree()
	{
		release();
	}
	bool is_full() const
	{
		return m_max_nodes == m_num_nodes;
	}
	const HuffmanTreeNode & get_root() const
	{
		return *m_root;
	}
	iterator root() const
	{
		return iterator(*this);
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
		RLESequenceElement(const uint16_t & code_length, const uint16_t & extra_bits)
				: m_code_length(code_length), m_extra_bits(extra_bits)
		{

		}
	};
	std::vector<RLESequenceElement> m_code_lengths;
public:
	void add(const uint16_t & code_length, const uint8_t & extra_bits){
		m_code_lengths.push_back(RLESequenceElement(code_length, extra_bits));
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

class HuffmanTreeCodes
{
public:
	typedef utils::array<uint8_t> code_length_t;
	typedef utils::array<uint16_t> codes_t;
private:
	struct HuffmanTree{
	  size_t 	total_count_;
	  int 		value_;
	  int 		pool_index_left_;
	  int		pool_index_right_;
	};
	uint16_t		m_num_symbols;
	codes_t			m_codes;
	code_length_t	m_code_length;
	HuffmanTreeCodes()
		: m_num_symbols(0)
	{

	}
	static int CompareHuffmanTrees(const void* ptr1, const void* ptr2)
	{
		const HuffmanTree* const t1 = (const HuffmanTree*)ptr1;
		const HuffmanTree* const t2 = (const HuffmanTree*)ptr2;
		if (t1->total_count_ > t2->total_count_)
			return -1;
		else if (t1->total_count_ < t2->total_count_)
			return 1;
		else {
			//assert(t1->value_ != t2->value_);
			return (t1->value_ < t2->value_) ? -1 : 1;
		}
	}
	void SetBitDepths(const HuffmanTree* const tree, const HuffmanTree* const pool,
						code_length_t & code_length, int level) {
		if (tree->pool_index_left_ >= 0) {
			SetBitDepths(&pool[tree->pool_index_left_], pool, code_length, level + 1);
			SetBitDepths(&pool[tree->pool_index_right_], pool, code_length, level + 1);
		} else
			code_length[tree->value_] = level;
	}
	uint32_t ReverseBits(uint8_t num_bits, uint32_t bits) {
		const uint8_t kReversedBits[16] = {
		  0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe,
		  0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf
		};
		uint32_t retval = 0;
		int i = 0;
		while (i < num_bits) {
			i += 4;
			retval |= kReversedBits[bits & 0xf] << (MAX_ALLOWED_CODE_LENGTH + 1 - i);
			bits >>= 4;
		}
		retval >>= (MAX_ALLOWED_CODE_LENGTH + 1 - num_bits);
		return retval;
	}
	void ConvertBitDepthsToSymbols() {
		uint32_t next_code[MAX_ALLOWED_CODE_LENGTH + 1];
		uint32_t depth_count[MAX_ALLOWED_CODE_LENGTH + 1] = { 0 };

		for (size_t i = 0; i < m_num_symbols; ++i) {
			int8_t code_length = m_code_length[i];
			++depth_count[code_length];
		}
		depth_count[0] = 0;  // ignore unused symbol
		next_code[0] = 0;

		uint32_t code = 0;
		for (size_t i = 1; i <= MAX_ALLOWED_CODE_LENGTH; ++i) {
			code = (code + depth_count[i - 1]) << 1;
			next_code[i] = code;
		}
		for (size_t i = 0; i < m_num_symbols; ++i) {
			uint8_t code_length = m_code_length[i];
			m_codes[i] = ReverseBits(code_length, next_code[code_length]++);
		}
	}
public:
	HuffmanTreeCodes(utils::array<uint16_t> & histo)
		: m_num_symbols(histo.size()), m_codes(histo.size()), m_code_length(histo.size())
	{
		HuffmanTree* tree_pool;
		HuffmanTree* tree;
		size_t tree_size_orig = 0;

		for (size_t i = 0; i < histo.size(); ++i)
			if (histo[i] != 0)
				++tree_size_orig;

		// 3 * tree_size is enough to cover all the nodes representing a
		// population and all the inserted nodes combining two existing nodes.
		// The tree pool needs 2 * (tree_size_orig - 1) entities, and the
		// tree needs exactly tree_size_orig entities.
		tree = new(std::nothrow) HuffmanTree[3 * tree_size_orig];
		if (tree == NULL)
			throw exception::MemoryAllocationException();
		tree_pool = tree + tree_size_orig;

		// For block sizes with less than 64k symbols we never need to do a
		// second iteration of this loop.
		// If we actually start running inside this loop a lot, we would perhaps
		// be better off with the Katajainen algorithm.
		//assert(tree_size_orig <= (1 << (tree_depth_limit - 1)));
		for (size_t count_min = 1; ; count_min *= 2) {
			size_t tree_size = tree_size_orig;
			// We need to pack the Huffman tree in tree_depth_limit bits.
			// So, we try by faking histogram entries to be at least 'count_min'.
			int idx = 0;
			for (size_t j = 0; j < histo.size(); ++j) {
				if (histo[j] != 0) {
					size_t count =	(histo[j] < count_min) ? count_min : histo[j];
					tree[idx].total_count_ = count;
					tree[idx].value_ = j;
					tree[idx].pool_index_left_ = -1;
					tree[idx].pool_index_right_ = -1;
					++idx;
				}
			}

			// Build the Huffman tree.
			qsort(tree, tree_size, sizeof(*tree), CompareHuffmanTrees);

			if (tree_size > 1) {  // Normal case.
				int tree_pool_size = 0;
				while (tree_size > 1) {  // Finish when we have only one root.
					size_t count;
					tree_pool[tree_pool_size++] = tree[tree_size - 1];
					tree_pool[tree_pool_size++] = tree[tree_size - 2];
					count = tree_pool[tree_pool_size - 1].total_count_ +
							tree_pool[tree_pool_size - 2].total_count_;
					tree_size -= 2;
					{
						// Search for the insertion point.
						size_t k;
						for (k = 0; k < tree_size; ++k)
							if (tree[k].total_count_ <= count)
								break;

						memmove(tree + (k + 1), tree + k, (tree_size - k) * sizeof(*tree));
						tree[k].total_count_ = count;
						tree[k].value_ = -1;

						tree[k].pool_index_left_ = tree_pool_size - 1;
						tree[k].pool_index_right_ = tree_pool_size - 2;
						tree_size = tree_size + 1;
					}
				}
				SetBitDepths(&tree[0], tree_pool, m_code_length, 0);
			}
			else if (tree_size == 1)   // Trivial case: only one element.
				m_code_length[tree[0].value_] = 1;
		}
		delete[] tree;
		ConvertBitDepthsToSymbols();
	}
	const uint16_t & get_num_symbols() const{
		return m_num_symbols;
	}
	const code_length_t & get_lengths() const{
		return m_code_length;
	}
	const codes_t & get_codes() const{
		return m_codes;
	}
	void compress(RLESequence & rle_sequence) const{
		uint16_t prev_code_length;
		//RLE
		for(size_t i = 0; i < m_num_symbols; ){
			//текущее значение
			uint16_t code_length = m_code_length[i];
			//индекс на след значение
			size_t k = i + 1;
			//кол-во повторений
			size_t runs;
			//определяем до какого индекса, значения повторяются непрерывно
			while (k < m_num_symbols && m_code_length[k] == code_length) ++k;
			//определяем кол-во повторяющихся значений
			runs = k - i;
			//серии повторяющихся 0ей сжимаем одним способом, другие серии - другим способом
			if (code_length == 0) {
				size_t reps = runs;
				while(reps >= 1){
					//если длина серии <3, то просто записываем ее
					if (reps < 3){
						for(size_t j = 0; j < reps; j++)
							rle_sequence.add(0, 0);
						break;
					}
					//если длина серии <11, то пишем код 17, и экстра биты = длина серии - 3(чтобы влезть в 3 бита)
					else if (reps < 11){
						rle_sequence.add(17, reps - 3);
						break;
					}
					//если длина серии <139, то пишем код 18, и экстра биты = длина серии - 11(чтобы влезть в 7 бит)
					else if (reps < 139){
						rle_sequence.add(18, reps - 11);
						break;
					}
					//иначе пишем код 18, и экстра биты = 127(чтобы влезть в 3 бита), уменьшаем длину серии и в цикл
					else{
						rle_sequence.add(18, 138 - 11);
						reps -= 138;
					}
				}
			}
			else {
				if (prev_code_length != code_length){
					size_t reps = runs;
					rle_sequence.add(code_length, 0);
					reps--;
					while(reps >= 1){
						if (reps < 3){
							for(size_t j = 0; j < reps; j++)
								rle_sequence.add(code_length, 0);
							break;
						}
						else if (reps < 7){
							rle_sequence.add(16, reps - 3);
							break;
						}
						else{
							rle_sequence.add(16, 6 - 3);
							reps -= 6;
						}
					}
				}
				prev_code_length = code_length;
			}
			i += runs;
		}
	}
};

}

}
}


#endif /* HUFFMAN_H_ */
