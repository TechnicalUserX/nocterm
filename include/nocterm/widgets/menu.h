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

#ifndef CONFIG_NOCTERM_MENU_ITEM_CONTENT_MAX_SIZE
    #define NOCTERM_MENU_ITEM_CONTENT_MAX_SIZE 128
#else
    #define NOCTERM_MENU_ITEM_CONTENT_MAX_SIZE CONFIG_NOCTERM_MENU_ITEM_CONTENT_MAX_SIZE
#endif

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
    nocterm_attribute_t selection_attribute;
    uint16_t current_item; // Index
    uint16_t selection_position; // Fixed inside the range < viewport.height
    nocterm_dimension_size_t visible_rows;  // Desired viewport height — preserved across flex resizes
    nocterm_dimension_size_t items_total;   // Max buffer capacity from constructor
    nocterm_menu_item_array_t* item_array;
    nocterm_menu_onselect_handler_t onselect_handler;
    void* user_data;
}nocterm_menu_t;

/**
 * @brief Creates a new menu widget.
 * 
 * @param items_displayed 
 * @param items_total 
 * @param item_width 
 * @param onselect_handler 
 * @param user_data 
 * @return nocterm_menu_t* 
 */
nocterm_menu_t* nocterm_menu_new(nocterm_dimension_size_t items_displayed, uint64_t items_total, nocterm_dimension_size_t item_width, nocterm_menu_onselect_handler_t onselect_handler, void* user_data);


/**
 * @brief Constructs a menu widget.
 * 
 * @param menu 
 * @param items_displayed 
 * @param items_total 
 * @param item_width 
 * @param onselect_handler 
 * @param user_data 
 * @return int 
 */
int nocterm_menu_constructor(nocterm_menu_t* menu, nocterm_dimension_size_t items_displayed, uint64_t items_total, nocterm_dimension_size_t item_width, nocterm_menu_onselect_handler_t onselect_handler, void* user_data);

/**
 * @brief Destructs a menu widget.
 * 
 * @param menu 
 * @return int 
 */
int nocterm_menu_destructor(nocterm_menu_t* menu);

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

/**
 * @brief Programmatically sets the current selection of a menu widget.
 *
 * Moves the selection to @p index and scrolls the viewport the minimal amount
 * needed to keep that item visible (mirroring the built-in key navigation).
 * Unlike the key handler, this can be driven from application code, which makes
 * data-driven behaviours such as "follow / stick to the newest item" possible.
 *
 * If @p index is past the last item it is clamped to the last item. If the menu
 * is empty the selection is reset to 0. Safe to call on an unfocused menu: the
 * selection state is updated and the highlight is applied on the next focus.
 *
 * @param menu
 * @param index Zero-based item index to select.
 * @return int
 */
int nocterm_menu_set_selection(nocterm_menu_t* menu, uint64_t index);

/**
 * @brief Sets selection attribute of a menu widget.
 *
 * @param menu
 * @param attribute
 * @return int
 */
int nocterm_menu_set_selection_attribute(nocterm_menu_t* menu, nocterm_attribute_t attribute);


#ifdef __cplusplus
    }
#endif

#endif
