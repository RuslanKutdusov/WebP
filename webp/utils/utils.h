#ifndef UTILS_H_
#define UTILS_H_

#include "../platform.h"
#include "../exception/exception.h"

namespace webp
{
namespace utils
{

template <class T>
class array
{
private:
	T*			m_array;
	size_t		m_size;
	void release()
	{
		if (m_array != NULL)
		{
			delete[] m_array;
			m_size = 0;
			m_array = NULL;
		}
	}
	void copy(const array & a)
	{
		if (a.m_array != m_array)
		{
			release();
			m_size = a.m_size;
			if (m_size == 0)
				m_array = NULL;
			else
			{
				m_array = new T[m_size];
				memcpy(m_array, a.m_array, a.m_size * sizeof(T));
			}
		}
	}
public:
	array()
		: m_array(NULL), m_size(0)
	{

	}
	array(const array & a)
		: m_array(NULL), m_size(0)
	{
		copy(a);
	}
	array & operator=(const array & a)
	{
		copy(a);
		return *this;
	}
	array(const uint32_t & size)
		: m_array(new T[size]), m_size(size)
	{
		fill(0);
	}
	void move_ref(array & a){
		release();
		this->m_array = a.m_array;
		this->m_size = a.m_size;
		a.m_array = NULL;
		a.m_size = 0;
	}
	virtual ~array()
	{
		release();
	}
	void realloc(const uint32_t & size)
	{
		release();
		m_array = new T[size];
		m_size = size;
	}
	T& operator[](const size_t & i)
	{
		return m_array[i];
	}
	T& operator[](const size_t & i) const
	{
		return m_array[i];
	}
	const size_t & size() const
	{
		return m_size;
	}
	const T& operator*() const
	{
		return *m_array;
	}
	T* operator+(const int & i)
	{
		return m_array + i;
	}
	void fill(int c)
	{
		memset(m_array, c, m_size * sizeof(T));
	}
	void sort(int(*compar)(const void *, const void *)){
		qsort(m_array, m_size, sizeof(T), compar);
	}
};

typedef array<uint32_t> pixel_array;
typedef array<uint8_t> byte_array;


uint8_t * ALPHA(const uint32_t & argb);
uint8_t * RED(const uint32_t & argb);
uint8_t * GREEN(const uint32_t & argb);
uint8_t * BLUE(const uint32_t & argb);

typedef uint8_t* (*COLOR_t)(const uint32_t & argb);
static const COLOR_t COLOR[4] = {ALPHA, RED, GREEN, BLUE};
void read_file(const std::string & file_name, uint32_t & file_length_out, array<uint8_t> & buf);

}
}
#endif /* UTILS_H_ */
