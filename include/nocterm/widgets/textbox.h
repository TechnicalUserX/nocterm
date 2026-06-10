/**
 * @file textbox.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2025-08-04
 *
 * @copyright Copyright (c) 2025
 *
 */

#ifndef NOCTERM_TEXTBOX_H
#define NOCTERM_TEXTBOX_H

#include <nocterm/common/nocterm.h>
#include <nocterm/base/widget.h>

#define NOCTERM_TEXTBOX(x) ((nocterm_textbox_t*)x)

#ifndef CONFIG_NOCTERM_TEXTBOX_BUFFER_MAX_SIZE
    #define NOCTERM_TEXTBOX_BUFFER_MAX_SIZE 8192
#else
    #define NOCTERM_TEXTBOX_BUFFER_MAX_SIZE CONFIG_NOCTERM_TEXTBOX_BUFFER_MAX_SIZE
#endif

#define NOCTERM_TEXTBOX_CURSOR_CHAR nocterm_char_from_ascii(' ')

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct nocterm_textbox_t{
    nocterm_widget_t widget;
    nocterm_attribute_t normal_attribute;
    nocterm_attribute_t cursor_attribute;
    nocterm_char_t text_store[NOCTERM_TEXTBOX_BUFFER_MAX_SIZE];
    uint64_t text_length;
    uint64_t buffer_position; // Absolute index into text_store; range [0, text_length]
    uint64_t scroll_offset;   // First visible wrapped-line index
}nocterm_textbox_t;

/**
 * @brief Creates a new textbox widget.
 *
 * @param height
 * @param width
 * @return nocterm_textbox_t*
 */
nocterm_textbox_t* nocterm_textbox_new(nocterm_dimension_size_t height, nocterm_dimension_size_t width);

/**
 * @brief Constructs a textbox widget.
 *
 * @param textbox
 * @param height
 * @param width
 * @return int
 */
int nocterm_textbox_constructor(nocterm_textbox_t* textbox, nocterm_dimension_size_t height, nocterm_dimension_size_t width);

/**
 * @brief Destructs a textbox widget.
 *
 * @param textbox
 * @return int
 */
int nocterm_textbox_destructor(nocterm_textbox_t* textbox);

/**
 * @brief Deletes a textbox widget.
 *
 * @param textbox
 * @return int
 */
int nocterm_textbox_delete(nocterm_textbox_t* textbox);

/**
 * @brief Sets the attributes of a textbox widget.
 *
 * @param textbox
 * @param text
 * @param cursor
 * @return int
 */
int nocterm_textbox_set_attribute(nocterm_textbox_t* textbox, nocterm_attribute_t text, nocterm_attribute_t cursor);

/**
 * @brief Retrieves text from a textbox widget.
 *
 * @param textbox
 * @param buffer
 * @param buffer_size
 * @param text_length
 * @return int
 */
int nocterm_textbox_get_text(nocterm_textbox_t* textbox, char* buffer, uint64_t buffer_size, uint64_t* text_length);

/**
 * @brief Sets text for a textbox widget.
 *
 * @param textbox
 * @param buffer
 * @param buffer_size
 * @return int
 */
int nocterm_textbox_set_text(nocterm_textbox_t* textbox, const char* buffer, uint64_t buffer_size);

/**
 * @brief Clears all text in a textbox widget.
 *
 * @param textbox
 * @return int
 */
int nocterm_textbox_clear(nocterm_textbox_t* textbox);

#ifdef __cplusplus
    }
#endif

#endif
