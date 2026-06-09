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

typedef enum nocterm_widget_type_t{
    NOCTERM_WIDGET_TYPE_REAL,
    NOCTERM_WIDGET_TYPE_VIRTUAL,
}nocterm_widget_type_t;

typedef enum nocterm_widget_focus_t{
    NOCTERM_WIDGET_FOCUS_ENTER,
    NOCTERM_WIDGET_FOCUS_LEAVE 
}nocterm_widget_focus_t;

typedef enum nocterm_widget_focusable_t{
    NOCTERM_WIDGET_FOCUSABLE_NO,
    NOCTERM_WIDGET_FOCUSABLE_YES
}nocterm_widget_focusable_t;

typedef struct nocterm_widget_align_percentages_t{
    nocterm_percentage_t horizontal;                 
    nocterm_percentage_t vertical;
}nocterm_widget_align_percentages_t;

typedef struct nocterm_widget_align_margins_t{
    nocterm_dimension_size_t horizontal;
    nocterm_dimension_size_t vertical;
}nocterm_widget_align_margins_t;

typedef struct nocterm_widget_align_flags_t{
    bool horizontal:1;
    bool vertical:1;
    struct{
        bool left:1;
        bool center:1;
        bool right:1;
    }horizontal_flags;    
    struct{
        bool top:1;
        bool center:1;
        bool bottom:1;
    }vertical_flags;
}nocterm_widget_align_flags_t;

typedef enum nocterm_widget_align_t{
    NOCTERM_WIDGET_ALIGN_NONE,
    NOCTERM_WIDGET_ALIGN_LEFT,
    NOCTERM_WIDGET_ALIGN_RIGHT,
    NOCTERM_WIDGET_ALIGN_TOP,
    NOCTERM_WIDGET_ALIGN_BOTTOM,
    NOCTERM_WIDGET_ALIGN_PERCENT_HORIZONTAL,
    NOCTERM_WIDGET_ALIGN_PERCENT_VERTICAL,
    NOCTERM_WIDGET_ALIGN_MARGIN_HORIZONTAL,
    NOCTERM_WIDGET_ALIGN_MARGIN_VERTICAL,
    NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL,
    NOCTERM_WIDGET_ALIGN_CENTER_VERTICAL,
}nocterm_widget_align_t;

typedef struct nocterm_widget_align_policy_t{
    nocterm_widget_align_flags_t flags;
    nocterm_widget_align_percentages_t percentages;
    nocterm_widget_align_margins_t margins;
}nocterm_widget_align_policy_t;

typedef enum nocterm_widget_flex_t{
    NOCTERM_WIDGET_FLEX_FIXED_HORIZONTAL,
    NOCTERM_WIDGET_FLEX_FIXED_VERTICAL,
    NOCTERM_WIDGET_FLEX_FIXED_BOTH,
    NOCTERM_WIDGET_FLEX_FILL_HORIZONTAL,
    NOCTERM_WIDGET_FLEX_FILL_VERTICAL,
    NOCTERM_WIDGET_FLEX_FILL_BOTH,
    NOCTERM_WIDGET_FLEX_PERCENT_HORIZONTAL,
    NOCTERM_WIDGET_FLEX_PERCENT_VERTICAL
}nocterm_widget_flex_t;

typedef enum nocterm_widget_flex_policy_permission_t{
    NOCTERM_WIDGET_FLEX_POLICY_PERMISSION_NONE       = 0x00,
    NOCTERM_WIDGET_FLEX_POLICY_PERMISSION_HORIZONTAL = 0x01,
    NOCTERM_WIDGET_FLEX_POLICY_PERMISSION_VERTICAL   = 0x02,
    NOCTERM_WIDGET_FLEX_POLICY_PERMISSION_BOTH       = 0x03
}nocterm_widget_flex_policy_permission_t;

typedef enum nocterm_widget_flex_policy_mode_t{
    NOCTERM_WIDGET_FLEX_POLICY_MODE_FIXED   = 0,
    NOCTERM_WIDGET_FLEX_POLICY_MODE_FILL    = 1,
    NOCTERM_WIDGET_FLEX_POLICY_MODE_PERCENT = 2
}nocterm_widget_flex_policy_mode_t;

typedef struct nocterm_widget_flex_policy_t{
    nocterm_widget_flex_policy_mode_t horizontal_mode;
    nocterm_widget_flex_policy_mode_t vertical_mode;
    nocterm_percentage_t horizontal_percentage;
    nocterm_percentage_t vertical_percentage;
}nocterm_widget_flex_policy_t;

typedef struct nocterm_widget_t nocterm_widget_t;

