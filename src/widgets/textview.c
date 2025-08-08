#include <nocterm/widgets/textview.h>

nocterm_textview_t* nocterm_textview_new(nocterm_dimension_t bounds, nocterm_attribute_t attribute){

    nocterm_textview_t* new_textview = (nocterm_textview_t*)malloc(sizeof(nocterm_textview_t));

    if(new_textview == NULL){
        return NULL;
    }

    memset(new_textview, 0x0, sizeof(nocterm_textview_t));

    if(nocterm_widget_constructor(NOCTERM_WIDGET(new_textview), bounds, false, false) == NOCTERM_FAILURE){
        return NULL;
    }

    new_textview->attribute = attribute;

    return new_textview;
}

int nocterm_textview_delete(nocterm_textview_t* textview){

    if(nocterm_widget_destructor(NOCTERM_WIDGET(textview)) == NOCTERM_FAILURE){
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

    nocterm_char_t new_text[text_size];
    memset(new_text, 0x0, sizeof(nocterm_char_t)*text_size);

    uint64_t new_text_length = nocterm_char_string_from_stream(new_text, text_size, text, text_size);

    if(new_text_length == 0){
        return NOCTERM_FAILURE;
    }

    nocterm_textview_clear(textview);


    for(uint64_t i = 0; i < new_text_length && i < NOCTERM_WIDGET(textview)->buffer_size; i++){
        nocterm_widget_update(NOCTERM_WIDGET(textview), 0, i, new_text[i], textview->attribute);      
    }

    NOCTERM_WIDGET(textview)->hard_refresh = true;

    return NOCTERM_SUCCESS;
}

int nocterm_textview_set_attribute(nocterm_textview_t* textview, nocterm_attribute_t attribute){
    if(textview == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    textview->attribute = attribute;

    return NOCTERM_SUCCESS;
}

int nocterm_textview_print_text(nocterm_textview_t* textview, nocterm_dimension_size_t row, nocterm_dimension_size_t col, const char* text, uint64_t text_size){

    if(textview == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    nocterm_char_t new_text[NOCTERM_WIDGET(textview)->buffer_size];

    uint64_t new_text_length = nocterm_char_string_from_stream(new_text, NOCTERM_WIDGET(textview)->buffer_size, text, text_size);

    if(new_text_length == 0){
        return NOCTERM_FAILURE;
    }

    uint64_t offset = row * NOCTERM_WIDGET(textview)->bounds.width + col;

    for(uint64_t i = 0; i < new_text_length && i + offset < NOCTERM_WIDGET(textview)->buffer_size; i++){
        nocterm_widget_update(NOCTERM_WIDGET(textview), 0, offset + i, new_text[i], textview->attribute);      
    }

    // No hard refresh
    
    return NOCTERM_SUCCESS;
}

int nocterm_textview_clear(nocterm_textview_t* textview){
    if(textview == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    nocterm_widget_clear(NOCTERM_WIDGET(textview));

    return NOCTERM_SUCCESS;
}
