/**
 * @file nocterm.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2025-08-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef NOCTERM_H
#define NOCTERM_H

#include <nocterm/common/nocterm.h>
#include <nocterm/base/mode.h>
#include <nocterm/base/attribute.h>
#include <nocterm/base/char.h>
#include <nocterm/base/capability.h>
#include <nocterm/base/io.h>
#include <nocterm/base/key.h>
#include <nocterm/base/signal.h>
#include <nocterm/base/widget.h>
#include <nocterm/base/timer.h>
#include <nocterm/base/page.h>
#include <nocterm/base/encoding.h>
#include <nocterm/base/screen.h>

#include <nocterm/widgets/loadingbar.h>
#include <nocterm/widgets/label.h>
#include <nocterm/widgets/button.h>
#include <nocterm/widgets/listview.h>
#include <nocterm/widgets/decorbox.h>
#include <nocterm/widgets/entry.h>
#include <nocterm/widgets/menu.h>
#include <nocterm/widgets/checkbox.h>
#include <nocterm/widgets/pixelgrid.h>
#include <nocterm/widgets/textview.h>

#ifdef __cplusplus
    extern "C" {
#endif

NOCTERM_INTERNAL
int nocterm_init(void);

NOCTERM_INTERNAL
int nocterm_end(void);

/**
 * @brief Main loop function, handles events, key strokes, timer callbacks, and refreshes screen.
 * 
 * @return int 
 */
int nocterm_loop(void);

#ifdef __cplusplus
    }
#endif

#endif
