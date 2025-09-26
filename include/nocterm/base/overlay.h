#ifndef NOCTERM_OVERLAY_H
#define NOCTERM_OVERLAY_H

#include <nocterm/common/nocterm.h>
#include <nocterm/base/widget.h>

#ifdef __cplusplus
    extern "C" {
#endif

#ifndef NOCTERM_OVERLAY_WIDGET_MAX_SIZE
    #define NOCTERM_OVERLAY_WIDGET_MAX_SIZE 16
#endif

typedef struct nocterm_overlay_t{
    nocterm_widget_t* widgets[NOCTERM_OVERLAY_WIDGET_MAX_SIZE]; // Widgets with no parents or with parents
    uint64_t widget_size;
    bool hard_refresh;
}nocterm_overlay_t;

extern nocterm_overlay_t* nocterm_overlay; // Global overlay

nocterm_overlay_t* nocterm_overlay_new();

int nocterm_overlay_delete(nocterm_overlay_t* overlay);

int nocterm_overlay_set(nocterm_overlay_t* overlay);

int nocterm_overlay_add_widget(nocterm_overlay_t* overlay, nocterm_widget_t* widget);

int nocterm_overlay_remove_widget(nocterm_overlay_t* overlay, nocterm_widget_t* widget);

int nocterm_overlay_refresh(nocterm_overlay_t* overlay);

#ifdef __cplusplus
    }
#endif

#endif