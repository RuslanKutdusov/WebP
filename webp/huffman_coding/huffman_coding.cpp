#include "huffman_coding.h"

namespace webp{
namespace huffman_coding{

static const size_t MAX_ALLOWED_CODE_LENGTH = 64;

int make_canonical_codes(const code_length_t* const code_lengths,
									const size_t & code_lengths_size, utils::array<code_t> & huff_codes)
{
	symbol_t symbol;
	code_length_t code_len;
	size_t code_length_hist[MAX_ALLOWED_CODE_LENGTH + 1] = { 0 };
	code_t curr_code;
	code_t next_codes[MAX_ALLOWED_CODE_LENGTH + 1] = { 0 };
	code_length_t max_code_length = 0;

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
	next_codes[0] = NON_EXISTENT_SYMBOL;   //кода с длиной 0 не может быть
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

int make_canonical_codes(const utils::array<code_length_t> & code_lengths,
	                                 utils::array<code_t> & huff_codes)
{
	return make_canonical_codes(&code_lengths[0], code_lengths.size(), huff_codes);
}

}
}
