#ifndef UTILS_H_
#define UTILS_H_

#include "../platform.h"
#include "../exception/exception.h"

namespace webp
{
namespace utils
{

char *read_file(const std::string & file_name, uint32_t & file_length_out);

}
}
#endif /* UTILS_H_ */
