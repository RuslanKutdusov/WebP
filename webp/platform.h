#ifndef PLATFORM_H_
#define PLATFORM_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string>
#include <string.h>
#include <vector>
#include <map>

#ifdef LINUX
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifdef WINDOWS
#include <Windows.h>
#include <math.h>
#define log2f(a) (log(a)/log(2))
#endif

#endif /* PLATFORM_H_ */
