#include <nocterm/widgets/button.h>
#include <nocterm/base/mouse.h>

int nocterm_widget_flex_policy_set_permission(nocterm_widget_t* widget, nocterm_widget_flex_policy_permission_t permission);

NOCTERM_INTERNAL NOCTERM_WIDGET_KEY_HANDLER(nocterm_button_key_handler);

NOCTERM_INTERNAL NOCTERM_WIDGET_FOCUS_HANDLER(nocterm_button_focus_handler);

NOCTERM_INTERNAL NOCTERM_WIDGET_RESIZE_HANDLER(nocterm_button_internal_resize_handler);

nocterm_button_t* nocterm_button_new(nocterm_dimension_size_t height, nocterm_dimension_size_t width, nocterm_button_onpress_handler_t onpress_handler, void* user_data){

    nocterm_button_t* new_button = (nocterm_button_t*)malloc(sizeof(nocterm_button_t));

    if(new_button == NULL){
        return NULL;
    }

    memset(new_button, 0x0, sizeof(nocterm_button_t));

    if(nocterm_button_constructor(new_button, height, width, onpress_handler, user_data) == NOCTERM_FAILURE){
        free(new_button);
        return NULL;
    }

    return new_button;
}

int nocterm_button_constructor(nocterm_button_t* button, nocterm_dimension_size_t height, nocterm_dimension_size_t width, nocterm_button_onpress_handler_t onpress_handler, void* user_data){

    if(button == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(height * width == 0){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(nocterm_widget_constructor(NOCTERM_WIDGET(button), height, width, NOCTERM_WIDGET_FOCUSABLE_YES, NOCTERM_WIDGET_TYPE_REAL) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    nocterm_widget_flex_policy_set_permission(NOCTERM_WIDGET(button), NOCTERM_WIDGET_FLEX_POLICY_PERMISSION_BOTH);

    nocterm_widget_set_click_activation(NOCTERM_WIDGET(button), true);

    for(nocterm_dimension_size_t i = 0; i < NOCTERM_WIDGET(button)->buffer_size; i++){
        nocterm_widget_update(NOCTERM_WIDGET(button), 0, i, nocterm_char_from_ascii(' '), NOCTERM_ATTRIBUTE_EMPTY);
    }

    nocterm_widget_set_key_handler(NOCTERM_WIDGET(button), nocterm_button_key_handler);

    nocterm_widget_set_focus_handler(NOCTERM_WIDGET(button), nocterm_button_focus_handler);

    NOCTERM_WIDGET(button)->internal_resize_handler = nocterm_button_internal_resize_handler;

    button->onpress_handler = onpress_handler;
    button->attribute_normal = NOCTERM_ATTRIBUTE_EMPTY;
    button->attribute_focused = NOCTERM_ATTRIBUTE_EMPTY;
    button->attribute_focused.inverse = true;
    button->text_length = 0;

    button->user_data = user_data;

    return NOCTERM_SUCCESS;
}

int nocterm_button_destructor(nocterm_button_t* button){

    if(nocterm_widget_destructor(NOCTERM_WIDGET(button)) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    return NOCTERM_SUCCESS;
}

int nocterm_button_delete(nocterm_button_t* button){

    if(nocterm_button_destructor(button) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    free(button);

    return NOCTERM_SUCCESS;
}

int nocterm_button_set_attribute(nocterm_button_t* button, nocterm_attribute_t normal, nocterm_attribute_t focused){

    if(button == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&NOCTERM_WIDGET(button)->lock);

    button->attribute_normal = normal;
    button->attribute_focused = focused;

    if(nocterm_widget_is_focused(NOCTERM_WIDGET(button))){
        for(nocterm_dimension_size_t i = 0; i < NOCTERM_WIDGET(button)->buffer_size; i++){
            nocterm_widget_update(NOCTERM_WIDGET(button), 0, i, NOCTERM_WIDGET(button)->buffer[i].character, focused);
        }
    }else{
        for(nocterm_dimension_size_t i = 0; i < NOCTERM_WIDGET(button)->buffer_size; i++){
            nocterm_widget_update(NOCTERM_WIDGET(button), 0, i, NOCTERM_WIDGET(button)->buffer[i].character, normal);
        }
    }

    pthread_mutex_unlock(&NOCTERM_WIDGET(button)->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_button_set_text(nocterm_button_t* button, const char* text, uint64_t text_size){
    
    if(button == NULL || text == NULL || text_size == 0){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&NOCTERM_WIDGET(button)->lock);

    nocterm_widget_t* widget = NOCTERM_WIDGET(button);

    // Clear the button with spaces
    for(uint64_t row = 0; row < widget->bounds.height; row++){
        for(uint64_t col = 0; col < widget->bounds.width; col++){
            nocterm_widget_update(widget, row, col, nocterm_char_from_ascii(' '), button->attribute_normal);
        }
    }


    // Center text horizontally and vertically
    uint64_t display_len = ((text_size - 1) < widget->bounds.width) ? (text_size - 1): widget->bounds.width;
    uint64_t start_col = (widget->bounds.width - display_len) / 2;
    uint64_t start_row = widget->bounds.height / 2;

    nocterm_char_t text_buffer[display_len+1];
    uint64_t parsed_len = nocterm_char_string_from_stream(text_buffer, display_len+1, text, text_size);

    nocterm_attribute_t attr = nocterm_widget_is_focused(widget) ? button->attribute_focused : button->attribute_normal;

    for(uint64_t i = 0; i < parsed_len; i++){
        nocterm_widget_update(widget, start_row, start_col + i, text_buffer[i], attr);
    }

    // Store the full text so the internal resize handler can redraw it
    uint64_t store_len = text_size > 0 ? text_size - 1 : 0;
    if(store_len > NOCTERM_BUTTON_TEXT_MAX_SIZE) store_len = NOCTERM_BUTTON_TEXT_MAX_SIZE;
    nocterm_char_t full_text_buffer[store_len + 1];
    uint64_t full_parsed_len = nocterm_char_string_from_stream(full_text_buffer, store_len + 1, text, text_size);
    if(full_parsed_len > NOCTERM_BUTTON_TEXT_MAX_SIZE) full_parsed_len = NOCTERM_BUTTON_TEXT_MAX_SIZE;
    memcpy(button->text, full_text_buffer, full_parsed_len * sizeof(nocterm_char_t));
    button->text_length = full_parsed_len;

    pthread_mutex_unlock(&NOCTERM_WIDGET(button)->lock);

    return NOCTERM_SUCCESS;
}

NOCTERM_WIDGET_KEY_HANDLER(nocterm_button_key_handler){
    switch(nocterm_key_translate(key)){
        // A click is delivered as a raw mouse event now that the mouse
        // controller no longer synthesizes an ENTER key on activation.
        case NOCTERM_KEY_EVENT_MOUSE:
            if(nocterm_mouse_translate(key).button != NOCTERM_MOUSE_BUTTON_LMB){
                break;
            }
        case NOCTERM_KEY_EVENT_ENTER:{
            if(NOCTERM_BUTTON(self)->onpress_handler){
                NOCTERM_BUTTON(self)->onpress_handler(self, NOCTERM_BUTTON(self)->user_data);
            }
        }break;

        default:
            break;
    }
}

NOCTERM_WIDGET_FOCUS_HANDLER(nocterm_button_focus_handler){
    switch(focus){

        case NOCTERM_WIDGET_FOCUS_ENTER:{
            for(uint64_t i = 0; i < self->buffer_size; i++){
                nocterm_widget_update(self, 0, i, self->buffer[i].character, NOCTERM_BUTTON(self)->attribute_focused);
            }
        }break;

        case NOCTERM_WIDGET_FOCUS_LEAVE:{
            for(uint64_t i = 0; i < self->buffer_size; i++){
                nocterm_widget_update(self, 0, i, self->buffer[i].character, NOCTERM_BUTTON(self)->attribute_normal);
            }
        }break;
    }
}

NOCTERM_WIDGET_RESIZE_HANDLER(nocterm_button_internal_resize_handler){
    nocterm_button_t* button = NOCTERM_BUTTON(self);
    nocterm_attribute_t attr = nocterm_widget_is_focused(self)
        ? button->attribute_focused
        : button->attribute_normal;

    // Refill every cell with spaces at the current attribute
    for(nocterm_dimension_size_t row = 0; row < self->bounds.height; row++){
        for(nocterm_dimension_size_t col = 0; col < self->bounds.width; col++){
            nocterm_widget_update(self, row, col, nocterm_char_from_ascii(' '), attr);
        }
    }

    // Re-center the stored text
    if(button->text_length > 0){
        uint64_t display_len = (button->text_length < self->bounds.width)
            ? button->text_length
            : self->bounds.width;
        uint64_t start_col = (self->bounds.width - display_len) / 2;
        uint64_t start_row = self->bounds.height / 2;
        for(uint64_t i = 0; i < display_len; i++){
            nocterm_widget_update(self, start_row, start_col + i, button->text[i], attr);
        }
    }
}
