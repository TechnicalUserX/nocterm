#include <nocterm/base/widget.h>

nocterm_widget_t* nocterm_widget_new(nocterm_dimension_t bounds, nocterm_widget_focusable_t focusable, nocterm_widget_type_t type){

    nocterm_widget_t* new_widget = (nocterm_widget_t*)malloc(sizeof(nocterm_widget_t));

    if(new_widget == NULL){
        errno = ENOMEM;
        return NULL;
    }
    
    memset(new_widget, 0x0, sizeof(nocterm_widget_t));

    if(nocterm_widget_constructor(new_widget, bounds, focusable, type) == NOCTERM_FAILURE){
        free(new_widget);
        return NULL;
    }

    return new_widget;
}

int nocterm_widget_constructor(nocterm_widget_t* widget, nocterm_dimension_t bounds, nocterm_widget_focusable_t focusable, nocterm_widget_type_t type){
    // Do not call this function directly, instead call nocterm_widget_new

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    widget->bounds.row = bounds.row;
    widget->bounds.col = bounds.col;
    widget->viewport.row = 0;
    widget->viewport.col = 0;

    if(bounds.width*bounds.height > 0){
        // It has an area in the terminal space
        widget->bounds.width = bounds.width;
        widget->bounds.height = bounds.height;
        widget->viewport.width = bounds.width;
        widget->viewport.height = bounds.height;
    }else{
        // It has no area at all
        widget->bounds.width = 0;
        widget->bounds.height = 0;
        widget->viewport.width = 0;
        widget->viewport.height = 0;
    }

    widget->visible = true; // New widgets are always visible
    if(focusable == NOCTERM_WIDGET_FOCUSABLE_YES){
        widget->focusable = true;
    }else{
        widget->focusable = false;
    }
    widget->center_horizontal = false;
    widget->center_vertical = false;

    widget->subwidgets_size = 0;
    widget->subwidgets = NULL;
    if(type == NOCTERM_WIDGET_TYPE_VIRTUAL){
        widget->is_virtual = true;
    }else{
        widget->is_virtual = false;
    }

    widget->buffer_size = bounds.width*bounds.height;

    if(widget->buffer_size != 0 && widget->is_virtual == false){
        widget->buffer = (nocterm_widget_cell_t*)malloc(sizeof(nocterm_widget_cell_t)*widget->buffer_size);
        if(widget->buffer == NULL){
            errno = ENOMEM;
            return NOCTERM_FAILURE;
        }
        memset(widget->buffer, 0x0, sizeof(nocterm_widget_cell_t)*widget->buffer_size);
    }else{
        widget->buffer = NULL; 
    }

    return NOCTERM_SUCCESS;
}

int nocterm_widget_viewport(nocterm_widget_t* widget, nocterm_dimension_t viewport){

    if(widget == NULL){
        errno = ENOMEM;
        return NOCTERM_FAILURE;
    }

    if(widget->bounds.width == 0 || widget->bounds.height == 0){
        return NOCTERM_SUCCESS;
    }

    if(viewport.width != 0 && viewport.width <= widget->bounds.width){
        widget->viewport.width = viewport.width;
    }

    if(viewport.height != 0 && viewport.height <= widget->bounds.height){
        widget->viewport.height = viewport.height;
    }

    if(viewport.row + widget->viewport.height > widget->bounds.height - 1){ // Exceed index range
        widget->viewport.row = widget->bounds.height - widget->viewport.height;
    }else{
        widget->viewport.row = viewport.row;
    }

    if(viewport.col + widget->viewport.width > widget->bounds.width - 1){ // Exceed index range
        widget->viewport.col = widget->bounds.width - widget->viewport.width;
    }else{
        widget->viewport.col = viewport.col;
    }    

    widget->hard_refresh = true;
    return NOCTERM_SUCCESS;
}

int nocterm_widget_viewport_up(nocterm_widget_t* widget){

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(widget->viewport.row == 0){
        return NOCTERM_SUCCESS;
    }

    return nocterm_widget_viewport(widget, (nocterm_dimension_t){widget->viewport.row - 1, widget->viewport.col, widget->viewport.height, widget->viewport.width});
}

