/**
 * @file types.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2025-08-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef NOCTERM_TYPES_H
#define NOCTERM_TYPES_H

#include <nocterm/common/features.h>
#include <nocterm/common/include.h>

#ifdef __cplusplus
    #define atomic_bool std::atomic_bool
#endif

#define NOCTERM_SUCCESS 0
#define NOCTERM_FAILURE 1

#ifdef __cplusplus
    extern "C" {
#endif

// Terminal dimension for character grid

typedef uint16_t nocterm_dimension_size_t;

typedef struct nocterm_dimension_t{
    nocterm_dimension_size_t row;
    nocterm_dimension_size_t col;
    nocterm_dimension_size_t height;
    nocterm_dimension_size_t width;
}nocterm_dimension_t;

typedef uint8_t nocterm_percentage_t;


#ifdef __cplusplus
    }
#endif

#define NOCTERM_DIMENSION_MAX UINT16_MAX

#ifdef NOCTERM_SHARED_LIBRARY
    #define NOCTERM_INTERNAL __attribute__((visibility("hidden")))
#else
    #define NOCTERM_INTERNAL
#endif


#endif
