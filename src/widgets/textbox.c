#include <nocterm/widgets/textbox.h>

NOCTERM_INTERNAL NOCTERM_WIDGET_RESIZE_HANDLER(nocterm_textbox_internal_resize_handler);

NOCTERM_INTERNAL NOCTERM_WIDGET_KEY_HANDLER(nocterm_textbox_key_handler);

NOCTERM_INTERNAL NOCTERM_WIDGET_FOCUS_HANDLER(nocterm_textbox_focus_handler);

NOCTERM_INTERNAL int nocterm_textbox_cursor_move_left(nocterm_textbox_t* textbox);

NOCTERM_INTERNAL int nocterm_textbox_cursor_move_right(nocterm_textbox_t* textbox);

NOCTERM_INTERNAL int nocterm_textbox_cursor_move_up(nocterm_textbox_t* textbox);

NOCTERM_INTERNAL int nocterm_textbox_cursor_move_down(nocterm_textbox_t* textbox);

NOCTERM_INTERNAL int nocterm_textbox_cursor_insert(nocterm_textbox_t* textbox, nocterm_char_t character);

NOCTERM_INTERNAL int nocterm_textbox_cursor_erase_left(nocterm_textbox_t* textbox);  // Backspace

NOCTERM_INTERNAL int nocterm_textbox_cursor_erase_right(nocterm_textbox_t* textbox); // Delete

// Adjusts scroll_offset so the cursor line is within the visible window.
NOCTERM_INTERNAL void nocterm_textbox_clamp_scroll(nocterm_textbox_t* textbox){
    nocterm_widget_t* w = NOCTERM_WIDGET(textbox);
    if(w->bounds.width == 0 || w->bounds.height == 0) return;
    uint64_t height      = (uint64_t)w->bounds.height;
    uint64_t width       = (uint64_t)w->bounds.width;
    uint64_t cursor_line = textbox->buffer_position / width;
    if(cursor_line < textbox->scroll_offset){
        textbox->scroll_offset = cursor_line;
    }else if(cursor_line >= textbox->scroll_offset + height){
        textbox->scroll_offset = cursor_line - height + 1;
    }
}

// Renders the visible portion of text_store into the widget cell buffer.
// The backing store is wrapped at bounds.width; scroll_offset selects the first visible line.
// Called with the widget lock already held (or from handlers that own the lock).
NOCTERM_INTERNAL void nocterm_textbox_render(nocterm_textbox_t* textbox){
    nocterm_widget_t* w = NOCTERM_WIDGET(textbox);
    if(w->bounds.width == 0 || w->bounds.height == 0) return;

    uint64_t width         = (uint64_t)w->bounds.width;
    uint64_t visible_chars = (uint64_t)w->bounds.height * width;
    uint64_t char_start    = textbox->scroll_offset * width;
    bool focused           = nocterm_widget_is_focused(w);

    for(uint64_t i = 0; i < visible_chars; i++){
        uint64_t src = char_start + i;
        nocterm_char_t ch;
        nocterm_attribute_t attr;

        if(focused && src == textbox->buffer_position){
            // Cursor cell: show the underlying character (or blank) highlighted.
            ch   = (src < textbox->text_length)
                       ? textbox->text_store[src]
                       : NOCTERM_TEXTBOX_CURSOR_CHAR;
            attr = textbox->cursor_attribute;
        }else if(src < textbox->text_length){
            ch   = textbox->text_store[src];
            attr = textbox->normal_attribute;
        }else{
            ch   = NOCTERM_CHAR_EMPTY;
            attr = NOCTERM_ATTRIBUTE_EMPTY;
        }

        // Flat indexing mirrors how textview renders into the widget buffer.
        nocterm_widget_update(w, 0, (nocterm_dimension_size_t)i, ch, attr);
    }

    w->hard_refresh = true;
}

nocterm_textbox_t* nocterm_textbox_new(nocterm_dimension_size_t height, nocterm_dimension_size_t width){

    nocterm_textbox_t* new_textbox = (nocterm_textbox_t*)malloc(sizeof(nocterm_textbox_t));

    if(new_textbox == NULL){
        return NULL;
    }

    memset(new_textbox, 0x0, sizeof(nocterm_textbox_t));

    if(nocterm_textbox_constructor(new_textbox, height, width) == NOCTERM_FAILURE){
        free(new_textbox);
        return NULL;
    }

    return new_textbox;
}

