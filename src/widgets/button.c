#include <nocterm/widgets/button.h>

nocterm_button_t* nocterm_button_new(nocterm_dimension_size_t row, nocterm_dimension_size_t col, const char* text, uint64_t text_size, nocterm_attribute_t normal, nocterm_attribute_t focused, nocterm_button_onpress_handler_t onpress_handler, void* user_data){

    if(text == NULL || text_size == 0){
        return NULL;
    }

    nocterm_button_t* new_button = (nocterm_button_t*)malloc(sizeof(nocterm_button_t));

    if(new_button == NULL){
        return NULL;
    }

    memset(new_button, 0x0, sizeof(nocterm_button_t));

    nocterm_char_t button_string[text_size];
    memset(button_string, 0x0, sizeof(nocterm_char_t) * text_size);
    uint64_t button_string_length = nocterm_char_string_from_stream(button_string, text_size, text, text_size);

    if(button_string_length == 0){
        return NULL;
    }
    
    if(nocterm_widget_constructor(NOCTERM_WIDGET(new_button), (nocterm_dimension_t){row, col, 1, button_string_length}, true, false) == NOCTERM_FAILURE){
        return NULL;
    }

    nocterm_widget_add_key_handler(NOCTERM_WIDGET(new_button), nocterm_button_key_handler);

    nocterm_widget_add_focus_handler(NOCTERM_WIDGET(new_button), nocterm_button_focus_handler);

    for(uint64_t i = 0; i < new_button->widget.buffer_size; i++){
        nocterm_widget_update(NOCTERM_WIDGET(new_button), 0, i, button_string[i], normal);
    }

    new_button->onpress_handler = onpress_handler;    
    new_button->attribute_normal = normal;
    new_button->attribute_focused = focused;
    new_button->user_data = user_data;
    
    return new_button;
}

int nocterm_button_delete(nocterm_button_t* button){

    if(nocterm_widget_destructor(NOCTERM_WIDGET(button)) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    free(button);

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
