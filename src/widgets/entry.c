#include <nocterm/widgets/entry.h>

nocterm_entry_t* nocterm_entry_new(nocterm_dimension_size_t row, nocterm_dimension_size_t col, nocterm_dimension_size_t width, nocterm_attribute_t attribute){

    nocterm_entry_t* new_entry = (nocterm_entry_t*)malloc(sizeof(nocterm_entry_t));

    if(new_entry == NULL){
        return NULL;
    }

    // Extra space for cursor
    if(nocterm_widget_constructor(NOCTERM_WIDGET(new_entry),(nocterm_dimension_t){row, col, 1, NOCTERM_ENTRY_BUFFER_MAX_SIZE + 1}, true, false) == NOCTERM_FAILURE){
        free(new_entry);
        return NULL;
    }

    new_entry->normal_attribute = attribute;
    new_entry->buffer_position = 0;
    new_entry->cursor_position = 0;
    new_entry->current_length = 0;
    
    new_entry->cursor_attribute = new_entry->normal_attribute;

    new_entry->cursor_attribute.color.ansi.bg = new_entry->normal_attribute.color.ansi.fg;
    new_entry->cursor_attribute.color.ansi.fg = new_entry->normal_attribute.color.ansi.bg;
    new_entry->cursor_attribute.color.ansi.codes.fg = new_entry->normal_attribute.color.ansi.codes.bg;
    new_entry->cursor_attribute.color.ansi.codes.bg = new_entry->normal_attribute.color.ansi.codes.fg;

    new_entry->cursor_attribute.color.c256.bg = new_entry->normal_attribute.color.c256.fg;
    new_entry->cursor_attribute.color.c256.fg = new_entry->normal_attribute.color.c256.bg;  
    new_entry->cursor_attribute.color.c256.codes.fg = new_entry->normal_attribute.color.c256.codes.bg;
    new_entry->cursor_attribute.color.c256.codes.bg = new_entry->normal_attribute.color.c256.codes.fg;


    new_entry->cursor_attribute.color.rgb.bg = new_entry->normal_attribute.color.rgb.fg;
    new_entry->cursor_attribute.color.rgb.fg = new_entry->normal_attribute.color.rgb.bg;
    new_entry->cursor_attribute.color.rgb.codes.fg = new_entry->normal_attribute.color.rgb.codes.bg;
    new_entry->cursor_attribute.color.rgb.codes.bg = new_entry->normal_attribute.color.rgb.codes.fg;

    if(nocterm_widget_add_key_handler(NOCTERM_WIDGET(new_entry), nocterm_entry_key_handler) == NOCTERM_FAILURE){
        free(new_entry);
        return NULL;
    }
    if(nocterm_widget_add_focus_handler(NOCTERM_WIDGET(new_entry), nocterm_entry_focus_handler) == NOCTERM_FAILURE){
        free(new_entry);
        return NULL;
    }

    nocterm_widget_viewport(NOCTERM_WIDGET(new_entry), (nocterm_dimension_t){0, 0, 1, width});

    nocterm_widget_update(NOCTERM_WIDGET(new_entry), 0, 0, NOCTERM_ENTRY_CURSOR_CHAR, new_entry->normal_attribute);
    return new_entry;
}