int nocterm_textbox_constructor(nocterm_textbox_t* textbox, nocterm_dimension_size_t height, nocterm_dimension_size_t width){

    if(textbox == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(nocterm_widget_constructor(NOCTERM_WIDGET(textbox), height, width, true, false) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    nocterm_widget_size_policy_set_permission(NOCTERM_WIDGET(textbox), NOCTERM_WIDGET_SIZE_POLICY_PERMISSION_BOTH);

    textbox->normal_attribute    = NOCTERM_ATTRIBUTE_EMPTY;
    textbox->cursor_attribute    = NOCTERM_ATTRIBUTE_EMPTY;
    textbox->cursor_attribute.inverse = true;
    textbox->text_length         = 0;
    textbox->buffer_position     = 0;
    textbox->scroll_offset       = 0;

    if(nocterm_widget_add_key_handler(NOCTERM_WIDGET(textbox), nocterm_textbox_key_handler) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }
    if(nocterm_widget_add_focus_handler(NOCTERM_WIDGET(textbox), nocterm_textbox_focus_handler) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    NOCTERM_WIDGET(textbox)->internal_resize_handler = nocterm_textbox_internal_resize_handler;

    return NOCTERM_SUCCESS;
}

int nocterm_textbox_destructor(nocterm_textbox_t* textbox){

    if(textbox == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(nocterm_widget_destructor(NOCTERM_WIDGET(textbox)) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    return NOCTERM_SUCCESS;
}

int nocterm_textbox_delete(nocterm_textbox_t* textbox){

    if(textbox == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(nocterm_textbox_destructor(textbox) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    free(textbox);

    return NOCTERM_SUCCESS;
}

int nocterm_textbox_cursor_move_left(nocterm_textbox_t* textbox){

    if(textbox == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(textbox->buffer_position == 0){
        return NOCTERM_SUCCESS;
    }

    textbox->buffer_position--;
    nocterm_textbox_clamp_scroll(textbox);
    nocterm_textbox_render(textbox);

    return NOCTERM_SUCCESS;
}

int nocterm_textbox_cursor_move_right(nocterm_textbox_t* textbox){

    if(textbox == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(textbox->buffer_position >= textbox->text_length){
        return NOCTERM_SUCCESS;
    }

    textbox->buffer_position++;
    nocterm_textbox_clamp_scroll(textbox);
    nocterm_textbox_render(textbox);

    return NOCTERM_SUCCESS;
}

int nocterm_textbox_cursor_move_up(nocterm_textbox_t* textbox){

    if(textbox == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    nocterm_widget_t* w = NOCTERM_WIDGET(textbox);
    if(w->bounds.width == 0) return NOCTERM_SUCCESS;

    uint64_t width = (uint64_t)w->bounds.width;

    if(textbox->buffer_position < width){
        // Already on the first visual line; cannot go higher.
        return NOCTERM_SUCCESS;
    }

    textbox->buffer_position -= width;
    nocterm_textbox_clamp_scroll(textbox);
    nocterm_textbox_render(textbox);

    return NOCTERM_SUCCESS;
}

int nocterm_textbox_cursor_move_down(nocterm_textbox_t* textbox){

    if(textbox == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    nocterm_widget_t* w = NOCTERM_WIDGET(textbox);
    if(w->bounds.width == 0) return NOCTERM_SUCCESS;

    uint64_t width   = (uint64_t)w->bounds.width;
    uint64_t new_pos = textbox->buffer_position + width;

    // Clamp to the end of the buffer so the cursor never goes past text_length.
    if(new_pos > textbox->text_length){
        new_pos = textbox->text_length;
    }

    if(new_pos == textbox->buffer_position){
        return NOCTERM_SUCCESS;
    }

    textbox->buffer_position = new_pos;
    nocterm_textbox_clamp_scroll(textbox);
    nocterm_textbox_render(textbox);

    return NOCTERM_SUCCESS;
}

int nocterm_textbox_cursor_insert(nocterm_textbox_t* textbox, nocterm_char_t character){

    if(textbox == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(textbox->text_length >= NOCTERM_TEXTBOX_BUFFER_MAX_SIZE){
        return NOCTERM_SUCCESS;
    }

    // Shift everything from buffer_position onward one step to the right.
    for(uint64_t i = textbox->text_length; i > textbox->buffer_position; i--){
        textbox->text_store[i] = textbox->text_store[i - 1];
    }

    textbox->text_store[textbox->buffer_position] = character;
    textbox->text_length++;
    textbox->buffer_position++;

    nocterm_textbox_clamp_scroll(textbox);
    nocterm_textbox_render(textbox);

    return NOCTERM_SUCCESS;
}

int nocterm_textbox_cursor_erase_left(nocterm_textbox_t* textbox){

    if(textbox == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(textbox->text_length == 0 || textbox->buffer_position == 0){
        return NOCTERM_SUCCESS;
    }

    // Shift everything from buffer_position onward one step to the left,
    // effectively erasing the character immediately before the cursor.
    for(uint64_t i = textbox->buffer_position - 1; i < textbox->text_length - 1; i++){
        textbox->text_store[i] = textbox->text_store[i + 1];
    }

    textbox->text_store[textbox->text_length - 1] = NOCTERM_CHAR_EMPTY;
    textbox->text_length--;
    textbox->buffer_position--;

    nocterm_textbox_clamp_scroll(textbox);
    nocterm_textbox_render(textbox);

    return NOCTERM_SUCCESS;
}

int nocterm_textbox_cursor_erase_right(nocterm_textbox_t* textbox){

    if(textbox == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(textbox->buffer_position >= textbox->text_length){
        return NOCTERM_SUCCESS;
    }

    // Shift everything after buffer_position one step to the left,
    // effectively erasing the character at the cursor.
    for(uint64_t i = textbox->buffer_position; i < textbox->text_length - 1; i++){
        textbox->text_store[i] = textbox->text_store[i + 1];
    }

    textbox->text_store[textbox->text_length - 1] = NOCTERM_CHAR_EMPTY;
    textbox->text_length--;

    // buffer_position is unchanged; scroll does not need adjustment.
    nocterm_textbox_render(textbox);

    return NOCTERM_SUCCESS;
}

int nocterm_textbox_set_attribute(nocterm_textbox_t* textbox, nocterm_attribute_t text, nocterm_attribute_t cursor){

    if(textbox == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&NOCTERM_WIDGET(textbox)->lock);

    textbox->normal_attribute = text;
    textbox->cursor_attribute = cursor;
    nocterm_textbox_render(textbox);

    pthread_mutex_unlock(&NOCTERM_WIDGET(textbox)->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_textbox_get_text(nocterm_textbox_t* textbox, char* buffer, uint64_t buffer_size, uint64_t* text_length){

    if(textbox == NULL || buffer == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&NOCTERM_WIDGET(textbox)->lock);

    if(textbox->text_length == 0){
        if(text_length) *text_length = 0;
        pthread_mutex_unlock(&NOCTERM_WIDGET(textbox)->lock);
        return NOCTERM_SUCCESS;
    }

    uint64_t length = nocterm_char_string_to_stream(buffer, buffer_size, textbox->text_store, textbox->text_length);
    if(length == 0){
        pthread_mutex_unlock(&NOCTERM_WIDGET(textbox)->lock);
        return NOCTERM_FAILURE;
    }
    if(text_length) *text_length = length;

    pthread_mutex_unlock(&NOCTERM_WIDGET(textbox)->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_textbox_set_text(nocterm_textbox_t* textbox, const char* buffer, uint64_t buffer_size){

    if(textbox == NULL || buffer == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&NOCTERM_WIDGET(textbox)->lock);

    memset(textbox->text_store, 0x0, sizeof(textbox->text_store));

    uint64_t len = nocterm_char_string_from_stream(
        textbox->text_store, NOCTERM_TEXTBOX_BUFFER_MAX_SIZE, buffer, buffer_size);

    if(len == 0){
        pthread_mutex_unlock(&NOCTERM_WIDGET(textbox)->lock);
        return NOCTERM_FAILURE;
    }

    textbox->text_length     = len;
    textbox->buffer_position = len; // Place cursor after the last character.
    textbox->scroll_offset   = 0;

    nocterm_textbox_clamp_scroll(textbox);
    nocterm_textbox_render(textbox);

    pthread_mutex_unlock(&NOCTERM_WIDGET(textbox)->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_textbox_clear(nocterm_textbox_t* textbox){

    if(textbox == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&NOCTERM_WIDGET(textbox)->lock);

    memset(textbox->text_store, 0x0, sizeof(textbox->text_store));
    textbox->text_length     = 0;
    textbox->buffer_position = 0;
    textbox->scroll_offset   = 0;

    nocterm_widget_clear(NOCTERM_WIDGET(textbox));
    NOCTERM_WIDGET(textbox)->hard_refresh = true;

    pthread_mutex_unlock(&NOCTERM_WIDGET(textbox)->lock);

    return NOCTERM_SUCCESS;
}

NOCTERM_WIDGET_KEY_HANDLER(nocterm_textbox_key_handler){
    nocterm_textbox_t* textbox = NOCTERM_TEXTBOX(self);

    pthread_mutex_lock(&self->lock);

    if(self->bounds.width == 0 || self->bounds.height == 0){
        pthread_mutex_unlock(&self->lock);
        return;
    }

    switch(nocterm_key_translate(key)){

        case NOCTERM_KEY_EVENT_LEFT:{
            nocterm_textbox_cursor_move_left(textbox);
        }break;

        case NOCTERM_KEY_EVENT_RIGHT:{
            nocterm_textbox_cursor_move_right(textbox);
        }break;

        case NOCTERM_KEY_EVENT_UP:{
            nocterm_textbox_cursor_move_up(textbox);
        }break;

        case NOCTERM_KEY_EVENT_DOWN:{
            nocterm_textbox_cursor_move_down(textbox);
        }break;

        case NOCTERM_KEY_EVENT_PRINTABLE:{
            nocterm_char_t temp = {0};
            memcpy(temp.bytes, key->buffer, key->buffer_length);
            temp.bytes_size = key->buffer_length;
            temp.is_utf8    = (key->buffer_length > 1 ? true : false);
            nocterm_textbox_cursor_insert(textbox, temp);
        }break;

        case NOCTERM_KEY_EVENT_BACKSPACE:{
            nocterm_textbox_cursor_erase_left(textbox);
        }break;

        case NOCTERM_KEY_EVENT_DELETE:{
            nocterm_textbox_cursor_erase_right(textbox);
        }break;

        default:
            break;
    }

    pthread_mutex_unlock(&self->lock);
}

NOCTERM_WIDGET_FOCUS_HANDLER(nocterm_textbox_focus_handler){
    nocterm_textbox_t* textbox = NOCTERM_TEXTBOX(self);
    if(self->bounds.width == 0 || self->bounds.height == 0) return;

    uint64_t width      = (uint64_t)self->bounds.width;
    uint64_t char_start = textbox->scroll_offset * width;

    // Only act if the cursor cell is currently visible in the viewport.
    if(textbox->buffer_position < char_start ||
       textbox->buffer_position >= char_start + (uint64_t)self->bounds.height * width){
        return;
    }

    uint64_t i = textbox->buffer_position - char_start;

    switch(focus){
        case NOCTERM_WIDGET_FOCUS_ENTER:{
            nocterm_char_t ch = (textbox->buffer_position < textbox->text_length)
                ? textbox->text_store[textbox->buffer_position]
                : NOCTERM_TEXTBOX_CURSOR_CHAR;
            nocterm_widget_update(self, 0, (nocterm_dimension_size_t)i, ch, textbox->cursor_attribute);
        }break;

        case NOCTERM_WIDGET_FOCUS_LEAVE:{
            nocterm_char_t ch   = (textbox->buffer_position < textbox->text_length)
                ? textbox->text_store[textbox->buffer_position]
                : NOCTERM_CHAR_EMPTY;
            nocterm_attribute_t attr = (textbox->buffer_position < textbox->text_length)
                ? textbox->normal_attribute
                : NOCTERM_ATTRIBUTE_EMPTY;
            nocterm_widget_update(self, 0, (nocterm_dimension_size_t)i, ch, attr);
        }break;
    }
}

NOCTERM_WIDGET_RESIZE_HANDLER(nocterm_textbox_internal_resize_handler){
    nocterm_textbox_t* textbox = NOCTERM_TEXTBOX(self);
    if(self->bounds.width == 0 || self->bounds.height == 0) return;

    uint64_t width  = (uint64_t)self->bounds.width;
    uint64_t height = (uint64_t)self->bounds.height;

    if(textbox->text_length > 0){
        // total_lines counts the cursor-at-end line so it equals text_length/width + 1.
        uint64_t total_lines = textbox->text_length / width + 1;
        if(total_lines <= height){
            textbox->scroll_offset = 0;
        }else if(textbox->scroll_offset + height > total_lines){
            textbox->scroll_offset = total_lines - height;
        }
    }else{
        textbox->scroll_offset = 0;
    }

    nocterm_textbox_clamp_scroll(textbox);
    nocterm_textbox_render(textbox);
}
