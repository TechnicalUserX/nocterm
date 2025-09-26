#include <nocterm/base/overlay.h>

nocterm_overlay_t* nocterm_overlay = {0}; // Global overlay

nocterm_overlay_t* nocterm_overlay_new(){

    nocterm_overlay_t* new_overlay = (nocterm_overlay_t*)malloc(sizeof(nocterm_overlay_t));

    if(new_overlay == NULL){
        errno = ENOMEM;
        return NULL;
    }
    
    memset(new_overlay, 0x0, sizeof(nocterm_widget_t));

    return new_overlay;
}

int nocterm_overlay_delete(nocterm_overlay_t* overlay){
    free(overlay);
    return NOCTERM_SUCCESS;
}

int nocterm_overlay_set(nocterm_overlay_t* overlay){

    if(overlay == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    nocterm_overlay = overlay;
    nocterm_overlay->hard_refresh = true;

    return NOCTERM_SUCCESS;
}

int nocterm_overlay_add_widget(nocterm_overlay_t* overlay, nocterm_widget_t* widget){

    if(overlay == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(overlay->widget_size == NOCTERM_OVERLAY_WIDGET_MAX_SIZE){
        return NOCTERM_FAILURE;
    }

    for(uint64_t i = 0; i < NOCTERM_OVERLAY_WIDGET_MAX_SIZE; i++){
        if(overlay->widgets[i] == NULL){
            overlay->widgets[i] = widget;
            overlay->widget_size++;
            break;
        }
    }

    return NOCTERM_SUCCESS;
}

int nocterm_overlay_remove_widget(nocterm_overlay_t* overlay, nocterm_widget_t* widget){

    if(overlay == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(overlay->widget_size == 0){
        return NOCTERM_FAILURE;
    }

    for(uint64_t i = 0; i < NOCTERM_OVERLAY_WIDGET_MAX_SIZE; i++){
        if(overlay->widgets[i] == widget){
            overlay->widgets[i] = NULL;
            overlay->widget_size--;
            return NOCTERM_SUCCESS;
        }
    }

    return NOCTERM_FAILURE;
}

int nocterm_overlay_refresh(nocterm_overlay_t* overlay){

    if(overlay == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    for(uint64_t i = 0; i < NOCTERM_OVERLAY_WIDGET_MAX_SIZE; i++){
        if(overlay->widgets[i]){
            if(overlay->hard_refresh){
                overlay->widgets[i]->hard_refresh = true;
            }
            nocterm_widget_refresh(overlay->widgets[i]);
        }
    }

    overlay->hard_refresh = false;
    
    return NOCTERM_SUCCESS;
}
