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
	void init(const histoarray & histo, const size_t & max_allowed_code_length){
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
		if (m_num_nodes == 0)
			return;
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
	}
public:
	HuffmanTree(){

	}
	HuffmanTree(const histoarray & histo, const size_t & max_allowed_code_length)//, const uint32_t & size)
		: m_roots(histo.size()), m_code_lengths(histo.size()), m_codes(histo.size()), m_num_symbols(histo.size()), m_num_nodes(0)
	{
		init(histo, max_allowed_code_length);
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

}

}
}


#endif /* HUFFMAN_H_ */
