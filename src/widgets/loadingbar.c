#include <nocterm/widgets/loadingbar.h>

nocterm_loadingbar_t* nocterm_loadingbar_new(nocterm_dimension_size_t row, nocterm_dimension_size_t col, uint64_t interval){

    nocterm_loadingbar_t* new_loadingbar = (nocterm_loadingbar_t*)malloc(sizeof(nocterm_loadingbar_t));

    if(new_loadingbar == NULL){
        return NULL;
    }

    memset(new_loadingbar, 0x0, sizeof(nocterm_loadingbar_t));

    if(nocterm_loadingbar_constructor(new_loadingbar, row, col, interval) == NOCTERM_FAILURE){
        free(new_loadingbar);
        return NULL;
    }

    return new_loadingbar;
}

int nocterm_loadingbar_constructor(nocterm_loadingbar_t* loadingbar, nocterm_dimension_size_t row, nocterm_dimension_size_t col, uint64_t interval){
    
    if(loadingbar == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(nocterm_widget_constructor(NOCTERM_WIDGET(loadingbar),(nocterm_dimension_t){row,col,1,1}, false, false) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    loadingbar->state = 0;
    loadingbar->timer = nocterm_timer_create(NOCTERM_WIDGET(&(loadingbar->widget)), interval, nocterm_loadingbar_timer_callback, NULL);
    loadingbar->attribute = NOCTERM_ATTRIBUTE_EMPTY;

    nocterm_char_t ch = {.bytes = {0}, .bytes_size = 3, .is_utf8 = true};
    memcpy(ch.bytes, "|", 1);

    if(loadingbar->timer == NULL){
        return NOCTERM_FAILURE;
    }

    nocterm_widget_update(NOCTERM_WIDGET(loadingbar), 0, 0, ch, loadingbar->attribute);

    return NOCTERM_SUCCESS;
}

int nocterm_loadingbar_destructor(nocterm_loadingbar_t* loadingbar){
    if(loadingbar == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(nocterm_timer_delete(loadingbar->timer) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    if(nocterm_widget_destructor(NOCTERM_WIDGET(loadingbar)) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    return NOCTERM_SUCCESS;
}

int nocterm_loadingbar_delete(nocterm_loadingbar_t* loadingbar){

    if(loadingbar == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(nocterm_loadingbar_destructor(loadingbar) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    free(loadingbar);

    return NOCTERM_SUCCESS;
}

int nocterm_loadingbar_set_attribute(nocterm_loadingbar_t* loadingbar, nocterm_attribute_t attribute){

    if(loadingbar == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }
    
    pthread_mutex_lock(&NOCTERM_WIDGET(loadingbar)->lock);

    loadingbar->attribute = attribute;
    nocterm_widget_update(NOCTERM_WIDGET(loadingbar), 0, 0, NOCTERM_WIDGET(loadingbar)->buffer[0].character, attribute);

    pthread_mutex_unlock(&NOCTERM_WIDGET(loadingbar)->lock);


    return NOCTERM_SUCCESS;
}

int nocterm_loadingbar_enable(nocterm_loadingbar_t* loadingbar){
    return nocterm_timer_start(loadingbar->timer);
}

int nocterm_loadingbar_disable(nocterm_loadingbar_t* loadingbar){
    return nocterm_timer_stop(loadingbar->timer);
}

NOCTERM_TIMER_CALLBACK(nocterm_loadingbar_timer_callback){

    nocterm_loadingbar_t* loadingbar = NOCTERM_LOADINGBAR(widget);

    nocterm_char_t ch = {0};
    ch.bytes_size = 3;
    ch.is_utf8 = true;

    switch(loadingbar->state){
        case 0:{
            memcpy(ch.bytes, "|", 1);
        }break;

        case 1:{
            memcpy(ch.bytes, "/", 1);

        }break;

        case 2:{
            memcpy(ch.bytes, "-", 1);

        }break;

        case 3:{
            memcpy(ch.bytes, "\\", 1);

        }break;
    }
    
    nocterm_widget_update(widget, 0, 0, ch, loadingbar->attribute);
    loadingbar->state = (loadingbar->state + 1) % 4;
}
