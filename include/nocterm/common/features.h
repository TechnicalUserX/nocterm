/**
 * @file features.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2025-08-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef NOCTERM_FEATURES_H
#define NOCTERM_FEATURES_H

// Supported standards: (> C11), (> C++11)
// Supported compilers: gcc, clang

#if !defined(__GNUC__) && !defined(__clang__)
#error "This library is only intended for gcc and clang compilers"
#endif

#ifdef __cplusplus

#if __cplusplus < 201103L
#error "Accepted minimum C++ standard is C++11"
#endif

#else

#if __STDC_VERSION__ < 201112L
#error "Accepted minimum C standard is C11"
#endif

#endif

#if !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#endif