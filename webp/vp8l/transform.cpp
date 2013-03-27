#include "transform.h"

namespace webp
{
namespace vp8l
{

void PixelsSum(uint32_t* a, uint32_t b)
{
	const uint32_t alpha_and_green = (*a & 0xff00ff00u) + (b & 0xff00ff00u);
	const uint32_t red_and_blue = (*a & 0x00ff00ffu) + (b & 0x00ff00ffu);
	*a = (alpha_and_green & 0xff00ff00u) | (red_and_blue & 0x00ff00ffu);
}

void PixelsSub(uint32_t* a, uint32_t b) {
  const uint32_t alpha_and_green = 0x00ff00ffu + (*a & 0xff00ff00u) - (b & 0xff00ff00u);
  const uint32_t red_and_blue = 0xff00ff00u + (*a & 0x00ff00ffu) - (b & 0x00ff00ffu);
  *a = (alpha_and_green & 0xff00ff00u) | (red_and_blue & 0x00ff00ffu);
}

uint32_t Average2(const uint32_t & a, const uint32_t & b)
{
	uint32_t ret;
	uint8_t * byte_a = (uint8_t*)&a;
	uint8_t * byte_b = (uint8_t*)&b;
	uint8_t * byte_r = (uint8_t*)&ret;
	for(size_t i = 0; i < 4; i++)
		byte_r[i] = (byte_a[i] + byte_b[i]) / 2;
	return ret;
}

//(T, L, TL
uint32_t Select(uint32_t L, uint32_t T, uint32_t TL)
{
  // L = left pixel, T = top pixel, TL = top left pixel.

  // ARGB component estimates for prediction.
  int32_t pAlpha = *utils::ALPHA(L) + *utils::ALPHA(T) - *utils::ALPHA(TL);
  int32_t pRed = *utils::RED(L) + *utils::RED(T) - *utils::RED(TL);
  int32_t pGreen = *utils::GREEN(L) + *utils::GREEN(T) - *utils::GREEN(TL);
  int32_t pBlue = *utils::BLUE(L) + *utils::BLUE(T) - *utils::BLUE(TL);

  // Manhattan distances to estimates for left and top pixels.
  int32_t pL = abs(pAlpha - *utils::ALPHA(L)) + abs(pRed - *utils::RED(L)) +
           abs(pGreen - *utils::GREEN(L)) + abs(pBlue - *utils::BLUE(L));
  int32_t pT = abs(pAlpha - *utils::ALPHA(T)) + abs(pRed - *utils::RED(T)) +
           abs(pGreen - *utils::GREEN(T)) + abs(pBlue - *utils::BLUE(T));

  // Return either left or top, the one closer to the prediction.
  if (pL <= pT)
    return L;
  else
    return T;
}

int32_t Clamp(const int32_t & a)
{
  return (a < 0) ? 0 : (a > 255) ?  255 : a;
}

uint32_t ClampAddSubtractFull(const uint32_t & a, const uint32_t & b, const uint32_t & c)
{
	uint32_t ret;
	for(size_t i = 0; i < 4; i++)
	{
		uint8_t * byte_a = utils::COLOR[i](a);
		uint8_t * byte_b = utils::COLOR[i](b);
		uint8_t * byte_c = utils::COLOR[i](c);
		uint8_t * byte_ret = utils::COLOR[i](ret);
		*byte_ret = Clamp(*byte_a + *byte_b - *byte_c);
	}
	return ret;
}

uint32_t ClampAddSubtractHalf(const uint32_t & a, const uint32_t & b)
{
	uint32_t ret;
	for(size_t i = 0; i < 4; i++)
	{
		uint8_t * byte_a = utils::COLOR[i](a);
		uint8_t * byte_b = utils::COLOR[i](b);
		uint8_t * byte_ret = utils::COLOR[i](ret);
		*byte_ret = Clamp(*byte_a + (*byte_a - *byte_b) / 2);
	}
	return ret;
}

void BundleColorMap(const uint8_t* const row, int width,
                           int xbits, uint32_t* const dst) {
  int x;
  if (xbits > 0) {
    const int bit_depth = 1 << (3 - xbits);
    const int mask = (1 << xbits) - 1;
    uint32_t code = 0xff000000;
    for (x = 0; x < width; ++x) {
      const int xsub = x & mask;
      if (xsub == 0) {
        code = 0xff000000;
      }
      code |= row[x] << (8 + bit_depth * xsub);
      dst[x >> xbits] = code;
    }
  } else {
    for (x = 0; x < width; ++x) dst[x] = 0xff000000 | (row[x] << 8);
  }
}

}
}
