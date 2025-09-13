/**
 * @file mouse.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2025-09-12
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef NOCTERM_MOUSE_H
#define NOCTERM_MOUSE_H

#include <nocterm/common/nocterm.h>
#include <nocterm/base/io.h>
#include <nocterm/base/widget.h>
#include <nocterm/base/key.h>
#include <nocterm/base/page.h>
#include <nocterm/base/screen.h>

#ifdef __cplusplus
    extern "C" {
#endif

NOCTERM_INTERNAL
extern bool nocterm_mouse_support_flag;

typedef enum nocterm_mouse_button_t{
    NOCTERM_MOUSE_BUTTON_LMB,
    NOCTERM_MOUSE_BUTTON_RMB,
    NOCTERM_MOUSE_BUTTON_MMB,
    NOCTERM_MOUSE_BUTTON_RELEASE,
    NOCTERM_MOUSE_BUTTON_SCROLL_UP,
    NOCTERM_MOUSE_BUTTON_SCROLL_DOWN,
    NOCTERM_MOUSE_BUTTON_UNKNOWN
}nocterm_mouse_button_t;

typedef struct nocterm_mouse_modifier_t{
    bool ctrl:1;
    bool shift:1;
    bool alt:1;
}nocterm_mouse_modifier_t;

typedef struct nocterm_mouse_event_t{
    nocterm_mouse_button_t button;
    nocterm_mouse_modifier_t modifier;
    nocterm_dimension_size_t row;
    nocterm_dimension_size_t col;
}nocterm_mouse_event_t;

extern nocterm_dimension_size_t nocterm_mouse_row, nocterm_mouse_col;

void nocterm_mouse_support(bool enable);

NOCTERM_INTERNAL
int nocterm_mouse_enable(void);

NOCTERM_INTERNAL
int nocterm_mouse_disable(void);

NOCTERM_INTERNAL
nocterm_mouse_event_t nocterm_mouse_event(uint8_t mouse_byte, uint8_t col_byte, uint8_t row_byte);

NOCTERM_INTERNAL
int nocterm_mouse_controller(nocterm_key_t* key);

#ifdef __cplusplus
    }
#endif

#endif