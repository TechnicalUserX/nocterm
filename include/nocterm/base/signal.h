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

#ifdef __cplusplus
    }
#endif

#endif
