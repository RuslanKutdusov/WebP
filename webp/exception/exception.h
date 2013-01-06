
#ifndef EXCEPTION_H_
#define EXCEPTION_H_

namespace webp
{
namespace exception
{

class Exception
{
public:
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

}
}

#endif
