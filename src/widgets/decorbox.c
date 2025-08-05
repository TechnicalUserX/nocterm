#include <nocterm/widgets/decorbox.h>

nocterm_decorbox_t* nocterm_decorbox_new(nocterm_dimension_size_t row, nocterm_dimension_size_t col, nocterm_widget_t* contained_widget){

    if(contained_widget == NULL){
        errno = EINVAL;
        return NULL;
    }

    nocterm_decorbox_t* new_decorbox = (nocterm_decorbox_t*)malloc(sizeof(nocterm_decorbox_t));
    if(new_decorbox == NULL){
        errno = ENOMEM;
        return NULL;
    }

    nocterm_dimension_size_t decorbox_heght = contained_widget->viewport.height + 2;
    nocterm_dimension_size_t decorbox_width = contained_widget->viewport.width + 2;

    if(nocterm_widget_constructor(NOCTERM_WIDGET(new_decorbox), (nocterm_dimension_t){row, col, decorbox_heght, decorbox_width}, NOCTERM_WIDGET_FOCUSABLE_YES, NOCTERM_WIDGET_TYPE_REAL) == NOCTERM_FAILURE){
        return NULL;
    }

    new_decorbox->contained_widget = contained_widget;
    new_decorbox->contained_widget->focusable = false; // Now the ownership of the focusability is switched to decorbox
    new_decorbox->contained_widget->bounds.row = 1; // Relative position just beneath the border 
    new_decorbox->contained_widget->bounds.col = 1; // Relative position just next to the border


    nocterm_widget_add_subwidget(NOCTERM_WIDGET(new_decorbox), contained_widget);

    if(nocterm_widget_add_focus_handler(NOCTERM_WIDGET(new_decorbox), nocterm_decorbox_focus_handler) == NOCTERM_FAILURE){
        free(new_decorbox);
        return NULL;
    }

    if(nocterm_widget_add_key_handler(NOCTERM_WIDGET(new_decorbox), nocterm_decorbox_key_handler) == NOCTERM_FAILURE){
        free(new_decorbox);
        return NULL;
    }

    return new_decorbox;
}

