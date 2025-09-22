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
 * @return nocterm_loadingbar_t* 
 */
nocterm_loadingbar_t* nocterm_loadingbar_new(uint64_t interval);

/**
 * @brief Constructs a loadingbar widget.
 * 
 * @param row 
 * @param col 
 * @param interval 
 * @return int 
 */
int nocterm_loadingbar_constructor(nocterm_loadingbar_t* loadingbar, uint64_t interval);

/**
 * @brief Destructs a loadingbar widget.
 * 
 * @param loadingbar 
 * @return int 
 */
int nocterm_loadingbar_destructor(nocterm_loadingbar_t* loadingbar);

/**
 * @brief Deletes a loadingbar widget.
 * 
 * @param loadingbar 
 * @return int 
 */
int nocterm_loadingbar_delete(nocterm_loadingbar_t* loadingbar);


/**
 * @brief Sets attribute of a loadingbar widget.
 * 
 * @param loadingbar 
 * @param attribute 
 * @return int 
 */
int nocterm_loadingbar_set_attribute(nocterm_loadingbar_t* loadingbar, nocterm_attribute_t attribute);

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
