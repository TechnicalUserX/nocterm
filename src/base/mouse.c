#include <nocterm/base/mouse.h>

bool nocterm_mouse_support_flag = false;

nocterm_dimension_size_t nocterm_mouse_row = 0, nocterm_mouse_col = 0;

void nocterm_mouse_support(bool enable){
    nocterm_mouse_support_flag = enable;
}

int nocterm_mouse_enable(void){
    return nocterm_io_write("\033[?1000h",9);
}

int nocterm_mouse_disable(void){
    return nocterm_io_write("\033[?1000l",9);
}

nocterm_mouse_event_t nocterm_mouse_event(uint8_t mouse_byte, uint8_t col_byte, uint8_t row_byte){

    mouse_byte -= 32;

    nocterm_mouse_event_t new_mouse_event = {0};

    new_mouse_event.modifier.ctrl = (mouse_byte & 0x10) ? true : false;
    new_mouse_event.modifier.alt = (mouse_byte & 0x08) ? true : false;
    new_mouse_event.modifier.shift = (mouse_byte & 0x04) ? true : false;

    new_mouse_event.row = row_byte - 32 - 1;
    new_mouse_event.col = col_byte - 32 - 1;

    // Is this a scroll event?
    bool is_scroll = ((mouse_byte & 0x40) == 0x40) ? true : false;

    if(is_scroll){
        bool is_scroll_down = ((mouse_byte & 0x01) == 0x01) ? true : false;
        if(is_scroll_down){
            new_mouse_event.button = NOCTERM_MOUSE_BUTTON_SCROLL_DOWN;
        }else{
            new_mouse_event.button = NOCTERM_MOUSE_BUTTON_SCROLL_UP;
        }
    }else{
        uint8_t button = mouse_byte & 0x03;

        switch(button){
            case 0:
                new_mouse_event.button = NOCTERM_MOUSE_BUTTON_LMB;
                break;

            case 1:
                new_mouse_event.button = NOCTERM_MOUSE_BUTTON_MMB;
                break;
            
            case 2:
                new_mouse_event.button = NOCTERM_MOUSE_BUTTON_RMB;
                break;
            
            case 3:
                new_mouse_event.button = NOCTERM_MOUSE_BUTTON_RELEASE;
                break;
        }

    }

    return new_mouse_event;
}