int nocterm_widget_viewport_down(nocterm_widget_t* widget){

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    return nocterm_widget_viewport(widget, (nocterm_dimension_t){widget->viewport.row + 1, widget->viewport.col, widget->viewport.height, widget->viewport.width});
}

int nocterm_widget_viewport_right(nocterm_widget_t* widget){

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    return nocterm_widget_viewport(widget, (nocterm_dimension_t){widget->viewport.row, widget->viewport.col + 1, widget->viewport.height, widget->viewport.width});
}

int nocterm_widget_viewport_left(nocterm_widget_t* widget){

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(widget->viewport.col == 0){
        return NOCTERM_SUCCESS;
    }

    return nocterm_widget_viewport(widget, (nocterm_dimension_t){widget->viewport.row, widget->viewport.col - 1, widget->viewport.height, widget->viewport.width});
}

int nocterm_widget_get_position(nocterm_widget_t* widget, nocterm_dimension_size_t* row, nocterm_dimension_size_t* col){

    if(widget == NULL || row == NULL || col == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(widget->parent == NULL){
        *row = widget->bounds.row;
        *col = widget->bounds.col;
    }else{
        uint16_t parent_row = 0, parent_col = 0;

        if(nocterm_widget_get_position(widget->parent, &parent_row, &parent_col) == NOCTERM_FAILURE){
            return NOCTERM_FAILURE;
        }

        *row = parent_row + widget->bounds.row;
        *col = parent_col + widget->bounds.col;
    }
    return NOCTERM_SUCCESS;
}

int nocterm_widget_set_position(nocterm_widget_t* widget, nocterm_dimension_size_t row, nocterm_dimension_size_t col){
    // Absolute
    
    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    
    if(widget->bounds.row != row || widget->bounds.col != col){
        widget->bounds.row = row;
        widget->bounds.col = col;
        widget->center_horizontal = false;
        widget->center_vertical = false;
        nocterm_screen_ownership_reset();
        nocterm_widget_enforce_root_refresh(widget);        
    }

    return NOCTERM_SUCCESS;
}

int nocterm_widget_set_position_row(nocterm_widget_t* widget, nocterm_dimension_size_t row){
    // Absolute
    
    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(widget->bounds.row != row){
        widget->bounds.row = row;
        widget->center_vertical = false;
        nocterm_screen_ownership_reset();
        nocterm_widget_enforce_root_refresh(widget);
    }

    return NOCTERM_SUCCESS;
}

int nocterm_widget_set_position_col(nocterm_widget_t* widget, nocterm_dimension_size_t col){
    // Absolute
    
    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(widget->bounds.col != col){
        widget->bounds.col = col;
        widget->center_horizontal = false;
        nocterm_screen_ownership_reset();
        nocterm_widget_enforce_root_refresh(widget);
    }

    return NOCTERM_SUCCESS;
}

int nocterm_widget_center_position_horizontal(nocterm_widget_t* widget){
    // Idempotent

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(widget->parent == NULL){
        // There is no parent, so we have to look for the actual terminal
        struct winsize w = {0};
        if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1){
            return NOCTERM_FAILURE;
        }
        // Let's approximate horizontal center
        if(w.ws_col == 0){
            return NOCTERM_SUCCESS;
        }

        if(w.ws_col >= widget->viewport.width){
            // Widget can fit to the terminal
            nocterm_dimension_size_t new_col = (w.ws_col - widget->viewport.width)/2;
            widget->bounds.col = new_col;
        }
        // Else no effect

    }else{
        // There is a parent
        if(widget->parent->bounds.width != 0 && widget->parent->bounds.height != 0){
            // Parent has dimension

            if(widget->parent->viewport.width >= widget->viewport.width){
                // We can center because parent has enough width
                nocterm_dimension_size_t new_col = (widget->parent->viewport.width - widget->viewport.width)/2;
                widget->bounds.col = new_col;
            }
            // Else no effect
        }
        // Otherwise, no effect

    }


    nocterm_widget_enforce_root_refresh(widget);
    widget->center_horizontal = true;

    return NOCTERM_SUCCESS;
}

