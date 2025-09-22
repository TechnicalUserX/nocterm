#include <nocterm/widgets/button.h>

NOCTERM_INTERNAL NOCTERM_WIDGET_KEY_HANDLER(nocterm_button_key_handler);

NOCTERM_INTERNAL NOCTERM_WIDGET_FOCUS_HANDLER(nocterm_button_focus_handler);

nocterm_button_t* nocterm_button_new(const char* text, uint64_t text_size, nocterm_button_onpress_handler_t onpress_handler, void* user_data){

    if(text == NULL || text_size == 0){
        return NULL;
    }

    nocterm_button_t* new_button = (nocterm_button_t*)malloc(sizeof(nocterm_button_t));

    if(new_button == NULL){
        return NULL;
    }

    memset(new_button, 0x0, sizeof(nocterm_button_t));

    if(nocterm_button_constructor(new_button, text, text_size, onpress_handler, user_data) == NOCTERM_FAILURE){
        free(new_button);
        return NULL;
    }

    return new_button;
}

int nocterm_button_constructor(nocterm_button_t* button, const char* text, uint64_t text_size, nocterm_button_onpress_handler_t onpress_handler, void* user_data){

    if(button == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    nocterm_char_t button_string[text_size];
    memset(button_string, 0x0, sizeof(nocterm_char_t) * text_size);
    uint64_t button_string_length = nocterm_char_string_from_stream(button_string, text_size, text, text_size);

    if(button_string_length == 0){
        return NOCTERM_FAILURE;
    }
    
    if(nocterm_widget_constructor(NOCTERM_WIDGET(button), 1, button_string_length, true, false) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    nocterm_widget_add_key_handler(NOCTERM_WIDGET(button), nocterm_button_key_handler);

    nocterm_widget_add_focus_handler(NOCTERM_WIDGET(button), nocterm_button_focus_handler);

    for(uint64_t i = 0; i < button->widget.buffer_size; i++){
        nocterm_widget_update(NOCTERM_WIDGET(button), 0, i, button_string[i], NOCTERM_ATTRIBUTE_EMPTY);
    }

    button->onpress_handler = onpress_handler;    
    button->attribute_normal = NOCTERM_ATTRIBUTE_EMPTY;
    button->attribute_focused = NOCTERM_ATTRIBUTE_EMPTY;
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

NOCTERM_WIDGET_KEY_HANDLER(nocterm_button_key_handler){
    switch(nocterm_key_translate(key)){
        case NOCTERM_KEY_EVENT_ENTER:{
            if(NOCTERM_BUTTON(self)->onpress_handler){
                NOCTERM_BUTTON(self)->onpress_handler(self, NOCTERM_BUTTON(self)->user_data);
            }
        }break;
        default:
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
