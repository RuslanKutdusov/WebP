#ifndef WEBP_H_
#define WEBP_H_
#include "platform.h"
#include "exception/exception.h"
#include "utils/utils.h"
#include "vp8/vp8.h"

#define WEBP_FILE_HEADER_LENGTH 12

namespace webp
{

enum FILE_FORMAT
{
	FILE_FORMAT_LOSSY,
	FILE_FORMAT_LOSSLESS
};

class VP8
{
private:
	char * vp8_data;
	uint32_t data_length;
	FILE_FORMAT file_format;
	VP8()
	{

	}
public:
	VP8(char ** iterable_pointer, const uint32_t & file_size)
	{
		if (memcmp(*iterable_pointer, "VP8 ", 4) == 0)
			file_format = FILE_FORMAT_LOSSY;
		else if (memcmp(*iterable_pointer, "VP8L", 4) == 0)
			file_format = FILE_FORMAT_LOSSLESS;
		else
			throw exception::UnsupportedVP8();
		*iterable_pointer += 4;
		data_length = file_size - 4;
		vp8_data = new char[data_length];
		memcpy(vp8_data, *iterable_pointer, data_length);
	}
	virtual ~VP8()
	{
		delete[] vp8_data;
	}
};

class WebP
{
private:
	uint32_t m_file_size;
	FILE_FORMAT m_file_format;
	uint8_t * m_vp8_data;
	uint32_t m_vp8_data_length;
	WebP()
	{

	}
	void read_webp_file_header(const uint8_t ** iterable_pointer)
	{
		if (memcmp(*iterable_pointer, "RIFF", 4) != 0)
			throw exception::InvalidWebPFileFormat();
		*iterable_pointer += 4;

		memcpy(&m_file_size, *iterable_pointer, sizeof(uint32_t));
		*iterable_pointer += sizeof(uint32_t);

		if (memcmp(*iterable_pointer, "WEBP", 4) != 0)
			throw exception::InvalidWebPFileFormat();
		*iterable_pointer += 4;
	}
	void init(const uint8_t * const encoded_data)
	{
		//чтобы бегать по данным и не потерять указатель на начало буфера
		const uint8_t * iterable_pointer = encoded_data;
		try
		{
			read_webp_file_header(&iterable_pointer);

			if (memcmp(iterable_pointer, "VP8 ", 4) == 0)
				m_file_format = FILE_FORMAT_LOSSY;
			else if (memcmp(iterable_pointer, "VP8L", 4) == 0)
				m_file_format = FILE_FORMAT_LOSSLESS;
			else
				throw exception::UnsupportedVP8();
			iterable_pointer += 4;

			//m_file_size это размер файла - 8(из заголовка RIFF(4 байта) и file_size(4 байта)),
			//data_length это m_file_size - 8(из заголовка WEBP(4 байта) и VP8_(4 байта))
			m_vp8_data_length = m_file_size - 8;
			m_vp8_data = new uint8_t[m_vp8_data_length];
			memcpy(m_vp8_data, iterable_pointer, m_vp8_data_length);
			if (m_file_format == FILE_FORMAT_LOSSLESS)
			{
				vp8l::VP8_LOSSLESS_DECODER decoder(m_vp8_data, m_vp8_data_length);
			}
		}
		catch(exception::InvalidWebPFileFormat & e)
		{
			throw e;
		}
		catch(exception::UnsupportedVP8 & e)
		{
			throw e;
		}
	}
public:
	WebP(const std::string & file_name)
	{
		uint32_t file_length;
		uint8_t * buf = NULL;
		try
		{
			buf = utils::read_file(file_name, file_length);
			if (file_length < WEBP_FILE_HEADER_LENGTH)
				throw exception::InvalidWebPFileFormat();
			init(buf);
		}
		catch(exception::InvalidWebPFileFormat & e)
		{
			delete[] buf;
			throw e;
		}
		catch(exception::UnsupportedVP8 & e)
		{
			delete[] buf;
			throw e;
		}
	}
	WebP(const uint8_t * const encoded_data, uint32_t encoded_data_length)
	{
		if (encoded_data_length < WEBP_FILE_HEADER_LENGTH)
			throw exception::InvalidWebPFileFormat();
		init(encoded_data);
	}
	virtual ~WebP()
	{
		delete m_vp8_data;
	}
};


}

#endif /* WEBP_H_ */
