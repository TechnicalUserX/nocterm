/**
 * @file page.h
 * @author TecnicalUserX (technicaluserx@outlook.com)
 * @date 2025-08-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef NOCTERM_PAGE_H
#define NOCTERM_PAGE_H

#include <nocterm/common/nocterm.h>
#include <nocterm/base/widget.h>
#include <nocterm/base/timer.h>

#ifndef NOCTERM_PAGE_TITLE_MAX_SIZE
    #define NOCTERM_PAGE_TITLE_MAX_SIZE 256
#endif

#ifndef NOCTERM_PAGE_STACK_MAX_SIZE
    #define NOCTERM_PAGE_STACK_MAX_SIZE 128
#endif

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct nocterm_page_t{
    nocterm_char_t title[NOCTERM_PAGE_TITLE_MAX_SIZE];
    nocterm_widget_t* root_widget; // It has its own children
    nocterm_widget_t* focused_widget;
}nocterm_page_t;

typedef enum nocterm_page_focus_t{
    NOCTERM_PAGE_FOCUS_NEXT = 0,
    NOCTERM_PAGE_FOCUS_PREV
}nocterm_page_focus_t;

extern uint64_t nocterm_page_stack_size;

extern nocterm_page_t* nocterm_page_stack[NOCTERM_PAGE_STACK_MAX_SIZE];

/**
 * @brief Create a new page.
 * 
 * @param title 
 * @param title_size 
 * @param root_widget 
 * @return nocterm_page_t* 
 */
nocterm_page_t* nocterm_page_new(const char* title, uint64_t title_size, nocterm_widget_t* root_widget);

/**
 * @brief Deletes a page.
 * 
 * @param page 
 * @return int 
 */
int nocterm_page_delete(nocterm_page_t* page);

/**
 * @brief Adds a new page to the page stack. Newly added page becomes the current page.
 * 
 * @param page 
 * @return int 
 */
int nocterm_page_stack_push(nocterm_page_t* page);

/**
 * @brief Removes the current page from the stack. If removed page was the only page, then mani loop exits.
 * 
 * @return int 
 */
int nocterm_page_stack_pop();

/**
 * @brief Searches the next focusable widget in the widget tree.
 * 
 * @param page 
 * @return nocterm_widget_t* 
 */
nocterm_widget_t* nocterm_page_find_next_focusable_widget(nocterm_page_t* page);

/**
 * @brief Searches the previous focusable widget in the widget tree.
 * 
 * @param page 
 * @return nocterm_widget_t* 
 */
nocterm_widget_t* nocterm_page_find_prev_focusable_widget(nocterm_page_t* page);

/**
 * @brief Manages focus in the page.
 * 
 * @param page 
 * @param focus 
 * @return int 
 */
int nocterm_page_change_focus(nocterm_page_t* page, nocterm_page_focus_t focus);

#ifdef __cplusplus
    }
#endif

#endif
