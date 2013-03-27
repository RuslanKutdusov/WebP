#ifndef BIT_WRITER_H_
#define BIT_WRITER_H_

#include "../platform.h"
#include "../exception/exception.h"
#include "utils.h"
#include "bit_readed.h"
#include <deque>
#include <string>

namespace webp
{
namespace utils
{

static const uint32_t BitWriterBufferArraySize = 16384;

class BitWriterTest;

class BitWriter
{
private:
	std::deque<byte_array>		m_buffer;
	uint32_t					m_size;//байт в буфере
	uint32_t					m_last_byte_index;//индекс на байт в который пишем биты
	uint32_t					m_bits_writed_in_byte;//сколько бит записали в байт
	void add_array()
	{
		m_buffer.push_back(byte_array());
		m_buffer.back().realloc(BitWriterBufferArraySize);
		m_buffer.back().fill(0);
		m_last_byte_index = 0;
		m_bits_writed_in_byte = 0;
	}
public:
	BitWriter()
		: m_size(0), m_last_byte_index(0), m_bits_writed_in_byte(0)
	{

	}
	virtual ~BitWriter()
	{

	}
	void WriteBit(const uint32_t & bit)
	{
		if (m_buffer.size() == 0)
			add_array();
		if ((m_last_byte_index == BitWriterBufferArraySize - 1) && (m_bits_writed_in_byte == 8))
			add_array();
		if (m_bits_writed_in_byte == 8)	{
			m_last_byte_index++;
			m_bits_writed_in_byte = 0;
		}
		if (m_bits_writed_in_byte == 0)
			m_size++;
		uint8_t * byte = &m_buffer.back()[m_last_byte_index];
		*byte = *byte | ((bit & 1u) << m_bits_writed_in_byte++);
	}
	void WriteBits(const uint32_t & bits, const uint32_t & count)
	{
		for(uint32_t i = 0; i < count; i++)
			WriteBit((bits >> i) & 1u);
	}
	void save2file(const std::string & file_name) const
	{
		FILE * fp = NULL;

		#ifdef LINUX
		  fp = fopen(file_name.c_str(), "ab+");
		#endif
		#ifdef WINDOWS
		  fopen_s(&fp, file_name.c_str(), "ab+");
		#endif

		if (fp == NULL)
			throw exception::FileOperationException();

		for(size_t i = 0; i < m_buffer.size() - 1; i++)
			fwrite(&m_buffer.at(i)[0], BitWriterBufferArraySize, 1, fp);
		int32_t last_array_index = m_buffer.size() - 1;
		size_t prev_len = BitWriterBufferArraySize * last_array_index;
		if (last_array_index != -1)
			fwrite(&m_buffer.at(last_array_index)[0], m_size - prev_len, 1, fp);

		fclose(fp);
	}
	const size_t size() const{
		return m_size;
	}
	friend class BitWriterTest;
};

class BitWriterTest
{
public:
	void test()
	{
		BitWriter bw;
		byte_array ar;
		uint32_t len;
		read_file("3.png", len, ar);
		BitReader br(&ar[0], len);
		while(!br.eos()){
			uint8_t bits2read = rand() % 8;
			bits2read = bits2read == 0 ? 1 : bits2read;
			uint32_t bits = br.ReadBits(bits2read);
			bw.WriteBits(bits, bits2read);
		}
		bw.save2file("bit_writer_test");
	}
};

}
}


#endif /* BIT_WRITER_H_ */
