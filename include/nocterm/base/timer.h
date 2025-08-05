/**
 * @file timer.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2025-08-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef NOCTERM_TIMER_H
#define NOCTERM_TIMER_H

#include <nocterm/common/nocterm.h>
#include <nocterm/base/widget.h>

#define NOCTERM_TIMER_CALLBACK(identifier) void identifier(nocterm_widget_t* widget, void* user_data)

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct nocterm_timer_t{
    nocterm_widget_t* widget;
    uint64_t interval; // ms
    uint64_t last_call; // ms
    bool active;
    void* user_data;
    void (*callback)(nocterm_widget_t* widget, void* user_data);
    struct nocterm_timer_t* next;
}nocterm_timer_t;

typedef void (*nocterm_timer_callback_t)(nocterm_widget_t* widget, void* user_data);

extern nocterm_timer_t* nocterm_timer_list_head;

/**
 * @brief Creates a new timer.
 * 
 * @param widget 
 * @param interval 
 * @param callback 
 * @param user_data 
 * @return nocterm_timer_t* 
 */
nocterm_timer_t* nocterm_timer_create(nocterm_widget_t* widget, uint64_t interval, nocterm_timer_callback_t callback, void* user_data );

/**
 * @brief Used to determine timeouts and exeucte callbacks in the main loop.
 * 
 * @return NOCTERM_INTERNAL 
 */
NOCTERM_INTERNAL
void nocterm_timer_tick(void);

/**
 * @brief Starts a timer.
 * 
 * @param timer 
 * @return int 
 */
int nocterm_timer_start(nocterm_timer_t* timer);

/**
 * @brief Starts all timers that belong to a widget.
 * 
 * @param widget 
 * @param recursive 
 * @return int 
 */
int nocterm_timer_start_all_of_widget(nocterm_widget_t* widget, bool recursive);

/**
 * @brief Stops a timer.
 * 
 * @param timer 
 * @return int 
 */
int nocterm_timer_stop(nocterm_timer_t* timer);

/**
 * @brief Stops all timers that belong to a widget.
 * 
 * @param widget 
 * @param recursive 
 * @return int 
 */
int nocterm_timer_stop_all_of_widget(nocterm_widget_t* widget, bool recursive);

/**
 * @brief Delets a timer.
 * 
 * @param timer 
 * @return int 
 */
int nocterm_timer_delete(nocterm_timer_t* timer);

/**
 * @brief Deletes all timers.
 * 
 * @return int 
 */
int nocterm_timer_delete_all(void);

/**
 * @brief Deletes all timers that belong to a widget.
 * 
 * @param widget 
 * @return int 
 */
int nocterm_timer_delete_all_of_widget(nocterm_widget_t* widget);

#ifdef __cplusplus
    }
#endif

#endif
