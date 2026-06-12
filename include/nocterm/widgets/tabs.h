/**
 * @file tabs.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2026-06-12
 *
 * @copyright Copyright (c) 2026
 *
 */

#ifndef NOCTERM_TABS_H
#define NOCTERM_TABS_H

#include <nocterm/common/nocterm.h>
#include <nocterm/base/widget.h>

#define NOCTERM_TABS(x) ((nocterm_tabs_t*)x)

#ifdef __cplusplus
    extern "C" {
#endif

/**
 * @brief A minimal tab container.
 *
 * A tabs widget holds several "tab" root widgets and shows exactly one of them
 * at a time.  Each tab is an ordinary widget that the caller populates with
 * subwidgets, exactly like the root widget of a page.  The tabs widget draws no
 * chrome of its own: there is no navigation bar.  Switching the visible tab is
 * done entirely by the caller through nocterm_tabs_navigate() — for example,
 * from the handler of a button the caller placed somewhere in the layout.
 *
 * The tab root widgets are not owned by the tabs widget; deleting the tabs
 * widget does not delete them.
 */
typedef struct nocterm_tabs_t{
    nocterm_widget_t widget;
    nocterm_widget_t** roots;   /* tab root widgets (not owned) */
    uint64_t roots_size;
    uint64_t active_index;
}nocterm_tabs_t;

/**
 * @brief Creates a new tabs widget.
 *
 * @param height
 * @param width
 * @return nocterm_tabs_t*
 */
nocterm_tabs_t* nocterm_tabs_new(nocterm_dimension_size_t height, nocterm_dimension_size_t width);

/**
 * @brief Constructs a tabs widget.
 *
 * @param tabs
 * @param height
 * @param width
 * @return int
 */
int nocterm_tabs_constructor(nocterm_tabs_t* tabs, nocterm_dimension_size_t height, nocterm_dimension_size_t width);

/**
 * @brief Destructs a tabs widget.
 *
 * @note The tab root widgets are not deleted, only detached.
 * @param tabs
 * @return int
 */
int nocterm_tabs_destructor(nocterm_tabs_t* tabs);

/**
 * @brief Deletes a tabs widget.
 *
 * @note The tab root widgets are not deleted, only detached.
 * @param tabs
 * @return int
 */
int nocterm_tabs_delete(nocterm_tabs_t* tabs);

/**
 * @brief Adds a tab root widget to a tabs widget.
 *
 * The added widget becomes a subwidget of the tabs widget; populate it with
 * subwidgets just as you would the root widget of a page.  The first tab added
 * becomes the active (visible) one; subsequent tabs start hidden.  Tabs are
 * indexed by the order in which they are added, starting at 0.
 *
 * @param tabs
 * @param root
 * @return int
 */
int nocterm_tabs_add(nocterm_tabs_t* tabs, nocterm_widget_t* root);

/**
 * @brief Switches the active (visible) tab.
 *
 * @param tabs
 * @param index
 * @return int
 */
int nocterm_tabs_navigate(nocterm_tabs_t* tabs, uint64_t index);

/**
 * @brief Returns the number of tabs.
 *
 * @param tabs
 * @return uint64_t
 */
uint64_t nocterm_tabs_get_count(nocterm_tabs_t* tabs);

/**
 * @brief Returns the index of the active tab, or -1 if there are no tabs.
 *
 * @param tabs
 * @return int64_t
 */
int64_t nocterm_tabs_get_active_index(nocterm_tabs_t* tabs);

/**
 * @brief Returns the root widget of the active tab, or NULL if there are none.
 *
 * @param tabs
 * @return nocterm_widget_t*
 */
nocterm_widget_t* nocterm_tabs_get_active(nocterm_tabs_t* tabs);

/**
 * @brief Returns the root widget of the tab at the given index, or NULL.
 *
 * @param tabs
 * @param index
 * @return nocterm_widget_t*
 */
nocterm_widget_t* nocterm_tabs_get_root(nocterm_tabs_t* tabs, uint64_t index);

#ifdef __cplusplus
    }
#endif

#endif
