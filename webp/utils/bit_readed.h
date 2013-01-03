#ifndef BIT_READED_H_
#define BIT_READED_H_
#include "../platform.h"

namespace webp
{
namespace utils
{

class BitReader
{
private:
	const uint8_t* const			m_data;
	mutable uint64_t				m_bits_readed;
	size_t							m_length;
	mutable bool					m_eos;
	mutable bool					m_error;
	BitReader & operator=(const BitReader&)
	{
		return *this;
	}
	uint32_t ReadBit() const
	{
		if (m_eos)
		{
			m_error = true;
			return 0;
		}
		uint64_t byte_index = m_bits_readed / 8;
		char byte = m_data[byte_index];
		uint64_t bit_index = m_bits_readed - byte_index * 8;
		byte >>= bit_index;
		m_bits_readed++;
		if (byte_index == m_length && m_bits_readed == 8)
			m_eos = true;
		return (uint32_t)byte & 1;
	}
public:
	BitReader()
		: m_data(NULL), m_bits_readed(0), m_length(0), m_eos(true), m_error(0)
	{

	}
	BitReader(const uint8_t * const data, size_t length)
		: m_data(data), m_bits_readed(0), m_length(length), m_eos(false), m_error(false)
	{

	}
	uint32_t ReadBits(uint32_t n_bits) const
	{
		uint32_t ret;
		memset(&ret, 0, sizeof(uint32_t));
		for(uint32_t i = 0; i < n_bits; i++)
		{
			ret |= ReadBit() << i;
		}
		return ret;
	}
	virtual ~BitReader()
	{

	}
	bool eos()
	{
		return m_eos;
	}
	bool error()
	{
		return m_error;
	}
};

}
}


#endif /* BIT_READED_H_ */
