/**
 * @file listview.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2025-08-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef NOCTERM_LISTVIEW_H
#define NOCTERM_LISTVIEW_H

#include <nocterm/common/nocterm.h>
#include <nocterm/base/widget.h>

#define NOCTERM_LISTVIEW(x) ((nocterm_listview_t*)x)

#ifndef NOCTERM_LISTVIEW_ITEM_CONTENT_MAX_SIZE
    #define NOCTERM_LISTVIEW_ITEM_CONTENT_MAX_SIZE 128
#endif

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct nocterm_listview_item_t{
    uint16_t content_length;
    struct{
        nocterm_char_t character;
        nocterm_attribute_t attribute;
    }content[NOCTERM_LISTVIEW_ITEM_CONTENT_MAX_SIZE];
}nocterm_listview_item_t;

typedef struct nocterm_listview_item_array_t{
    nocterm_listview_item_t* items;
    uint64_t size;
    uint64_t capacity;
}nocterm_listview_item_array_t;

typedef enum nocterm_listview_autoscroll_t{
    NOCTERM_LISTVIEW_AUTOSCROLL_NONE,
    NOCTERM_LISTVIEW_AUTOSCROLL_DOWN,
    NOCTERM_LISTVIEW_AUTOSCROLL_UP
}nocterm_listview_autoscroll_t;

typedef struct nocterm_listview_t{
    nocterm_widget_t widget;
    nocterm_listview_item_array_t* item_array;
    nocterm_listview_autoscroll_t autoscroll; // Autoscrolls on push direction
    nocterm_dimension_size_t visible_rows;  // Desired viewport height — preserved across flex resizes
    nocterm_dimension_size_t items_total;   // Max buffer capacity from constructor
}nocterm_listview_t;

/**
 * @brief Creates a new listview widget.
 * 
 * @param items_displayed 
 * @param items_total 
 * @param item_width 
 * @return nocterm_listview_t* 
 */
nocterm_listview_t* nocterm_listview_new(nocterm_dimension_size_t items_displayed, uint64_t items_total, nocterm_dimension_size_t item_width);

/**
 * @brief Constructs a listview widget.
 * 
 * @param listview 
 * @param items_displayed 
 * @param items_total 
 * @param item_width 
 * @return int 
 */
int nocterm_listview_constructor(nocterm_listview_t* listview, nocterm_dimension_size_t items_displayed, uint64_t items_total, nocterm_dimension_size_t item_width);

/**
 * @brief Destructs a listview widget.
 * 
 * @param listview 
 * @return int 
 */
int nocterm_listview_destructor(nocterm_listview_t* listview);

/**
 * @brief Deletes a listview widget.
 * 
 * @param listview 
 * @return int 
 */
int nocterm_listview_delete(nocterm_listview_t* listview);

/**
 * @brief Adds an item to the end of the listview widget.
 * 
 * @param listview 
 * @param item 
 * @return int 
 */
int nocterm_listview_push_back(nocterm_listview_t* listview, nocterm_listview_item_t item);

/**
 * @brief Adds an item to the beginning of a listview widget.
 * 
 * @param listview 
 * @param item 
 * @return int 
 */
int nocterm_listview_push_front(nocterm_listview_t* listview, nocterm_listview_item_t item);

/**
 * @brief Removes an item from the end of a listview widget.
 * 
 * @param listview 
 * @param item 
 * @return int 
 */
int nocterm_listview_pop_back(nocterm_listview_t* listview, nocterm_listview_item_t* item);

/**
 * @brief Removes an item from the beginning of a listview widget.
 * 
 * @param listview 
 * @param item 
 * @return int 
 */
int nocterm_listview_pop_front(nocterm_listview_t* listview, nocterm_listview_item_t* item);

/**
 * @brief Inserts an item at an index to a listview widget.
 * 
 * @param listview 
 * @param item 
 * @param index 
 * @return int 
 */
int nocterm_listview_insert(nocterm_listview_t* listview, nocterm_listview_item_t item, uint64_t index);

/**
 * @brief Removes an item at index from a listview widget.
 * 
 * @param listview 
 * @param item 
 * @param index 
 * @return int 
 */
int nocterm_listview_remove(nocterm_listview_t* listview, nocterm_listview_item_t* item, uint64_t index);

/**
 * @brief Constructs a listview item.
 * 
 * @param item 
 * @param text 
 * @param text_size 
 * @param attribute 
 * @return int 
 */
int nocterm_listview_item_constructor(nocterm_listview_item_t* item, const char* text, uint64_t text_size, nocterm_attribute_t attribute);

/**
 * @brief Enables/disables auto-scroll feature for a listview widget.
 * 
 * @param listview 
 * @param autoscroll 
 * @return int 
 */
int nocterm_listview_set_autoscroll(nocterm_listview_t* listview, nocterm_listview_autoscroll_t autoscroll);

/**
 * @brief Clears all items on a listview widget.
 * 
 * @param listview 
 * @return int 
 */
int nocterm_listview_clear(nocterm_listview_t* listview);

#ifdef __cplusplus
    }
#endif

#endif
