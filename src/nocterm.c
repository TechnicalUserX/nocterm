#include <nocterm/nocterm.h>

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

    if(nocterm_io_cursor_visible(false) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    if(nocterm_mode_raw() == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
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

    if(nocterm_io_cursor_visible(true) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    if(nocterm_attribute_clear() == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    if(nocterm_mode_restore() == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
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

    // INITIALIZE PHASE
    if(nocterm_init() == NOCTERM_FAILURE){
        return EXIT_FAILURE;
    }

    nocterm_page_t* current_page = NULL;

    while(1){

        nocterm_key_t key = {0};

        if(nocterm_page_stack_size > 0){

            nocterm_page_t* top_page = NULL;

            if(current_page != nocterm_page_stack[nocterm_page_stack_size-1]){
                // This runs everytime the page changes
                // Including initial opening
                top_page = nocterm_page_stack[nocterm_page_stack_size-1];
                
                if(top_page->root_widget->focusable){
                    top_page->focused_widget = top_page->root_widget;
                    if(top_page->focused_widget->focus_handler){
                        top_page->focused_widget->focus_handler(top_page->focused_widget,NOCTERM_WIDGET_FOCUS_ENTER);
                    }
                }
                nocterm_io_clear();
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
            }else if(key_event == NOCTERM_KEY_EVENT_SHIFT_TAB){
                // Change focus to the previous item in the tree
                nocterm_page_change_focus(current_page, NOCTERM_PAGE_FOCUS_PREV);
            }else if(key_event == NOCTERM_KEY_EVENT_ESCAPE){
                // Call the key_handler for the focused widget
                if(nocterm_page_stack_size > 1){
                    nocterm_page_stack_pop();
                }else{
                    nocterm_io_clear();
                    nocterm_io_cursor_move(0,0);
                    break;
                }
            }else{
                if(current_page && current_page->focused_widget && current_page->focused_widget->key_handler){
                    current_page->focused_widget->key_handler(current_page->focused_widget, &key);
                }
            }
    
        }

        // EXECUTE TIMER CALLBACKS PHASE
        nocterm_timer_tick();

        if(nocterm_signal_flags.nocterm_signal_sigwinch){

            current_page->root_widget->hard_refresh = true;
            nocterm_signal_flags.nocterm_signal_sigwinch = false;

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
        
            memset(nocterm_screen_ownership, 0x0, sizeof(nocterm_screen_ownership_t) * nocterm_screen_height * nocterm_screen_width);

            // We need to traverse over the widgets to reset their positions after resize
            nocterm_widget_center_position_update(current_page->root_widget);
        }

        // REFRESH PHASE
        if(current_page->root_widget->hard_refresh){
            nocterm_io_clear();
        }

        // Screen refreshes after each individual update has been made
        nocterm_widget_refresh(current_page->root_widget); 

        // To not stress the CPU
        nanosleep(&loop_sleep, NULL); 
    }

    // END PHASE

    if(nocterm_end() == NOCTERM_FAILURE){
        return EXIT_FAILURE;
    }    

    return EXIT_SUCCESS;
}
