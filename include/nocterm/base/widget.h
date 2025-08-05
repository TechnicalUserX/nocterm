/**
 * @file widget.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2025-08-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef NOCTERM_WIDGET_H
#define NOCTERM_WIDGET_H

#include <nocterm/common/nocterm.h>
#include <nocterm/base/key.h>
#include <nocterm/base/char.h>
#include <nocterm/base/attribute.h>
#include <nocterm/base/capability.h>
#include <nocterm/base/encoding.h>
#include <nocterm/base/screen.h>

#ifndef NOCTERM_WIDGET_MAX_DEPTH
    #define NOCTERM_WIDGET_MAX_DEPTH 128
#endif

#define NOCTERM_WIDGET(x) ((nocterm_widget_t*)x)

#ifdef __cplusplus
    extern "C" {
#endif

typedef uint32_t nocterm_widget_buffer_size_t;

typedef struct nocterm_widget_cell_t{
    nocterm_char_t character;
    nocterm_attribute_t attribute;
    bool refresh;
}nocterm_widget_cell_t;

typedef enum nocterm_widget_focus_t{
    NOCTERM_WIDGET_FOCUS_ENTER,
    NOCTERM_WIDGET_FOCUS_LEAVE 
}nocterm_widget_focus_t;

typedef struct nocterm_widget_t nocterm_widget_t;

typedef void (*nocterm_widget_key_handler_t)(nocterm_widget_t* self, nocterm_key_t* key);
typedef void (*nocterm_widget_focus_handler_t)(nocterm_widget_t* self, nocterm_widget_focus_t focus);

#define NOCTERM_WIDGET_KEY_HANDLER(identifier) void identifier(nocterm_widget_t* self, nocterm_key_t* key)

#define NOCTERM_WIDGET_FOCUS_HANDLER(identifier) void identifier(nocterm_widget_t* self, nocterm_widget_focus_t focus)

typedef struct nocterm_widget_t{

    struct nocterm_widget_t* parent;

    uint64_t subwidgets_size;
    struct nocterm_widget_t** subwidgets;

    // Real dimensions
    nocterm_dimension_t bounds;

    // Relative to the bounds of the widget
    nocterm_dimension_t viewport;

    // (NULL) or (bounds.row * bounds.col * sizeof(nocterm_widget_cell_t))
    nocterm_widget_buffer_size_t buffer_size;
    nocterm_widget_cell_t* buffer; 

    bool soft_refresh; // There is a change in the buffer
    bool hard_refresh; // Complete redraw required
    bool is_virtual;
    bool visible; // No longer drawn if false, all subwdigets also not drawn
    bool focusable;
    bool center_horizontal; // Omits col bound when true
    bool center_vertical; // Omits row bound when true

    nocterm_widget_key_handler_t key_handler;
    nocterm_widget_focus_handler_t focus_handler;

}nocterm_widget_t;

typedef enum nocterm_widget_type_t{
    NOCTERM_WIDGET_TYPE_REAL,
    NOCTERM_WIDGET_TYPE_VIRTUAL,
}nocterm_widget_type_t;

typedef enum nocterm_widget_focusable_t{
    NOCTERM_WIDGET_FOCUSABLE_NO,
    NOCTERM_WIDGET_FOCUSABLE_YES
}nocterm_widget_focusable_t;

/**
 * @brief Creates a new widget.
 * 
 * @param bounds 
 * @param focusable 
 * @param type 
 * @return nocterm_widget_t* 
 */
nocterm_widget_t* nocterm_widget_new(nocterm_dimension_t bounds, nocterm_widget_focusable_t focusable, nocterm_widget_type_t type);

/**
 * @brief Constructs a widget on the allocated memory.
 * 
 * @param widget 
 * @param bounds 
 * @param focusable 
 * @param type 
 * @return int 
 */
int nocterm_widget_constructor(nocterm_widget_t* widget, nocterm_dimension_t bounds, nocterm_widget_focusable_t focusable, nocterm_widget_type_t type);

/**
 * @brief Changes viewport of a widget.
 * 
 * @param widget 
 * @param viewport 
 * @return int 
 */
int nocterm_widget_viewport(nocterm_widget_t* widget, nocterm_dimension_t viewport);

/**
 * @brief Moves up the viewport of a widget.
 * 
 * @param widget 
 * @return int 
 */
int nocterm_widget_viewport_up(nocterm_widget_t* widget);

/**
 * @brief Moves down the viewport of a widget.
 * 
 * @param widget 
 * @return int 
 */
int nocterm_widget_viewport_down(nocterm_widget_t* widget);

/**
 * @brief Moves right the viewport of a widget.
 * 
 * @param widget 
 * @return int 
 */
int nocterm_widget_viewport_right(nocterm_widget_t* widget);

/**
 * @brief Moves left the viewport of a widget.
 * 
 * @param widget 
 * @return int 
 */
