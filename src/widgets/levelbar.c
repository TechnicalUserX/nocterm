#include <nocterm/widgets/levelbar.h>

int nocterm_widget_flex_policy_set_permission(nocterm_widget_t* widget, nocterm_widget_flex_policy_permission_t permission);


NOCTERM_INTERNAL NOCTERM_WIDGET_RESIZE_HANDLER(nocterm_levelbar_internal_resize_handler);

nocterm_levelbar_t* nocterm_levelbar_new(uint64_t length, uint64_t min_value, uint64_t max_value, nocterm_levelbar_type_t type, bool flip){

    if(length == 0){
        return NULL;
    }

    nocterm_levelbar_t* new_levelbar = (nocterm_levelbar_t*)malloc(sizeof(nocterm_levelbar_t));

    if(new_levelbar == NULL){
        return NULL;
    }

    memset(new_levelbar, 0x0, sizeof(nocterm_levelbar_t));

    if(nocterm_levelbar_constructor(new_levelbar, length, min_value, max_value, type, flip) == NOCTERM_FAILURE){
        free(new_levelbar);
        return NULL;
    }

    return new_levelbar;    
}

int nocterm_levelbar_constructor(nocterm_levelbar_t* levelbar, uint64_t length, uint64_t min_value, uint64_t max_value, nocterm_levelbar_type_t type, bool flip){

    if(levelbar == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(min_value > max_value){
        return NOCTERM_FAILURE;
    }
    
    nocterm_dimension_size_t height = (type == NOCTERM_LEVELBAR_TYPE_HORIZONTAL) ? 1 : length;
    nocterm_dimension_size_t width = (type == NOCTERM_LEVELBAR_TYPE_VERTICAL) ? 1 : length;

    if(nocterm_widget_constructor(NOCTERM_WIDGET(levelbar), height, width, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_REAL) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    nocterm_widget_flex_policy_permission_t levelbar_permission =
        (type == NOCTERM_LEVELBAR_TYPE_HORIZONTAL)
            ? NOCTERM_WIDGET_FLEX_POLICY_PERMISSION_HORIZONTAL
            : NOCTERM_WIDGET_FLEX_POLICY_PERMISSION_VERTICAL;
    nocterm_widget_flex_policy_set_permission(NOCTERM_WIDGET(levelbar), levelbar_permission);

    levelbar->type = type;
    levelbar->flip = flip;
    levelbar->length = length;
    levelbar->min_value = min_value;
    levelbar->max_value = max_value;
    levelbar->current_value = min_value;

    levelbar->attribute = NOCTERM_ATTRIBUTE_EMPTY;

    NOCTERM_WIDGET(levelbar)->internal_resize_handler = nocterm_levelbar_internal_resize_handler;

    nocterm_char_t levelbar_default_character = {
        .bytes = {'#'},
        .bytes_size = 1,
        .is_utf8 = false
    };

    levelbar->character = levelbar_default_character;

    return NOCTERM_SUCCESS;
}

int nocterm_levelbar_destructor(nocterm_levelbar_t* levelbar){
    if(levelbar == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(nocterm_widget_destructor(NOCTERM_WIDGET(levelbar)) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    return NOCTERM_SUCCESS;
}

int nocterm_levelbar_delete(nocterm_levelbar_t* levelbar){
    if(levelbar == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(nocterm_levelbar_destructor(levelbar) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    free(levelbar);

    return NOCTERM_SUCCESS;    
}

int nocterm_levelbar_set_value(nocterm_levelbar_t* levelbar, uint64_t value){

    if(levelbar == NULL || value < levelbar->min_value || value > levelbar->max_value){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&NOCTERM_WIDGET(levelbar)->lock);

    nocterm_dimension_size_t mapped_magnitude_new = (levelbar->length * (value - levelbar->min_value) ) / (levelbar->max_value - levelbar->min_value);
    nocterm_dimension_size_t mapped_magnitude_old = (levelbar->length * (levelbar->current_value - levelbar->min_value) ) / (levelbar->max_value - levelbar->min_value);

    // For efficiency we can update individual cells, clearing the whole widget is an inefficient process

    if(levelbar->flip){

        if(mapped_magnitude_new > mapped_magnitude_old){

            for(nocterm_dimension_size_t i = mapped_magnitude_old; i < mapped_magnitude_new; i++){
                nocterm_widget_update(NOCTERM_WIDGET(levelbar), 0, (levelbar->length - 1) - i, levelbar->character, levelbar->attribute);
            }

        }else if (mapped_magnitude_new < mapped_magnitude_old){
            for(nocterm_dimension_size_t i = 0; i < mapped_magnitude_old - mapped_magnitude_new; i++){
                nocterm_widget_update(NOCTERM_WIDGET(levelbar), 0, (levelbar->length - 1) - (mapped_magnitude_old - i - 1), NOCTERM_CHAR_EMPTY, NOCTERM_ATTRIBUTE_EMPTY);
            }
        }
        // Else, no update is required

    }else{

        if(mapped_magnitude_new > mapped_magnitude_old){

            for(nocterm_dimension_size_t i = mapped_magnitude_old; i < mapped_magnitude_new; i++){
                nocterm_widget_update(NOCTERM_WIDGET(levelbar), 0, i, levelbar->character, levelbar->attribute);
            }

        }else if (mapped_magnitude_new < mapped_magnitude_old){

            for(nocterm_dimension_size_t i = 0; i < mapped_magnitude_old - mapped_magnitude_new; i++){
                nocterm_widget_update(NOCTERM_WIDGET(levelbar), 0, mapped_magnitude_old - 1 - i, NOCTERM_CHAR_EMPTY, NOCTERM_ATTRIBUTE_EMPTY);
            }

        }
        // Else, no update is required

    }

    levelbar->current_value = value;

    pthread_mutex_unlock(&NOCTERM_WIDGET(levelbar)->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_levelbar_get_value(nocterm_levelbar_t* levelbar, uint64_t* value){

    if(levelbar == NULL || value == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    *value = levelbar->current_value;

    return NOCTERM_SUCCESS;
}

int nocterm_levelbar_set_attribute(nocterm_levelbar_t* levelbar, nocterm_attribute_t attribute){

    if(levelbar == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&NOCTERM_WIDGET(levelbar)->lock);

    nocterm_dimension_size_t mapped_magnitude = (levelbar->length * (levelbar->current_value - levelbar->min_value) ) / (levelbar->max_value - levelbar->min_value);

    levelbar->attribute = attribute;

    for(nocterm_dimension_size_t i = 0; i < mapped_magnitude; i++){
        nocterm_widget_update(NOCTERM_WIDGET(levelbar), 0, i, NOCTERM_WIDGET(levelbar)->buffer[i].character, attribute);
    }

    pthread_mutex_unlock(&NOCTERM_WIDGET(levelbar)->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_levelbar_set_character(nocterm_levelbar_t* levelbar, nocterm_char_t character){

    if(levelbar == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&NOCTERM_WIDGET(levelbar)->lock);

    levelbar->character = character;

    nocterm_dimension_size_t mapped_magnitude = (levelbar->length * (levelbar->current_value - levelbar->min_value) ) / (levelbar->max_value - levelbar->min_value);

    for(nocterm_dimension_size_t i = 0; i < mapped_magnitude; i++){
        nocterm_widget_update(NOCTERM_WIDGET(levelbar), 0, i, character, NOCTERM_WIDGET(levelbar)->buffer[i].attribute);
    }

    pthread_mutex_unlock(&NOCTERM_WIDGET(levelbar)->lock);

    return NOCTERM_SUCCESS;
}

NOCTERM_WIDGET_RESIZE_HANDLER(nocterm_levelbar_internal_resize_handler){
    nocterm_levelbar_t* levelbar = NOCTERM_LEVELBAR(self);

    // The flex dimension becomes the new bar length — levelbar scales to fill its space.
    levelbar->length = (levelbar->type == NOCTERM_LEVELBAR_TYPE_HORIZONTAL)
        ? bounds.width
        : bounds.height;

    if(levelbar->length == 0 || levelbar->max_value == levelbar->min_value) return;

    nocterm_dimension_size_t mapped = (nocterm_dimension_size_t)(
        (levelbar->length * (levelbar->current_value - levelbar->min_value))
        / (levelbar->max_value - levelbar->min_value));

    for(nocterm_dimension_size_t i = 0; i < mapped; i++){
        nocterm_dimension_size_t pos = levelbar->flip ? (levelbar->length - 1) - i : i;
        nocterm_widget_update(self, 0, pos, levelbar->character, levelbar->attribute);
    }
}
