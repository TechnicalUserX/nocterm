#include <nocterm/nocterm.h>

// ====================== Internal Access ====================== //

int nocterm_mouse_controller(nocterm_key_t* key);

nocterm_mouse_event_t nocterm_mouse_event(uint8_t mouse_byte, uint8_t col_byte, uint8_t row_byte);

int nocterm_mouse_enable(void);

int nocterm_mouse_disable(void);

int nocterm_overlay_refresh(nocterm_overlay_t* overlay);

nocterm_widget_t* nocterm_page_find_next_focusable_widget(nocterm_page_t* page);

nocterm_widget_t* nocterm_page_find_prev_focusable_widget(nocterm_page_t* page);

int nocterm_page_change_focus(nocterm_page_t* page, nocterm_page_focus_t focus);

int nocterm_page_refresh(nocterm_page_t* page);

void nocterm_screen_ownership_reset(void);

extern nocterm_screen_ownership_t* nocterm_screen_ownership;

extern nocterm_signal_flags_t nocterm_signal_flags;

void nocterm_timer_tick(void);

int nocterm_widget_refresh(nocterm_widget_t* widget);

int nocterm_widget_flex_update(nocterm_widget_t* widget);

int nocterm_signal_init(void);


extern nocterm_widget_t* nocterm_widget_focused;

// ====================== Internal Access ====================== //


