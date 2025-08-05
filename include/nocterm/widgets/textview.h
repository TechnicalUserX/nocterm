/**
 * @file textview.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2025-08-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef NOCTERM_TEXTVIEW_H
#define NOCTERM_TEXTVIEW_H

#include <nocterm/common/nocterm.h>
#include <nocterm/base/widget.h>

#define NOCTERM_TEXTVIEW(x) ((nocterm_textview_t*)x)

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct nocterm_textview_t{
    nocterm_widget_t widget;
    nocterm_attribute_t attribute;
}nocterm_textview_t;

/**
 * @brief Creates a new textview widget.
 * 
 * @param bounds 
 * @param attribute 
 * @return nocterm_textview_t* 
 */
nocterm_textview_t* nocterm_textview_new(nocterm_dimension_t bounds, nocterm_attribute_t attribute);

/**
 * @brief Deletes a textview widget.
 * 
 * @param textview 
 * @return int 
 */
int nocterm_textview_delete(nocterm_textview_t* textview);

/**
 * @brief Sets text for a textview widget.
 * 
 * @param textview 
 * @param text 
 * @param text_size 
 * @return int 
 */
int nocterm_textview_set_text(nocterm_textview_t* textview, const char* text, uint64_t text_size);

/**
 * @brief Sets attribute for a textview widget.
 * 
 * @param textview 
 * @param attribute 
 * @return int 
 */
int nocterm_textview_set_attribute(nocterm_textview_t* textview, nocterm_attribute_t attribute);

/**
 * @brief Prints formatted text to a textview widget.
 * 
 * @param textview 
 * @param row 
 * @param col 
 * @param text 
 * @param text_size 
 * @return int 
 */
int nocterm_textview_print_text(nocterm_textview_t* textview, nocterm_dimension_size_t row, nocterm_dimension_size_t col, const char* text, uint64_t text_size);

/**
 * @brief Clears all text on a textview widget.
 * 
 * @param textview 
 * @return int 
 */
int nocterm_textview_clear(nocterm_textview_t* textview);

#ifdef __cplusplus
    }
#endif

#endif
