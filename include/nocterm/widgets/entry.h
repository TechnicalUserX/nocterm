/**
 * @file entry.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2025-08-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef NOCTERM_ENTRY_H
#define NOCTERM_ENTRY_H

#include <nocterm/common/nocterm.h>
#include <nocterm/base/widget.h>

#define NOCTERM_ENTRY(x) ((nocterm_entry_t*)x)

#define NOCTERM_ENTRY_BUFFER_MAX_SIZE 128

#define NOCTERM_ENTRY_CURSOR_CHAR NOCTERM_CHAR_SPACE

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct nocterm_entry_t{
    nocterm_widget_t widget;
    nocterm_attribute_t normal_attribute;
    nocterm_attribute_t cursor_attribute;
    uint16_t cursor_position; // Viewport related
    uint16_t buffer_position; // Widget buffer related
    uint64_t current_length;
}nocterm_entry_t;

/**
 * @brief Creates a new entry widget.
 * 
 * @param row 
 * @param col 
 * @param width 
 * @param attribute 
 * @return nocterm_entry_t* 
 */
nocterm_entry_t* nocterm_entry_new(nocterm_dimension_size_t row, nocterm_dimension_size_t col, nocterm_dimension_size_t width, nocterm_attribute_t attribute);

/**
 * @brief Deletes an entry widget.
 * 
 * @param entry 
 * @return int 
 */
int nocterm_entry_delete(nocterm_entry_t* entry);

NOCTERM_INTERNAL
int nocterm_entry_cursor_move_left(nocterm_entry_t* entry);

NOCTERM_INTERNAL
int nocterm_entry_cursor_move_right(nocterm_entry_t* entry);

NOCTERM_INTERNAL
int nocterm_entry_cursor_insert(nocterm_entry_t* entry, nocterm_char_t character);

NOCTERM_INTERNAL
int nocterm_entry_cursor_erase_right(nocterm_entry_t* entry); // Delete Button

NOCTERM_INTERNAL
int nocterm_entry_cursor_erase_left(nocterm_entry_t* entry); // Backspace Button

NOCTERM_INTERNAL
NOCTERM_WIDGET_KEY_HANDLER(nocterm_entry_key_handler);

NOCTERM_INTERNAL
NOCTERM_WIDGET_FOCUS_HANDLER(nocterm_entry_focus_handler);

/**
 * @brief Retrieves text from an entry widget.
 * 
 * @param entry 
 * @param buffer 
 * @param buffer_size 
 * @param entry_length 
 * @return int 
 */
int nocterm_entry_get_text(nocterm_entry_t* entry, char* buffer, uint64_t buffer_size, uint64_t* entry_length);

/**
 * @brief Sets text for an entry widget.
 * 
 * @param entry 
 * @param buffer 
 * @param buffer_size 
 * @return int 
 */
int nocterm_entry_set_text(nocterm_entry_t* entry, char* buffer, uint64_t buffer_size);

#ifdef __cplusplus
    }
#endif

#endif
