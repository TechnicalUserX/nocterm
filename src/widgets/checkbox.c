#include <nocterm/widgets/checkbox.h>

nocterm_checkbox_t* nocterm_checkbox_new(nocterm_dimension_size_t row, nocterm_dimension_size_t col, nocterm_attribute_t attribute, nocterm_checkbox_oncheck_handler_t oncheck_handler, bool checked, void* user_data){

    nocterm_checkbox_t* new_checkbox = (nocterm_checkbox_t*)malloc(sizeof(nocterm_checkbox_t));

    if(new_checkbox == NULL){
        return NULL;
    }

    memset(new_checkbox, 0x0, sizeof(nocterm_checkbox_t));

    if(nocterm_checkbox_constructor(new_checkbox, row, col, attribute, oncheck_handler, checked, user_data) == NOCTERM_FAILURE){
        free(new_checkbox);
        return NULL;
    }

    return new_checkbox;
}

int nocterm_checkbox_constructor(nocterm_checkbox_t* checkbox, nocterm_dimension_size_t row, nocterm_dimension_size_t col, nocterm_attribute_t attribute, nocterm_checkbox_oncheck_handler_t oncheck_handler, bool checked, void* user_data){

    if(checkbox == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(nocterm_widget_constructor(NOCTERM_WIDGET(checkbox), (nocterm_dimension_t){row, col, 1, 3}, true, false) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    checkbox->attribute_normal = attribute;
    checkbox->attribute_cursor = attribute;

    checkbox->attribute_cursor.color.ansi.bg = checkbox->attribute_normal.color.ansi.fg;
    checkbox->attribute_cursor.color.ansi.fg = checkbox->attribute_normal.color.ansi.bg;
    checkbox->attribute_cursor.color.ansi.codes.fg = checkbox->attribute_normal.color.ansi.codes.bg;
    checkbox->attribute_cursor.color.ansi.codes.bg = checkbox->attribute_normal.color.ansi.codes.fg;

    checkbox->attribute_cursor.color.c256.bg = checkbox->attribute_normal.color.c256.fg;
    checkbox->attribute_cursor.color.c256.fg = checkbox->attribute_normal.color.c256.bg;  
    checkbox->attribute_cursor.color.c256.codes.fg = checkbox->attribute_normal.color.c256.codes.bg;
    checkbox->attribute_cursor.color.c256.codes.bg = checkbox->attribute_normal.color.c256.codes.fg;

    checkbox->attribute_cursor.color.rgb.bg = checkbox->attribute_normal.color.rgb.fg;
    checkbox->attribute_cursor.color.rgb.fg = checkbox->attribute_normal.color.rgb.bg;
    checkbox->attribute_cursor.color.rgb.codes.fg = checkbox->attribute_normal.color.rgb.codes.bg;
    checkbox->attribute_cursor.color.rgb.codes.bg = checkbox->attribute_normal.color.rgb.codes.fg;

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

    nocterm_widget_update(NOCTERM_WIDGET(checkbox), 0, 0, left_side, attribute);
    nocterm_widget_update(NOCTERM_WIDGET(checkbox), 0, 2, right_side, attribute);

    if(checked){
        nocterm_char_t check_character = {
            .bytes = {'x'},
            .bytes_size = 1,
            .is_utf8 = false
        }; 
        nocterm_widget_update(NOCTERM_WIDGET(checkbox), 0, 1, check_character, attribute);
    }else{
        nocterm_char_t space_character = {
            .bytes = {' '},
            .bytes_size = 1,
            .is_utf8 = false
        };
        nocterm_widget_update(NOCTERM_WIDGET(checkbox), 0, 1, space_character, attribute);

    }

    nocterm_widget_add_key_handler(NOCTERM_WIDGET(checkbox), nocterm_checkbox_key_handler);

    nocterm_widget_add_focus_handler(NOCTERM_WIDGET(checkbox), nocterm_checkbox_focus_handler);

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

NOCTERM_WIDGET_KEY_HANDLER(nocterm_checkbox_key_handler){

    switch(nocterm_key_translate(key)){

        case NOCTERM_KEY_EVENT_ENTER:{
            nocterm_char_t check_character = {
                .bytes = {'x'},
                .bytes_size = 1,
                .is_utf8 = false
            };     
            nocterm_char_t space_character = {
                .bytes = {' '},
                .bytes_size = 1,
                .is_utf8 = false
            };             
            if(NOCTERM_CHECKBOX(self)->checked){
                nocterm_widget_update(self, 0,1, space_character, NOCTERM_CHECKBOX(self)->attribute_cursor);
                NOCTERM_CHECKBOX(self)->checked = false;
                if(NOCTERM_CHECKBOX(self)->oncheck_handler){
                    NOCTERM_CHECKBOX(self)->oncheck_handler(self, NOCTERM_CHECKBOX_ACTION_UNCHECK, NOCTERM_CHECKBOX(self)->user_data);
                }
            }else{
                NOCTERM_CHECKBOX(self)->checked = true;
                nocterm_widget_update(self, 0,1, check_character, NOCTERM_CHECKBOX(self)->attribute_cursor);
                if(NOCTERM_CHECKBOX(self)->oncheck_handler){
                    NOCTERM_CHECKBOX(self)->oncheck_handler(self, NOCTERM_CHECKBOX_ACTION_CHECK, NOCTERM_CHECKBOX(self)->user_data);
                }
            }

        }break;

        case NOCTERM_KEY_EVENT_PRINTABLE:{
            // Check space or x character

            if(key->buffer_length == 1 && (tolower((int)key->buffer[0]) == 'x' || key->buffer[0] == ' ')){
                // This is a valid key press for checking or unchecking
                nocterm_char_t check_character = {
                    .bytes = {'x'},
                    .bytes_size = 1,
                    .is_utf8 = false
                };     
                nocterm_char_t space_character = {
                    .bytes = {' '},
                    .bytes_size = 1,
                    .is_utf8 = false
                };                 
                if(NOCTERM_CHECKBOX(self)->checked){
                    nocterm_widget_update(self, 0,1, space_character, NOCTERM_CHECKBOX(self)->attribute_cursor);
                    NOCTERM_CHECKBOX(self)->checked = false;
                    if(NOCTERM_CHECKBOX(self)->oncheck_handler){
                        NOCTERM_CHECKBOX(self)->oncheck_handler(self, NOCTERM_CHECKBOX_ACTION_UNCHECK, NOCTERM_CHECKBOX(self)->user_data);
                    }
                }else{
                    NOCTERM_CHECKBOX(self)->checked = true;
                    nocterm_widget_update(self, 0,1, check_character, NOCTERM_CHECKBOX(self)->attribute_cursor);
                    if(NOCTERM_CHECKBOX(self)->oncheck_handler){
                        NOCTERM_CHECKBOX(self)->oncheck_handler(self, NOCTERM_CHECKBOX_ACTION_CHECK, NOCTERM_CHECKBOX(self)->user_data);
                    }
                }
            }

        }break;
        default:
    }
}

NOCTERM_WIDGET_FOCUS_HANDLER(nocterm_checkbox_focus_handler){

    switch(focus){
        case NOCTERM_WIDGET_FOCUS_ENTER:{
            nocterm_widget_update(self, 0,1, self->buffer[1].character, NOCTERM_CHECKBOX(self)->attribute_cursor);
        }break;
        case NOCTERM_WIDGET_FOCUS_LEAVE:{
            nocterm_widget_update(self, 0,1, self->buffer[1].character, NOCTERM_CHECKBOX(self)->attribute_normal);
        }break;
    }

}
