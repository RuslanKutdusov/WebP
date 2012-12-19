#ifndef COLOR_CACHE_H_
#define COLOR_CACHE_H_
#include "../platform.h"

#define MAX_COLOR_CACHE_BITS 11

namespace webp
{
namespace vp8l
{

class VP8_LOSSLESS_COLOR_CACHE
{
private:
	VP8_LOSSLESS_COLOR_CACHE()
			: m_bits(0), m_cache(NULL)
	{

	}
	VP8_LOSSLESS_COLOR_CACHE & operator=(const VP8_LOSSLESS_COLOR_CACHE &)
	{
		return *this;
	}
private:
	uint32_t	m_bits;
	uint32_t*	m_cache;
public:
	VP8_LOSSLESS_COLOR_CACHE(const uint32_t & color_cache_bits)
		: m_bits(color_cache_bits)
	{
		uint32_t size = 1 << color_cache_bits;
		m_cache = new uint32_t[size];
		memset(m_cache, 0, size);
	}
	void insert(const uint32_t & color)
	{
		 uint32_t key = (0x1e35a7bd * color) >> (32 - m_bits);
		 m_cache[key] = color;
	}
	const uint32_t & get(const uint32_t & key) const
	{
		return m_cache[key];
	}
	virtual ~VP8_LOSSLESS_COLOR_CACHE()
	{
		if (m_cache != NULL)
			delete[] m_cache;
	}
};

}
}



#endif /* COLOR_CACHE_H_ */
