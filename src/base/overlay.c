#include <nocterm/base/overlay.h>

// ====================== Internal Access ====================== //

int nocterm_widget_refresh(nocterm_widget_t* widget);

// ====================== Internal Access ====================== //


nocterm_overlay_t* nocterm_overlay = {0}; // Global overlay

nocterm_overlay_t* nocterm_overlay_new(void){

    nocterm_overlay_t* new_overlay = (nocterm_overlay_t*)malloc(sizeof(nocterm_overlay_t));

    if(new_overlay == NULL){
        errno = ENOMEM;
        return NULL;
    }
    
    memset(new_overlay, 0x0, sizeof(nocterm_overlay_t));

    return new_overlay;
}

int nocterm_overlay_delete(nocterm_overlay_t* overlay){
    free(overlay);
    if(nocterm_overlay == overlay){
        nocterm_overlay = NULL;
    }
    
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

int nocterm_overlay_unset(void){
    nocterm_overlay = NULL;
    return NOCTERM_SUCCESS;
}

int nocterm_overlay_invalidate(nocterm_overlay_t* overlay){

    if(overlay == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    // Request an ownership-resetting hard refresh on the next frame.  Call this
    // after changing an overlay widget at runtime (visibility or position) so it
    // can reclaim/release cells that the page would otherwise keep owning.
    overlay->hard_refresh = true;

    return NOCTERM_SUCCESS;
}

int nocterm_overlay_add_widget(nocterm_overlay_t* overlay, nocterm_widget_t* widget){

    if(overlay == NULL || widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(overlay->widget_size == NOCTERM_OVERLAY_WIDGET_MAX_SIZE){
        return NOCTERM_FAILURE;
    }

    // Reject duplicates: the same widget in two slots would be refreshed twice
    // and would inflate widget_size.
    for(uint64_t i = 0; i < NOCTERM_OVERLAY_WIDGET_MAX_SIZE; i++){
        if(overlay->widgets[i] == widget){
            return NOCTERM_FAILURE;
        }
    }

    for(uint64_t i = 0; i < NOCTERM_OVERLAY_WIDGET_MAX_SIZE; i++){
        if(overlay->widgets[i] == NULL){
            overlay->widgets[i] = widget;
            overlay->widget_size++;
            break;
        }
    }

    // The overlay composition changed: force an ownership-resetting hard refresh
    // so the new widget can claim its cells even where the page already owns them.
    overlay->hard_refresh = true;

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
            // Composition changed: force a hard refresh so the page reclaims the
            // cells the removed widget used to occupy (otherwise they ghost).
            overlay->hard_refresh = true;
            return NOCTERM_SUCCESS;
        }
    }

    return NOCTERM_FAILURE;
}

NOCTERM_INTERNAL int nocterm_overlay_refresh(nocterm_overlay_t* overlay){

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
