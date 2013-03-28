
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
	Exception()
		: message("Exception")
	{
	}
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
		message = "File operation exception";
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
	MemoryAllocationException(){
		message = "Memory allocation exception";
	}
	virtual ~MemoryAllocationException()
	{

	}
};


class InvalidWebPFileFormat : public Exception
{
public:
	InvalidWebPFileFormat(){
		message = "Invalid WebP File format";
	}
	virtual ~InvalidWebPFileFormat()
	{

	}
};

class UnsupportedVP8 : public Exception
{
public:
	UnsupportedVP8(){
		message = "VP8 is unsupported";
	}
	virtual ~UnsupportedVP8()
	{

	}
};

class InvalidVP8L : public Exception
{
public:
	InvalidVP8L(){
		message = "Invalid VP8L";
	}
	virtual ~InvalidVP8L()
	{

	}
};

class InvalidHuffman : public Exception
{
public:
	InvalidHuffman(){
		message = "Invalid format";
	}
	virtual ~InvalidHuffman()
	{

	}
};

class PNGError : public Exception
{
public:
	PNGError(){
		message = "PNG error";
	}
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

class InvalidARGBImage : public Exception
{
public:
	InvalidARGBImage(){
		message = "Invalid ARGB image";
	}
	virtual ~InvalidARGBImage(){

	}
};

class TooBigARGBImage : public Exception
{
public:
	TooBigARGBImage(const size_t & max_size){
		std::cout << "Too big ARGB Image. Max allowed width(height)=" << max_size << std::endl;
	}
	virtual ~TooBigARGBImage(){

	}
};

}
}

#endif