int nocterm_widget_center_position_vertical(nocterm_widget_t* widget){
    // Idempotent

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(widget->parent == NULL){
        // There is no parent, so we have to look for the actual terminal
        struct winsize w = {0};
        if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1){
            return NOCTERM_FAILURE;
        }
        // Let's approximate horizontal center
        if(w.ws_row == 0){
            return NOCTERM_SUCCESS;
        }

        if(w.ws_row >= widget->viewport.height){
            // Widget can fit to the terminal
            nocterm_dimension_size_t new_row = (w.ws_row - widget->viewport.height)/2;
            widget->bounds.row = new_row;
        }
        // Else no effect

    }else{
        // There is a parent
        if(widget->parent->bounds.width != 0 && widget->parent->bounds.height != 0){
            // Parent has dimension

            if(widget->parent->viewport.height >= widget->viewport.height){
                // We can center because parent has enough width
                nocterm_dimension_size_t new_row = (widget->parent->viewport.height - widget->viewport.height)/2;
                widget->bounds.row = new_row;
            }
            // Else no effect
        }
        // Otherwise, no effect

    }


    nocterm_widget_enforce_root_refresh(widget);
    widget->center_vertical = true;

    return NOCTERM_SUCCESS;
}

int nocterm_widget_center_position_update(nocterm_widget_t* widget){
    
    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(widget->center_horizontal){
        nocterm_widget_center_position_horizontal(widget);
    }
    if(widget->center_vertical){
        nocterm_widget_center_position_vertical(widget);
    }

    for(uint64_t i = 0; i < widget->subwidgets_size; i++){
        nocterm_widget_center_position_update(widget->subwidgets[i]);
    }

    
    return NOCTERM_SUCCESS;
}

