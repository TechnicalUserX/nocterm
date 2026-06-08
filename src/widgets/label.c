#include <nocterm/widgets/label.h>

nocterm_label_t* nocterm_label_new(const char* text, uint64_t text_size){

    if(text == NULL || text_size == 0){
        return NULL;
    }

    nocterm_label_t* new_label = (nocterm_label_t*)malloc(sizeof(nocterm_label_t));

    if(new_label == NULL){
        return NULL;
    }

    memset(new_label, 0x0, sizeof(nocterm_label_t));

    if(nocterm_label_constructor(new_label, text, text_size) == NOCTERM_FAILURE){
        free(new_label);
        return NULL;
    }

    return new_label;
}

int nocterm_label_constructor(nocterm_label_t* label, const char* text, uint64_t text_size){

    if(label == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    // Giving width as text_size considers the worst case
    if(nocterm_widget_constructor(NOCTERM_WIDGET(label), 1, text_size-1, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_REAL) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }
    nocterm_widget_size_policy_set_permission(NOCTERM_WIDGET(label), NOCTERM_WIDGET_SIZE_POLICY_PERMISSION_NONE);

    nocterm_char_t label_string[text_size];
    uint64_t len = nocterm_char_string_from_stream(label_string, text_size, text, text_size);

    if(len == 0){
        return NOCTERM_FAILURE;
    }

    for(uint64_t i = 0; i < len; i++){
        nocterm_widget_update(NOCTERM_WIDGET(label),0,i,label_string[i], NOCTERM_ATTRIBUTE_EMPTY);
    }

    return NOCTERM_SUCCESS;
}

int nocterm_label_set_attribute(nocterm_label_t* label, nocterm_attribute_t attribute){

    if(label == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&NOCTERM_WIDGET(label)->lock);

    for(nocterm_dimension_size_t i = 0; i < NOCTERM_WIDGET(label)->buffer_size; i++){
        nocterm_widget_update(NOCTERM_WIDGET(label), 0, i, NOCTERM_WIDGET(label)->buffer[i].character, attribute);
    }

    pthread_mutex_unlock(&NOCTERM_WIDGET(label)->lock);

    return NOCTERM_SUCCESS;    
}

int nocterm_label_destructor(nocterm_label_t* label){

    if(label == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(nocterm_widget_destructor(NOCTERM_WIDGET(label)) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    return NOCTERM_SUCCESS;
}

int nocterm_label_delete(nocterm_label_t* label){

    if(label == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(nocterm_label_destructor(label) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    free(label);

    return NOCTERM_SUCCESS;
}