int nocterm_init(void){

    if(nocterm_mode_init() == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }
    
    if(nocterm_io_init() == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    if(nocterm_signal_init() == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }    

    if(nocterm_io_set_cursor_visible(false) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    if(nocterm_mode_set_raw() == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    if(nocterm_mouse_get_support() != NOCTERM_MOUSE_SUPPORT_NONE){
        if(nocterm_mouse_enable() == NOCTERM_FAILURE){
            return NOCTERM_FAILURE;
        }
    }

    if(nocterm_io_clear() == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    struct winsize w = {0};
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1){
        return NOCTERM_FAILURE;
    }
    
    nocterm_screen_height = w.ws_row;
    nocterm_screen_width = w.ws_col;

    nocterm_screen_ownership = (nocterm_screen_ownership_t*)malloc(sizeof(nocterm_screen_ownership_t) * nocterm_screen_height * nocterm_screen_width);

    if(nocterm_screen_ownership == NULL){
        return NOCTERM_FAILURE;
    }

    memset(nocterm_screen_ownership, 0x0, sizeof(nocterm_screen_ownership_t) * nocterm_screen_height * nocterm_screen_width);

    return NOCTERM_SUCCESS;
}

int nocterm_end(void){

    if(nocterm_io_set_cursor_visible(true) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    if(nocterm_attribute_clear() == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    if(nocterm_mode_restore() == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    if(nocterm_mouse_get_support() != NOCTERM_MOUSE_SUPPORT_NONE){
        if(nocterm_mouse_disable() == NOCTERM_FAILURE){
            return NOCTERM_FAILURE;
        }
    }

    if(nocterm_io_end() == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    if(nocterm_timer_delete_all() == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    free(nocterm_screen_ownership);

    return NOCTERM_SUCCESS;
}

int nocterm_loop(void){



    nocterm_page_t* current_page = NULL;
    nocterm_overlay_t* current_overlay = NULL;

    // EVENT LOOP
    while(1){

        nocterm_key_t key = {0};

        // Checking the existance of an overlay
        if(nocterm_overlay){
            current_overlay = nocterm_overlay;
        }

        if(nocterm_page_stack_size > 0){

            nocterm_page_t* top_page = NULL;

            if(current_page != nocterm_page_stack[nocterm_page_stack_size-1]){
                // This runs everytime the page changes
                // Including initial opening
                top_page = nocterm_page_stack[nocterm_page_stack_size-1];

                if(top_page->root_widget){

                    if(top_page->root_widget->focusable){

                        top_page->focused_widget = top_page->root_widget;
                        
                        // Set the focused widget
                        nocterm_widget_focused = top_page->focused_widget;

                        if(top_page->focused_widget->focus_handler){

                            top_page->focused_widget->focus_handler(top_page->focused_widget,NOCTERM_WIDGET_FOCUS_ENTER);
                        }
                    }else{
                        nocterm_widget_focused = NULL;
                    }

                    // Everytime the page changes, flex and alignments are updated,
                    // not waiting a screen resize event

                    nocterm_widget_flex_update(top_page->root_widget);

                    if(current_overlay){
                        for(uint64_t i = 0; i < NOCTERM_OVERLAY_WIDGET_MAX_SIZE; i++){
                            if(current_overlay->widgets[i]){
                                nocterm_widget_flex_update(current_overlay->widgets[i]);
                                nocterm_widget_align_update(current_overlay->widgets[i]);
                            }
                        }
                    }

                    nocterm_widget_align_update(top_page->root_widget);

                }

                nocterm_io_clear();
                nocterm_screen_ownership_reset();
                top_page->root_widget->hard_refresh = true;
                if(current_overlay){
                    current_overlay->hard_refresh = true;
                }

            }

            current_page = nocterm_page_stack[nocterm_page_stack_size-1];

        }else{
            nocterm_io_clear();
            nocterm_io_cursor_move(0,0);            
            break;
        }

        // CAPTURE KEY PHASE

        struct timespec loop_sleep = {0, 1000000}; // 1 ms
        struct timeval io_wait_interval = { .tv_sec = 0, .tv_usec = 0 };

        bool available = false;
        bool key_captured = false;
    
        if(nocterm_io_read_available(&available,io_wait_interval) == NOCTERM_SUCCESS && available){
            if(nocterm_key_capture(&key) == NOCTERM_SUCCESS){
                key_captured = true;
            }
        }
    
        if(key_captured){

            nocterm_key_event_t key_event = nocterm_key_translate(&key);

            if(key_event == NOCTERM_KEY_EVENT_TAB){
                // Change focus to the next item in the tree
                nocterm_page_change_focus(current_page, NOCTERM_PAGE_FOCUS_NEXT);
                nocterm_widget_focused = current_page->focused_widget;
            
            }else if(key_event == NOCTERM_KEY_EVENT_SHIFT_TAB){
                // Change focus to the previous item in the tree
                nocterm_page_change_focus(current_page, NOCTERM_PAGE_FOCUS_PREV);
                nocterm_widget_focused = current_page->focused_widget;
            
            }else if(key_event == NOCTERM_KEY_EVENT_ESCAPE){

                // If the current page is the only page in the stack, exit the application
                // otherwise no effect
                
                if(nocterm_page_stack_size == 1){
                    nocterm_page_stack_pop();
                    nocterm_io_clear();
                    nocterm_io_cursor_move(0,0);                    
                    break;
                }
            }else if(key_event == NOCTERM_KEY_EVENT_MOUSE){
                nocterm_mouse_controller(&key);
                nocterm_widget_focused = current_page->focused_widget;
            }else{
                if(current_page && current_page->focused_widget && current_page->focused_widget->key_handler){
                    current_page->focused_widget->key_handler(current_page->focused_widget, &key);
                }
            }
    
        }

        // EXECUTE TIMER CALLBACKS PHASE
        nocterm_timer_tick();

        // Resize control
        if(nocterm_signal_flags.sigwinch){

            current_page->root_widget->hard_refresh = true;
            if(current_overlay){
                current_overlay->hard_refresh = true;
            }
            nocterm_signal_flags.sigwinch = false;

            struct winsize w = {0};

            if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1){
                return NOCTERM_FAILURE;
            }

            nocterm_screen_height = w.ws_row;
            nocterm_screen_width = w.ws_col;
            
            free(nocterm_screen_ownership);

            nocterm_screen_ownership = (nocterm_screen_ownership_t*)malloc(sizeof(nocterm_screen_ownership_t) * nocterm_screen_height * nocterm_screen_width);
            if(nocterm_screen_ownership == NULL){
                return NOCTERM_FAILURE;
            }

            nocterm_screen_ownership_reset();

            // Flex update resizes widgets before alignment repositions them
            nocterm_widget_flex_update(current_page->root_widget);
            nocterm_widget_align_update(current_page->root_widget);

            if(current_overlay){
                for(uint64_t i = 0; i < NOCTERM_OVERLAY_WIDGET_MAX_SIZE; i++){
                    if(current_overlay->widgets[i]){
                        nocterm_widget_flex_update(current_overlay->widgets[i]);
                        nocterm_widget_align_update(current_overlay->widgets[i]);
                    }
                }
            }

        }

        // After timer callbacks happened and before the refresh phase,
        // we will check whether the current focused widget is set invisible
        if(current_page->focused_widget && current_page->focused_widget->visible == false){
            nocterm_page_change_focus(current_page, NOCTERM_PAGE_FOCUS_NEXT);
            nocterm_widget_focused = current_page->focused_widget;            
        }

        // REFRESH PHASE
        if(current_page->root_widget->hard_refresh){
            if(current_overlay){
                current_overlay->hard_refresh = true;
            }
        }

        if(current_page->root_widget->hard_refresh || (current_overlay && current_overlay->hard_refresh)){
            nocterm_io_clear();
            nocterm_screen_ownership_reset();
        }

        // Overlay refreshes before the pages
        if(current_overlay){
            nocterm_overlay_refresh(current_overlay);
        }

        // Screen refreshes after each individual update has been made
        nocterm_page_refresh(current_page); 

        // To not stress the CPU
        nanosleep(&loop_sleep, NULL); 
    }

    // END PHASE

    
    return NOCTERM_SUCCESS;
}