int nocterm_mouse_controller(nocterm_key_t* key){

    static uint8_t mouse_progress_state = 0;
    static nocterm_widget_t* previous_mouse_widget = NULL;
    static nocterm_mouse_event_t previous_mouse_event = {0};

    if(key == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(key->buffer_length != 6){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    nocterm_mouse_event_t current_mouse_event = nocterm_mouse_event(key->buffer[3], key->buffer[4], key->buffer[5]);
    
    if(current_mouse_event.row >= nocterm_screen_height && current_mouse_event.col >= nocterm_screen_width){
        return NOCTERM_FAILURE;
    }

    nocterm_screen_ownership_t current_mouse_event_owner = nocterm_screen_ownership[nocterm_screen_width * current_mouse_event.row + current_mouse_event.col];

    nocterm_widget_t* current_mouse_widget = NOCTERM_WIDGET(current_mouse_event_owner.owner);
    
    if(current_mouse_widget == NULL){
        previous_mouse_widget = NULL;
        previous_mouse_event = (nocterm_mouse_event_t){0};
        mouse_progress_state = 0;
        return NOCTERM_SUCCESS;
    }

    nocterm_page_t* current_page = nocterm_page_stack[nocterm_page_stack_size-1];
    if(current_page == NULL){
        return NOCTERM_FAILURE;
    }

    nocterm_key_t crafted_key = {0};

    // Single shot event
    if(current_mouse_event.button == NOCTERM_MOUSE_BUTTON_SCROLL_DOWN || current_mouse_event.button == NOCTERM_MOUSE_BUTTON_SCROLL_UP){
        
        if(current_mouse_event.button == NOCTERM_MOUSE_BUTTON_SCROLL_DOWN){
            memcpy(crafted_key.buffer, "\e\x5B\x42", 3);
            crafted_key.buffer_length = 3;
            crafted_key.type = NOCTERM_KEY_TYPE_ESCSEQ;
        }else{
            memcpy(crafted_key.buffer, "\e\x5B\x41", 3);
            crafted_key.buffer_length = 3;
            crafted_key.type = NOCTERM_KEY_TYPE_ESCSEQ;
        }

        mouse_progress_state = 0;

        if(current_page->focused_widget != current_mouse_widget->owner){
            if(current_page->focused_widget && current_page->focused_widget->focus_handler){
                current_page->focused_widget->focus_handler(current_page->focused_widget, NOCTERM_WIDGET_FOCUS_LEAVE);
            }
    
            if(current_mouse_widget->owner->focusable && current_mouse_widget->owner->focus_handler){
                current_mouse_widget->owner->focus_handler(current_mouse_widget->owner, NOCTERM_WIDGET_FOCUS_ENTER);
            }
        }

        if(current_mouse_widget->owner->key_handler){
            current_mouse_widget->owner->key_handler(current_mouse_widget->owner, &crafted_key);
        }

        current_page->focused_widget = current_mouse_widget->owner;

        return NOCTERM_SUCCESS;
    }


    // Multi shot event
    if(mouse_progress_state == 0){
        if(current_mouse_event.button == NOCTERM_MOUSE_BUTTON_RELEASE){
            previous_mouse_widget = NULL;
            previous_mouse_event = (nocterm_mouse_event_t){0};            
            return NOCTERM_FAILURE;
        }

        previous_mouse_widget = current_mouse_widget;
        previous_mouse_event = current_mouse_event;
        mouse_progress_state++;

    }else{
        if(current_mouse_event.button != NOCTERM_MOUSE_BUTTON_RELEASE){
            previous_mouse_widget = current_mouse_widget;
            previous_mouse_event = current_mouse_event;
            return NOCTERM_SUCCESS;
        }

        if(previous_mouse_widget->owner == current_mouse_widget->owner){
            // We cliked on the same thing

            // Let's see whether there is a motion
            if(previous_mouse_event.row == current_mouse_event.row && previous_mouse_event.col == current_mouse_event.col){
                // Simple click

                // FOCUS_ENTER & FOCUS_LEAVE
                if(current_page->focused_widget->owner != current_mouse_widget->owner){
                    if(current_page->focused_widget && current_page->focused_widget->focus_handler){
                        current_page->focused_widget->focus_handler(current_page->focused_widget, NOCTERM_WIDGET_FOCUS_LEAVE);
                    }
            
                    if(current_mouse_widget->owner->focusable && current_mouse_widget->owner->focus_handler){
                        current_mouse_widget->owner->focus_handler(current_mouse_widget->owner, NOCTERM_WIDGET_FOCUS_ENTER);
                    }

                }

                if(previous_mouse_event.button == NOCTERM_MOUSE_BUTTON_LMB){

                    if(current_page->focused_widget == current_mouse_widget->owner || previous_mouse_event.modifier.ctrl){
                        
                        if(current_mouse_widget->owner->key_handler){
                            memcpy(crafted_key.buffer, "\n", 1);
                            crafted_key.buffer_length = 1;
                            crafted_key.type = NOCTERM_KEY_TYPE_CONTROL;  
    
                            current_mouse_widget->owner->key_handler(current_mouse_widget->owner, &crafted_key);
                        }

                    }

                }else if(previous_mouse_event.button == NOCTERM_MOUSE_BUTTON_MMB){
                    
                    if(current_mouse_widget->owner->key_handler){

                        memcpy(crafted_key.buffer, "\n", 1);
                        crafted_key.buffer_length = 1;
                        crafted_key.type = NOCTERM_KEY_TYPE_CONTROL;   
                        
                        current_mouse_widget->owner->key_handler(current_mouse_widget->owner, &crafted_key);
                    } 

                }

                if(current_mouse_widget->owner->focusable){
                    current_page->focused_widget = current_mouse_widget->owner;
                }else{
                    current_page->focused_widget = NULL;
                }

            }else{
                // There is motion
                // We have to check the direction and magnitude of the 2 dimensional motion

                // If the widget that the drag operation is performed on is not already focused,
                // discard the operation

                if(current_page->focused_widget->owner == current_mouse_widget->owner){

                    nocterm_dimension_size_t vertical_magnitude = 0;
                    nocterm_dimension_size_t horizontal_magnitude = 0;

                    if(previous_mouse_event.row != current_mouse_event.row){
                        // There is vertical movement
                        if(current_mouse_event.row < previous_mouse_event.row){
                            // Downwards motion
                            vertical_magnitude =  previous_mouse_event.row - current_mouse_event.row;
                            memcpy(crafted_key.buffer, "\e\x5B\x42", 3);
                            crafted_key.buffer_length = 3;
                            crafted_key.type = NOCTERM_KEY_TYPE_ESCSEQ;                              
                        }else{
                            // Upwards motion
                            vertical_magnitude = current_mouse_event.row - previous_mouse_event.row;
                            memcpy(crafted_key.buffer, "\e\x5B\x41", 3);
                            crafted_key.buffer_length = 3;
                            crafted_key.type = NOCTERM_KEY_TYPE_ESCSEQ;  
                        }
                        
                        
                        nocterm_dimension_size_t squared_vertical_magnitude = (nocterm_dimension_size_t)sqrt(vertical_magnitude);

                        if(current_mouse_widget->owner->key_handler){
                            for(nocterm_dimension_size_t i = 0; i < squared_vertical_magnitude; i++){
                                current_mouse_widget->owner->key_handler(current_mouse_widget->owner, &crafted_key);
                            }
                        }
                    }

                    if(previous_mouse_event.col != current_mouse_event.col){
                        // There is horizontal movement
                        if(current_mouse_event.col < previous_mouse_event.col){
                            // Rightwards motion
                            horizontal_magnitude =  previous_mouse_event.col - current_mouse_event.col;
                            memcpy(crafted_key.buffer, "\e\x5B\x43", 3);
                            crafted_key.buffer_length = 3;
                            crafted_key.type = NOCTERM_KEY_TYPE_ESCSEQ;                              
                        }else{
                            // Leftwards motion
                            horizontal_magnitude = current_mouse_event.col - previous_mouse_event.col;
                            memcpy(crafted_key.buffer, "\e\x5B\x44", 3);
                            crafted_key.buffer_length = 3;
                            crafted_key.type = NOCTERM_KEY_TYPE_ESCSEQ;  
                        }

                        nocterm_dimension_size_t squared_horizontal_magnitude = (nocterm_dimension_size_t)sqrt(horizontal_magnitude);
                        if(current_mouse_widget->owner->key_handler){
                            for(nocterm_dimension_size_t i = 0; i < squared_horizontal_magnitude; i++){
                                current_mouse_widget->owner->key_handler(current_mouse_widget->owner, &crafted_key);
                            }
                        }
                    }

                }
                // Discard operation
            }

            mouse_progress_state = 0;

        }else{
            // Operation discarded
            mouse_progress_state = 0;
            return NOCTERM_SUCCESS;
        }
        
    }


    return NOCTERM_SUCCESS;
}
