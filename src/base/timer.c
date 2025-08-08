#include <nocterm/base/timer.h>

nocterm_timer_t* nocterm_timer_list_head = NULL;

nocterm_timer_t* nocterm_timer_create(nocterm_widget_t* widget, uint64_t interval, nocterm_timer_callback_t callback, void* user_data){

    if(widget == NULL || callback == NULL){
        return NULL;
    } 

    nocterm_timer_t* t = (nocterm_timer_t*)malloc(sizeof(nocterm_timer_t));
    if (t == NULL){
        return NULL;
    }

    memset(t, 0x0, sizeof(nocterm_timer_t));

    t->widget = widget;

    t->callback = callback;

    t->interval = interval;

    t->last_call = 0;  // No call since 1970 :)

    t->user_data = user_data; // User data can be NULL

    t->active = false;

    // Insert into global timer list (at head)
    t->next = nocterm_timer_list_head;
    nocterm_timer_list_head = t;
    return t;
}

void nocterm_timer_tick(void){

    struct timeval tv = {0};
    gettimeofday(&tv,NULL);
    uint64_t now = (tv.tv_sec * 1000000 + tv.tv_usec)/1000; // current time in ms

    nocterm_timer_t* t = nocterm_timer_list_head;
    while (t) {
        if (t->active && (now - t->last_call >= t->interval)) {
            t->callback(t->widget, t->user_data);
            t->last_call = now;  // Update last fire time
        }
        t = t->next;
    }
}

int nocterm_timer_start(nocterm_timer_t* timer){
    if(timer == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }
    timer->active = true;
    return NOCTERM_SUCCESS;
}

int nocterm_timer_start_all_of_widget(nocterm_widget_t* widget, bool recursive){

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(recursive){
        for(uint64_t i = 0; i <  widget->subwidgets_size; i++){
            nocterm_timer_start_all_of_widget(widget->subwidgets[i], true);
        }
    }

    nocterm_timer_t* current = nocterm_timer_list_head;

    while (current) {
        if (current->widget == widget) {
            nocterm_timer_start(current);
        }
        current = current->next;
    }

    return NOCTERM_SUCCESS;

}

int nocterm_timer_stop(nocterm_timer_t* timer){
    if(timer == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }
    timer->active = false;
    return NOCTERM_SUCCESS;
}

int nocterm_timer_stop_all_of_widget(nocterm_widget_t* widget, bool recursive){

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }
   
    if(recursive){
        for(uint64_t i = 0; i <  widget->subwidgets_size; i++){
            nocterm_timer_stop_all_of_widget(widget->subwidgets[i], true);
        }
    }
    
    nocterm_timer_t* current = nocterm_timer_list_head;

    while (current) {
        if (current->widget == widget) {
            nocterm_timer_stop(current);
        }
        current = current->next;
    }

    return NOCTERM_SUCCESS;

}

int nocterm_timer_delete(nocterm_timer_t* timer){
    
    if(timer == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    nocterm_timer_t** current = &nocterm_timer_list_head;

    while (*current) {
        if (*current == timer) {
            nocterm_timer_t* to_free = *current;
            *current = to_free->next;
            free(to_free);
            break;
        }
        current = &(*current)->next;
    }

    return NOCTERM_SUCCESS;
}

int nocterm_timer_delete_all(void){
    
    nocterm_timer_t** current = &nocterm_timer_list_head;

    while(*current){
        nocterm_timer_t* to_free = *current;
        *current = to_free->next;
        free(to_free);
    }

    *current = NULL;

    return NOCTERM_SUCCESS; // Always succeeds
}

int nocterm_timer_delete_all_of_widget(nocterm_widget_t* widget){
    
    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }
   
    for(uint64_t i = 0; i <  widget->subwidgets_size; i++){
        nocterm_timer_delete_all_of_widget(widget->subwidgets[i]);
    }

    nocterm_timer_t** current = &nocterm_timer_list_head;

    while (*current) {
        if ((*current)->widget == widget) {
            nocterm_timer_t* to_free = *current;
            *current = to_free->next;
            free(to_free);
        } else {
            current = &(*current)->next;
        }
    }

    return NOCTERM_SUCCESS;
}

