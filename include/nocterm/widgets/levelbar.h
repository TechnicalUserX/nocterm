/**
 * @file levelbar.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2025-09-13
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef NOCTERM_LEVELBAR_H
#define NOCTERM_LEVELBAR_H

#include <nocterm/common/nocterm.h>
#include <nocterm/base/widget.h>

#define NOCTERM_LEVELBAR(x) ((nocterm_levelbar_t*)x)


#ifdef __cplusplus
    extern "C" {
#endif

typedef enum nocterm_levelbar_type_t{
    NOCTERM_LEVELBAR_TYPE_HORIZONTAL = 0,
    NOCTERM_LEVELBAR_TYPE_VERTICAL
}nocterm_levelbar_type_t;

typedef struct nocterm_levelbar_t{
    nocterm_widget_t widget;
    nocterm_levelbar_type_t type;
    nocterm_dimension_size_t length;
    bool flip; // By default, left-to-right or top-to-bottom
    nocterm_attribute_t attribute; // This overrides default attribute
    nocterm_char_t character; // This overrides default character
    uint64_t min_value;
    uint64_t max_value;
    uint64_t current_value;
}nocterm_levelbar_t;

/**
 * @brief Creates a levelbar widget.
 * 
 * @param row 
 * @param col 
 * @param length 
 * @param min_value 
 * @param max_value 
 * @param type 
 * @param flip 
 * @return nocterm_levelbar_t* 
 */
nocterm_levelbar_t* nocterm_levelbar_new(nocterm_dimension_size_t row, nocterm_dimension_size_t col, uint64_t length, uint64_t min_value, uint64_t max_value, nocterm_levelbar_type_t type, bool flip);

/**
 * @brief Constructs a levelbar widget.
 * 
 * @param levelbar 
 * @param row 
 * @param col 
 * @param length 
 * @param min_value 
 * @param max_value 
 * @param type 
 * @param flip 
 * @return int 
 */
int nocterm_levelbar_constructor(nocterm_levelbar_t* levelbar, nocterm_dimension_size_t row, nocterm_dimension_size_t col, uint64_t length, uint64_t min_value, uint64_t max_value, nocterm_levelbar_type_t type, bool flip);

/**
 * @brief Destructs a levelbar widget.
 * 
 * @param levelbar 
 * @return int 
 */
int nocterm_levelbar_destructor(nocterm_levelbar_t* levelbar);

/**
 * @brief Deletes a levelbar widget.
 * 
 * @param levelbar 
 * @return int 
 */
int nocterm_levelbar_delete(nocterm_levelbar_t* levelbar);

/**
 * @brief Sets the levelbar value.
 * 
 * @param levelbar 
 * @param value 
 * @return int 
 */
int nocterm_levelbar_set_value(nocterm_levelbar_t* levelbar, uint64_t value);

/**
 * @brief Gets the levelbar value.
 * 
 * @param levelbar 
 * @param value 
 * @return int 
 */
int nocterm_levelbar_get_value(nocterm_levelbar_t* levelbar, uint64_t* value);

/**
 * @brief Sets the character attribute of the levelbar widget.
 * 
 * @param levelbar 
 * @param attribute 
 * @return int 
 */
int nocterm_levelbar_set_attribute(nocterm_levelbar_t* levelbar, nocterm_attribute_t attribute);

/**
 * @brief Sets the printed character of the levelbar widget.
 * 
 * @param levelbar 
 * @param character 
 * @return int 
 */
int nocterm_levelbar_set_character(nocterm_levelbar_t* levelbar, nocterm_char_t character);

#ifdef __cplusplus
    }
#endif

#endif
