#include "lz77.h"

namespace webp{
namespace lz77
{

struct point
{
	int8_t x;
	int8_t y;
	point(){}
	point(int8_t x, int8_t y)
		: x(x), y(y)
	{

	}
};

const uint32_t BORDER_DISTANCE_CODE = 120;
const point dist_codes2dist[BORDER_DISTANCE_CODE] = {
		point(0, 1), point(1, 0),  point(1, 1),  point(-1, 1), point(0, 2),  point(2, 0),  point(1, 2),  point(-1, 2),
		point(2, 1),  point(-2, 1), point(2, 2),  point(-2, 2), point(0, 3),  point(3, 0),  point(1, 3),  point(-1, 3),
		point(3, 1),  point(-3, 1), point(2, 3),  point(-2, 3), point(3, 2),  point(-3, 2), point(0, 4),  point(4, 0),
		point(1, 4),  point(-1, 4), point(4, 1),  point(-4, 1), point(3, 3),  point(-3, 3), point(2, 4),  point(-2, 4),
		point(4, 2),  point(-4, 2), point(0, 5),  point(3, 4),  point(-3, 4), point(4, 3),  point(-4, 3), point(5, 0),
		point(1, 5),  point(-1, 5), point(5, 1),  point(-5, 1), point(2, 5),  point(-2, 5), point(5, 2),  point(-5, 2),
		point(4, 4),  point(-4, 4), point(3, 5),  point(-3, 5), point(5, 3),  point(-5, 3), point(0, 6),  point(6, 0),
		point(1, 6),  point(-1, 6), point(6, 1),  point(-6, 1), point(2, 6),  point(-2, 6), point(6, 2),  point(-6, 2),
		point(4, 5),  point(-4, 5), point(5, 4),  point(-5, 4), point(3, 6),  point(-3, 6), point(6, 3),  point(-6, 3),
		point(0, 7),  point(7, 0),  point(1, 7),  point(-1, 7), point(5, 5),  point(-5, 5), point(7, 1),  point(-7, 1),
		point(4, 6),  point(-4, 6), point(6, 4),  point(-6, 4), point(2, 7),  point(-2, 7), point(7, 2),  point(-7, 2),
		point(3, 7),  point(-3, 7), point(7, 3),  point(-7, 3), point(5, 6),  point(-5, 6), point(6, 5),  point(-6, 5),
		point(8, 0),  point(4, 7),  point(-4, 7), point(7, 4),  point(-7, 4), point(8, 1),  point(8, 2),  point(6, 6),
		point(-6, 6), point(8, 3),  point(5, 7),  point(-5, 7), point(7, 5),  point(-7, 5), point(8, 4),  point(6, 7),
		point(-6, 7), point(7, 6),  point(-7, 6), point(8, 5),  point(7, 7),  point(-7, 7), point(8, 6),  point(8, 7)};


const uint8_t dist2dist_code[128] = {
	 96,   73,  55,  39,  23,  13,   5,  1,  255, 255, 255, 255, 255, 255, 255, 255,
	 101,  78,  58,  42,  26,  16,   8,  2,    0,   3,  9,   17,  27,  43,  59,  79,
	 102,  86,  62,  46,  32,  20,  10,  6,    4,   7,  11,  21,  33,  47,  63,  87,
	 105,  90,  70,  52,  37,  28,  18,  14,  12,  15,  19,  29,  38,  53,  71,  91,
	 110,  99,  82,  66,  48,  35,  30,  24,  22,  25,  31,  36,  49,  67,  83, 100,
	 115, 108,  94,  76,  64,  50,  44,  40,  34,  41,  45,  51,  65,  77,  95, 109,
	 118, 113, 103,  92,  80,  68,  60,  56,  54,  57,  61,  69,  81,  93, 104, 114,
	 119, 116, 111, 106,  97,  88,  84,  74,  72,  75,  85,  89,  98, 107, 112, 117
	};


uint32_t prefix_coding_decode(const uint32_t & prefix_code, utils::BitReader & br)
{
	if (prefix_code < 4)
		return prefix_code + 1;
	uint32_t extra_bits = (prefix_code - 2) >> 1;
	uint32_t offset = (2 + (prefix_code & 1)) << extra_bits;
	return offset + br.ReadBits(extra_bits) + 1;
}

uint32_t distance_code2distance(const uint32_t & xsize, const uint32_t & lz77_distance_code)
{
	if (lz77_distance_code <= BORDER_DISTANCE_CODE)
	{
		const point & diff = dist_codes2dist[lz77_distance_code - 1];
		uint32_t lz77_distance = diff.x + diff.y * xsize;
		lz77_distance = lz77_distance >= 1 ? lz77_distance : 1;
		return lz77_distance;
	}
	else
		return lz77_distance_code - BORDER_DISTANCE_CODE;
}

int BitsLog2Floor(uint32_t n) {
	int log = 0;
	uint32_t value = n;
	int i;

	if (value == 0) return -1;
	for (i = 4; i >= 0; --i) {
		const int shift = (1 << i);
		const uint32_t x = value >> shift;
		if (x != 0) {
			value = x;
			log += shift;
		}
	}
	return log;
}

void prefix_coding_encode(uint32_t distance, uint16_t & symbol,
									 size_t & extra_bits_count,
									 size_t & extra_bits_value) {
	// Collect the two most significant bits where the highest bit is 1.
	const int highest_bit = BitsLog2Floor(--distance);
	// & 0x3f is to make behavior well defined when highest_bit
	// does not exist or is the least significant bit.
	const int second_highest_bit =
	(distance >> ((highest_bit - 1) & 0x3f)) & 1;
	extra_bits_count = (highest_bit > 0) ? (highest_bit - 1) : 0;
	extra_bits_value = distance & ((1 << extra_bits_count) - 1);
	symbol = (highest_bit > 0) ? (2 * highest_bit + second_highest_bit)
				: (highest_bit == 0) ? 1 : 0;
}

size_t distance2dist_code(const size_t & xsize, const size_t & dist) {
	size_t yoffset = dist / xsize;
	size_t xoffset = dist - yoffset * xsize;
	if (xoffset <= 8 && yoffset < 8)
		return dist2dist_code[yoffset * 16 + 8 - xoffset] + 1;
	else if (xoffset > xsize - 8 && yoffset < 7)
		return dist2dist_code[(yoffset + 1) * 16 + 8 + (xsize - xoffset)] + 1;
	return dist + BORDER_DISTANCE_CODE;
}

}
}
