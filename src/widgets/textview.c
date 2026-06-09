#include <nocterm/widgets/textview.h>

NOCTERM_INTERNAL NOCTERM_WIDGET_RESIZE_HANDLER(nocterm_textview_internal_resize_handler);

NOCTERM_INTERNAL NOCTERM_WIDGET_KEY_HANDLER(nocterm_textview_key_handler);

// Render text_buffer into the widget buffer starting at scroll_offset lines down.
// Called with widget lock already held (or from resize handler which holds it too).
NOCTERM_INTERNAL void nocterm_textview_render(nocterm_textview_t* textview){
    nocterm_widget_t* w = NOCTERM_WIDGET(textview);
    if(w->bounds.width == 0 || w->bounds.height == 0) return;

    uint64_t width        = (uint64_t)w->bounds.width;
    uint64_t visible_chars = (uint64_t)w->bounds.height * width;
    uint64_t char_start   = textview->scroll_offset * width;

    for(uint64_t i = 0; i < visible_chars; i++){
        uint64_t src = char_start + i;
        nocterm_char_t ch = (src < textview->text_length)
            ? textview->text_buffer[src]
            : NOCTERM_CHAR_EMPTY;
        nocterm_widget_update(w, 0, (nocterm_dimension_size_t)i, ch, textview->attribute);
    }

    w->hard_refresh = true;
}

nocterm_textview_t* nocterm_textview_new(nocterm_dimension_size_t height, nocterm_dimension_size_t width){

    nocterm_textview_t* new_textview = (nocterm_textview_t*)malloc(sizeof(nocterm_textview_t));

    if(new_textview == NULL){
        return NULL;
    }

    memset(new_textview, 0x0, sizeof(nocterm_textview_t));

    if(nocterm_textview_constructor(new_textview, height, width) == NOCTERM_FAILURE){
        free(new_textview);
        return NULL;
    }

    return new_textview;
}

