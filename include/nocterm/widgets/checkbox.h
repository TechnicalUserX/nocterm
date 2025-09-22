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
    nocterm_attribute_t main_attribute;
    nocterm_attribute_t cursor_attribute;
    nocterm_char_t check_marker;
    nocterm_char_t left_side;
    nocterm_char_t right_side;
    nocterm_checkbox_oncheck_handler_t oncheck_handler;
    bool checked;
    void* user_data;
}nocterm_checkbox_t;

/**
 * @brief Creates a new checkbox widget.
 * 
 * @param row 
 * @param col 
 * @param oncheck_handler 
 * @param checked 
 * @param user_data 
 * @return nocterm_checkbox_t* 
 */
nocterm_checkbox_t* nocterm_checkbox_new(nocterm_checkbox_oncheck_handler_t oncheck_handler, bool checked, void* user_data);

/**
 * @brief Constructs a checkbox widget.
 * 
 * @param checkbox 
 * @param row 
 * @param col 
 * @param oncheck_handler 
 * @param checked 
 * @param user_data 
 * @return int 
 */
int nocterm_checkbox_constructor(nocterm_checkbox_t* checkbox, nocterm_checkbox_oncheck_handler_t oncheck_handler, bool checked, void* user_data);

/**
 * @brief Destructs a checkbox widget.
 * 
 * @param checkbox 
 * @return int 
 */
int nocterm_checkbox_destructor(nocterm_checkbox_t* checkbox);

/**
 * @brief Deletes a checkbox widget.
 * 
 * @param checkbox 
 * @return int 
 */
int nocterm_checkbox_delete(nocterm_checkbox_t* checkbox);

/**
 * @brief Sets the attribute of a checkbox widget.
 * 
 * @param checkbox 
 * @param attribute 
 * @return int 
 */
int nocterm_checkbox_set_attribute(nocterm_checkbox_t* checkbox, nocterm_attribute_t attribute);

/**
 * @brief Sets the marker character of a checkbox widget.
 * 
 * @param checkbox 
 * @param marker 
 * @return int 
 */
int nocterm_checkbox_set_marker(nocterm_checkbox_t* checkbox, nocterm_char_t marker);

/**
 * @brief Sets left and right side characters of a checkbox widget.
 * 
 * @param checkbox 
 * @param left 
 * @param right 
 * @return int 
 */
int nocterm_checkbox_set_sides(nocterm_checkbox_t* checkbox, nocterm_char_t left, nocterm_char_t right);

#ifdef __cplusplus
    }
#endif

#endif