int nocterm_widget_get_visible(nocterm_widget_t* widget, bool* visible){
    if(widget == NULL || visible == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    *visible = widget->visible;

    return NOCTERM_SUCCESS;
}

int nocterm_widget_set_visible(nocterm_widget_t* widget, bool visible){

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    // Post order visibility setting
    for(uint64_t i = 0; i < widget->subwidgets_size; i++){
        nocterm_widget_set_visible(widget->subwidgets[i], visible);
    }

    if(widget->visible == visible){
        return NOCTERM_SUCCESS;
    }

    // There is a change in visibility
    if(visible){
        // Show Widget!

        // If a widget becomes visible, then screen ownerships must be re-assigned
        nocterm_screen_ownership_reset();

    }else{
        // Hide Widget!        
        nocterm_widget_lose_screen_ownership(widget);
    }

    nocterm_widget_enforce_root_refresh(widget);
    widget->visible = visible;

    return NOCTERM_SUCCESS;
}

int nocterm_widget_add_subwidget(nocterm_widget_t* widget, nocterm_widget_t* subwidget){

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(subwidget == NULL){
        return NOCTERM_SUCCESS;
    }

    if(subwidget->parent != NULL){
        errno = EINVAL;
        return NOCTERM_SUCCESS;
    }

    nocterm_widget_t** old_subwidgets = widget->subwidgets;
    nocterm_widget_t** new_subwidgets = (nocterm_widget_t**)malloc(sizeof(nocterm_widget_t*)*(widget->subwidgets_size+1));
    
    for(uint64_t i = 0; i < widget->subwidgets_size; i++){
        new_subwidgets[i] = old_subwidgets[i];
    }
    new_subwidgets[widget->subwidgets_size] = subwidget;
    
    subwidget->parent = widget;

    widget->subwidgets_size++;
    widget->subwidgets = new_subwidgets;

    free(old_subwidgets);
    return NOCTERM_SUCCESS;
}

int nocterm_widget_add_key_handler(nocterm_widget_t* widget, nocterm_widget_key_handler_t key_handler){

    if(widget == NULL || key_handler == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    widget->key_handler = key_handler;

    return NOCTERM_SUCCESS;
}

int nocterm_widget_add_focus_handler(nocterm_widget_t* widget, nocterm_widget_focus_handler_t focus_handler){

    if(widget == NULL || focus_handler == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    widget->focus_handler = focus_handler;

    return NOCTERM_SUCCESS;
}

int nocterm_widget_delete(nocterm_widget_t* widget){
    
    if(widget == NULL){
        return NOCTERM_SUCCESS;
    }

    if(nocterm_widget_destructor(widget) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    free(widget);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_destructor(nocterm_widget_t* widget){
    // Do not call this function directly, instead call nocterm_widget_delete

    if(widget == NULL){
        return NOCTERM_SUCCESS;
    }

    free(widget->buffer);
    free(widget->subwidgets);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_update(nocterm_widget_t* widget, nocterm_dimension_size_t row, nocterm_dimension_size_t col, nocterm_char_t character, nocterm_attribute_t attribute){

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(widget->is_virtual){
        return NOCTERM_SUCCESS;
    }

    if(widget->buffer == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    widget->soft_refresh = true;
    
    widget->buffer[row * widget->bounds.width + col].refresh = true;
    widget->buffer[row * widget->bounds.width + col].character = character;

    attribute.clear = true; // Enforce clean attribute set
    widget->buffer[row * widget->bounds.width + col].attribute = attribute;

    return NOCTERM_SUCCESS;
}

int nocterm_widget_refresh(nocterm_widget_t* widget){
    // Pre-order tree refresh (recursive)

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(widget->visible == false){
        return NOCTERM_SUCCESS;
    }

    // Refresh loop
    uint16_t relative_row, relative_col;
    if(nocterm_widget_get_position(widget, &relative_row, &relative_col) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }
    

    // POST ORDER TREE TRAVERSAL

    // If widget includes sub widgets, those are going to be freed recursively
    for(uint64_t i = 0; i < widget->subwidgets_size; i++){
        if(widget->subwidgets[i] != NULL && widget->subwidgets[i]->visible == true){
            if(widget->hard_refresh){
                widget->subwidgets[i]->hard_refresh = true;
            }
            nocterm_widget_refresh(widget->subwidgets[i]);
        }
    }

    bool at_least_one_refresh_remaining = false;

    // If there is no change at all, then there is no need to perform this loop, so exhausting! :D
    if((widget->soft_refresh || widget->hard_refresh) && widget->is_virtual == false){
        
        for(nocterm_dimension_size_t row = 0; row < widget->viewport.height; row++ ){
            for(nocterm_dimension_size_t col = 0; col < widget->viewport.width; col++){
                
                uint16_t buffer_index = (widget->viewport.row + row) * widget->bounds.width + (widget->viewport.col + col);

                if(widget->hard_refresh || widget->buffer[buffer_index].refresh){
                    
                    uint64_t screen_index = (relative_row + row) * nocterm_screen_width + (relative_col + col);
                    uint64_t screen_size = nocterm_screen_height * nocterm_screen_width;
                    nocterm_screen_ownership_t* current_ownership = NULL;

                    if(screen_index < screen_size){
                        current_ownership = &(nocterm_screen_ownership[screen_index]);
                    }

                    if(current_ownership && (current_ownership->owner == (void*)widget || current_ownership->owner == NULL)){

                        if(widget->buffer[buffer_index].character.bytes_size != 0){
                            if(nocterm_attribute_set(widget->buffer[buffer_index].attribute) == NOCTERM_FAILURE){
                                return NOCTERM_FAILURE;
                            }                        
                            if(nocterm_io_put_char_at(relative_row + row, relative_col + col, widget->buffer[buffer_index].character) == NOCTERM_FAILURE){
                                return NOCTERM_FAILURE;
                            }  
                            if(nocterm_attribute_clear() == NOCTERM_FAILURE){
                                return NOCTERM_FAILURE;
                            }    
                        }else{
                            if(nocterm_io_erase_char_at(relative_row + row, relative_col + col) == NOCTERM_FAILURE){
                                return NOCTERM_FAILURE;
                            }
                        }

                        current_ownership->owner = (void*)widget;
                        widget->buffer[buffer_index].refresh = false;
                    }else{
                        at_least_one_refresh_remaining = true;
                    }

                }
            }
        }

    }

    if(at_least_one_refresh_remaining == true){
        widget->soft_refresh = true;
    }else{
        widget->soft_refresh = false;
    }

    widget->hard_refresh = false;

    return NOCTERM_SUCCESS;
}

int nocterm_widget_enforce_root_refresh(nocterm_widget_t* widget){
    
    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    nocterm_widget_t* root_widget = widget->parent;
    if(root_widget){
        while(root_widget->parent){
            root_widget = root_widget->parent;
        }
        root_widget->hard_refresh = true;
    }else{
        widget->hard_refresh = true;
    }

    return NOCTERM_SUCCESS;
}

int nocterm_widget_clear(nocterm_widget_t* widget){
    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(widget->is_virtual){
        return NOCTERM_SUCCESS;
    }

    // Short way of filling with NOCTERM_CHAR_EMPTY
    if(widget->buffer != NULL){
        memset(widget->buffer, 0x0, widget->buffer_size * sizeof(nocterm_widget_cell_t));
    }

    return NOCTERM_SUCCESS;
}

int nocterm_widget_resize(nocterm_widget_t* widget, nocterm_dimension_size_t height, nocterm_dimension_size_t width, bool preserve_buffer){
    
    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(widget->is_virtual){
        // Bounds row and column do not change, but area does!
        widget->bounds.height = height;
        widget->bounds.width = width;

        // Viewport resets after resize
        widget->viewport.row = 0;
        widget->viewport.col = 0;
        widget->viewport.height = widget->bounds.height;
        widget->viewport.width = widget->bounds.width; 
        nocterm_screen_ownership_reset(); 
        nocterm_widget_enforce_root_refresh(widget);
        return NOCTERM_SUCCESS;
    }

    // At least something is changed internally
    if(width != widget->bounds.width || height != widget->bounds.height){

        uint32_t new_buffer_size = width*height;

        if(new_buffer_size > 0){
            nocterm_widget_cell_t* new_buffer = (nocterm_widget_cell_t*)malloc(sizeof(nocterm_widget_cell_t) * new_buffer_size);
            memset(new_buffer, 0x0, sizeof(nocterm_widget_cell_t) * new_buffer_size);

            // Preserve contents as much as possible if said so
            if(preserve_buffer){
                if(new_buffer_size > widget->buffer_size){
                    memcpy(new_buffer, widget->buffer, sizeof(nocterm_widget_cell_t) * widget->buffer_size); // New buffer is larger
                }else{
                    memcpy(new_buffer, widget->buffer, sizeof(nocterm_widget_cell_t) * new_buffer_size); // New buffer is smaller
                }
            }

            widget->bounds.height = height;
            widget->bounds.width = width;
            widget->buffer_size = new_buffer_size;
            free(widget->buffer);
            widget->buffer = new_buffer;

        }else{
            widget->bounds.width = 0;
            widget->bounds.height = 0;
            widget->buffer_size = 0;
            free(widget->buffer);
            widget->buffer = NULL;
        }

        // Viewport resets after resize
        widget->viewport.row = 0;
        widget->viewport.col = 0;
        widget->viewport.height = widget->bounds.height;
        widget->viewport.width = widget->bounds.width;
    
    }
    
    nocterm_screen_ownership_reset(); 
    nocterm_widget_enforce_root_refresh(widget);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_lose_screen_ownership(nocterm_widget_t* widget){
    
    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }
    
    uint16_t relative_row, relative_col;
    if(nocterm_widget_get_position(widget, &relative_row, &relative_col) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }
    
    for(nocterm_dimension_size_t row = 0; row < widget->viewport.height; row++ ){
        for(nocterm_dimension_size_t col = 0; col < widget->viewport.width; col++){

            nocterm_screen_ownership_t* current_ownership = &(nocterm_screen_ownership[(relative_row + row) * nocterm_screen_width + (relative_col + col)]);

            if(current_ownership->owner == (void*)widget){
                current_ownership->owner = NULL;
            }
        }
    }    
    return NOCTERM_SUCCESS;
}


