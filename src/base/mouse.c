#include "nocterm/base/key.h"
#include <nocterm/base/mouse.h>

// ====================== Internal Access ====================== //

extern nocterm_screen_ownership_t* nocterm_screen_ownership;

// ====================== Internal Access ====================== //


NOCTERM_INTERNAL nocterm_mouse_support_t nocterm_mouse_support_flag = NOCTERM_MOUSE_SUPPORT_NONE;

NOCTERM_INTERNAL nocterm_dimension_size_t nocterm_mouse_row = 0, nocterm_mouse_col = 0;

void nocterm_mouse_set_support(nocterm_mouse_support_t support){
    nocterm_mouse_support_flag = support;
}

int nocterm_mouse_enable(void){
    switch(nocterm_mouse_support_flag){
        case NOCTERM_MOUSE_SUPPORT_ADVANCED:
            return nocterm_io_write("\033[?1000h\033[?1003h",17);
        case NOCTERM_MOUSE_SUPPORT_SIMPLE:
            return nocterm_io_write("\033[?1000h",9);
        case NOCTERM_MOUSE_SUPPORT_NONE:
            return NOCTERM_SUCCESS;
    }
    return NOCTERM_FAILURE;
}

int nocterm_mouse_disable(void){
    switch(nocterm_mouse_support_flag){
        case NOCTERM_MOUSE_SUPPORT_ADVANCED:
            return nocterm_io_write("\033[?1000l\033[?1003l",17);
        case NOCTERM_MOUSE_SUPPORT_SIMPLE:
            return nocterm_io_write("\033[?1000l",9);
        case NOCTERM_MOUSE_SUPPORT_NONE:
            return NOCTERM_SUCCESS;
    }
    return NOCTERM_FAILURE;
}

