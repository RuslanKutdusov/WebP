#ifndef HUFFMAN_H_
#define HUFFMAN_H_

#include <list>
#include "../exception/exception.h"
#include "../utils/utils.h"

namespace webp
{
namespace huffman
{

#define NON_EXISTENT_SYMBOL (-1)
#define MAX_ALLOWED_CODE_LENGTH      15

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
		/*m_root = new(std::nothrow) HuffmanTreeNode[m_max_nodes]();
		if (m_root == NULL)
			return 0;*/
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
		printf("%d ", symbol);
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
		printf("\n");
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
			/*int32_t * codes = new(std::nothrow) int[code_lengths_size];
			if (codes == NULL)
				cnstr_error();*/
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
		printf("\n");
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
}


#endif /* HUFFMAN_H_ */
