#include "../platform.h"
#include <deque>

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
	std::deque<T>		m_search_buffer;
	std::deque<T>		m_look_ahead_buffer;
	std::deque<token>	m_output;
	uint32_t			m_max_distance;
	uint32_t			m_max_length;
	void pack(const T* data, const uint32_t & size){
		size_t i = 0;
		/*while(i != size || m_look_ahead_buffer.size() != 0){
			for(; i != size && m_look_ahead_buffer.size() < m_max_length; i++)
				m_look_ahead_buffer.push_back(data[i]);

			if (m_look_ahead_buffer.size() == 1){
				T symbol = m_look_ahead_buffer.front();
				m_look_ahead_buffer.pop_front();
				m_search_buffer.push_back(symbol);
				if (m_search_buffer.size() > m_max_distance)
					m_search_buffer.pop_front();
				m_output.push_back(token(0, 0, symbol));
				continue;
			}

			token temp_token;
			temp_token.length = 0;
			bool match = false;
			for(int j = m_search_buffer.size() - 1; j >= 0; j--){
				uint32_t length = 0;
				uint32_t distance = m_search_buffer.size() - j;
				for(size_t k = j; k < m_search_buffer.size(); k++){
					if (m_search_buffer[k] == m_look_ahead_buffer[k - j])
						length++;
					else
						break;
				}
				if (length){
					if (length >= temp_token.length)
						temp_token = token(distance, length, m_look_ahead_buffer[length]);
					match = true;
				}
			}
			if (match){
				m_output.push_back(temp_token);
				for(size_t j = 0; j <= temp_token.length; j++){
					T symbol = m_look_ahead_buffer.front();
					m_look_ahead_buffer.pop_front();
					m_search_buffer.push_back(symbol);
					if (m_search_buffer.size() > m_max_distance)
						m_search_buffer.pop_front();
				}
			}
			else{
				T symbol = m_look_ahead_buffer.front();
				m_look_ahead_buffer.pop_front();
				m_search_buffer.push_back(symbol);
				if (m_search_buffer.size() > m_max_distance)
						m_search_buffer.pop_front();
				m_output.push_back(token(0, 0, symbol));
			}
		}*/
		/*for(size_t i = 0; i < m_output.size(); i++){
			printf("%u %u %c\n", m_output[i].distance, m_output[i].length, m_output[i].symbol);
		}*/
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

}
}
