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
 * @param row 
 * @param col 
 * @param text 
 * @param text_size 
 * @param attribute 
 * @return nocterm_label_t* 
 */
nocterm_label_t* nocterm_label_new(nocterm_dimension_size_t row, nocterm_dimension_size_t col, const char* text, uint64_t text_size, nocterm_attribute_t attribute);

/**
 * @brief Deletes a label widget.
 * 
 * @param label 
 * @return int 
 */
int nocterm_label_delete(nocterm_label_t* label);

#ifdef __cplusplus
    }
#endif

#endif
