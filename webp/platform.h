#ifndef PLATFORM_H_
#define PLATFORM_H_

#define LINUX
//#define WINDOWS

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string>
#include <string.h>

#ifdef LINUX
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifdef WINDOWS
#include <Windows.h>
#endif

#endif /* PLATFORM_H_ */
