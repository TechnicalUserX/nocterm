/**
 * @file loadingbar.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2025-08-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef NOCTERM_LOADINGBAR_H
#define NOCTERM_LOADINGBAR_H

#include <nocterm/common/nocterm.h>
#include <nocterm/base/widget.h>
#include <nocterm/base/timer.h>

#define NOCTERM_LOADINGBAR(x) ((nocterm_loadingbar_t*)x)

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct nocterm_loadingbar_t{
    nocterm_widget_t widget; // Widget Inheritance
    nocterm_timer_t* timer;
    uint8_t state;
    nocterm_attribute_t attribute;
}nocterm_loadingbar_t;

/**
 * @brief Creates a new loadingbar widget.
 * 
 * @param row 
 * @param col 
 * @param interval 
 * @param attribute 
 * @return nocterm_loadingbar_t* 
 */
nocterm_loadingbar_t* nocterm_loadingbar_new(nocterm_dimension_size_t row, nocterm_dimension_size_t col, uint64_t interval, nocterm_attribute_t attribute);

/**
 * @brief Deletes a loadingbar widget.
 * 
 * @param loadingbar 
 * @return int 
 */
int nocterm_loadingbar_delete(nocterm_loadingbar_t* loadingbar);

NOCTERM_INTERNAL
NOCTERM_TIMER_CALLBACK(nocterm_loadingbar_timer_callback);

/**
 * @brief Enables loadingbar widget animation.
 * 
 * @param loadingbar 
 * @return int 
 */
int nocterm_loadingbar_enable(nocterm_loadingbar_t* loadingbar);

/**
 * @brief Disables loadingbar widget animation.
 * 
 * @param loadingbar 
 * @return int 
 */
int nocterm_loadingbar_disable(nocterm_loadingbar_t* loadingbar);

#ifdef __cplusplus
    }
#endif

#endif
