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

#ifndef CONFIG_NOCTERM_BUTTON_TEXT_MAX_SIZE
    #define NOCTERM_BUTTON_TEXT_MAX_SIZE 256
#else
    #define NOCTERM_BUTTON_TEXT_MAX_SIZE CONFIG_NOCTERM_BUTTON_TEXT_MAX_SIZE    
#endif

/**
 * @brief Macro for creating an "on press" button handler.
 *
 */
#define NOCTERM_BUTTON_ONPRESS_HANDLER(identifier) void identifier(nocterm_widget_t* self, void* user_data)

#ifdef __cplusplus
    extern "C" {
#endif

typedef void (*nocterm_button_onpress_handler_t)(nocterm_widget_t* self, void* user_data);

typedef struct nocterm_button_t{
    nocterm_widget_t widget;
    nocterm_attribute_t attribute_normal;
    nocterm_attribute_t attribute_focused;
    nocterm_button_onpress_handler_t onpress_handler;
    void* user_data;
    nocterm_char_t text[NOCTERM_BUTTON_TEXT_MAX_SIZE];
    uint64_t text_length;
}nocterm_button_t;

/**
 * @brief Creates a button widget.
 * 
 * @param height 
 * @param width 
 * @param onpress_handler 
 * @param user_data 
 * @return nocterm_button_t* 
 */
nocterm_button_t* nocterm_button_new(nocterm_dimension_size_t height, nocterm_dimension_size_t width, nocterm_button_onpress_handler_t onpress_handler, void* user_data);

/**
 * @brief Constructs a button widget.
 * 
 * @param button 
 * @param height 
 * @param width 
 * @param onpress_handler 
 * @param user_data 
 * @return int 
 */
int nocterm_button_constructor(nocterm_button_t* button, nocterm_dimension_size_t height, nocterm_dimension_size_t width, nocterm_button_onpress_handler_t onpress_handler, void* user_data);

/**
 * @brief Destructs a button widget.
 * 
 * @param button 
 * @return int 
 */
int nocterm_button_destructor(nocterm_button_t* button);

/**
 * @brief Deletes a button widget.
 * 
 * @param button 
 * @return int 
 */
int nocterm_button_delete(nocterm_button_t* button);

/**
 * @brief Sets the attributes of a button widget.
 * 
 * @param button 
 * @param normal 
 * @param focused 
 * @return int 
 */
int nocterm_button_set_attribute(nocterm_button_t* button, nocterm_attribute_t normal, nocterm_attribute_t focused);

/**
 * @brief Sets the text of a button widget.
 * 
 * @param button 
 * @param text 
 * @param text_size 
 * @return int 
 */
int nocterm_button_set_text(nocterm_button_t* button, const char* text, uint64_t text_size);

#ifdef __cplusplus
    }
#endif

#endif
