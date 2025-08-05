/**
 * @file screen.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2025-08-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef NOCTERM_SCREEN_H
#define NOCTERM_SCREEN_H

#include <nocterm/common/nocterm.h>

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct nocterm_screen_ownership_t{
    void* owner; // Owner widget
}nocterm_screen_ownership_t;

extern nocterm_screen_ownership_t* nocterm_screen_ownership;
extern nocterm_dimension_size_t nocterm_screen_height;
extern nocterm_dimension_size_t nocterm_screen_width;

/**
 * @brief Resets screen ownerships of all widgets.
 * 
 */
void nocterm_screen_ownership_reset(void);

#ifdef __cplusplus
    }
#endif



#endif