int nocterm_decorbox_delete(nocterm_decorbox_t* decorbox){

    if(decorbox == NULL){
        return NOCTERM_FAILURE;
    }

    if(nocterm_widget_destructor(NOCTERM_WIDGET(decorbox)) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    free(decorbox);

    return NOCTERM_SUCCESS;
}

int nocterm_decorbox_border_draw(nocterm_decorbox_t* decorbox, nocterm_decorbox_border_shape_t border_shape, nocterm_attribute_t attribute)
{

    if(decorbox == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    for(nocterm_dimension_size_t i = 1; i < NOCTERM_WIDGET(decorbox)->bounds.height-1; i++){
        if(nocterm_widget_update(NOCTERM_WIDGET(decorbox),i,0, border_shape.vertical,attribute) == NOCTERM_FAILURE){
            return NOCTERM_FAILURE;
        }
        if(nocterm_widget_update(NOCTERM_WIDGET(decorbox),i, NOCTERM_WIDGET(decorbox)->bounds.width-1, border_shape.vertical,attribute) == NOCTERM_FAILURE){
            return NOCTERM_FAILURE;
        }

    }

    for(nocterm_dimension_size_t i = 1; i < NOCTERM_WIDGET(decorbox)->bounds.width-1; i++){
        if(nocterm_widget_update(NOCTERM_WIDGET(decorbox),0,i, border_shape.horizontal, attribute) == NOCTERM_FAILURE){
            return NOCTERM_FAILURE;
        }
        if(nocterm_widget_update(NOCTERM_WIDGET(decorbox),NOCTERM_WIDGET(decorbox)->bounds.height-1, i, border_shape.horizontal, attribute) == NOCTERM_FAILURE){
            return NOCTERM_FAILURE;
        } 
    }

    // Top left
    if(nocterm_widget_update(NOCTERM_WIDGET(decorbox), 0,0, border_shape.top_left, attribute) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    } 

    // Top right
    if(nocterm_widget_update(NOCTERM_WIDGET(decorbox), 0,NOCTERM_WIDGET(decorbox)->bounds.width-1, border_shape.top_right, attribute) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    } 

    // Bottom left
    if(nocterm_widget_update(NOCTERM_WIDGET(decorbox), NOCTERM_WIDGET(decorbox)->bounds.height-1,0, border_shape.bottom_left, attribute) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    } 

    // Bottom right
    if(nocterm_widget_update(NOCTERM_WIDGET(decorbox), NOCTERM_WIDGET(decorbox)->bounds.height-1, NOCTERM_WIDGET(decorbox)->bounds.width-1, border_shape.bottom_right, attribute) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    } 

    return NOCTERM_SUCCESS;
}

int nocterm_decorbox_border(nocterm_decorbox_t* decorbox, nocterm_decorbox_border_shape_t border_shape, nocterm_attribute_t normal, nocterm_attribute_t focused){

    if(decorbox == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    decorbox->border.shape = border_shape;
    decorbox->border.enabled = true;
    decorbox->border.attributes.normal = normal;
    decorbox->border.attributes.focused = focused;

    nocterm_decorbox_border_draw(decorbox, border_shape, normal);
    
    return NOCTERM_SUCCESS;
}

nocterm_decorbox_border_shape_t nocterm_decorbox_border_shape(nocterm_decorbox_border_shape_type_t type){

    switch(type){
        default:
        case NOCTERM_DECORBOX_BORDER_SHAPE_TYPE_ASCII:{
            nocterm_char_t vertical = {
                .bytes = "|",
                .bytes_size = 3,
                .is_utf8 = false
            };

            nocterm_char_t horizontal = {
                .bytes = "-",
                .bytes_size = 3,
                .is_utf8 = false
            };        

            nocterm_char_t top_left = {
                .bytes = "+",
                .bytes_size = 3,
                .is_utf8 = false
            };   
            nocterm_char_t top_right = {
                .bytes = "+",
                .bytes_size = 3,
                .is_utf8 = false
            };           
            nocterm_char_t bottom_left = {
                .bytes = "+",
                .bytes_size = 3,
                .is_utf8 = false
            };   
            nocterm_char_t bottom_right = {
                .bytes = "+",
                .bytes_size = 3,
                .is_utf8 = false
            };   

            nocterm_decorbox_border_shape_t border_shape = {
                .vertical = vertical,
                .horizontal = horizontal,
                .top_left = top_left,
                .top_right = top_right,
                .bottom_left = bottom_left,
                .bottom_right = bottom_right
            };
            return border_shape;
        }break;

        case NOCTERM_DECORBOX_BORDER_SHAPE_TYPE_UNICODE_SHARP:{
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

            nocterm_decorbox_border_shape_t border_shape = {
                .vertical = vertical,
                .horizontal = horizontal,
                .top_left = top_left,
                .top_right = top_right,
                .bottom_left = bottom_left,
                .bottom_right = bottom_right
            };
            return border_shape;
        }break;

        case NOCTERM_DECORBOX_BORDER_SHAPE_TYPE_UNICODE_ROUND:{
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

            nocterm_decorbox_border_shape_t border_shape = {
                .vertical = vertical,
                .horizontal = horizontal,
                .top_left = top_left,
                .top_right = top_right,
                .bottom_left = bottom_left,
                .bottom_right = bottom_right
            };
            return border_shape;
        }break;

    }
}

int nocterm_decorbox_label(nocterm_decorbox_t* decorbox, const char* label, uint64_t label_size, nocterm_attribute_t attribute, uint64_t left_offset){
    if(decorbox == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    nocterm_char_t label_string[NOCTERM_DECORBOX_LABEL_MAX_SIZE] = {0};
    uint64_t label_string_length = nocterm_char_string_from_stream(label_string,NOCTERM_DECORBOX_LABEL_MAX_SIZE, label, label_size);

    if(label_string_length == 0){
        return NOCTERM_FAILURE;
    }
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
            
            if(NOCTERM_DECORBOX(self)->border.enabled){
                nocterm_decorbox_border_draw(NOCTERM_DECORBOX(self),NOCTERM_DECORBOX(self)->border.shape, NOCTERM_DECORBOX(self)->border.attributes.focused);
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
            
            if(NOCTERM_DECORBOX(self)->border.enabled){
                nocterm_decorbox_border_draw(NOCTERM_DECORBOX(self),NOCTERM_DECORBOX(self)->border.shape, NOCTERM_DECORBOX(self)->border.attributes.normal);
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