nocterm_mouse_event_t nocterm_mouse_translate(nocterm_key_t* key){

    if(key == NULL || nocterm_key_translate(key) != NOCTERM_KEY_EVENT_MOUSE){
        return (nocterm_mouse_event_t){.button = NOCTERM_MOUSE_BUTTON_UNDEFINED};
    }


    uint8_t mouse_byte = key->buffer[3];
    uint8_t col_byte = key->buffer[4];
    uint8_t row_byte = key->buffer[5];
    
    mouse_byte -= 32;

    nocterm_mouse_event_t new_mouse_event = {0};

    new_mouse_event.modifier.ctrl = (mouse_byte & 0x10) ? true : false;
    new_mouse_event.modifier.alt = (mouse_byte & 0x08) ? true : false;
    new_mouse_event.modifier.shift = (mouse_byte & 0x04) ? true : false;

    new_mouse_event.row = row_byte - 32 - 1;
    new_mouse_event.col = col_byte - 32 - 1;

    // Is this a scroll event?
    bool is_scroll = ((mouse_byte & 0x40) == 0x40) ? true : false;
    bool is_hover = ((mouse_byte & 0x20) == 0x20) ? true : false;

    if(is_scroll){
        bool is_scroll_down = ((mouse_byte & 0x01) == 0x01) ? true : false;
        if(is_scroll_down){
            new_mouse_event.button = NOCTERM_MOUSE_BUTTON_SCROLL_DOWN;
        }else{
            new_mouse_event.button = NOCTERM_MOUSE_BUTTON_SCROLL_UP;
        }
    }else if(is_hover){
        new_mouse_event.button = NOCTERM_MOUSE_BUTTON_MOVE;
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
    static nocterm_key_t previous_mouse_key = {0};

    if(key == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(key->buffer_length != 6){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    nocterm_mouse_event_t current_mouse_event = nocterm_mouse_translate(key);
    
    if(current_mouse_event.row >= nocterm_screen_height || current_mouse_event.col >= nocterm_screen_width){
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

    if(nocterm_page_stack_size == 0){
        return NOCTERM_FAILURE;
    }

    nocterm_page_t* current_page = nocterm_page_stack[nocterm_page_stack_size-1];
    if(current_page == NULL){
        return NOCTERM_FAILURE;
    }

    nocterm_key_t crafted_key = {0};

    // Single shot events

    if((current_mouse_event.button == NOCTERM_MOUSE_BUTTON_SCROLL_DOWN || current_mouse_event.button == NOCTERM_MOUSE_BUTTON_SCROLL_UP) && mouse_progress_state == 0){
        
        if(current_page->focused_widget != current_mouse_widget->owner){
            return NOCTERM_SUCCESS;
        }

        if(current_mouse_event.button == NOCTERM_MOUSE_BUTTON_SCROLL_DOWN){
            memcpy(crafted_key.buffer, "\e\x5B\x42", 3);
            crafted_key.buffer_length = 3;
            crafted_key.type = NOCTERM_KEY_TYPE_ESCSEQ;
        }else{
            memcpy(crafted_key.buffer, "\e\x5B\x41", 3);
            crafted_key.buffer_length = 3;
            crafted_key.type = NOCTERM_KEY_TYPE_ESCSEQ;
        }

        if(current_mouse_widget->owner->key_handler){
            current_mouse_widget->owner->key_handler(current_mouse_widget->owner, &crafted_key);
        }

        if(current_page->focused_widget && current_page->focused_widget->owner->focusable){
            current_page->focused_widget = current_mouse_widget->owner;
        }

        return NOCTERM_SUCCESS;
    
    }else if(current_mouse_event.button == NOCTERM_MOUSE_BUTTON_MOVE && mouse_progress_state == 0){

        nocterm_widget_t* hovered = current_mouse_widget->owner;

        // Re-highlight from the hovered cell alone: when the widget under the
        // cursor differs from the currently highlighted one, leave the old and
        // enter the new.  The ENTER must NOT be nested inside the LEAVE check:
        // moving off a focusable widget onto a non-focusable region clears the
        // highlight (focused_widget = NULL), and the cursor returning to that
        // same widget has to be able to re-enter it.  Previously the ENTER was
        // gated on focused_widget still pointing at the (already-left) widget,
        // so returning to it never re-highlighted it.
        if(current_page->focused_widget != hovered){

            if(current_page->focused_widget && current_page->focused_widget->focus_handler){
                current_page->focused_widget->focus_handler(current_page->focused_widget, NOCTERM_WIDGET_FOCUS_LEAVE);
            }

            if(hovered && hovered->focusable){
                if(hovered->focus_handler){
                    hovered->focus_handler(hovered, NOCTERM_WIDGET_FOCUS_ENTER);
                }
                current_page->focused_widget = hovered;
            }else{
                // Over a non-focusable region: nothing is highlighted now, so a
                // later hover back onto a focusable widget can enter it again.
                current_page->focused_widget = NULL;
            }
        }
        return NOCTERM_SUCCESS;
    }


    // Multi shot event
    if(mouse_progress_state == 0){

        switch(current_mouse_event.button){
            case NOCTERM_MOUSE_BUTTON_LMB:
            case NOCTERM_MOUSE_BUTTON_RMB:{
                previous_mouse_widget = current_mouse_widget;
                previous_mouse_event = current_mouse_event;
                previous_mouse_key = *key;
                mouse_progress_state = 1;
            }break;

            default:
                break;
        }

    }else{

        if(previous_mouse_widget->owner == current_mouse_widget->owner){
            // We cliked on the same thing

            // Let's see whether there is a motion
            if(previous_mouse_event.row == current_mouse_event.row && previous_mouse_event.col == current_mouse_event.col && current_mouse_event.button == NOCTERM_MOUSE_BUTTON_RELEASE && previous_mouse_event.button != NOCTERM_MOUSE_BUTTON_MOVE){
                // Simple click

                // FOCUS_ENTER & FOCUS_LEAVE
                if(current_page->focused_widget != current_mouse_widget->owner){
                    if(current_page->focused_widget && current_page->focused_widget->focus_handler){
                        current_page->focused_widget->focus_handler(current_page->focused_widget, NOCTERM_WIDGET_FOCUS_LEAVE);
                    }
            
                    if(current_mouse_widget->owner->focusable && current_mouse_widget->owner->focus_handler){
                        current_mouse_widget->owner->focus_handler(current_mouse_widget->owner, NOCTERM_WIDGET_FOCUS_ENTER);
                    }

                }

                switch(previous_mouse_event.button){
                    case NOCTERM_MOUSE_BUTTON_LMB:
                    case NOCTERM_MOUSE_BUTTON_RMB:
                    case NOCTERM_MOUSE_BUTTON_MMB:{
                        if(current_page->focused_widget == current_mouse_widget->owner || previous_mouse_widget->click_activation){
                            
                            if(current_mouse_widget->owner->key_handler){
                                // Deliver the original press key (LMB/RMB/MMB) rather than
                                // the release event, so widgets can translate the button.
                                current_mouse_widget->owner->key_handler(current_mouse_widget->owner, &previous_mouse_key);
                            }

                        }
                    }break;

                    default:break;
                }


                if(current_mouse_widget->owner->focusable){
                    current_page->focused_widget = current_mouse_widget->owner;
                }else{
                    current_page->focused_widget = NULL;
                }

                mouse_progress_state = 0;
                previous_mouse_event = (nocterm_mouse_event_t){0};
                previous_mouse_widget = NULL;
            }else if(current_mouse_event.button == NOCTERM_MOUSE_BUTTON_MOVE){
                // There is motion
                // We have to check the direction of the 2 dimensional motion

                // If the widget that the drag operation is performed on is not already focused,
                // discard the operation

                if(current_page->focused_widget->owner == current_mouse_widget->owner){

                    if(previous_mouse_event.row != current_mouse_event.row){
                        // There is vertical movement
                        if(current_mouse_event.row < previous_mouse_event.row){
                            // Downwards motion
                            memcpy(crafted_key.buffer, "\e\x5B\x42", 3);
                            crafted_key.buffer_length = 3;
                            crafted_key.type = NOCTERM_KEY_TYPE_ESCSEQ;                              
                        }else{
                            // Upwards motion
                            memcpy(crafted_key.buffer, "\e\x5B\x41", 3);
                            crafted_key.buffer_length = 3;
                            crafted_key.type = NOCTERM_KEY_TYPE_ESCSEQ;  
                        }
                        
                        if(current_mouse_widget->owner->key_handler){
                            current_mouse_widget->owner->key_handler(current_mouse_widget->owner, &crafted_key);
                        }
                    }

                    if(previous_mouse_event.col != current_mouse_event.col){
                        // There is horizontal movement
                        if(current_mouse_event.col < previous_mouse_event.col){
                            // Rightwards motion
                            memcpy(crafted_key.buffer, "\e\x5B\x43", 3);
                            crafted_key.buffer_length = 3;
                            crafted_key.type = NOCTERM_KEY_TYPE_ESCSEQ;                              
                        }else{
                            // Leftwards motion
                            memcpy(crafted_key.buffer, "\e\x5B\x44", 3);
                            crafted_key.buffer_length = 3;
                            crafted_key.type = NOCTERM_KEY_TYPE_ESCSEQ;  
                        }

                        if(current_mouse_widget->owner->key_handler){
                            current_mouse_widget->owner->key_handler(current_mouse_widget->owner, &crafted_key);
                        }
                    }

                }

                previous_mouse_event = current_mouse_event;

                // Discard operation
            }else if(current_mouse_event.button == NOCTERM_MOUSE_BUTTON_RELEASE){
                mouse_progress_state = 0; 
                previous_mouse_event = (nocterm_mouse_event_t){0};
                previous_mouse_widget = NULL;                  
            }

        }else{
            // Operation discarded
            mouse_progress_state = 0;
            previous_mouse_event = (nocterm_mouse_event_t){0};
            previous_mouse_widget = NULL;            
        }
        
    }

    return NOCTERM_SUCCESS;
}

nocterm_mouse_support_t nocterm_mouse_get_support(void){
    return nocterm_mouse_support_flag;
}
