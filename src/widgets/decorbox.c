#include <nocterm/widgets/decorbox.h>

int nocterm_widget_flex_policy_set_permission(nocterm_widget_t* widget, nocterm_widget_flex_policy_permission_t permission);

NOCTERM_INTERNAL int nocterm_decorbox_border_draw(nocterm_decorbox_t* decorbox, nocterm_decorbox_border_t border, nocterm_attribute_t attribute);

NOCTERM_INTERNAL NOCTERM_WIDGET_KEY_HANDLER(nocterm_decorbox_key_handler);

NOCTERM_INTERNAL NOCTERM_WIDGET_FOCUS_HANDLER(nocterm_decorbox_focus_handler);

NOCTERM_INTERNAL NOCTERM_WIDGET_RESIZE_HANDLER(nocterm_decorbox_internal_resize_handler);


nocterm_decorbox_t* nocterm_decorbox_new(nocterm_widget_t* contained_widget){

    if(contained_widget == NULL){
        errno = EINVAL;
        return NULL;
    }

    nocterm_decorbox_t* new_decorbox = (nocterm_decorbox_t*)malloc(sizeof(nocterm_decorbox_t));
    
    if(new_decorbox == NULL){
        errno = ENOMEM;
        return NULL;
    }

    memset(new_decorbox, 0x0, sizeof(nocterm_decorbox_t));

    if(nocterm_decorbox_constructor(new_decorbox, contained_widget) == NOCTERM_FAILURE){
        free(new_decorbox);
        return NULL;
    }

    return new_decorbox;
}

