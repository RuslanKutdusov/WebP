#pragma once
#include "../platform.h"
#include <deque>
#include "../utils/bit_writer.h"


namespace webp
{
namespace lz77
{
//			abab|abab

template <class T>
class LZ77{
public:
	struct token{
		uint32_t	distance;
		uint32_t	length;
		T			symbol;
		token(){}
		token(const uint32_t & dist, const uint32_t & len, const T & s)
			: distance(dist), length(len), symbol(s){}
	};
private:
	std::deque<token>	m_output;
	uint32_t			m_max_distance;
	uint32_t			m_max_length;
	void pack(const T* data, const uint32_t & size){
		size_t i = 0;
		uint32_t search_buffer_index = 0;
		uint32_t la_buffer_index = 0;
		uint32_t la_buffer_length = 0;
		while(i != size || la_buffer_length != 0){
			if (la_buffer_length < m_max_length && i != size)
			{
				uint32_t remain_in_buf = m_max_length - la_buffer_length;
				uint32_t remain_in_data = size - i;
				la_buffer_length += remain_in_buf > remain_in_data ? remain_in_data : remain_in_buf;
				i += remain_in_buf > remain_in_data ? remain_in_data : remain_in_buf;
			}

			if (la_buffer_length == 1){
				T symbol = data[la_buffer_index++];
				la_buffer_length--;
				if (la_buffer_index -  search_buffer_index > m_max_distance)
					search_buffer_index++;
				m_output.push_back(token(0, 0, symbol));
				continue;
			}

			token temp_token;
			temp_token.length = 0;
			bool match = false;
			const T* sb_stop = data + la_buffer_index;
			const T* la_stop = data + la_buffer_index + la_buffer_length;
			for(int j = la_buffer_index - 1; j >= (int)search_buffer_index; j--){
				const T* search_buffer_iter = &data[j];
				const T* la_buffer_iter = &data[la_buffer_index];
				uint32_t length = 0;
				uint32_t distance = la_buffer_index - j;
				while(length <= m_max_length && search_buffer_iter != sb_stop && la_buffer_iter != la_stop){
					if (*search_buffer_iter++ == *la_buffer_iter++)
						length++;
					else
						break;
				}
				if (length){
					if (length >= temp_token.length)
						temp_token = token(distance, length, data[length + la_buffer_index]);
					match = true;
				}
			}
			if (match){
				m_output.push_back(temp_token);

				la_buffer_index += temp_token.length ;
				la_buffer_length -= temp_token.length;
				if (la_buffer_length > m_max_length)
					la_buffer_length = 0;
				if (la_buffer_index - search_buffer_index > m_max_distance){
					search_buffer_index = la_buffer_index - m_max_distance;
				}
			}
			else{
				T symbol = data[la_buffer_index++];
				la_buffer_length--;
				if (la_buffer_index -  search_buffer_index > m_max_distance)
					search_buffer_index++;
				m_output.push_back(token(0, 0, symbol));
				continue;
			}
		}
	}
public:
	LZ77(const uint32_t & max_distance, const uint32_t & max_length, const utils::array<T> & data)
		: m_max_distance(max_distance), m_max_length(max_length)
	{
		pack(&data[0], data.size());
	}
	LZ77(const uint32_t & max_distance, const uint32_t & max_length, const T * data, const uint32_t & size)
		: m_max_distance(max_distance), m_max_length(max_length)
	{
		pack(data, size);
	}
	const std::deque<token> & output() const{
		return m_output;
	}
};

/*
 * LZ77_prefix_coding_encode
 * Бросает исключения: нет
 * Назначение:
 * длины совпадения и смещения LZ77 закодированы, эта функция декодирует их значения
 */
uint32_t prefix_coding_decode(const uint32_t & prefix_code, utils::BitReader & br);


/*
 * LZ77_distance_code2distance
 * Бросает исключения: нет
 * Назначение:
 * коды длины совпадения LZ77 меньшие 120 закодированы, эта функция декодирует их значения
 */
uint32_t distance_code2distance(const uint32_t & xsize, const uint32_t & lz77_distance_code);

void prefix_coding_encode(uint32_t distance, uint16_t & symbol,
									 size_t & extra_bits_count,
									 size_t & extra_bits_value);

size_t distance2dist_code(const size_t & xsize, const size_t & dist);

}
}
