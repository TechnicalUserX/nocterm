/**
 * @file button.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2025-08-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef NOCTERM_BUTTON_H
#define NOCTERM_BUTTON_H

#include <nocterm/common/nocterm.h>
#include <nocterm/base/widget.h>

#define NOCTERM_BUTTON(x) ((nocterm_button_t*)x)

/**
 * @brief Macro for creating an "on press" button handler.
 * 
 */
#define NOCTERM_BUTTON_ONPRESS_HANDLER(identifier) void identifier(nocterm_widget_t* self, void* user_data)

#ifdef __cplusplus
    extern "C" {
#endif

typedef void (*nocterm_button_onpress_handler_t)(nocterm_widget_t* self, void* user_data);
typedef void (*nocterm_button_focus_handler_t)(nocterm_widget_t* self);

typedef struct nocterm_button_t{
    nocterm_widget_t widget;
    nocterm_attribute_t attribute_normal;
    nocterm_attribute_t attribute_focused;
    nocterm_button_onpress_handler_t onpress_handler;
    void* user_data;
}nocterm_button_t;

/**
 * @brief Creates a new button widget.
 * 
 * @param row 
 * @param col 
 * @param text 
 * @param text_size 
 * @param normal 
 * @param focused 
 * @param onpress_handler 
 * @param user_data 
 * @return nocterm_button_t* 
 */
nocterm_button_t* nocterm_button_new(nocterm_dimension_size_t row, nocterm_dimension_size_t col, const char* text, uint64_t text_size, nocterm_attribute_t normal, nocterm_attribute_t focused, nocterm_button_onpress_handler_t onpress_handler, void* user_data);

/**
 * @brief Deletes a button widget.
 * 
 * @param button 
 * @return int 
 */
int nocterm_button_delete(nocterm_button_t* button);

NOCTERM_INTERNAL
NOCTERM_WIDGET_KEY_HANDLER(nocterm_button_key_handler);

NOCTERM_INTERNAL
NOCTERM_WIDGET_FOCUS_HANDLER(nocterm_button_focus_handler);

#ifdef __cplusplus
    }
#endif

#endif
