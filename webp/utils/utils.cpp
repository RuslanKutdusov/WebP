#include "utils.h"

namespace webp
{
namespace utils
{

uint32_t get_file_size(FILE * file)
{
	if (fseek(file, 0, SEEK_END) != 0)
		throw exception::FileOperationException();
	long int len_long_int = ftell(file);
	if (len_long_int == -1L)
		throw exception::FileOperationException();
	if (fseek(file, 0, SEEK_SET) != 0)
		throw exception::FileOperationException();
	return len_long_int;
}

uint8_t *read_file(const std::string & file_name, uint32_t & file_length_out)
{
	uint8_t *ret = NULL;
	FILE *fp = NULL;

#ifdef LINUX
	fp = fopen(file_name.c_str(), "r");
#endif
#ifdef WINDOWS
	fopen_s(&fp, file_name.c_str(), "r");
#endif
	if (fp == NULL)
		throw exception::FileOperationException();

	try
	{
		file_length_out = get_file_size(fp);
	}
	catch (exception::FileOperationException & file_operation_exception)
	{
		fclose(fp);
		throw file_operation_exception;
	}

	ret = new(std::nothrow) uint8_t[file_length_out];
	if (!ret)
	{
		fclose(fp);
		throw exception::MemoryAllocationException();
	}

	size_t readed = fread(ret, 1, file_length_out, fp);
	if (readed != file_length_out)
	{
		delete[] ret;
		fclose(fp);
		throw exception::FileOperationException();
	}

	fclose(fp);

	return ret;
}

uint8_t * ALPHA(const uint32_t & argb)
{
        return (uint8_t*)(&argb) + 3;
}

uint8_t * RED(const uint32_t & argb)
{
        return (uint8_t*)(&argb) + 2;
}

uint8_t * GREEN(const uint32_t & argb)
{
        return (uint8_t*)(&argb) + 1;
}

uint8_t * BLUE(const uint32_t & argb)
{
        return (uint8_t*)(&argb);
}

}
}