typedef void (*nocterm_widget_key_handler_t)(nocterm_widget_t* self, nocterm_key_t* key);
typedef void (*nocterm_widget_focus_handler_t)(nocterm_widget_t* self, nocterm_widget_focus_t focus);
typedef void (*nocterm_widget_resize_handler_t)(nocterm_widget_t* self, nocterm_dimension_t new_bounds, nocterm_dimension_t new_viewport);

#define NOCTERM_WIDGET_KEY_HANDLER(identifier) void identifier(nocterm_widget_t* self, nocterm_key_t* key)

#define NOCTERM_WIDGET_FOCUS_HANDLER(identifier) void identifier(nocterm_widget_t* self, nocterm_widget_focus_t focus)

#define NOCTERM_WIDGET_RESIZE_HANDLER(identifier) void identifier(nocterm_widget_t* self, nocterm_dimension_t bounds, nocterm_dimension_t viewport)

typedef struct nocterm_widget_t{

    struct nocterm_widget_t* parent;
    struct nocterm_widget_t* owner;

    uint64_t subwidgets_size;
    struct nocterm_widget_t** subwidgets;

    // General Widget Lock
    pthread_mutex_t lock;

    // Real dimensions
    nocterm_dimension_t bounds;

    // Relative to the bounds of the widget
    nocterm_dimension_t viewport;

    // (NULL) or (bounds.row * bounds.col * sizeof(nocterm_widget_cell_t))
    nocterm_widget_buffer_size_t buffer_size;
    nocterm_widget_cell_t* buffer; 

    atomic_bool soft_refresh; // There is a change in the buffer
    atomic_bool hard_refresh; // Complete redraw required
    atomic_bool visible; // No longer drawn if false, all subwdigets also not drawn
    atomic_bool redraw; // This is different than refresh, it completely redraws

    bool is_virtual;
    bool focusable;

    // For mouse support: This flag indicates that when clicked with mouse/touch input, it will trigger ENTER key instantly
    // This flag must be handled carefully by widget constructors
    bool click_activation; 

    // If this is activated, widgets can be drawn outside the borders of it's parent.
    bool floating_subwidgets;

    nocterm_widget_align_policy_t align_policy;

    nocterm_widget_flex_policy_permission_t flex_policy_permission;
    nocterm_widget_flex_policy_t flex_policy;

    // Per-axis padding (in cells) subtracted from this widget's viewport when
    // computing flex/percent dimensions for direct children.  Decorbox sets
    // these to 1 to represent its single-cell border on every side.
    uint8_t flex_policy_inner_padding_h;
    uint8_t flex_policy_inner_padding_w;

    nocterm_widget_key_handler_t key_handler;
    nocterm_widget_focus_handler_t focus_handler;
    nocterm_widget_resize_handler_t internal_resize_handler;
    nocterm_widget_resize_handler_t resize_handler;

}nocterm_widget_t;



/**
 * @brief Keeps track of the currently focused widget, globally.
 * 
 */
NOCTERM_INTERNAL
extern nocterm_widget_t* nocterm_widget_focused;

/**
 * @brief Creates a new widget.
 * 
 * @param height 
 * @param width 
 * @param focusable 
 * @param type 
 * @return nocterm_widget_t* 
 */
nocterm_widget_t* nocterm_widget_new(nocterm_dimension_size_t height, nocterm_dimension_size_t width, nocterm_widget_focusable_t focusable, nocterm_widget_type_t type);

/**
 * @brief Constructs a widget on the allocated memory.
 * 
 * @param widget 
 * @param height 
 * @param width 
 * @param focusable 
 * @param type 
 * @return int 
 */
int nocterm_widget_constructor(nocterm_widget_t* widget, nocterm_dimension_size_t  height, nocterm_dimension_size_t width, nocterm_widget_focusable_t focusable, nocterm_widget_type_t type);

/**
 * @brief Destructs a widget on an allocated memory.
 * 
 * @param widget 
 * @return int 
 */
int nocterm_widget_destructor(nocterm_widget_t* widget);

/**
 * @brief Deletes a widget.
 * 
 * @param widget 
 * @return int 
 */
int nocterm_widget_delete(nocterm_widget_t* widget);

/**
 * @brief Changes viewport of a widget.
 * 
 * @param widget 
 * @param viewport 
 * @return int 
 */
int nocterm_widget_set_viewport(nocterm_widget_t* widget, nocterm_dimension_t viewport);

/**
 * @brief Moves up the viewport of a widget.
 * 
 * @param widget 
 * @return int 
 */
int nocterm_widget_set_viewport_up(nocterm_widget_t* widget);

/**
 * @brief Moves down the viewport of a widget.
 * 
 * @param widget 
 * @return int 
 */
