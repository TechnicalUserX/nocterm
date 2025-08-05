/**
 * @file capability.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2025-08-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef NOCTERM_CAPABILITY_H
#define NOCTERM_CAPABILITY_H

#include <nocterm/common/nocterm.h>

#ifdef __cplusplus
    extern "C" {
#endif

typedef enum nocterm_capability_t{
    NOCTERM_CAPABILITY_UNKNOWN, 
    NOCTERM_CAPABILITY_XTERM,
    NOCTERM_CAPABILITY_XTERM256,
    NOCTERM_CAPABILITY_TRUECOLOR,
    NOCTERM_CAPABILITY_24BIT = NOCTERM_CAPABILITY_TRUECOLOR
}nocterm_capability_t;


/**
 * @brief Returns the terminal capability.
 * 
 * @return nocterm_capability_t 
 */
nocterm_capability_t nocterm_capability_get(void);

#ifdef __cplusplus
    }
#endif

#endif