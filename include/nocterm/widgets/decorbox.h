/**
 * @file decorbox.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2025-08-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef NOCTERM_DECORBOX_H
#define NOCTERM_DECORBOX_H

#include <nocterm/common/nocterm.h>
#include <nocterm/base/widget.h>

#define NOCTERM_DECORBOX(x) ((nocterm_decorbox_t*)x)

#ifndef CONFIG_NOCTERM_DECORBOX_LABEL_MAX_SIZE
    #define NOCTERM_DECORBOX_LABEL_MAX_SIZE 64
#else
    #define NOCTERM_DECORBOX_LABEL_MAX_SIZE CONFIG_NOCTERM_DECORBOX_LABEL_MAX_SIZE
#endif

#ifdef __cplusplus
    extern "C" {
#endif

typedef enum nocterm_decorbox_border_shape_t{
    NOCTERM_DECORBOX_BORDER_SHAPE_ASCII,
    NOCTERM_DECORBOX_BORDER_SHAPE_UNICODE_SHARP,
    NOCTERM_DECORBOX_BORDER_SHAPE_UNICODE_ROUND
}nocterm_decorbox_border_shape_t;

typedef struct nocterm_decorbox_border_t{
    nocterm_char_t horizontal;
    nocterm_char_t vertical;
    nocterm_char_t top_left;
    nocterm_char_t top_right;
    nocterm_char_t bottom_left;
    nocterm_char_t bottom_right;
}nocterm_decorbox_border_t;

typedef struct nocterm_decorbox_t{
    nocterm_widget_t widget;
    nocterm_widget_t* contained_widget;

    struct{
        bool enabled;
        nocterm_decorbox_border_t border;
        struct{
            nocterm_attribute_t normal;
            nocterm_attribute_t focused;
        }attributes;
    }border_settings;

    struct{
        bool enabled;
        uint64_t left_offset;
        uint64_t content_length;
        struct{
            nocterm_char_t character;
            nocterm_attribute_t attribute;
        }content[NOCTERM_DECORBOX_LABEL_MAX_SIZE];
    }label;

}nocterm_decorbox_t;

/**
 * @brief Creates a new decorbox widget.
 * 
 * @param contained_widget 
 * @return nocterm_decorbox_t* 
 */
nocterm_decorbox_t* nocterm_decorbox_new(nocterm_widget_t* contained_widget);

/**
 * @brief Constructs a decorbox widget.
 * 
 * @param decorbox 
 * @param contained_widget 
 * @return int 
 */
int nocterm_decorbox_constructor(nocterm_decorbox_t* decorbox, nocterm_widget_t* contained_widget);

/**
 * @brief Destructs a decorbox widget.
 * 
 * @param decorbox 
 * @return int 
 */
int nocterm_decorbox_destructor(nocterm_decorbox_t* decorbox);


/**
 * @brief Deletes a decorbox widget.
 * 
 * @param decorbox 
 * @return int 
 */
int nocterm_decorbox_delete(nocterm_decorbox_t* decorbox);

/**
 * @brief Enables border feature for decorbox.
 *
 * @param decorbox
 * @param border
 * @param normal
 * @param focused
 * @return int
 */
int nocterm_decorbox_set_border(nocterm_decorbox_t* decorbox, nocterm_decorbox_border_t border, nocterm_attribute_t normal, nocterm_attribute_t focused);

/**
 * @brief Retrieves a predefined border for decorbox.
 *
 * @param shape
 * @return nocterm_decorbox_border_t
 */
nocterm_decorbox_border_t nocterm_decorbox_border_from_shape(nocterm_decorbox_border_shape_t shape);

/**
 * @brief Enables label feature for decorbox.
 * 
 * @param decorbox 
 * @param label 
 * @param label_size 
 * @param attribute 
 * @param left_offset 
 * @return int 
 */
int nocterm_decorbox_set_label(nocterm_decorbox_t* decorbox, const char* label, uint64_t label_size, nocterm_attribute_t attribute, uint64_t left_offset);

#ifdef __cplusplus
    }
#endif

#endif