int nocterm_decorbox_constructor(nocterm_decorbox_t* decorbox, nocterm_widget_t* contained_widget){

    if(decorbox == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    nocterm_dimension_size_t decorbox_height = contained_widget->viewport.height + 2;
    nocterm_dimension_size_t decorbox_width = contained_widget->viewport.width + 2;

    if(nocterm_widget_constructor(NOCTERM_WIDGET(decorbox), decorbox_height, decorbox_width, NOCTERM_WIDGET_FOCUSABLE_YES, NOCTERM_WIDGET_TYPE_REAL) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    nocterm_widget_flex_policy_set_permission(NOCTERM_WIDGET(decorbox), NOCTERM_WIDGET_FLEX_POLICY_PERMISSION_BOTH);

    // One-cell border on every side: tell flex_update to use inner area for children
    NOCTERM_WIDGET(decorbox)->flex_policy_inner_padding_h = 1;
    NOCTERM_WIDGET(decorbox)->flex_policy_inner_padding_w = 1;

    NOCTERM_WIDGET(decorbox)->internal_resize_handler = nocterm_decorbox_internal_resize_handler;

    decorbox->contained_widget = contained_widget;
    decorbox->contained_widget->focusable = false; // Now the ownership of the focusability is switched to decorbox
    decorbox->contained_widget->bounds.row = 1; // Relative position just beneath the border 
    decorbox->contained_widget->bounds.col = 1; // Relative position just next to the border
    decorbox->contained_widget->owner = NOCTERM_WIDGET(decorbox);

    // click activation is inherited from the contained widget
    NOCTERM_WIDGET(decorbox)->click_activation = contained_widget->click_activation;

    nocterm_widget_add_subwidget(NOCTERM_WIDGET(decorbox), contained_widget);

    if(nocterm_widget_set_focus_handler(NOCTERM_WIDGET(decorbox), nocterm_decorbox_focus_handler) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    if(nocterm_widget_set_key_handler(NOCTERM_WIDGET(decorbox), nocterm_decorbox_key_handler) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    return NOCTERM_SUCCESS;
}

int nocterm_decorbox_destructor(nocterm_decorbox_t* decorbox){

    if(decorbox == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(nocterm_widget_destructor(NOCTERM_WIDGET(decorbox)) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    return NOCTERM_SUCCESS;
}

int nocterm_decorbox_delete(nocterm_decorbox_t* decorbox){

    if(decorbox == NULL){
        return NOCTERM_FAILURE;
    }

    if(nocterm_decorbox_destructor(decorbox) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    free(decorbox);

    return NOCTERM_SUCCESS;
}

int nocterm_decorbox_set_border(nocterm_decorbox_t* decorbox, nocterm_decorbox_border_t border, nocterm_attribute_t normal, nocterm_attribute_t focused){

    if(decorbox == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&NOCTERM_WIDGET(decorbox)->lock);

    decorbox->border_settings.border = border;
    decorbox->border_settings.enabled = true;
    decorbox->border_settings.attributes.normal = normal;
    decorbox->border_settings.attributes.focused = focused;

    nocterm_decorbox_border_draw(decorbox, border, normal);
    
    pthread_mutex_unlock(&NOCTERM_WIDGET(decorbox)->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_decorbox_set_label(nocterm_decorbox_t* decorbox, const char* label, uint64_t label_size, nocterm_attribute_t attribute, uint64_t left_offset){
    if(decorbox == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    nocterm_char_t label_string[NOCTERM_DECORBOX_LABEL_MAX_SIZE] = {0};
    uint64_t label_string_length = nocterm_char_string_from_stream(label_string,NOCTERM_DECORBOX_LABEL_MAX_SIZE, label, label_size);

    if(label_string_length == 0){
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&NOCTERM_WIDGET(decorbox)->lock);

    for(uint64_t i = 0; i < label_string_length; i++){
        decorbox->label.content[i].character = label_string[i];
        decorbox->label.content[i].attribute = attribute;
    }
    
    for(uint64_t i = 0; i < label_string_length; i++){
        nocterm_widget_update(NOCTERM_WIDGET(decorbox), 0, i + left_offset, decorbox->label.content[i].character, decorbox->label.content[i].attribute);
    }
    decorbox->label.left_offset = left_offset;
    decorbox->label.enabled = true;
    decorbox->label.content_length = label_string_length;
    
    pthread_mutex_unlock(&NOCTERM_WIDGET(decorbox)->lock);

    return NOCTERM_SUCCESS;
}

nocterm_decorbox_border_t nocterm_decorbox_border_from_shape(nocterm_decorbox_border_shape_t shape){

    switch(shape){
        default:
        case NOCTERM_DECORBOX_BORDER_SHAPE_ASCII:{
            nocterm_char_t vertical = {
                .bytes = "|",
                .bytes_size = 1,
                .is_utf8 = false
            };

            nocterm_char_t horizontal = {
                .bytes = "-",
                .bytes_size = 1,
                .is_utf8 = false
            };

            nocterm_char_t top_left = {
                .bytes = "+",
                .bytes_size = 1,
                .is_utf8 = false
            };
            nocterm_char_t top_right = {
                .bytes = "+",
                .bytes_size = 1,
                .is_utf8 = false
            };
            nocterm_char_t bottom_left = {
                .bytes = "+",
                .bytes_size = 1,
                .is_utf8 = false
            };
            nocterm_char_t bottom_right = {
                .bytes = "+",
                .bytes_size = 1,
                .is_utf8 = false
            };   

            nocterm_decorbox_border_t border = {
                .vertical = vertical,
                .horizontal = horizontal,
                .top_left = top_left,
                .top_right = top_right,
                .bottom_left = bottom_left,
                .bottom_right = bottom_right
            };
            return border;
        }break;

        case NOCTERM_DECORBOX_BORDER_SHAPE_UNICODE_SHARP:{
            nocterm_char_t vertical = {
                .bytes_size = 3,
                .is_utf8 = false
            };
            memcpy(vertical.bytes, "│",3);

            nocterm_char_t horizontal = {
                .bytes_size = 3,
                .is_utf8 = false
            };        
            memcpy(horizontal.bytes, "─",3);

            nocterm_char_t top_left = {
                .bytes_size = 3,
                .is_utf8 = false
            };   
            memcpy(top_left.bytes, "┌",3);
            
            nocterm_char_t top_right = {
                .bytes_size = 3,
                .is_utf8 = false
            };      
            memcpy(top_right.bytes, "┐",3);
            
            nocterm_char_t bottom_left = {
                .bytes_size = 3,
                .is_utf8 = false
            };   
            memcpy(bottom_left.bytes, "└",3);
            
            nocterm_char_t bottom_right = {
                .bytes_size = 3,
                .is_utf8 = false
            };   
            memcpy(bottom_right.bytes, "┘",3);

            nocterm_decorbox_border_t border = {
                .vertical = vertical,
                .horizontal = horizontal,
                .top_left = top_left,
                .top_right = top_right,
                .bottom_left = bottom_left,
                .bottom_right = bottom_right
            };
            return border;
        }break;

        case NOCTERM_DECORBOX_BORDER_SHAPE_UNICODE_ROUND:{
            nocterm_char_t vertical = {
                .bytes_size = 3,
                .is_utf8 = false
            };
            memcpy(vertical.bytes, "│",3);

            nocterm_char_t horizontal = {
                .bytes_size = 3,
                .is_utf8 = false
            };        
            memcpy(horizontal.bytes, "─",3);

            nocterm_char_t top_left = {
                .bytes_size = 3,
                .is_utf8 = false
            };   
            memcpy(top_left.bytes, "╭",3);

            nocterm_char_t top_right = {
                .bytes_size = 3,
                .is_utf8 = false
            };
            memcpy(top_right.bytes, "╮",3);

            nocterm_char_t bottom_left = {
                .bytes_size = 3,
                .is_utf8 = false
            };
            memcpy(bottom_left.bytes, "╰",3);

            nocterm_char_t bottom_right = {
                .bytes_size = 3,
                .is_utf8 = false
            };   
            memcpy(bottom_right.bytes, "╯",3);

            nocterm_decorbox_border_t border = {
                .vertical = vertical,
                .horizontal = horizontal,
                .top_left = top_left,
                .top_right = top_right,
                .bottom_left = bottom_left,
                .bottom_right = bottom_right
            };
            return border;
        }break;

    }
}

int nocterm_decorbox_border_draw(nocterm_decorbox_t* decorbox, nocterm_decorbox_border_t border, nocterm_attribute_t attribute){

    if(decorbox == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    for(nocterm_dimension_size_t i = 1; i < NOCTERM_WIDGET(decorbox)->bounds.height-1; i++){
        if(nocterm_widget_update(NOCTERM_WIDGET(decorbox),i,0, border.vertical,attribute) == NOCTERM_FAILURE){
            return NOCTERM_FAILURE;
        }

        if(nocterm_widget_update(NOCTERM_WIDGET(decorbox), i, NOCTERM_WIDGET(decorbox)->bounds.width-1, border.vertical, attribute) == NOCTERM_FAILURE){
            return NOCTERM_FAILURE;
        }

    }

    for(nocterm_dimension_size_t i = 1; i < NOCTERM_WIDGET(decorbox)->bounds.width-1; i++){
        if(nocterm_widget_update(NOCTERM_WIDGET(decorbox),0,i, border.horizontal, attribute) == NOCTERM_FAILURE){
            return NOCTERM_FAILURE;
        }
        if(nocterm_widget_update(NOCTERM_WIDGET(decorbox),NOCTERM_WIDGET(decorbox)->bounds.height-1, i, border.horizontal, attribute) == NOCTERM_FAILURE){
            return NOCTERM_FAILURE;
        }
    }

    // Top left
    if(nocterm_widget_update(NOCTERM_WIDGET(decorbox), 0,0, border.top_left, attribute) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    } 

    // Top right
    if(nocterm_widget_update(NOCTERM_WIDGET(decorbox), 0,NOCTERM_WIDGET(decorbox)->bounds.width-1, border.top_right, attribute) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    } 

    // Bottom left
    if(nocterm_widget_update(NOCTERM_WIDGET(decorbox), NOCTERM_WIDGET(decorbox)->bounds.height-1,0, border.bottom_left, attribute) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    } 

    // Bottom right
    if(nocterm_widget_update(NOCTERM_WIDGET(decorbox), NOCTERM_WIDGET(decorbox)->bounds.height-1, NOCTERM_WIDGET(decorbox)->bounds.width-1, border.bottom_right, attribute) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    } 

    return NOCTERM_SUCCESS;
}

NOCTERM_WIDGET_KEY_HANDLER(nocterm_decorbox_key_handler){
    if(NOCTERM_DECORBOX(self)->contained_widget && NOCTERM_DECORBOX(self)->contained_widget->key_handler){
        NOCTERM_DECORBOX(self)->contained_widget->key_handler(NOCTERM_DECORBOX(self)->contained_widget, key);    
    }
}

NOCTERM_WIDGET_FOCUS_HANDLER(nocterm_decorbox_focus_handler){

    switch(focus){
        case NOCTERM_WIDGET_FOCUS_ENTER:{


            if(NOCTERM_DECORBOX(self)->border_settings.enabled){
                nocterm_decorbox_border_draw(NOCTERM_DECORBOX(self),NOCTERM_DECORBOX(self)->border_settings.border, NOCTERM_DECORBOX(self)->border_settings.attributes.focused);
            }

            if(NOCTERM_DECORBOX(self)->label.enabled){
                for(uint64_t i = 0; i < NOCTERM_DECORBOX(self)->label.content_length; i++){
                    nocterm_widget_update(NOCTERM_WIDGET(self), 0, i + NOCTERM_DECORBOX(self)->label.left_offset, NOCTERM_DECORBOX(self)->label.content[i].character, NOCTERM_DECORBOX(self)->label.content[i].attribute);
                }
            }

            if(NOCTERM_DECORBOX(self)->contained_widget && NOCTERM_DECORBOX(self)->contained_widget->focus_handler){
                NOCTERM_DECORBOX(self)->contained_widget->focus_handler(NOCTERM_DECORBOX(self)->contained_widget, NOCTERM_WIDGET_FOCUS_ENTER);
            }




        }break;
        case NOCTERM_WIDGET_FOCUS_LEAVE:{

            if(NOCTERM_DECORBOX(self)->border_settings.enabled){
                nocterm_decorbox_border_draw(NOCTERM_DECORBOX(self),NOCTERM_DECORBOX(self)->border_settings.border, NOCTERM_DECORBOX(self)->border_settings.attributes.normal);
            }

            if(NOCTERM_DECORBOX(self)->label.enabled){
                for(uint64_t i = 0; i < NOCTERM_DECORBOX(self)->label.content_length; i++){
                    nocterm_widget_update(NOCTERM_WIDGET(self), 0, i + NOCTERM_DECORBOX(self)->label.left_offset, NOCTERM_DECORBOX(self)->label.content[i].character, NOCTERM_DECORBOX(self)->label.content[i].attribute);
                }
            }

            if(NOCTERM_DECORBOX(self)->contained_widget && NOCTERM_DECORBOX(self)->contained_widget->focus_handler){
                NOCTERM_DECORBOX(self)->contained_widget->focus_handler(NOCTERM_DECORBOX(self)->contained_widget, NOCTERM_WIDGET_FOCUS_LEAVE);
            }
        }break;
    }

}

NOCTERM_WIDGET_RESIZE_HANDLER(nocterm_decorbox_internal_resize_handler){
    nocterm_decorbox_t* decorbox = NOCTERM_DECORBOX(self);

    // Need at least 3 cells in both dimensions for a 1-cell inner area + 2 border cells
    if(bounds.height < 3 || bounds.width < 3){
        return;
    }

    // The contained widget is a subwidget — flex_update recurses into it and
    // applies its own size policy. We only redraw the border and label here.

    if(decorbox->border_settings.enabled){
        nocterm_attribute_t attr = nocterm_widget_is_focused(self)
            ? decorbox->border_settings.attributes.focused
            : decorbox->border_settings.attributes.normal;
        nocterm_decorbox_border_draw(decorbox, decorbox->border_settings.border, attr);
    }

    if(decorbox->label.enabled){
        for(uint64_t i = 0; i < decorbox->label.content_length; i++){
            uint64_t col = decorbox->label.left_offset + i;
            if(col >= (uint64_t)self->bounds.width){
                break;
            }
            nocterm_widget_update(self, 0, col,
                decorbox->label.content[i].character,
                decorbox->label.content[i].attribute);
        }
    }
}

