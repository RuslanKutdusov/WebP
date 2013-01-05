#ifndef TRANSFORM_H_
#define TRANSFORM_H_
#include "../platform.h"
#include "../exception/exception.h"
#include "../utils/utils.h"


#define DIV_ROUND_UP(num, den) ((num) + (den) - 1) / (den)

namespace webp
{
namespace vp8l
{

void AddPixelsEq(uint32_t* a, uint32_t b);
uint32_t Average2(const uint32_t & a, const uint32_t & b);
uint32_t Select(uint32_t T, uint32_t L, uint32_t TL);
int32_t Clamp(const int32_t & a);
uint32_t ClampAddSubtractFull(const uint32_t & a, const uint32_t & b, const uint32_t & c);
uint32_t ClampAddSubtractHalf(const uint32_t & a, const uint32_t & b);

struct ColorTransformElement
{
	uint8_t green_to_red;
	uint8_t green_to_blue;
	uint8_t red_to_blue;
	ColorTransformElement(const uint32_t & argb)
	{
		red_to_blue = *utils::RED(argb);
		green_to_blue = *utils::GREEN(argb);
		green_to_red = *utils::BLUE(argb);
	}
};

class VP8_LOSSLESS_TRANSFORM
{
public:
	enum Type {
		  PREDICTOR_TRANSFORM			= 0,
		  COLOR_TRANSFORM				= 1,
		  SUBTRACT_GREEN				= 2,
		  COLOR_INDEXING_TRANSFORM		= 3,
		  TRANSFORM_NUMBER				= 4
	};
private:
	Type					m_type;
	uint32_t				m_xsize;
	uint32_t				m_ysize;
	uint32_t				m_bits;
	utils::array<uint32_t>	m_data;
	/*
		 * InverseColorIndexingTransform
		 * Бросает исключения: нет
		 * Назначение:
		 * инвертирует color indexing tranform если имеется
		 */
		void InverseColorIndexingTransform(utils::array<uint32_t> & argb_image, const uint32_t & image_width, const uint32_t & image_height)
		{
			//в argb_image находятся индексы, скопируем их, т.к в argb_image будем формировать конечное изображение
			utils::array<uint32_t> indices = argb_image;
			//кол-во пикселей(по сути индексов) в одном байте indices
			size_t pixels_per_byte = 1 << m_bits;
			//сколько бит приходится на один пиксель(индекс)
			size_t bits_per_pixel = 8 >> m_bits;
			size_t mask = (1 << bits_per_pixel) - 1;
			uint32_t color_indexing_xsize = DIV_ROUND_UP(image_width, 1 << m_bits);
			//пробегаемся по всем строкам
			for(size_t y = 0; y < image_height; y++)
			{
				//берем указатель на строку в конечном изображении
				uint32_t * row_iter = &argb_image[y * image_width];
				//пробегаемся по всем байтам indices
				for(size_t x = 0; x < color_indexing_xsize; x++)
					//пробегаемся по всем индексам в байте
					for(size_t i = 0; i < pixels_per_byte; i++)
					{
						//вытаскиваем индекс
						uint32_t color_table_index = (*utils::GREEN(indices[y * color_indexing_xsize + x]) >> (i * bits_per_pixel)) & mask;
						//вытаскиваем цвет из палитры по индексу
						*row_iter = m_data[color_table_index];
						//сдвигаемся к следущему пикселю в строке
						row_iter++;
					}
			}
		}
		/*
		 * InverseSubstractGreenTransform
		 * Бросает исключения: нет
		 * Назначение:
		 * инвертирует subtract green tranform если имеется
		 */
		void InverseSubstractGreenTransform(utils::array<uint32_t> & argb_image, const uint32_t & image_width, const uint32_t & image_height)
		{
			for(size_t y = 0; y < image_height; y++)
				for(size_t x = 0; x < image_width; x++)
				{
					uint32_t i = y * image_width + x;
					*utils::RED(argb_image[i])  = (*utils::RED(argb_image[i])  + *utils::GREEN(argb_image[i])) & 0xff;
					*utils::BLUE(argb_image[i]) = (*utils::BLUE(argb_image[i]) + *utils::GREEN(argb_image[i])) & 0xff;
				}
		}
		/*
		 * InversePredictorTransform
		 * Бросает исключения: нет
		 * Назначение:
		 * инвертирует subtract green tranform если имеется
		 */
		void InversePredictorTransform(utils::array<uint32_t> & argb_image, const uint32_t & image_width, const uint32_t & image_height)
		{
			for(size_t y = 0; y < image_height; y++)
				for(size_t x = 0; x < image_width; x++)
				{
					size_t i = y * image_width + x;
					uint32_t P;//&argb_image[i];
					if (x == 0 && y == 0)
					{
						P = 0xff000000;
						AddPixelsEq(&argb_image[i], P);
						continue;
					}
					if (x == 0)
					{
						P = argb_image[i - image_width];
						AddPixelsEq(&argb_image[i], P);
						continue;
					}
					if (y == 0)
					{
						P = argb_image[i - 1];
						AddPixelsEq(&argb_image[i], P);
						continue;
					}
					uint32_t L = argb_image[i - 1];
					uint32_t T = argb_image[i - image_width];
					uint32_t TR = (x == image_width - 1) ? L : argb_image[i - image_width + 1];
					uint32_t TL = argb_image[i - image_width - 1];
					int block_index = (y >> m_bits) * m_xsize + (x >> m_bits);
					uint32_t mode = *utils::GREEN(m_data[block_index]);
					switch (mode) {
						case 0:
							P = 0xff000000;
							break;
						case 1:
							P = L;
							break;
						case 2:
							P = T;
							break;
						case 3:
							P = TR;
							break;
						case 4:
							P = TL;
							break;
						case 5:
							P = Average2(Average2(L, TR), T);
							break;
						case 6:
							P = Average2(L, TL);
							break;
						case 7:
							P = Average2(L, T);
							break;
						case 8:
							P = Average2(TL, T);
							break;
						case 9:
							P = Average2(T, TR);
							break;
						case 10:
							P = Average2(Average2(L, TL), Average2(T, TR));
							break;
						case 11:
							//Select(top[0], left, top[-1]);
							P = Select(T, L, TL);//Select(L, T, TL); - косяк документации?
							break;
						case 12:
							P = ClampAddSubtractFull(L, T, TL);
							break;
						case 13:
							P = ClampAddSubtractHalf(Average2(L, T), TL);
							break;
						default:
							throw exception::InvalidVP8L();
							break;
					}
					AddPixelsEq(&argb_image[i], P);
				}
		}
		int8_t ColorTransformDelta(int8_t t, int8_t c)
		{
			return (t * c) >> 5;
		}
		void InverseColorTransform(utils::array<uint32_t> & argb_image, const uint32_t & image_width, const uint32_t & image_height)
		{
			for(size_t y = 0; y < image_height; y++)
				for(size_t x = 0; x < image_width; x++)
				{
					size_t i = y * image_width + x;
					int block_index = (y >> m_bits) * m_xsize + (x >> m_bits);
					uint32_t data = m_data[block_index];
					ColorTransformElement cte(data);

					uint8_t red = *utils::RED(argb_image[i]);
					uint8_t green = *utils::GREEN(argb_image[i]);
					uint8_t blue = *utils::BLUE(argb_image[i]);

					//еще один косяк документации, в написано, что надо вычитать
					red  += ColorTransformDelta(cte.green_to_red,  green);
					blue += ColorTransformDelta(cte.green_to_blue, green);
					blue += ColorTransformDelta(cte.red_to_blue, red & 0xff);

					*utils::RED(argb_image[i]) = red & 0xff;
					*utils::BLUE(argb_image[i]) = blue & 0xff;
				}
		}
public:
	VP8_LOSSLESS_TRANSFORM()
		: m_type(TRANSFORM_NUMBER)
	{

	}
	VP8_LOSSLESS_TRANSFORM(Type type)
		: m_type(type)
	{

	}
	virtual ~VP8_LOSSLESS_TRANSFORM()
	{

	}
	void init_data(uint32_t xsize, uint32_t ysize, uint32_t bits)
	{
		m_xsize = xsize;
		m_ysize = ysize;
		m_bits = bits;
		m_data.realloc(xsize * ysize);
	}
	const Type & type() const
	{
		return m_type;
	}
	const uint32_t & bits() const
	{
		return m_bits;
	}
	const uint32_t & xsize() const
	{
		return m_xsize;
	}
	const uint32_t & ysize() const
	{
		return m_ysize;
	}
	utils::array<uint32_t>& data()
	{
		return m_data;
	}
	uint32_t data_length()
	{
		return m_data.size();
	}
	void inverse(utils::array<uint32_t> & argb_image, const uint32_t & image_width, const uint32_t & image_height)
	{
		if (m_type == VP8_LOSSLESS_TRANSFORM::PREDICTOR_TRANSFORM)
			InversePredictorTransform(argb_image, image_width, image_height);
		if (m_type == VP8_LOSSLESS_TRANSFORM::COLOR_TRANSFORM)
			InverseColorTransform(argb_image, image_width, image_height);
		if (m_type == VP8_LOSSLESS_TRANSFORM::COLOR_INDEXING_TRANSFORM)
			InverseColorIndexingTransform(argb_image, image_width, image_height);
		if (m_type == VP8_LOSSLESS_TRANSFORM::SUBTRACT_GREEN)
			InverseSubstractGreenTransform(argb_image, image_width, image_height);
	}
};

}
}

#endif /* TRANSFORM_H_ */
