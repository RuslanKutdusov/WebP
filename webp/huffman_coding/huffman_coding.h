#ifndef HUFFMAN_H_
#define HUFFMAN_H_

#include <list>
#include "../exception/exception.h"
#include "../utils/utils.h"

namespace webp
{

typedef uint8_t code_length_t;
typedef uint32_t code_t;
typedef uint16_t symbol_t;
typedef utils::array<uint32_t> histoarray;

namespace huffman_coding
{

static const code_t NON_EXISTENT_SYMBOL = -1;

int make_canonical_codes(const code_length_t* const code_lengths, const size_t & code_lengths_size, utils::array<code_t> & huff_codes);
int make_canonical_codes(const utils::array<code_length_t> & code_lengths, utils::array<code_t> & huff_codes);

namespace dec
{

class HuffmanTree;

class HuffmanTreeNode
{
private:
	symbol_t m_symbol;
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
	symbol_t symbol() const
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
	utils::array<HuffmanTreeNode>	m_root;
	uint32_t						m_max_nodes;
	uint32_t						m_num_nodes;
	HuffmanTree()

	{
		iterator iter(*this);
		HuffmanTreeNode node = *iter;
	}
	int init(const size_t & num_leaves)
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
	int add_symbol(symbol_t symbol, code_t code, code_length_t code_length)
	{
		HuffmanTreeNode* node = &m_root[0];
		const HuffmanTreeNode* const max_node = m_root + m_max_nodes;
		//while (code_length-- > 0)
		while(1)
		{
			if (code_length == 0)
				break;
			code_length--;
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
	void cnstr_error()
	{
		release();
		throw exception::InvalidHuffman();
	}
	void explicit_init(const code_length_t * const code_lengths,
						const code_t * const codes,
						const symbol_t * const symbols,
						symbol_t max_symbol,
						 size_t num_symbols){
		if (!init(num_symbols))
			cnstr_error();

		for (size_t i = 0; i < num_symbols; ++i)
		{
			if (codes[i] != NON_EXISTENT_SYMBOL)
			{
				if (symbols[i] >= max_symbol)
					cnstr_error();
				if (!add_symbol(symbols[i], codes[i], code_lengths[i]))
					cnstr_error();
			}
		}
		if (!is_full())
			cnstr_error();
	}
	void implicit_init(const code_length_t * const code_lengths,
						const size_t & code_length_size){
		symbol_t symbol;
		size_t num_symbols = 0;//кол-во символов
		symbol_t root_symbol = 0;//в случае если символ только один, то его можно легко найти сразу в след. цикле

		for (symbol = 0; symbol < code_length_size; ++symbol)
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
			const int max_symbol = code_length_size;
			if (root_symbol >= max_symbol)
				cnstr_error();
			if (!add_symbol(root_symbol, 0, 0))
				cnstr_error();
		}
		else
		{
			utils::array<code_t> codes(code_length_size);


			if (!make_canonical_codes(code_lengths, code_length_size, codes))
				cnstr_error();

			for (symbol = 0; symbol < code_length_size; ++symbol)
			{
				if (code_lengths[symbol] > 0)
					if (!add_symbol(symbol, codes[symbol], code_lengths[symbol]))
						cnstr_error();
			}
		}
		if (!is_full())
			cnstr_error();
	}
public:
	//явная инициализация дерева, задаются длины кодов, сами коды хаффмана, символы и кол-во символов
	HuffmanTree(const utils::array<code_length_t> & code_lengths,
				 const utils::array<code_t> & codes,
				 const utils::array<symbol_t> & symbols,
				 symbol_t max_symbol,
				 size_t num_symbols)
	{
		explicit_init(&code_lengths[0], &codes[0], &symbols[0], max_symbol, num_symbols);
	}
	HuffmanTree(const code_length_t * const code_lengths,
							const code_t * const codes,
							const symbol_t * const symbols,
							symbol_t max_symbol,
							 size_t num_symbols)
	{
		explicit_init(&code_lengths[0], &codes[0], &symbols[0], max_symbol, num_symbols);
	}
	//неявная инициализация дерева, задаются только длины кодов, и их кол-во
	HuffmanTree(const utils::array<code_length_t> & code_lengths)
	{
		implicit_init(&code_lengths[0], code_lengths.size());
	}
	HuffmanTree(const code_length_t * const code_lengths,
			const size_t & code_length_size)
	{
		implicit_init(code_lengths, code_length_size);
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

#define MY

#ifndef MY

class HuffmanTree
{
private:
	struct tree_t{
	  size_t 	total_count_;
	  int 		value_;
	  int 		pool_index_left_;
	  int		pool_index_right_;
	};
	uint16_t		m_num_symbols;
	utils::array<code_t>			m_codes;
	utils::array<code_length_t>	m_code_length;
	HuffmanTree()
		: m_num_symbols(0)
	{

	}
	static int CompareHuffmanTrees(const void* ptr1, const void* ptr2)
	{
		const tree_t* const t1 = (const tree_t*)ptr1;
		const tree_t* const t2 = (const tree_t*)ptr2;
		if (t1->total_count_ > t2->total_count_)
			return -1;
		else if (t1->total_count_ < t2->total_count_)
			return 1;
		else {
			//assert(t1->value_ != t2->value_);
			return (t1->value_ < t2->value_) ? -1 : 1;
		}
	}
	void SetBitDepths(const tree_t* const tree, const tree_t* const pool,
						utils::array<code_length_t> & code_length, int level) {
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
	int ValuesShouldBeCollapsedToStrideAverage(int a, int b) {
	  return abs(a - b) < 4;
	}
	int optimize(histoarray & histo) {
		uint8_t* good_for_rle;
		// 1) Let's make the Huffman code more compatible with rle encoding.

		size_t length = m_num_symbols;
		for (; length >= 0; --length) {
			if (length == 0) {
				return 1;  // All zeros.
			}
			if (histo[length - 1] != 0) {
				// Now counts[0..length - 1] does not have trailing zeros.
				break;
			}
		}
		// 2) Let's mark all population counts that already can be encoded
		// with an rle code.
		good_for_rle = (uint8_t*)calloc(length, 1);
		if (good_for_rle == NULL) {
			return 0;
		}
		// Let's not spoil any of the existing good rle codes.
		// Mark any seq of 0's that is longer as 5 as a good_for_rle.
		// Mark any seq of non-0's that is longer as 7 as a good_for_rle.
		int symbol = histo[0];
		int stride = 0;
		for (size_t i = 0; i < length + 1; ++i) {
			if (i == length || histo[i] != symbol) {
				if ((symbol == 0 && stride >= 5) || 	(symbol != 0 && stride >= 7)) {
					int k;
					for (k = 0; k < stride; ++k) {
						good_for_rle[i - k - 1] = 1;
					}
				}
				stride = 1;
				if (i != length) {
					symbol = histo[i];
				}
			} else {
				++stride;
			}
		}
		// 3) Let's replace those population counts that lead to more rle codes.
		stride = 0;
		int limit = histo[0];
		int sum = 0;
		for (size_t i = 0; i < length + 1; ++i) {
			if (i == length || good_for_rle[i] || (i != 0 && good_for_rle[i - 1]) || !ValuesShouldBeCollapsedToStrideAverage(histo[i], limit)) {
				if (stride >= 4 || (stride >= 3 && sum == 0)) {
					int k;
					// The stride must end, collapse what we have, if we have enough (4).
					int count = (sum + stride / 2) / stride;
					if (count < 1) {
						count = 1;
					}
					if (sum == 0) {
						// Don't make an all zeros stride to be upgraded to ones.
						count = 0;
					}
					for (k = 0; k < stride; ++k) {
						// We don't want to change value at counts[i],
						// that is already belonging to the next stride. Thus - 1.
						histo[i - k - 1] = count;
					}
				}
				stride = 0;
				sum = 0;
				if (i < length - 3) {
					// All interesting strides have a count of at least 4,
					// at least when non-zeros.
					limit = (histo[i] + histo[i + 1] +
							histo[i + 2] + histo[i + 3] + 2) / 4;
				} else if (i < length) {
					limit = histo[i];
				} else {
					limit = 0;
				}
			}
			++stride;
			if (i != length) {
				sum += histo[i];
				if (stride >= 4) {
					limit = (sum + stride / 2) / stride;
				}
			}
		}
		free(good_for_rle);
		return 1;
	}
public:
	HuffmanTree(const histoarray & histo_, const size_t & depth_limit)
		: m_num_symbols(histo_.size()), m_codes(histo_.size()), m_code_length(histo_.size())
	{
		histoarray histo(histo_);
		optimize(histo);
		tree_t* tree_pool;
		tree_t* tree;
		size_t tree_size_orig = 0;

		for (size_t i = 0; i < histo.size(); ++i)
			if (histo[i] != 0)
				++tree_size_orig;

		// 3 * tree_size is enough to cover all the nodes representing a
		// population and all the inserted nodes combining two existing nodes.
		// The tree pool needs 2 * (tree_size_orig - 1) entities, and the
		// tree needs exactly tree_size_orig entities.
		tree = new(std::nothrow) tree_t[3 * tree_size_orig];
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
			size_t max_depth = m_code_length[0];
			for (size_t j = 1; j < m_code_length.size(); ++j) {
				if (max_depth < m_code_length[j]) {
					max_depth = m_code_length[j];
				}
			}
			if (max_depth <= depth_limit) {
				break;
			}
		}
		delete[] tree;
		ConvertBitDepthsToSymbols();
	}
	const uint16_t & get_num_symbols() const{
		return m_num_symbols;
	}
	const utils::array<code_length_t> & get_lengths() const{
		return m_code_length;
	}
	const utils::array<code_t> & get_codes() const{
		return m_codes;
	}
	void zero_reps(RLESequence & rle_sequence, size_t reps) const{
		while(reps >= 1){
			//если длина серии <3, то просто записываем ее
			if (reps < 3){
				for(size_t j = 0; j < reps; j++)
					rle_sequence.add(0, 0);
				break;
			}
			//если длина серии <11, то пишем код 17, и экстра биты = длина серии - 3(чтобы влезть в 3 бита)
			else if (reps < 11){
				rle_sequence.add(ZERO_11_REPS_CODE, reps - 3);
				break;
			}
			//если длина серии <139, то пишем код 18, и экстра биты = длина серии - 11(чтобы влезть в 7 бит)
			else if (reps < 139){
				rle_sequence.add(ZERO_138_REPS_CODE, reps - 11);
				break;
			}
			//иначе пишем код 18, и экстра биты = 127(чтобы влезть в 3 бита), уменьшаем длину серии и в цикл
			else{
				rle_sequence.add(ZERO_138_REPS_CODE, 138 - 11);
				reps -= 138;
			}
		}
	}
	void code_length_reps(RLESequence & rle_sequence, size_t reps, const code_length_t & prev_code_length,
						const code_length_t & code_length) const{
		if (prev_code_length != code_length){
			rle_sequence.add(code_length, 0);
			reps--;
		}
		while(reps >= 1){
			if (reps < 3){
				for(size_t j = 0; j < reps; j++)
					rle_sequence.add(code_length, 0);
				break;
			}
			else if (reps < 7){
				rle_sequence.add(NON_ZERO_REPS_CODE, reps - 3);
				break;
			}
			else{
				rle_sequence.add(NON_ZERO_REPS_CODE, 6 - 3);
				reps -= 6;
			}
		}
	}
	void compress(RLESequence & rle_sequence) const{
		uint16_t prev_code_length = 8;
		//RLE
		for(size_t i = 0; i < m_num_symbols; ){
			//текущее значение
			code_length_t code_length = m_code_length[i];
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
				zero_reps(rle_sequence, runs);
			}
			else {
				code_length_reps(rle_sequence, runs, prev_code_length, code_length);
				prev_code_length = code_length;
			}
			i += runs;
		}
	}
	void print(){
		for(size_t i = 0; i < m_num_symbols; i++){
			printf("%u ", m_code_length[i]);
		}
		printf("\n");
	}
};

#endif

#ifdef MY

class HuffmanTree;

class HuffmanNode{
private:
	HuffmanNode*	m_left0;
	HuffmanNode*	m_right1;
	uint64_t		m_p;
	uint16_t		m_symbol;
	bool			m_marker;
public:
	HuffmanNode()
		: m_left0(NULL), m_right1(NULL), m_p(0), m_symbol(65535), m_marker(false)
	{

	}
	HuffmanNode(HuffmanNode* left0, HuffmanNode* right1, const uint64_t & p, const uint32_t & symbol = -1)
		: m_left0(left0), m_right1(right1), m_p(p), m_symbol(symbol), m_marker(false)
	{

	}
	friend class HuffmanTree;
};

class HuffmanTree{
private:
	utils::array<HuffmanNode*> m_roots;
	utils::array<code_length_t> m_code_lengths;
	utils::array<code_t> m_codes;
	size_t						m_num_symbols;
	std::list<HuffmanNode*>		m_all_nodes;
	size_t						m_num_nodes;
	static int sort_roots(const void * n1, const void * n2){
		const HuffmanNode * n1_ = *(HuffmanNode**)n1;
		const HuffmanNode * n2_ = *(HuffmanNode**)n2;

		if (n1_ != NULL && n2_ == NULL )
			return -1;
		if (n1_ == NULL && n2_ == NULL)
			return 0;
		if (n1_ == NULL && n2_ != NULL)
			return 1;

		if (n1_->m_p < n2_->m_p)
			return -1;
		else if (n1_->m_p > n2_->m_p)
			return 1;
		return 0;
	}
	bool compare_nodes(const HuffmanNode & n1, const HuffmanNode & n2){
		return n1.m_p < n2.m_p;
	}
	void make_codes(const code_t & code, const code_length_t & code_length, HuffmanNode * iter){
		if (iter->m_left0 != NULL)
			make_codes((code << 1) | 0, code_length + 1, iter->m_left0);
		if (iter->m_right1 != NULL)
			make_codes((code << 1) | 1, code_length + 1, iter->m_right1);
		if (iter->m_left0 == NULL && iter->m_right1 == NULL){
			uint16_t symbol = iter->m_symbol;
			m_code_lengths[symbol] = code_length;
			m_codes[symbol] = code;
		}
	}
	void release(){
		for(std::list<HuffmanNode*>::iterator iter = m_all_nodes.begin(); iter != m_all_nodes.end(); ++iter)
			delete *iter;
		m_all_nodes.clear();
	}
	void reverse_code(code_t & code, const code_length_t & length){
		code_t ret = 0;
		for(int i = length - 1; i >= 0; i--){
			code_t bit = (code >> i) & 1;
			ret |= bit << (length - i - 1);
		}
		code = ret;
	}
	HuffmanTree(const HuffmanTree & tree){}
	HuffmanTree & operator=(const HuffmanTree & tree){return *this;}
	char * bit2str(uint16_t b, uint8_t len)
		{
			char * str = (char*)malloc(len + 1);
			int i ;
			for(i = 0; i < len; i++)
			{
				int shift = len - i - 1;
				str[i] = ((b >> shift) & 1) == 1 ? '1' : '0';
			}
			str[len] = '\0';
			return str;
		}
public:
	HuffmanTree(){

	}
	HuffmanTree(const histoarray & histo, const size_t & max_allowed_code_length)//, const uint32_t & size)
		: m_roots(histo.size()), m_code_lengths(histo.size()), m_codes(histo.size()), m_num_symbols(histo.size()), m_num_nodes(0)
	{
		m_code_lengths.fill(0);
		m_codes.fill(0);
		for(size_t i = 0; i < m_num_symbols; i++){
			if (histo[i] == 0){
				m_code_lengths[i] = 0;
				m_codes[i] = 0;
				m_roots[i] = NULL;
				continue;
			}
			m_roots[i] = new HuffmanNode(NULL, NULL, histo[i], i);
			m_num_nodes++;
			m_all_nodes.push_back(m_roots[i]);
		}
		m_roots.sort(sort_roots);

		HuffmanNode* n1 = m_roots[0];
		HuffmanNode* n2 = m_roots[1];
		while(n2 != NULL){
			HuffmanNode * new_node = new HuffmanNode(n1, n2, n1->m_p + n2->m_p);
			m_all_nodes.push_back(new_node);

			m_roots[0] = new_node;
			m_roots[1] = NULL;
			m_roots.sort(sort_roots);
			n1 = m_roots[0];
			n2 = m_roots[1];

		}
		//если только один символ
		if (m_roots[0]->m_left0 == NULL && m_roots[0]->m_right1 == NULL){
			uint16_t symbol = m_roots[0]->m_symbol;
			m_code_lengths[symbol] = 1;
			m_codes[symbol] = 0;
		}
		else
			make_codes(0, 0, m_roots[0]);

		size_t max_code_length = 0;
		for(size_t i = 0; i < m_num_symbols; i++)
			if (m_code_lengths[i] > max_code_length)
				max_code_length = m_code_lengths[i];
		if (max_code_length > max_allowed_code_length){
			release();
			throw exception::TooBigCodeLength(max_allowed_code_length, max_code_length);
		}
		make_canonical_codes(m_code_lengths, m_codes);
		for(size_t i = 0; i < m_num_symbols; i++){
			if (m_code_lengths[i] != 0)
				reverse_code(m_codes[i], m_code_lengths[i]);
		}
		/*for(size_t i = 0; i < m_num_symbols; i++){
			if (m_code_lengths[i] != 0){
				printf("%u %u %s\n", i, m_code_lengths[i], bit2str(m_codes[i], m_code_lengths[i]));
			}
		}*/
	}
	const utils::array<code_length_t> & get_lengths() const{
		return m_code_lengths;
	}
	const utils::array<code_t> & get_codes() const {
		return m_codes;
	}
	const size_t & get_num_symbols() const{
		return m_num_symbols;
	}
	const size_t & get_num_nodes() const{
		return m_num_nodes;
	}
	virtual ~HuffmanTree(){
		release();
	}
};

#endif

}

}
}


#endif /* HUFFMAN_H_ */
