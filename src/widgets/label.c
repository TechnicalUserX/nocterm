#include <nocterm/widgets/label.h>

nocterm_label_t* nocterm_label_new(nocterm_dimension_size_t row, nocterm_dimension_size_t col, const char* text, uint64_t text_size,  nocterm_attribute_t attribute){

    if(text == NULL || text_size == 0){
        return NULL;
    }

    nocterm_label_t* new_label = (nocterm_label_t*)malloc(sizeof(nocterm_label_t));

    if(new_label == NULL){
        return NULL;
    }

    // Giving width as text_size considers the worst case
    if(nocterm_widget_constructor(NOCTERM_WIDGET(new_label),(nocterm_dimension_t){row, col, 1, text_size-1}, false, false) == NOCTERM_FAILURE){
        return NULL;
    }

    nocterm_char_t label_string[text_size];
    uint64_t len = nocterm_char_string_from_stream(label_string, text_size, text, text_size);

    if(len == 0){
        return NULL;
    }

    for(uint64_t i = 0; i < len; i++){
        nocterm_widget_update(NOCTERM_WIDGET(new_label),0,i,label_string[i],attribute);
    }

    return new_label;
}

int nocterm_label_delete(nocterm_label_t* label){

    if(nocterm_widget_destructor(NOCTERM_WIDGET(label)) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    free(label);

    return NOCTERM_SUCCESS;
}
