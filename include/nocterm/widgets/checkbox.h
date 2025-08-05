/**
 * @file checkbox.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2025-08-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef NOCTERM_CHECKBOX_H
#define NOCTERM_CHECKBOX_H

#include <nocterm/common/nocterm.h>
#include <nocterm/base/widget.h>

#define NOCTERM_CHECKBOX(x) ((nocterm_checkbox_t*)x)

/**
 * @brief Macro for creating an "on check" checkbox handler.
 * 
 */
#define NOCTERM_CHECKBOX_ONCHECK_HANDLER(identifier) void identifier(nocterm_widget_t* self, nocterm_checkbox_action_t action, void* user_data)

#ifdef __cplusplus
    extern "C" {
#endif

typedef enum nocterm_checkbox_action_t{
    NOCTERM_CHECKBOX_ACTION_CHECK,
    NOCTERM_CHECKBOX_ACTION_UNCHECK
}nocterm_checkbox_action_t;

typedef void (*nocterm_checkbox_oncheck_handler_t)(nocterm_widget_t* self, nocterm_checkbox_action_t action, void* user_data);

typedef struct nocterm_checkbox_t{
    nocterm_widget_t widget;
    nocterm_attribute_t attribute_normal;
    nocterm_attribute_t attribute_cursor;
    nocterm_checkbox_oncheck_handler_t oncheck_handler;
    bool checked;
    void* user_data;
}nocterm_checkbox_t;

/**
 * @brief Creates a new checkbox widget.
 * 
 * @param row 
 * @param col 
 * @param attribute 
 * @param oncheck_handler 
 * @param checked 
 * @param user_data 
 * @return nocterm_checkbox_t* 
 */
nocterm_checkbox_t* nocterm_checkbox_new(nocterm_dimension_size_t row, nocterm_dimension_size_t col, nocterm_attribute_t attribute, nocterm_checkbox_oncheck_handler_t oncheck_handler, bool checked, void* user_data);

/**
 * @brief Deletes a checkbox widget.
 * 
 * @param checkbox 
 * @return int 
 */
int nocterm_checkbox_delete(nocterm_checkbox_t* checkbox);

NOCTERM_INTERNAL
NOCTERM_WIDGET_KEY_HANDLER(nocterm_checkbox_key_handler);

NOCTERM_INTERNAL
NOCTERM_WIDGET_FOCUS_HANDLER(nocterm_checkbox_focus_handler);

#ifdef __cplusplus
    }
#endif

#endif
