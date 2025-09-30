/**
 * @file char.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2025-08-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef NOCTERM_CHAR_H
#define NOCTERM_CHAR_H

#include <nocterm/common/nocterm.h>

#define NOCTERM_CHAR_EMPTY (nocterm_char_t){0}

#ifdef __cplusplus
    extern "C" {
#endif

// Printable
typedef struct nocterm_char_t{
    uint8_t bytes[4];
    uint8_t is_utf8:1;
    uint8_t bytes_size:3;
}nocterm_char_t;

/**
 * @brief Returns a nocterm_char_t array from C char array.
 * 
 * @param dest 
 * @param dest_size 
 * @param src 
 * @param src_size 
 * @return uint64_t 
 */
uint64_t nocterm_char_string_from_stream(nocterm_char_t* dest, uint64_t dest_size, const char* src, uint64_t src_size);


/**
 * @brief Returns a C char array from nocterm_char_t array.
 * 
 * @param dest 
 * @param dest_size 
 * @param src 
 * @param src_size 
 * @return uint64_t 
 */
uint64_t nocterm_char_string_to_stream(char* dest, uint64_t dest_size, const nocterm_char_t* src, uint64_t src_size);

/**
 * @brief Converts a C char to nocterm_char_t.
 * 
 * @param ch 
 * @return nocterm_char_t 
 */
nocterm_char_t nocterm_char_from_ascii(char ch);

/**
 * @brief Converts a wchar_t to nocterm_char_t.
 * 
 * @param ch 
 * @return nocterm_char_t 
 */
nocterm_char_t nocterm_char_from_utf8(wchar_t ch);

/**
 * @brief Checks whether a noctemr_char_t is equivalent to C null terminator char.
 * 
 * @param ch 
 * @return true 
 * @return false 
 */
bool nocterm_char_is_null(nocterm_char_t ch);

#ifdef __cplusplus
    }
#endif

#endif
