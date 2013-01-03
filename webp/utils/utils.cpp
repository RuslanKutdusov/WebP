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

void read_file(const std::string & file_name, uint32_t & file_length_out, array<uint8_t> & buf)
{
	FILE *fp = NULL;

#ifdef LINUX
	fp = fopen(file_name.c_str(), "rb");
#endif
#ifdef WINDOWS
	fopen_s(&fp, file_name.c_str(), "rb");
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

	buf.realloc(file_length_out);

	size_t readed = fread(&buf, 1, file_length_out, fp);
	if (readed != file_length_out)
	{
		fclose(fp);
		throw exception::FileOperationException();
	}

	fclose(fp);
}

}
}
