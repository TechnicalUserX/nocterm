/**
 * @file signal.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2025-08-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef NOCTERM_SIGNAL_H
#define NOCTERM_SIGNAL_H

#include <nocterm/common/nocterm.h>
#include <nocterm/base/io.h>

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct nocterm_signal_flags_t{
    uint8_t nocterm_signal_sigwinch: 1;
}nocterm_signal_flags_t;

extern nocterm_signal_flags_t nocterm_signal_flags;

/**
 * @brief Attaches related signals.
 * 
 * @return int 
 */
int nocterm_signal_init(void);

/**
 * @brief Generic signal handler for signals.
 * 
 * @param sig 
 */
NOCTERM_INTERNAL
void nocterm_signal_handler(int sig);

#ifdef __cplusplus
    }
#endif

#endif
