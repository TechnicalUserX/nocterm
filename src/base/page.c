#include <nocterm/base/page.h>

uint64_t nocterm_page_stack_size = 0;

nocterm_page_t* nocterm_page_stack[NOCTERM_PAGE_STACK_MAX_SIZE] = {0};

nocterm_page_t* nocterm_page_new(const char* title, uint64_t title_size, nocterm_widget_t* root_widget){

    if(root_widget == NULL){
        errno = EINVAL;
        return NULL;
    }

    nocterm_page_t* new_page = (nocterm_page_t*)malloc(sizeof(nocterm_page_t));

    if(new_page == NULL){
        errno = ENOMEM;
        return NULL;
    }

    memset(new_page, 0x0, sizeof(nocterm_page_t));

    uint64_t button_string_length = nocterm_char_string_from_stream(new_page->title, NOCTERM_PAGE_TITLE_MAX_SIZE, title, title_size);

    if(button_string_length == 0){
        free(new_page);
        return NULL;
    }

    new_page->root_widget = root_widget;
    new_page->focused_widget = NULL;

    return new_page;
}

int nocterm_page_delete(nocterm_page_t* page){
    if(page == NULL){
        return NOCTERM_SUCCESS;
    }
    
    free(page);

    return NOCTERM_SUCCESS;
}

int nocterm_page_stack_push(nocterm_page_t* page){

    if(page == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(nocterm_page_stack_size < NOCTERM_PAGE_STACK_MAX_SIZE){
        nocterm_page_stack[nocterm_page_stack_size] = page;
        nocterm_page_stack_size++;

        // We may not now whether we need to start some timers

        page->root_widget->hard_refresh = true; // Draw everyting on the push time

        return NOCTERM_SUCCESS;
    }else{
        errno = ENOMEM;
        return NOCTERM_FAILURE;
    }

}

int nocterm_page_stack_pop(){
    
    if(nocterm_page_stack_size > 0){
        
        if(nocterm_timer_stop_all_of_widget(nocterm_page_stack[nocterm_page_stack_size-1]->root_widget, true) == NOCTERM_FAILURE){
            return NOCTERM_FAILURE;
        }
        
        // We are not deleting the page, just removing it from the stack

        nocterm_page_stack[nocterm_page_stack_size-1] = NULL; 
        nocterm_page_stack_size--; // Do not deallocate memory
    }

    return NOCTERM_SUCCESS;
}

nocterm_widget_t* nocterm_page_find_next_focusable_widget(nocterm_page_t* page) {
    if (page == NULL || page->root_widget == NULL) {
        return NULL;
    }

    nocterm_widget_t* start = page->focused_widget ? page->focused_widget : page->root_widget;
    bool found_current = (page->focused_widget == NULL); // If no focused widget, find first focusable immediately

    nocterm_widget_t* stack[NOCTERM_WIDGET_MAX_DEPTH];
    int stack_top = 0;

    stack[stack_top++] = page->root_widget;

    while (stack_top > 0) {
        nocterm_widget_t* current = stack[--stack_top];

        if (found_current && current->focusable && current->visible) {
            return current;
        }

        if (current == start) {
            found_current = true;
        }

        // Push children onto the stack (reversed so left-to-right traversal)
        for (int64_t i = current->subwidgets_size - 1; i >= 0; i--) {
            stack[stack_top++] = current->subwidgets[i];
        }
    }

    // If nothing found after current focused widget, wrap around
    if (page->focused_widget != NULL) {
        page->focused_widget = NULL; // Temporarily pretend no focus
        return nocterm_page_find_next_focusable_widget(page);
    }

    return NULL; // No focusable widget at all
}

nocterm_widget_t* nocterm_page_find_prev_focusable_widget(nocterm_page_t* page) {
    if (page == NULL || page->root_widget == NULL) {
        return NULL;
    }

    nocterm_widget_t* start = page->focused_widget ? page->focused_widget : page->root_widget;

    // Traversal stack
    nocterm_widget_t* stack[NOCTERM_WIDGET_MAX_DEPTH];
    int stack_top = 0;

    stack[stack_top++] = page->root_widget;

    // We will record the *last* focusable widget before the currently focused one
    nocterm_widget_t* last_focusable = NULL;

    while (stack_top > 0) {
        nocterm_widget_t* current = stack[--stack_top];

        if (current == start) {
            break; // Stop traversal
        }

        if (current->focusable && current->visible) {
            last_focusable = current;
        }

        // Push children onto the stack (left-to-right order)
        for (int64_t i = current->subwidgets_size - 1; i >= 0; i--) {
            stack[stack_top++] = current->subwidgets[i];
        }
    }

    if (last_focusable) {
        return last_focusable;
    }

    // If not found, wrap around to the last focusable widget in the whole tree
    last_focusable = NULL;
    stack_top = 0;
    stack[stack_top++] = page->root_widget;

    while (stack_top > 0) {
        nocterm_widget_t* current = stack[--stack_top];

        if (current->focusable && current->visible) {
            last_focusable = current;
        }

        for (int64_t i = current->subwidgets_size - 1; i >= 0; i--) {
            stack[stack_top++] = current->subwidgets[i];
        }
    }

    return last_focusable;
}

int nocterm_page_change_focus(nocterm_page_t* page, nocterm_page_focus_t focus){

    if(page == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(focus == NOCTERM_PAGE_FOCUS_NEXT){

        nocterm_widget_t* current_widget = page->focused_widget;
        page->focused_widget = nocterm_page_find_next_focusable_widget(page);

        if(page->focused_widget == NULL){
            return NOCTERM_SUCCESS;
        }

        if(page->focused_widget != current_widget && current_widget && current_widget->focus_handler){
            current_widget->focus_handler(current_widget, NOCTERM_WIDGET_FOCUS_LEAVE);
        }

        if(page->focused_widget != current_widget && page->focused_widget->focus_handler){
            page->focused_widget->focus_handler(page->focused_widget, NOCTERM_WIDGET_FOCUS_ENTER);
        }
        
    }else{
        nocterm_widget_t* current_widget = page->focused_widget;
        
        page->focused_widget = nocterm_page_find_prev_focusable_widget(page);

        if(page->focused_widget == NULL){
            return NOCTERM_SUCCESS;
        }

        if(page->focused_widget != current_widget && current_widget && current_widget->focus_handler){
            current_widget->focus_handler(current_widget, NOCTERM_WIDGET_FOCUS_LEAVE);
        }

        if(page->focused_widget != current_widget && page->focused_widget->focus_handler){
            page->focused_widget->focus_handler(page->focused_widget, NOCTERM_WIDGET_FOCUS_ENTER);
        }

    }

    return NOCTERM_SUCCESS;
}
