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

#ifndef CONFIG_NOCTERM_ENTRY_BUFFER_MAX_SIZE
    #define NOCTERM_ENTRY_BUFFER_MAX_SIZE 4096
#else
    #define NOCTERM_ENTRY_BUFFER_MAX_SIZE CONFIG_NOCTERM_ENTRY_BUFFER_MAX_SIZE
#endif

#define NOCTERM_ENTRY_CURSOR_CHAR nocterm_char_from_ascii(' ')

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
    nocterm_char_t text_store[NOCTERM_ENTRY_BUFFER_MAX_SIZE + 1]; // Backing store — survives flex resizes
}nocterm_entry_t;

/**
 * @brief Creates a new entry widget.
 * 
 * @param width 
 * @return nocterm_entry_t* 
 */
nocterm_entry_t* nocterm_entry_new(nocterm_dimension_size_t width);

/**
 * @brief Constructs an entry widget.
 * 
 * @param entry 
 * @param width 
 * @return int 
 */
int nocterm_entry_constructor(nocterm_entry_t* entry, nocterm_dimension_size_t width);

/**
 * @brief Destructs an entry widget.
 * 
 * @param entry 
 * @return int 
 */
int nocterm_entry_destructor(nocterm_entry_t* entry);

/**
 * @brief Deletes an entry widget.
 * 
 * @param entry 
 * @return int 
 */
int nocterm_entry_delete(nocterm_entry_t* entry);

/**
 * @brief Sets the attribute of an entry widget.
 * 
 * @param entry 
 * @param text 
 * @param cursor 
 * @return int 
 */
int nocterm_entry_set_attribute(nocterm_entry_t* entry, nocterm_attribute_t text, nocterm_attribute_t cursor);

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