int nocterm_textview_constructor(nocterm_textview_t* textview, nocterm_dimension_size_t height, nocterm_dimension_size_t width){

    if(textview == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(nocterm_widget_constructor(NOCTERM_WIDGET(textview), height, width, true, false) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    nocterm_widget_flex_policy_set_permission(NOCTERM_WIDGET(textview), NOCTERM_WIDGET_FLEX_POLICY_PERMISSION_BOTH);

    textview->attribute    = NOCTERM_ATTRIBUTE_EMPTY;
    textview->text_length  = 0;
    textview->scroll_offset = 0;

    nocterm_widget_add_key_handler(NOCTERM_WIDGET(textview), nocterm_textview_key_handler);

    NOCTERM_WIDGET(textview)->internal_resize_handler = nocterm_textview_internal_resize_handler;

    return NOCTERM_SUCCESS;
}

int nocterm_textview_destructor(nocterm_textview_t* textview){

    if(textview == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(nocterm_widget_destructor(NOCTERM_WIDGET(textview)) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    return NOCTERM_SUCCESS;
}

int nocterm_textview_delete(nocterm_textview_t* textview){

    if(textview == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(nocterm_textview_destructor(textview) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    free(textview);

    return NOCTERM_SUCCESS;
}

int nocterm_textview_set_text(nocterm_textview_t* textview, const char* text, uint64_t text_size){

    if(textview == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&NOCTERM_WIDGET(textview)->lock);

    uint64_t len = nocterm_char_string_from_stream(
        textview->text_buffer, NOCTERM_TEXTVIEW_BUFFER_SIZE, text, text_size);

    if(len == 0){
        pthread_mutex_unlock(&NOCTERM_WIDGET(textview)->lock);
        return NOCTERM_FAILURE;
    }

    textview->text_length   = len;
    textview->scroll_offset = 0;

    nocterm_textview_render(textview);

    pthread_mutex_unlock(&NOCTERM_WIDGET(textview)->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_textview_set_attribute(nocterm_textview_t* textview, nocterm_attribute_t attribute){

    if(textview == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&NOCTERM_WIDGET(textview)->lock);

    textview->attribute = attribute;
    nocterm_textview_render(textview);

    pthread_mutex_unlock(&NOCTERM_WIDGET(textview)->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_textview_print_text(nocterm_textview_t* textview, nocterm_dimension_size_t row, nocterm_dimension_size_t col, const char* text, uint64_t text_size){

    if(textview == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&NOCTERM_WIDGET(textview)->lock);

    uint64_t offset = (uint64_t)row * (uint64_t)NOCTERM_WIDGET(textview)->bounds.width + (uint64_t)col;

    if(offset >= NOCTERM_TEXTVIEW_BUFFER_SIZE){
        pthread_mutex_unlock(&NOCTERM_WIDGET(textview)->lock);
        return NOCTERM_FAILURE;
    }

    uint64_t avail = NOCTERM_TEXTVIEW_BUFFER_SIZE - offset;
    uint64_t len = nocterm_char_string_from_stream(
        textview->text_buffer + offset, avail, text, text_size);

    if(len == 0){
        pthread_mutex_unlock(&NOCTERM_WIDGET(textview)->lock);
        return NOCTERM_FAILURE;
    }

    if(offset + len > textview->text_length){
        textview->text_length = offset + len;
    }

    nocterm_textview_render(textview);

    pthread_mutex_unlock(&NOCTERM_WIDGET(textview)->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_textview_clear(nocterm_textview_t* textview){

    if(textview == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&NOCTERM_WIDGET(textview)->lock);

    memset(textview->text_buffer, 0x0, sizeof(textview->text_buffer));
    textview->text_length   = 0;
    textview->scroll_offset = 0;

    nocterm_widget_clear(NOCTERM_WIDGET(textview));
    NOCTERM_WIDGET(textview)->hard_refresh = true;

    pthread_mutex_unlock(&NOCTERM_WIDGET(textview)->lock);

    return NOCTERM_SUCCESS;
}

NOCTERM_WIDGET_KEY_HANDLER(nocterm_textview_key_handler){
    nocterm_textview_t* textview = NOCTERM_TEXTVIEW(self);

    pthread_mutex_lock(&self->lock);

    if(self->bounds.width == 0 || textview->text_length == 0){
        pthread_mutex_unlock(&self->lock);
        return;
    }

    uint64_t width       = (uint64_t)self->bounds.width;
    uint64_t total_lines = (textview->text_length + width - 1) / width;
    uint64_t visible     = (uint64_t)self->bounds.height;

    switch(nocterm_key_translate(key)){

        case NOCTERM_KEY_EVENT_UP:{
            if(textview->scroll_offset > 0){
                textview->scroll_offset--;
                nocterm_textview_render(textview);
            }
        }break;

        case NOCTERM_KEY_EVENT_DOWN:{
            if(total_lines > visible && textview->scroll_offset + visible < total_lines){
                textview->scroll_offset++;
                nocterm_textview_render(textview);
            }
        }break;

        default:
            break;
    }

    pthread_mutex_unlock(&self->lock);
}

NOCTERM_WIDGET_RESIZE_HANDLER(nocterm_textview_internal_resize_handler){
    nocterm_textview_t* textview = NOCTERM_TEXTVIEW(self);

    if(self->bounds.width == 0 || self->bounds.height == 0) return;

    if(textview->text_length > 0){
        uint64_t width       = (uint64_t)self->bounds.width;
        uint64_t total_lines = (textview->text_length + width - 1) / width;
        uint64_t visible     = (uint64_t)self->bounds.height;

        if(total_lines <= visible){
            textview->scroll_offset = 0;
        } else if(textview->scroll_offset + visible > total_lines){
            textview->scroll_offset = total_lines - visible;
        }
    } else {
        textview->scroll_offset = 0;
    }

    nocterm_textview_render(textview);
}
