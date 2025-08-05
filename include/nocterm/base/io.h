/**
 * @file io.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2025-08-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef NOCTERM_IO_H
#define NOCTERM_IO_H

#include <nocterm/common/nocterm.h>
#include <nocterm/base/char.h>

#ifndef NOCTERM_IO_BUFFER_SIZE
    #define NOCTERM_IO_BUFFER_SIZE 8192
#endif

#ifdef __cplusplus
    extern "C" {
#endif


/**
 * @brief Initializes stdout and stdin.
 * 
 * @return int 
 */
int nocterm_io_init(void);


/**
 * @brief De-initializes stdout and stdin.
 * 
 * @return int 
 */
int nocterm_io_end(void);

/**
 * @brief Prints formatted output at the current cursor position.
 * 
 * @param format 
 * @param ... 
 * @return int 
 */
int nocterm_io_print(const char* format, ...);


/**
 * @brief Prints formatted output at the specified position.
 * 
 * @param row 
 * @param col 
 * @param format 
 * @param ... 
 * @return int 
 */
int nocterm_io_print_at(nocterm_dimension_size_t row, nocterm_dimension_size_t col, const char* format, ...);

/**
 * @brief Checks whether stdin buffer contains readable bytes.
 * 
 * @param available 
 * @param timeout 
 * @return int 
 */
int nocterm_io_read_available(bool* available, struct timeval timeout);


/**
 * @brief Reads bytes from stdin.
 * 
 * @param buffer 
 * @param buffer_size 
 * @return int 
 */
int nocterm_io_read(void* buffer, uint64_t buffer_size);


/**
 * @brief Reads bytes just to throw them away until stdin buffer is empty.
 * 
 * @return int 
 */
int nocterm_io_consume(void);

/**
 * @brief Writes bytes at the current cursor position.
 * 
 * @param buffer 
 * @param buffer_size 
 * @return int 
 */
int nocterm_io_write(void* buffer, uint64_t buffer_size);

/**
 * @brief Writes bytes at the specified cursor position.
 * 
 * @param row 
 * @param col 
 * @param buffer 
 * @param buffer_size 
 * @return int 
 */
int nocterm_io_write_at(nocterm_dimension_size_t row, nocterm_dimension_size_t col, void* buffer, uint64_t buffer_size);

/**
 * @brief Clears terminal screen.
 * 
 * @return int 
 */
int nocterm_io_clear(void);

/**
 * @brief Prints nocterm_char_t character at the current cursor position.
 * 
 * @param c 
 * @return int 
 */
int nocterm_io_put_char(nocterm_char_t c);

/**
 * @brief Prints nocterm_char_t character at the specified cursor position.
 * 
 * @param row 
 * @param col 
 * @param c 
 * @return int 
 */
int nocterm_io_put_char_at(nocterm_dimension_size_t row, nocterm_dimension_size_t col, nocterm_char_t c);

/**
 * @brief Deletes a character at the current cursor position.
 * 
 * @return int 
 */
int nocterm_io_erase_char(void);

/**
 * @brief Deletes a character at the specified cursor position.
 * 
 * @param row 
 * @param col 
 * @return int 
 */
int nocterm_io_erase_char_at(nocterm_dimension_size_t row, nocterm_dimension_size_t col);

/**
 * @brief Hides or shows cursor.
 * 
 * @param visible 
 * @return int 
 */
int nocterm_io_cursor_visible(bool visible);

/**
 * @brief Moves cursor to a specified position.
 * 
 * @param row 
 * @param col 
 * @return int 
 */
int nocterm_io_cursor_move(nocterm_dimension_size_t row, nocterm_dimension_size_t col);

#ifdef __cplusplus
    }
#endif

#endif