int nocterm_widget_viewport_left(nocterm_widget_t* widget);

/**
 * @brief Retrieves the row and column position of a widget.
 * 
 * @param widget 
 * @param row 
 * @param col 
 * @return int 
 */
int nocterm_widget_get_position(nocterm_widget_t* widget, nocterm_dimension_size_t* row, nocterm_dimension_size_t* col);

/**
 * @brief Sets the row and column position of a widget.
 * 
 * @param widget 
 * @param row 
 * @param col 
 * @return int 
 */
int nocterm_widget_set_position(nocterm_widget_t* widget, nocterm_dimension_size_t row, nocterm_dimension_size_t col);

/**
 * @brief Sets the row position of a widget.
 * 
 * @param widget 
 * @param row 
 * @return int 
 */
int nocterm_widget_set_position_row(nocterm_widget_t* widget, nocterm_dimension_size_t row);

/**
 * @brief Sets the column position of a widget.
 * 
 * @param widget 
 * @param col 
 * @return int 
 */
int nocterm_widget_set_position_col(nocterm_widget_t* widget, nocterm_dimension_size_t col);

/**
 * @brief Centers a widget horizontally.
 * 
 * @param widget 
 * @return int 
 */
int nocterm_widget_center_position_horizontal(nocterm_widget_t* widget);

/**
 * @brief Centers a widget vertically.
 * 
 * @param widget 
 * @return int 
 */
int nocterm_widget_center_position_vertical(nocterm_widget_t* widget);

/**
 * @brief Update positions of widgets if they are centered in any way.
 * 
 * @param widget 
 * @return int 
 */
int nocterm_widget_center_position_update(nocterm_widget_t* widget);

/**
 * @brief Gets the visibility of a widget.
 * 
 * @param widget 
 * @param visible 
 * @return int 
 */
int nocterm_widget_get_visible(nocterm_widget_t* widget, bool* visible);

/**
 * @brief Sets the visibility of a widget.
 * 
 * @param widget 
 * @param visible 
 * @return int 
 */
int nocterm_widget_set_visible(nocterm_widget_t* widget, bool visible);

/**
 * @brief Adds a subwidget to a widget.
 * 
 * @param widget 
 * @param subwidget 
 * @return int 
 */
int nocterm_widget_add_subwidget(nocterm_widget_t* widget, nocterm_widget_t* subwidget);

/**
 * @brief Assigns a key handler callback to a widget.
 * 
 * @param widget 
 * @param key_handler 
 * @return int 
 */
int nocterm_widget_add_key_handler(nocterm_widget_t* widget, nocterm_widget_key_handler_t key_handler);

/**
 * @brief Assigns a focus handler callback to a widget.
 * 
 * @param widget 
 * @param focus_handler 
 * @return int 
 */
int nocterm_widget_add_focus_handler(nocterm_widget_t* widget, nocterm_widget_focus_handler_t focus_handler);

/**
 * @brief Deletes a widget.
 * 
 * @param widget 
 * @return int 
 */
int nocterm_widget_delete(nocterm_widget_t* widget);

/**
 * @brief Destructs a widget on an allocated memory.
 * 
 * @param widget 
 * @return int 
 */
int nocterm_widget_destructor(nocterm_widget_t* widget);

/**
 * @brief Updates a single cell in the widget cell buffer.
 * 
 * @param widget 
 * @param row 
 * @param col 
 * @param ch 
 * @param attr 
 * @return int 
 */
int nocterm_widget_update(nocterm_widget_t* widget, nocterm_dimension_size_t row, nocterm_dimension_size_t col, nocterm_char_t ch, nocterm_attribute_t attr);

/**
 * @brief Refreshes given and all subwidgets, draws them on the screen.
 * 
 * @param widget 
 * @return int 
 */
int nocterm_widget_refresh(nocterm_widget_t* widget);

/**
 * @brief Enforces root to be refreshed.
 * 
 * @param widget 
 * @return int 
 */
int nocterm_widget_enforce_root_refresh(nocterm_widget_t* widget);

/**
 * @brief Clears the cell buffer of a widget.
 * 
 * @param widget 
 * @return int 
 */
int nocterm_widget_clear(nocterm_widget_t* widget);

/**
 * @brief Resizes the cell buffer of a widget.
 * 
 * @param widget 
 * @param height 
 * @param width 
 * @param preserve_buffer 
 * @return int 
 */
int nocterm_widget_resize(nocterm_widget_t* widget, nocterm_dimension_size_t height, nocterm_dimension_size_t width, bool preserve_buffer);

/**
 * @brief Make a widget give away the cell ownerships on the screen.
 * 
 * @param widget 
 * @return int 
 */
int nocterm_widget_lose_screen_ownership(nocterm_widget_t* widget);

#ifdef __cplusplus
    }
#endif

#endif
