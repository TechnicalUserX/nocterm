#include <nocterm/widgets/loadingbar.h>

nocterm_loadingbar_t* nocterm_loadingbar_new(nocterm_dimension_size_t row, nocterm_dimension_size_t col, uint64_t interval, nocterm_attribute_t attribute){

    nocterm_loadingbar_t* new_loadingbar = (nocterm_loadingbar_t*)malloc(sizeof(nocterm_loadingbar_t));

    if(new_loadingbar == NULL){
        return NULL;
    }

    if(nocterm_widget_constructor(NOCTERM_WIDGET(new_loadingbar),(nocterm_dimension_t){row,col,1,1}, false, false) == NOCTERM_FAILURE){
        return NULL;
    }

    new_loadingbar->state = 0;
    new_loadingbar->timer = nocterm_timer_create(NOCTERM_WIDGET(&(new_loadingbar->widget)), interval, nocterm_loadingbar_timer_callback, NULL);
    new_loadingbar->attribute = attribute;

    nocterm_char_t ch = {.bytes = {0}, .bytes_size = 3, .is_utf8 = true};
    memcpy(ch.bytes, "|", 1);

    if(new_loadingbar->timer == NULL){
        return NULL;
    }

    nocterm_widget_update(NOCTERM_WIDGET(new_loadingbar), 0, 0, ch, new_loadingbar->attribute);

    return new_loadingbar;
}

int nocterm_loadingbar_delete(nocterm_loadingbar_t* loadingbar){

    if(nocterm_timer_delete(loadingbar->timer) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    if(nocterm_widget_destructor(NOCTERM_WIDGET(loadingbar)) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    free(loadingbar);

    return NOCTERM_SUCCESS;
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


int nocterm_loadingbar_enable(nocterm_loadingbar_t* loadingbar){
    return nocterm_timer_start(loadingbar->timer);
}

int nocterm_loadingbar_disable(nocterm_loadingbar_t* loadingbar){
    return nocterm_timer_stop(loadingbar->timer);
}
