/**
 * @file overlay.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2025-09-29
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef NOCTERM_OVERLAY_H
#define NOCTERM_OVERLAY_H

#include <nocterm/common/nocterm.h>
#include <nocterm/base/widget.h>

#ifdef __cplusplus
    extern "C" {
#endif

#ifndef CONFIG_NOCTERM_OVERLAY_WIDGET_MAX_SIZE
    #define NOCTERM_OVERLAY_WIDGET_MAX_SIZE 16
#else
    #define NOCTERM_OVERLAY_WIDGET_MAX_SIZE CONFIG_NOCTERM_OVERLAY_WIDGET_MAX_SIZE
#endif

typedef struct nocterm_overlay_t{
    nocterm_widget_t* widgets[NOCTERM_OVERLAY_WIDGET_MAX_SIZE]; // Widgets with no parents or with parents
    uint64_t widget_size;
    bool hard_refresh;
}nocterm_overlay_t;

extern nocterm_overlay_t* nocterm_overlay; // Global overlay

/**
 * @brief Creates a new overlay.
 * 
 * @return nocterm_overlay_t* 
 */
nocterm_overlay_t* nocterm_overlay_new(void);

/**
 * @brief Deletes an overlay.
 * 
 * @param overlay 
 * @return int 
 */
int nocterm_overlay_delete(nocterm_overlay_t* overlay);

/**
 * @brief Sets the current overlay.
 * 
 * @param overlay 
 * @return int 
 */
int nocterm_overlay_set(nocterm_overlay_t* overlay);

/**
 * @brief Adds a widget to an overlay.
 * 
 * @param overlay 
 * @param widget 
 * @return int 
 */
int nocterm_overlay_add_widget(nocterm_overlay_t* overlay, nocterm_widget_t* widget);

/**
 * @brief Removes a widget from an overlay.
 * 
 * @param overlay 
 * @param widget 
 * @return int 
 */
int nocterm_overlay_remove_widget(nocterm_overlay_t* overlay, nocterm_widget_t* widget);

#ifdef __cplusplus
    }
#endif

#endif
