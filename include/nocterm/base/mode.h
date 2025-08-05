/**
 * @file mode.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2025-08-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef NOCTERM_MODE_H
#define NOCTERM_MODE_H

#include <nocterm/common/nocterm.h>

#ifdef __cplusplus
    extern "C" {
#endif

/**
 * @brief Saves current terminal mode.
 * 
 * @return int 
 */
int nocterm_mode_init(void);

/**
 * @brief Restores terminal to its original form.
 * 
 * @return int 
 */
int nocterm_mode_restore(void);

/**
 * @brief Puts terminal in raw mode.
 * 
 * @return int 
 */
int nocterm_mode_raw(void);

#ifdef __cplusplus
    }
#endif

#endif