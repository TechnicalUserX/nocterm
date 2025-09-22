/**
 * @file include.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2025-08-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef NOCTERM_INCLUDE_H
#define NOCTERM_INCLUDE_H

#include <nocterm/common/features.h>

// Language independent
#include <termios.h>
#include <unistd.h>
#include <langinfo.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#if defined(_PTHREAD_H)
    #error "Do not include pthread.h before including this library"
#endif

#include <pthread.h>
#include <signal.h>

#ifdef __cplusplus
    #include <atomic>
    #include <cstdarg>
    #include <cstdio>
    #include <cinttypes>
    #include <cstring>
    #include <cstdlib>
    #include <cstdbool>
    #include <cwchar>
    #include <ctime>
    #include <cerrno>
    #include <thread>
    #include <clocale>
    #include <cctype>
#else
    #include <stdatomic.h>
    #include <stdarg.h>
    #include <stdbool.h>
    #include <stdio.h>
    #include <inttypes.h>
    #include <stdlib.h>
    #include <string.h>
    #include <wchar.h>
    #include <time.h>
    #include <errno.h>
    #include <locale.h>
    #include <ctype.h>
#endif

#endif