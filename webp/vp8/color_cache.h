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
			: m_bits(0)
	{

	}
	VP8_LOSSLESS_COLOR_CACHE & operator=(const VP8_LOSSLESS_COLOR_CACHE &)
	{
		return *this;
	}
private:
	uint32_t	m_bits;
	utils::array<uint32_t>	m_cache;
	bool					m_is_presented;
public:
	VP8_LOSSLESS_COLOR_CACHE(const uint32_t & color_cache_bits)
		: m_bits(color_cache_bits)
	{
		if (color_cache_bits == 0)
			m_is_presented = false;
		else
		{
			uint32_t size = 1 << color_cache_bits;
			m_cache.realloc(size);
			m_cache.fill(0);
			m_is_presented = true;
		}
	}
	void insert(const uint32_t & color)
	{
		if (!m_is_presented)
			return;
		 uint32_t key = (0x1e35a7bd * color) >> (32 - m_bits);
		 m_cache[key] = color;
	}
	const uint32_t get(const uint32_t & key) const
	{
		if (!m_is_presented)
			return 0;
		return m_cache[key];
	}
	bool is_presented()
	{
		return m_is_presented;
	}
	virtual ~VP8_LOSSLESS_COLOR_CACHE()
	{

	}
};

}
}



#endif /* COLOR_CACHE_H_ */
