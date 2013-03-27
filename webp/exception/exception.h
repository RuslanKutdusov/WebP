
#ifndef EXCEPTION_H_
#define EXCEPTION_H_

#include <string>
#include <stdint.h>
#include <iostream>

namespace webp
{
namespace exception
{

class Exception
{
public:
	std::string message;
	virtual ~Exception()
	{

	}
};

class FileOperationException : public Exception
{
private:
	int m_errno;
public:
	FileOperationException()
	{

	}
	/*FileOperationException(int errno_)
		: m_errno(errno_)
	{

	}*/
	virtual ~FileOperationException()
	{

	}
};

class MemoryAllocationException : public Exception
{
public:
	virtual ~MemoryAllocationException()
	{

	}
};


class InvalidWebPFileFormat : public Exception
{
public:
	virtual ~InvalidWebPFileFormat()
	{

	}
};

class UnsupportedVP8 : public Exception
{
public:
	virtual ~UnsupportedVP8()
	{

	}
};

class InvalidVP8L : public Exception
{
public:
	virtual ~InvalidVP8L()
	{

	}
};

class InvalidHuffman : public Exception
{
public:
	virtual ~InvalidHuffman()
	{

	}
};

class PNGError : public Exception
{
public:
	virtual ~PNGError()
	{

	}
};

class TooBigCodeLength : public Exception
{
public:
	TooBigCodeLength(const uint32_t & max_allowed_code_length, const uint32_t & max_code_length){
		std::cout << "Too big Huffman code length(" << max_code_length << "). Max allowed code length = " << max_allowed_code_length << std::endl;
	}
	virtual ~TooBigCodeLength()
	{

	}
};

}
}

#endif
