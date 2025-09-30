/**
 * @file label.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2025-08-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef NOCTERM_LABEL_H
#define NOCTERM_LABEL_H

#include <nocterm/common/nocterm.h>
#include <nocterm/base/widget.h>

#define NOCTERM_LABEL(x) ((nocterm_label_t*)x)

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct nocterm_label_t{
    nocterm_widget_t widget;
    nocterm_attribute_t attribute;
}nocterm_label_t;

/**
 * @brief Creates a new label widget.
 * 
 * @param text 
 * @param text_size 
 * @return nocterm_label_t* 
 */
nocterm_label_t* nocterm_label_new(const char* text, uint64_t text_size);

/**
 * @brief Constructs a label widget.
 * 
 * @param label 
 * @param text 
 * @param text_size 
 * @return int 
 */
int nocterm_label_constructor(nocterm_label_t* label, const char* text, uint64_t text_size);

/**
 * @brief Destructs a label widget.
 * 
 * @param label 
 * @return int 
 */
int nocterm_label_destructor(nocterm_label_t* label);

/**
 * @brief Deletes a label widget.
 * 
 * @param label 
 * @return int 
 */
int nocterm_label_delete(nocterm_label_t* label);

/**
 * @brief Sets attribute of a label widget.
 * 
 * @param label 
 * @param attribute 
 * @return int 
 */
int nocterm_label_set_attribute(nocterm_label_t* label, nocterm_attribute_t attribute);

#ifdef __cplusplus
    }
#endif

#endif
