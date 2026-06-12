/**
 * @file radiobutton.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2026-06-12
 *
 * @copyright Copyright (c) 2026
 *
 */

#ifndef NOCTERM_RADIOBUTTON_H
#define NOCTERM_RADIOBUTTON_H

#include <nocterm/common/nocterm.h>
#include <nocterm/base/widget.h>

#define NOCTERM_RADIOBUTTON(x) ((nocterm_radiobutton_t*)x)
#define NOCTERM_RADIOBUTTON_GROUP(x) ((nocterm_radiobutton_group_t*)x)

/**
 * @brief Macro for creating an "on select" radio button handler.
 *
 */
#define NOCTERM_RADIOBUTTON_ONSELECT_HANDLER(identifier) void identifier(nocterm_widget_t* self, nocterm_radiobutton_action_t action, void* user_data)

#ifdef __cplusplus
    extern "C" {
#endif

typedef enum nocterm_radiobutton_action_t{
    NOCTERM_RADIOBUTTON_ACTION_SELECT,
    NOCTERM_RADIOBUTTON_ACTION_DESELECT
}nocterm_radiobutton_action_t;

typedef struct nocterm_radiobutton_t nocterm_radiobutton_t;
typedef struct nocterm_radiobutton_group_t nocterm_radiobutton_group_t;

typedef void (*nocterm_radiobutton_onselect_handler_t)(nocterm_widget_t* self, nocterm_radiobutton_action_t action, void* user_data);

/**
 * @brief A group that ties multiple radio buttons together so that only a
 *        single member can be selected at any given time.
 *
 * @note A group does not own its members; deleting a group does not delete the
 *       radio button widgets that belong to it.
 */
typedef struct nocterm_radiobutton_group_t{
    nocterm_radiobutton_t** members;
    uint64_t members_size;
    nocterm_radiobutton_t* selected;
}nocterm_radiobutton_group_t;

typedef struct nocterm_radiobutton_t{
    nocterm_widget_t widget;
    nocterm_attribute_t main_attribute;
    nocterm_attribute_t cursor_attribute;
    nocterm_char_t select_marker;
    nocterm_char_t left_side;
    nocterm_char_t right_side;
    nocterm_radiobutton_group_t* group;
    nocterm_radiobutton_onselect_handler_t onselect_handler;
    bool selected;
    void* user_data;
}nocterm_radiobutton_t;

/**
 * @brief Creates a new radio button group.
 *
 * @return nocterm_radiobutton_group_t*
 */
nocterm_radiobutton_group_t* nocterm_radiobutton_group_new(void);

/**
 * @brief Constructs a radio button group.
 *
 * @param group
 * @return int
 */
int nocterm_radiobutton_group_constructor(nocterm_radiobutton_group_t* group);

/**
 * @brief Destructs a radio button group.
 *
 * @param group
 * @return int
 */
int nocterm_radiobutton_group_destructor(nocterm_radiobutton_group_t* group);

/**
 * @brief Deletes a radio button group.
 *
 * @note The member radio buttons are not deleted, only detached from the group.
 * @param group
 * @return int
 */
int nocterm_radiobutton_group_delete(nocterm_radiobutton_group_t* group);

/**
 * @brief Adds a radio button to a group.
 *
 * @param group
 * @param radiobutton
 * @return int
 */
int nocterm_radiobutton_group_add(nocterm_radiobutton_group_t* group, nocterm_radiobutton_t* radiobutton);

/**
 * @brief Removes a radio button from a group.
 *
 * @param group
 * @param radiobutton
 * @return int
 */
int nocterm_radiobutton_group_remove(nocterm_radiobutton_group_t* group, nocterm_radiobutton_t* radiobutton);

/**
 * @brief Returns the currently selected radio button of a group, or NULL.
 *
 * @param group
 * @return nocterm_radiobutton_t*
 */
nocterm_radiobutton_t* nocterm_radiobutton_group_get_selected(nocterm_radiobutton_group_t* group);

/**
 * @brief Creates a new radio button widget.
 *
 * @param group
 * @param onselect_handler
 * @param selected
 * @param user_data
 * @return nocterm_radiobutton_t*
 */
nocterm_radiobutton_t* nocterm_radiobutton_new(nocterm_radiobutton_group_t* group, nocterm_radiobutton_onselect_handler_t onselect_handler, bool selected, void* user_data);

/**
 * @brief Constructs a radio button widget.
 *
 * @param radiobutton
 * @param group
 * @param onselect_handler
 * @param selected
 * @param user_data
 * @return int
 */
int nocterm_radiobutton_constructor(nocterm_radiobutton_t* radiobutton, nocterm_radiobutton_group_t* group, nocterm_radiobutton_onselect_handler_t onselect_handler, bool selected, void* user_data);

/**
 * @brief Destructs a radio button widget.
 *
 * @param radiobutton
 * @return int
 */
int nocterm_radiobutton_destructor(nocterm_radiobutton_t* radiobutton);

/**
 * @brief Deletes a radio button widget.
 *
 * @param radiobutton
 * @return int
 */
int nocterm_radiobutton_delete(nocterm_radiobutton_t* radiobutton);

/**
 * @brief Selects a radio button, deselecting any other member of its group.
 *
 * @param radiobutton
 * @return int
 */
int nocterm_radiobutton_select(nocterm_radiobutton_t* radiobutton);

/**
 * @brief Checks whether a radio button is selected.
 *
 * @param radiobutton
 * @return true
 * @return false
 */
bool nocterm_radiobutton_is_selected(nocterm_radiobutton_t* radiobutton);

/**
 * @brief Sets the attribute of a radio button widget.
 *
 * @param radiobutton
 * @param attribute
 * @return int
 */
int nocterm_radiobutton_set_attribute(nocterm_radiobutton_t* radiobutton, nocterm_attribute_t attribute);

/**
 * @brief Sets the marker character of a radio button widget.
 *
 * @param radiobutton
 * @param marker
 * @return int
 */
int nocterm_radiobutton_set_marker(nocterm_radiobutton_t* radiobutton, nocterm_char_t marker);

/**
 * @brief Sets left and right side characters of a radio button widget.
 *
 * @param radiobutton
 * @param left
 * @param right
 * @return int
 */
int nocterm_radiobutton_set_sides(nocterm_radiobutton_t* radiobutton, nocterm_char_t left, nocterm_char_t right);

#ifdef __cplusplus
    }
#endif

#endif
