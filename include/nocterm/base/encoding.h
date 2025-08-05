/**
 * @file encoding.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2025-08-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef NOCTERM_ENCODING_H
#define NOCTERM_ENCODING_H

#include <nocterm/common/nocterm.h>

#ifdef __cplusplus
    extern "C" {
#endif

typedef enum nocterm_encoding_t{
    NOCTERM_ENCODING_UNKNOWN,
    NOCTERM_ENCODING_ASCII,
    NOCTERM_ENCODING_UTF8
}nocterm_encoding_t;

#ifdef __cplusplus
    }
#endif

/**
 * @brief Returns current terminal's character encoding.
 * 
 * @return nocterm_encoding_t 
 */
nocterm_encoding_t nocterm_encoding_get(void);

#endif
