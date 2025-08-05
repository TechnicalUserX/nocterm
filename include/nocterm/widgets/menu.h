/**
 * @file menu.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2025-08-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef NOCTERM_MENU_H
#define NOCTERM_MENU_H

#include <nocterm/common/nocterm.h>
#include <nocterm/base/widget.h>

#define NOCTERM_MENU_ITEM_CONTENT_MAX_SIZE 64

#define NOCTERM_MENU(x) ((nocterm_menu_t*)x)

/**
 * @brief Macro for creating an "on select" menu handler.
 * 
 */
#define NOCTERM_MENU_ONSELECT_HANDLER(identifier) void identifier(nocterm_widget_t* self, uint16_t selected_item, void* user_data)

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct nocterm_menu_item_t{
    uint64_t content_length;
    struct{
        nocterm_char_t character;
        nocterm_attribute_t attribute;
    }content[NOCTERM_MENU_ITEM_CONTENT_MAX_SIZE];
}nocterm_menu_item_t;

typedef struct nocterm_menu_item_array_t{
    nocterm_menu_item_t* items;
    uint64_t size;
    uint64_t capacity;
}nocterm_menu_item_array_t;

typedef void (*nocterm_menu_onselect_handler_t)(nocterm_widget_t* self, uint16_t selected_item, void* user_data);

typedef struct nocterm_menu_t{
    nocterm_widget_t widget;
    nocterm_attribute_t attribute_selection;
    uint16_t current_item; // Index
    uint16_t selection_position; // Fixed inside the range < viewport.height
    nocterm_menu_item_array_t* item_array;
    nocterm_menu_onselect_handler_t onselect_handler;
    void* user_data;
}nocterm_menu_t;

/**
 * @brief Creates a new menu widget.
 * 
 * @param row 
 * @param col 
 * @param selection 
 * @param items_displayed 
 * @param items_total 
 * @param item_width 
 * @param onselect_handler 
 * @return nocterm_menu_t* 
 */
nocterm_menu_t* nocterm_menu_new(nocterm_dimension_size_t row, nocterm_dimension_size_t col, nocterm_attribute_t selection, nocterm_dimension_size_t items_displayed, uint64_t items_total, nocterm_dimension_size_t item_width, nocterm_menu_onselect_handler_t onselect_handler, void* user_data);

/**
 * @brief Deletes a menu widget.
 * 
 * @param menu 
 * @return int 
 */
int nocterm_menu_delete(nocterm_menu_t* menu);

/**
 * @brief Adds an item to the menu widget.
 * 
 * @param menu 
 * @param item 
 * @return int 
 */
int nocterm_menu_add_item(nocterm_menu_t* menu, nocterm_menu_item_t item);

/**
 * @brief Adds multiple items at once to the menu widget.
 * 
 * @param menu 
 * @param items 
 * @param items_size 
 * @return int 
 */
int nocterm_menu_add_item_multiple(nocterm_menu_t* menu, nocterm_menu_item_t* items, uint64_t items_size);


/**
 * @brief Constructs a menu item.
 * 
 * @param item 
 * @param content 
 * @param content_size 
 * @param attribute 
 * @return int 
 */
int nocterm_menu_item_constructor(nocterm_menu_item_t* item, const char* content, uint64_t content_size, nocterm_attribute_t attribute);

/**
 * @brief Clears all items on a menu widget.
 * 
 * @param menu 
 * @return int 
 */
int nocterm_menu_clear(nocterm_menu_t* menu);

/**
 * @brief Retrieves current selected item on a menu widget.
 * 
 * @param menu 
 * @param selection 
 * @return int 
 */
int nocterm_menu_get_selection(nocterm_menu_t* menu, uint64_t* selection);

NOCTERM_INTERNAL
int nocterm_menu_selection_move_up(nocterm_menu_t* menu);

NOCTERM_INTERNAL
int nocterm_menu_selection_move_down(nocterm_menu_t* menu);

NOCTERM_INTERNAL
int nocterm_menu_selection_move_top(nocterm_menu_t* menu);

NOCTERM_INTERNAL
int nocterm_menu_selection_move_bottom(nocterm_menu_t* menu);

NOCTERM_INTERNAL
NOCTERM_WIDGET_KEY_HANDLER(nocterm_menu_key_handler);

NOCTERM_INTERNAL
NOCTERM_WIDGET_FOCUS_HANDLER(nocterm_menu_focus_handler);

// Dynamic Array Implementation

NOCTERM_INTERNAL
nocterm_menu_item_array_t* nocterm_menu_item_array_new(void);

NOCTERM_INTERNAL
void nocterm_menu_item_array_delete(nocterm_menu_item_array_t* nocterm_menu_item_array);

NOCTERM_INTERNAL
void nocterm_menu_item_array_increase_capacity(nocterm_menu_item_array_t* nocterm_menu_item_array);

NOCTERM_INTERNAL
int nocterm_menu_item_array_push_back(nocterm_menu_item_array_t* nocterm_menu_item_array, nocterm_menu_item_t item);

NOCTERM_INTERNAL
int nocterm_menu_item_array_pop_back(nocterm_menu_item_array_t* nocterm_menu_item_array, nocterm_menu_item_t* item);

NOCTERM_INTERNAL
int nocterm_menu_item_array_push_front(nocterm_menu_item_array_t* nocterm_menu_item_array, nocterm_menu_item_t item);

NOCTERM_INTERNAL
int nocterm_menu_item_array_pop_front(nocterm_menu_item_array_t* nocterm_menu_item_array, nocterm_menu_item_t* item);

NOCTERM_INTERNAL
int nocterm_menu_item_array_insert(nocterm_menu_item_array_t* nocterm_menu_item_array, nocterm_menu_item_t item, uint64_t index);

NOCTERM_INTERNAL
int nocterm_menu_item_array_remove(nocterm_menu_item_array_t* nocterm_menu_item_array,  nocterm_menu_item_t* item, uint64_t index);

NOCTERM_INTERNAL
int nocterm_menu_item_array_clear(nocterm_menu_item_array_t* nocterm_menu_item_array);

NOCTERM_INTERNAL
int nocterm_menu_item_array_shrink_to_fit(nocterm_menu_item_array_t* nocterm_menu_item_array);

#ifdef __cplusplus
    }
#endif

#endif
