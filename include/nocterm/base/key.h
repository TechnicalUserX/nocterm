/**
 * @file key.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2025-08-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef NOCTERM_KEY_H
#define NOCTERM_KEY_H

#ifndef NOCTERM_KEY_BUFFER_SIZE
    #define NOCTERM_KEY_BUFFER_SIZE 256
#endif

// Escape sequence next character
// wait interval in milliseconds

#ifndef NOCTERM_KEY_ESCSEQ_INTERVAL
    #define NOCTERM_KEY_ESCSEQ_INTERVAL 50
#endif

#include <nocterm/common/nocterm.h>

#ifdef __cplusplus
    extern "C" {
#endif

typedef enum nocterm_key_type_t {
    NOCTERM_KEY_TYPE_PRINTABLE,
    NOCTERM_KEY_TYPE_CONTROL,
    NOCTERM_KEY_TYPE_ESCSEQ
}nocterm_key_type_t;

typedef enum nocterm_key_event_t {
    NOCTERM_KEY_EVENT_PRINTABLE = 0,
    NOCTERM_KEY_EVENT_UP,
    NOCTERM_KEY_EVENT_DOWN,
    NOCTERM_KEY_EVENT_RIGHT,
    NOCTERM_KEY_EVENT_LEFT,
    NOCTERM_KEY_EVENT_BACKSPACE,
    NOCTERM_KEY_EVENT_ENTER,
    NOCTERM_KEY_EVENT_DELETE,
    NOCTERM_KEY_EVENT_TAB,
    NOCTERM_KEY_EVENT_SHIFT_TAB,
    NOCTERM_KEY_EVENT_ESCAPE,
    NOCTERM_KEY_EVENT_BREAK,  // Ctrl + C
    NOCTERM_KEY_EVENT_EOF,    // Ctrl + D
    NOCTERM_KEY_EVENT_BS,    // Ctrl + H
    NOCTERM_KEY_EVENT_FF,    // Ctrl + L
    NOCTERM_KEY_EVENT_UNDEFINED
}nocterm_key_event_t;

typedef struct nocterm_key_t {
    uint8_t buffer[NOCTERM_KEY_BUFFER_SIZE];
    uint8_t buffer_length;
    nocterm_key_type_t type;
}nocterm_key_t;

/**
 * @brief Returns a key event from the given captured key.
 * 
 * @param key 
 * @return nocterm_key_event_t 
 */
nocterm_key_event_t nocterm_key_translate(nocterm_key_t* key);

/**
 * @brief Captures a key press. Works in raw mode.
 * 
 * @param key 
 * @return int 
 */
int nocterm_key_capture(nocterm_key_t* key);

#ifdef __cplusplus
    }
#endif

#endif
