#include "nocterm/base/screen.h"
#include <nocterm/widgets/checkbox.h>
#include <nocterm/base/mouse.h>

int nocterm_widget_flex_policy_set_permission(nocterm_widget_t* widget, nocterm_widget_flex_policy_permission_t permission);


NOCTERM_INTERNAL NOCTERM_WIDGET_KEY_HANDLER(nocterm_checkbox_key_handler);

NOCTERM_INTERNAL NOCTERM_WIDGET_FOCUS_HANDLER(nocterm_checkbox_focus_handler);

nocterm_checkbox_t* nocterm_checkbox_new(nocterm_checkbox_oncheck_handler_t oncheck_handler, bool checked, void* user_data){

    nocterm_checkbox_t* new_checkbox = (nocterm_checkbox_t*)malloc(sizeof(nocterm_checkbox_t));

    if(new_checkbox == NULL){
        return NULL;
    }

    memset(new_checkbox, 0x0, sizeof(nocterm_checkbox_t));

    if(nocterm_checkbox_constructor(new_checkbox, oncheck_handler, checked, user_data) == NOCTERM_FAILURE){
        free(new_checkbox);
        return NULL;
    }

    return new_checkbox;
}

int nocterm_checkbox_constructor(nocterm_checkbox_t* checkbox, nocterm_checkbox_oncheck_handler_t oncheck_handler, bool checked, void* user_data){

    if(checkbox == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(nocterm_widget_constructor(NOCTERM_WIDGET(checkbox), 1, 3, NOCTERM_WIDGET_FOCUSABLE_YES, NOCTERM_WIDGET_TYPE_REAL) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }
    nocterm_widget_flex_policy_set_permission(NOCTERM_WIDGET(checkbox), NOCTERM_WIDGET_FLEX_POLICY_PERMISSION_NONE);

    nocterm_widget_set_click_activation(NOCTERM_WIDGET(checkbox), true);

    checkbox->checked = checked;

    checkbox->oncheck_handler = oncheck_handler;

    checkbox->user_data = user_data;

    nocterm_char_t left_side = {
        .bytes = {'['},
        .bytes_size = 1,
        .is_utf8 = false
    };
    nocterm_char_t right_side = {
        .bytes = {']'},
        .bytes_size = 1,
        .is_utf8 = false
    };    

    nocterm_char_t check_character = {
        .bytes = {'x'},
        .bytes_size = 1,
        .is_utf8 = false
    }; 

    checkbox->main_attribute = NOCTERM_ATTRIBUTE_EMPTY;
    checkbox->cursor_attribute = checkbox->main_attribute;
    checkbox->cursor_attribute.inverse = true;
    checkbox->check_marker = check_character;

    nocterm_widget_update(NOCTERM_WIDGET(checkbox), 0, 0, left_side, checkbox->main_attribute);
    nocterm_widget_update(NOCTERM_WIDGET(checkbox), 0, 2, right_side, checkbox->main_attribute);

    if(checked){
        nocterm_widget_update(NOCTERM_WIDGET(checkbox), 0, 1, checkbox->check_marker, checkbox->main_attribute);
    }else{
        nocterm_widget_update(NOCTERM_WIDGET(checkbox), 0, 1, nocterm_char_from_ascii(' '), checkbox->main_attribute);
    }

    nocterm_widget_set_key_handler(NOCTERM_WIDGET(checkbox), nocterm_checkbox_key_handler);

    nocterm_widget_set_focus_handler(NOCTERM_WIDGET(checkbox), nocterm_checkbox_focus_handler);

    return NOCTERM_SUCCESS;
}

int nocterm_checkbox_destructor(nocterm_checkbox_t* checkbox){

    if(nocterm_widget_destructor(NOCTERM_WIDGET(checkbox)) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    return NOCTERM_SUCCESS;
}

int nocterm_checkbox_delete(nocterm_checkbox_t* checkbox){

    if(nocterm_checkbox_destructor(checkbox) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    free(checkbox);

    return NOCTERM_SUCCESS;
}

int nocterm_checkbox_set_attribute(nocterm_checkbox_t* checkbox, nocterm_attribute_t attribute){
    
    if(checkbox == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&NOCTERM_WIDGET(checkbox)->lock);

    checkbox->main_attribute = attribute;
    checkbox->cursor_attribute = attribute;
    checkbox->cursor_attribute.inverse = attribute.inverse ? false : true;

    if(nocterm_widget_is_focused(NOCTERM_WIDGET(checkbox))){

        nocterm_widget_update(NOCTERM_WIDGET(checkbox), 0, 0, NOCTERM_WIDGET(checkbox)->buffer[0].character, checkbox->main_attribute);
        nocterm_widget_update(NOCTERM_WIDGET(checkbox), 0, 1, NOCTERM_WIDGET(checkbox)->buffer[1].character, checkbox->cursor_attribute);
        nocterm_widget_update(NOCTERM_WIDGET(checkbox), 0, 2, NOCTERM_WIDGET(checkbox)->buffer[2].character, checkbox->main_attribute);

    }else{
        for(nocterm_dimension_size_t i = 0; i < 3; i++){
            nocterm_widget_update(NOCTERM_WIDGET(checkbox), 0,i, NOCTERM_WIDGET(checkbox)->buffer[i].character, checkbox->main_attribute);
        }
    }

    pthread_mutex_unlock(&NOCTERM_WIDGET(checkbox)->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_checkbox_set_marker(nocterm_checkbox_t* checkbox, nocterm_char_t marker){
    if(checkbox == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&NOCTERM_WIDGET(checkbox)->lock);

    checkbox->check_marker = marker;

    if(nocterm_widget_is_focused(NOCTERM_WIDGET(checkbox))){
        if(checkbox->checked){
            nocterm_widget_update(NOCTERM_WIDGET(checkbox), 0,1, marker, checkbox->cursor_attribute);
        }
    }else{
        if(checkbox->checked){
            nocterm_widget_update(NOCTERM_WIDGET(checkbox), 0,1, marker, checkbox->main_attribute);
        }
    }

    pthread_mutex_unlock(&NOCTERM_WIDGET(checkbox)->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_checkbox_set_sides(nocterm_checkbox_t* checkbox, nocterm_char_t left, nocterm_char_t right){
    if(checkbox == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&NOCTERM_WIDGET(checkbox)->lock);

    checkbox->left_side = left;
    checkbox->right_side = right;

    nocterm_widget_update(NOCTERM_WIDGET(checkbox), 0, 0, left, checkbox->main_attribute);
    nocterm_widget_update(NOCTERM_WIDGET(checkbox), 0, 2, right, checkbox->main_attribute);

    pthread_mutex_unlock(&NOCTERM_WIDGET(checkbox)->lock);

    return NOCTERM_SUCCESS;
}

NOCTERM_WIDGET_KEY_HANDLER(nocterm_checkbox_key_handler){

    switch(nocterm_key_translate(key)){

        // A click is delivered as a raw mouse event now that the mouse
        // controller no longer synthesizes an ENTER key on activation.
        case NOCTERM_KEY_EVENT_MOUSE:
            nocterm_io_print_at(nocterm_screen_height-1, 0, "%d", nocterm_mouse_translate(key).button);
            if(nocterm_mouse_translate(key).button != NOCTERM_MOUSE_BUTTON_LMB){
                break;
            }
        case NOCTERM_KEY_EVENT_ENTER:{


            if(NOCTERM_CHECKBOX(self)->checked){
                nocterm_widget_update(self, 0,1, nocterm_char_from_ascii(' '), NOCTERM_CHECKBOX(self)->cursor_attribute);
                NOCTERM_CHECKBOX(self)->checked = false;
                if(NOCTERM_CHECKBOX(self)->oncheck_handler){
                    NOCTERM_CHECKBOX(self)->oncheck_handler(self, NOCTERM_CHECKBOX_ACTION_UNCHECK, NOCTERM_CHECKBOX(self)->user_data);
                }
            }else{
                NOCTERM_CHECKBOX(self)->checked = true;
                nocterm_widget_update(self, 0,1, NOCTERM_CHECKBOX(self)->check_marker, NOCTERM_CHECKBOX(self)->cursor_attribute);
                if(NOCTERM_CHECKBOX(self)->oncheck_handler){
                    NOCTERM_CHECKBOX(self)->oncheck_handler(self, NOCTERM_CHECKBOX_ACTION_CHECK, NOCTERM_CHECKBOX(self)->user_data);
                }
            }

        }break;

        case NOCTERM_KEY_EVENT_PRINTABLE:{
            // Check space or x character

            if(key->buffer_length == 1 && (tolower((int)key->buffer[0]) == 'x' || key->buffer[0] == ' ')){
                // This is a valid key press for checking or unchecking
               
                if(NOCTERM_CHECKBOX(self)->checked){
                    nocterm_widget_update(self, 0,1, nocterm_char_from_ascii(' '), NOCTERM_CHECKBOX(self)->cursor_attribute);
                    NOCTERM_CHECKBOX(self)->checked = false;
                    if(NOCTERM_CHECKBOX(self)->oncheck_handler){
                        NOCTERM_CHECKBOX(self)->oncheck_handler(self, NOCTERM_CHECKBOX_ACTION_UNCHECK, NOCTERM_CHECKBOX(self)->user_data);
                    }
                }else{
                    NOCTERM_CHECKBOX(self)->checked = true;
                    nocterm_widget_update(self, 0,1, NOCTERM_CHECKBOX(self)->check_marker, NOCTERM_CHECKBOX(self)->cursor_attribute);
                    if(NOCTERM_CHECKBOX(self)->oncheck_handler){
                        NOCTERM_CHECKBOX(self)->oncheck_handler(self, NOCTERM_CHECKBOX_ACTION_CHECK, NOCTERM_CHECKBOX(self)->user_data);
                    }
                }
            }

        }break;
        default:
            break;
    }
}

NOCTERM_WIDGET_FOCUS_HANDLER(nocterm_checkbox_focus_handler){

    // The buffer can be NULL / shrunk below the cursor cell when a flex
    // collapses the checkbox width; don't dereference it on a focus transition.
    if(self->buffer == NULL || self->buffer_size <= 1){
        return;
    }

    switch(focus){
        case NOCTERM_WIDGET_FOCUS_ENTER:{
            nocterm_widget_update(self, 0,1, self->buffer[1].character, NOCTERM_CHECKBOX(self)->cursor_attribute);
        }break;
        case NOCTERM_WIDGET_FOCUS_LEAVE:{
            nocterm_widget_update(self, 0,1, self->buffer[1].character, NOCTERM_CHECKBOX(self)->main_attribute);
        }break;
    }

}