int nocterm_entry_delete(nocterm_entry_t* entry){

    if(nocterm_widget_destructor(NOCTERM_WIDGET(entry)) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    free(entry);

    return NOCTERM_FAILURE;
}

int nocterm_entry_cursor_move_left(nocterm_entry_t* entry){
    
    if(entry == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(entry->cursor_position == 0 && entry->buffer_position == 0){
        // We are at the leftmost possible with the cursor
        return NOCTERM_SUCCESS;
    }

    // If cursor position is greater than 0, then it is guaranteed that we can move left
    // And also buffer position is guaranteed to be greater than zero, so...
    if(entry->cursor_position > 0){
        
        nocterm_widget_update(NOCTERM_WIDGET(entry), 0, entry->buffer_position, NOCTERM_WIDGET(entry)->buffer[entry->buffer_position].character, entry->normal_attribute);
        nocterm_widget_update(NOCTERM_WIDGET(entry), 0, entry->buffer_position - 1, NOCTERM_WIDGET(entry)->buffer[entry->buffer_position - 1].character, entry->cursor_attribute);
        entry->buffer_position--;
        entry->cursor_position--;
    }else{
        // Cursor is at the leftmost position, but let's see whether there are more items on the left?
        if(entry->buffer_position > 0){
            // Yes there are more items on the left
            nocterm_widget_update(NOCTERM_WIDGET(entry), 0, entry->buffer_position, NOCTERM_WIDGET(entry)->buffer[entry->buffer_position].character, entry->normal_attribute);
            nocterm_widget_update(NOCTERM_WIDGET(entry), 0, entry->buffer_position - 1, NOCTERM_WIDGET(entry)->buffer[entry->buffer_position - 1].character, entry->cursor_attribute);
            entry->buffer_position--;
            nocterm_widget_viewport_left(NOCTERM_WIDGET(entry));
        }
    }

    return NOCTERM_SUCCESS;
}

int nocterm_entry_cursor_move_right(nocterm_entry_t* entry){
    
    if(entry == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(entry->buffer_position < entry->current_length){
        // At least 1 character to go right, this could be the last character reserved for cursor or a character,
        // but it is clear that we need to move to right

        // Turning current character to normal attribute, if we are to move right, then buffer_position MUST point to a valid input character
        nocterm_widget_update(NOCTERM_WIDGET(entry), 0, entry->buffer_position, NOCTERM_WIDGET(entry)->buffer[entry->buffer_position].character, entry->normal_attribute);

        // Let's check if we are pointing to the last input character, if so, we need to add a space character at the end
        if(entry->buffer_position == entry->current_length-1){
            nocterm_widget_update(NOCTERM_WIDGET(entry), 0, entry->buffer_position+1, NOCTERM_ENTRY_CURSOR_CHAR, entry->cursor_attribute);        
        }else{
            // Otherwise, just make the current character the cursor attribute
            nocterm_widget_update(NOCTERM_WIDGET(entry), 0, entry->buffer_position+1, NOCTERM_WIDGET(entry)->buffer[entry->buffer_position+1].character, entry->cursor_attribute);        
        }
        
        entry->buffer_position++;

        // Now we need to check whether we should move the viewport to right, if the cursor was previously at the very end, then we had to modify viewport
        if(entry->cursor_position == NOCTERM_WIDGET(entry)->viewport.width-1){
            // Cursor at rightmost
            nocterm_widget_viewport_right(NOCTERM_WIDGET(entry));
        }else{
            entry->cursor_position++;
        }

    }

    // Otherwise, no effect
    return NOCTERM_SUCCESS;
}

int nocterm_entry_cursor_insert(nocterm_entry_t* entry, nocterm_char_t input){
    if(entry == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    // Let's check whether we have sufficient buffer area
    if(entry->current_length < NOCTERM_ENTRY_BUFFER_MAX_SIZE){

        // OK, we can insert

        // And let's move everything at the right side by one to right first
        if(entry->current_length > 0){
            for(uint16_t i = entry->current_length - entry->buffer_position; i > 0; i--){
                nocterm_widget_update(NOCTERM_WIDGET(entry), 0, entry->buffer_position + i, NOCTERM_WIDGET(entry)->buffer[entry->buffer_position + i - 1].character, entry->normal_attribute);
            }
        }
        
        nocterm_widget_update(NOCTERM_WIDGET(entry), 0, entry->buffer_position, input, entry->normal_attribute);
        entry->current_length++;

        nocterm_entry_cursor_move_right(entry);
    }

    // Otherwise, no effect
    return NOCTERM_SUCCESS;
}

int nocterm_entry_cursor_erase_right(nocterm_entry_t* entry){
    if(entry == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    // We are empty
    if(entry->current_length == 0){
        return NOCTERM_SUCCESS;
    }

    // Let's see whether we are at the last position, which is for cursor only,
    // because that time we cannot erase something on the right
    if(entry->buffer_position == entry->current_length){
        return NOCTERM_SUCCESS;        
    }

    // Okay, there are items at the right to delete
    
    // Let's move everything to left by one, this shall not change buffer position
    for(uint64_t i = entry->buffer_position; i < entry->current_length-1; i++){
        nocterm_widget_update(NOCTERM_WIDGET(entry), 0, i, NOCTERM_WIDGET(entry)->buffer[i+1].character, entry->normal_attribute);
    }
    nocterm_widget_update(NOCTERM_WIDGET(entry), 0, entry->current_length-1, NOCTERM_CHAR_EMPTY, NOCTERM_ATTRIBUTE_EMPTY);

    entry->current_length--;
    // According to the erasing the right side logic, cursor position shall not change
    // But let's check whether we end up at the rightmost or not
    if(entry->buffer_position == entry->current_length){
        nocterm_widget_update(NOCTERM_WIDGET(entry), 0, entry->buffer_position, NOCTERM_ENTRY_CURSOR_CHAR, entry->cursor_attribute);
    }else{
        nocterm_widget_update(NOCTERM_WIDGET(entry), 0, entry->buffer_position, NOCTERM_WIDGET(entry)->buffer[entry->buffer_position].character, entry->cursor_attribute);
    }

    return NOCTERM_SUCCESS;
}

int nocterm_entry_cursor_erase_left(nocterm_entry_t* entry){
    if(entry == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    // We are empty
    if(entry->current_length == 0){
        return NOCTERM_SUCCESS;
    }

    // We are at the leftmost, there is nothing to erase on the left
    if(entry->cursor_position == 0 && entry->buffer_position == 0){
        return NOCTERM_SUCCESS;
    }

    if(entry->cursor_position > 0){
        // It is guaranteed that there are items on the left
        // Let's move everything to left, but this time it is tricky

        for(uint64_t i = entry->buffer_position; i < entry->current_length; i++){
            nocterm_widget_update(NOCTERM_WIDGET(entry), 0, i-1, NOCTERM_WIDGET(entry)->buffer[i].character, entry->normal_attribute);
        }
        nocterm_widget_update(NOCTERM_WIDGET(entry), 0, entry->current_length, NOCTERM_CHAR_EMPTY, NOCTERM_ATTRIBUTE_EMPTY);  

        entry->cursor_position--;
        entry->buffer_position--;
        entry->current_length--;

        if(entry->buffer_position == entry->current_length){
            // The cursor was at rightmost in the buffer   
            nocterm_widget_update(NOCTERM_WIDGET(entry), 0, entry->buffer_position, NOCTERM_ENTRY_CURSOR_CHAR, entry->cursor_attribute);
        }else{
            nocterm_widget_update(NOCTERM_WIDGET(entry), 0, entry->current_length, NOCTERM_CHAR_EMPTY, NOCTERM_ATTRIBUTE_EMPTY);        
            nocterm_widget_update(NOCTERM_WIDGET(entry), 0, entry->buffer_position, NOCTERM_WIDGET(entry)->buffer[entry->buffer_position].character, entry->cursor_attribute);
        }

        if(entry->cursor_position == 0){
            // We have to change viewport so that the remaining text is still visible for convenience

            if(entry->current_length >= NOCTERM_WIDGET(entry)->viewport.width-1){
                entry->cursor_position = NOCTERM_WIDGET(entry)->viewport.width-1;
                nocterm_widget_viewport(NOCTERM_WIDGET(entry), (nocterm_dimension_t){0, entry->current_length-NOCTERM_WIDGET(entry)->viewport.width+1,1,NOCTERM_WIDGET(entry)->viewport.width});

            }else{
                entry->cursor_position = entry->current_length;
                nocterm_widget_viewport(NOCTERM_WIDGET(entry),(nocterm_dimension_t){0,0,1,NOCTERM_WIDGET(entry)->viewport.width});
            }

        }

    }

    return NOCTERM_SUCCESS;
}

NOCTERM_WIDGET_KEY_HANDLER(nocterm_entry_key_handler){

    switch(nocterm_key_translate(key)){

        case NOCTERM_KEY_EVENT_RIGHT:{
            nocterm_entry_cursor_move_right(NOCTERM_ENTRY(self));
        }break;
        
        case NOCTERM_KEY_EVENT_LEFT:{
            nocterm_entry_cursor_move_left(NOCTERM_ENTRY(self));
        }break;

        case NOCTERM_KEY_EVENT_PRINTABLE:{
            // New character detected
            nocterm_char_t temp = {0};
            memcpy(temp.bytes, key->buffer, key->buffer_length);
            temp.bytes_size = key->buffer_length;
            temp.is_utf8 = (key->buffer_length > 1 ? true : false);
            nocterm_entry_cursor_insert(NOCTERM_ENTRY(self),temp);
        }break;

        case NOCTERM_KEY_EVENT_BACKSPACE:{
            nocterm_entry_cursor_erase_left(NOCTERM_ENTRY(self));
        }break;

        case NOCTERM_KEY_EVENT_DELETE:{
            nocterm_entry_cursor_erase_right(NOCTERM_ENTRY(self));
        }break;

        default:
    }
}

NOCTERM_WIDGET_FOCUS_HANDLER(nocterm_entry_focus_handler){
    
    switch(focus){
        case NOCTERM_WIDGET_FOCUS_ENTER:{
            nocterm_widget_update(self, 0, NOCTERM_ENTRY(self)->buffer_position, self->buffer[NOCTERM_ENTRY(self)->buffer_position].character, NOCTERM_ENTRY(self)->cursor_attribute);
        }break;

        case NOCTERM_WIDGET_FOCUS_LEAVE:{
            nocterm_widget_update(self, 0, NOCTERM_ENTRY(self)->buffer_position, self->buffer[NOCTERM_ENTRY(self)->buffer_position].character, NOCTERM_ENTRY(self)->normal_attribute);
        }break;
    }
}

int nocterm_entry_get_text(nocterm_entry_t* entry, char* buffer, uint64_t buffer_size, uint64_t* entry_length){

    if(entry == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(entry->current_length == 0){
        *entry_length = 0;
        return NOCTERM_SUCCESS;
    }

    nocterm_char_t entry_string[NOCTERM_ENTRY_BUFFER_MAX_SIZE] = {0};
    for(uint64_t i = 0; i < entry->current_length; i++){
        entry_string[i] = NOCTERM_WIDGET(entry)->buffer[i].character;
    }

    uint64_t length = nocterm_char_string_to_stream(buffer, buffer_size, entry_string, NOCTERM_ENTRY_BUFFER_MAX_SIZE);
    if(length == 0){
        return NOCTERM_FAILURE;
    }
    if(entry_length){
        *entry_length = length;
    }
    return NOCTERM_SUCCESS;
}

int nocterm_entry_set_text(nocterm_entry_t* entry, char* buffer, uint64_t buffer_size){

    if(entry == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    nocterm_char_t new_entry_text[NOCTERM_ENTRY_BUFFER_MAX_SIZE] = {0};

    uint64_t new_entry_text_length = nocterm_char_string_from_stream(new_entry_text, NOCTERM_ENTRY_BUFFER_MAX_SIZE, buffer, buffer_size);

    if(new_entry_text_length == 0){
        return NOCTERM_FAILURE;
    }

    nocterm_widget_clear(NOCTERM_WIDGET(entry));


    for(uint64_t i = 0; i < new_entry_text_length; i++){
        nocterm_widget_update(NOCTERM_WIDGET(entry), 0, i, new_entry_text[i], entry->normal_attribute);
    }

    nocterm_widget_update(NOCTERM_WIDGET(entry), 0, new_entry_text_length, NOCTERM_ENTRY_CURSOR_CHAR, entry->normal_attribute);

    entry->buffer_position = new_entry_text_length;
    entry->current_length = new_entry_text_length;

    if(new_entry_text_length <= NOCTERM_WIDGET(entry)->viewport.width - 1){
        entry->cursor_position = new_entry_text_length;
        nocterm_widget_viewport(NOCTERM_WIDGET(entry), (nocterm_dimension_t){0, 0, NOCTERM_WIDGET(entry)->viewport.height, NOCTERM_WIDGET(entry)->viewport.width});
    }else{
        entry->cursor_position = NOCTERM_WIDGET(entry)->viewport.width - 1;
        nocterm_widget_viewport(NOCTERM_WIDGET(entry), (nocterm_dimension_t){0, new_entry_text_length - NOCTERM_WIDGET(entry)->viewport.width + 1, NOCTERM_WIDGET(entry)->viewport.height , NOCTERM_WIDGET(entry)->viewport.width});

    }
    
    return NOCTERM_SUCCESS;
}