int nocterm_widget_set_viewport_down(nocterm_widget_t* widget);

/**
 * @brief Moves right the viewport of a widget.
 * 
 * @param widget 
 * @return int 
 */
int nocterm_widget_set_viewport_right(nocterm_widget_t* widget);

/**
 * @brief Moves left the viewport of a widget.
 * 
 * @param widget 
 * @return int 
 */
int nocterm_widget_set_viewport_left(nocterm_widget_t* widget);

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
int nocterm_widget_set_row(nocterm_widget_t* widget, nocterm_dimension_size_t row);

/**
 * @brief Sets the column position of a widget.
 * 
 * @param widget 
 * @param col 
 * @return int 
 */
int nocterm_widget_set_col(nocterm_widget_t* widget, nocterm_dimension_size_t col);

/**
 * @brief Aligns a widget.
 * 
 * @param widget 
 * @param align 
 * @return int 
 */
int nocterm_widget_align(nocterm_widget_t* widget, nocterm_widget_align_t align, ...);

/**
 * @brief Update positions of widgets if they are centered in any way.
 * 
 * @param widget 
 * @return int 
 */
int nocterm_widget_align_update(nocterm_widget_t* widget);

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
 * @brief Sets click activation feature of a widget.
 * 
 * @param widget 
 * @param click_activation 
 * @return int 
 */
int nocterm_widget_set_click_activation(nocterm_widget_t* widget, bool click_activation);

/**
 * @brief Adds a subwidget to a widget.
 * 
 * @param widget 
 * @param subwidget 
 * @return int 
 */
int nocterm_widget_add_subwidget(nocterm_widget_t* widget, nocterm_widget_t* subwidget);

/**
 * @brief Removes a subwidget from a widget.
 * 
 * @param widget 
 * @param subwidget 
 * @return int 
 */
int nocterm_widget_remove_subwidget(nocterm_widget_t* widget, nocterm_widget_t* subwidget);

/**
 * @brief Checks whether a widget contains a specified subwidget.
 * 
 * @param widget 
 * @param subwidget 
 * @return true 
 * @return false 
 */
bool nocterm_widget_contains_subwidget(nocterm_widget_t* widget, nocterm_widget_t* subwidget);

/**
 * @brief Enables whether the subwidgets can be drawn outside the boundaries of their parent.
 * 
 * @note This function has no effect for widgets with zero dimensional bounds.
 * @param widget 
 * @param floating_subwidgets 
 * @return int 
 */
int nocterm_widget_set_floating_subwidgets(nocterm_widget_t* widget, bool floating_subwidgets);

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
 * @brief Assigns a resize handler callback to a widget.
 *
 * @param widget
 * @param resize_handler
 * @return int
 */
int nocterm_widget_add_resize_handler(nocterm_widget_t* widget, nocterm_widget_resize_handler_t resize_handler);

/**
 * @brief General purpse widget flex maniuplation function.
 *
 * @param widget
 * @param nocterm_widget_flex_t
 * @return int
 */
int nocterm_widget_flex(nocterm_widget_t* widget, nocterm_widget_flex_t flex, ...);

NOCTERM_INTERNAL
int nocterm_widget_flex_policy_set_permission(nocterm_widget_t* widget, nocterm_widget_flex_policy_permission_t permission);

NOCTERM_INTERNAL
int nocterm_widget_flex_policy_set_fixed_horizontal(nocterm_widget_t* widget);

NOCTERM_INTERNAL
int nocterm_widget_flex_policy_set_fixed_vertical(nocterm_widget_t* widget);


NOCTERM_INTERNAL
int nocterm_widget_flex_policy_set_fill_horizontal(nocterm_widget_t* widget);

NOCTERM_INTERNAL
int nocterm_widget_flex_policy_set_fill_vertical(nocterm_widget_t* widget);

NOCTERM_INTERNAL
int nocterm_widget_flex_policy_set_fill_both(nocterm_widget_t* widget);

NOCTERM_INTERNAL
int nocterm_widget_flex_policy_set_percent_horizontal(nocterm_widget_t* widget, nocterm_percentage_t percentage);

NOCTERM_INTERNAL
int nocterm_widget_flex_policy_set_percent_vertical(nocterm_widget_t* widget, nocterm_percentage_t percentage);

NOCTERM_INTERNAL
int nocterm_widget_flex_update(nocterm_widget_t* widget);

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
 * @brief Checks whether the current widget is the focused one.
 * 
 * @param widget 
 * @return true 
 * @return false 
 */
bool nocterm_widget_is_focused(nocterm_widget_t* widget);

#ifdef __cplusplus
    }
#endif

#endif
