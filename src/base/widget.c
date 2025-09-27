#include <nocterm/base/widget.h>

nocterm_widget_t* nocterm_widget_focused = NULL;

nocterm_widget_t* nocterm_widget_new(nocterm_dimension_size_t height, nocterm_dimension_size_t width, nocterm_widget_focusable_t focusable, nocterm_widget_type_t type){

    nocterm_widget_t* new_widget = (nocterm_widget_t*)malloc(sizeof(nocterm_widget_t));

    if(new_widget == NULL){
        errno = ENOMEM;
        return NULL;
    }
    
    memset(new_widget, 0x0, sizeof(nocterm_widget_t));

    if(nocterm_widget_constructor(new_widget, height, width, focusable, type) == NOCTERM_FAILURE){
        free(new_widget);
        return NULL;
    }

    return new_widget;
}

int nocterm_widget_constructor(nocterm_widget_t* widget, nocterm_dimension_size_t height, nocterm_dimension_size_t width, nocterm_widget_focusable_t focusable, nocterm_widget_type_t type){
    // Do not call this function directly, instead call nocterm_widget_new

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(pthread_mutex_init(&(widget->lock), NULL) != 0){
        return NOCTERM_FAILURE;
    }

    widget->owner = widget;

    widget->bounds.row = 0;
    widget->bounds.col = 0;
    widget->viewport.row = 0;
    widget->viewport.col = 0;

    if(width*height > 0){
        // It has an area in the terminal space
        widget->bounds.width = width;
        widget->bounds.height = height;
        widget->viewport.width = width;
        widget->viewport.height = height;
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
    widget->align.flags.percent_horizontal = false;
    widget->align.flags.percent_vertical = false;

    widget->subwidgets_size = 0;
    widget->subwidgets = NULL;
    if(type == NOCTERM_WIDGET_TYPE_VIRTUAL){
        widget->is_virtual = true;
    }else{
        widget->is_virtual = false;
    }

    widget->buffer_size = width*height;

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

int nocterm_widget_destructor(nocterm_widget_t* widget){
    // Do not call this function directly, instead call nocterm_widget_delete

    if(widget == NULL){
        return NOCTERM_SUCCESS;
    }

    if(pthread_mutex_destroy(&(widget->lock)) != 0){
        return NOCTERM_FAILURE;
    }

    free(widget->buffer);
    free(widget->subwidgets);

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

int nocterm_widget_set_viewport(nocterm_widget_t* widget, nocterm_dimension_t viewport){

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

    if(viewport.col + widget->viewport.width >= widget->bounds.width){ // Exceed index range
        widget->viewport.col = widget->bounds.width - widget->viewport.width;
    }else{

        widget->viewport.col = viewport.col;
    }    

    widget->hard_refresh = true;
    return NOCTERM_SUCCESS;
}

int nocterm_widget_set_viewport_up(nocterm_widget_t* widget){

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(widget->viewport.row == 0){
        return NOCTERM_SUCCESS;
    }

    return nocterm_widget_set_viewport(widget, (nocterm_dimension_t){widget->viewport.row - 1, widget->viewport.col, widget->viewport.height, widget->viewport.width});
}

int nocterm_widget_set_viewport_down(nocterm_widget_t* widget){

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    return nocterm_widget_set_viewport(widget, (nocterm_dimension_t){widget->viewport.row + 1, widget->viewport.col, widget->viewport.height, widget->viewport.width});
}

int nocterm_widget_set_viewport_right(nocterm_widget_t* widget){

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }
    
    if(widget->viewport.col + widget->viewport.width >= widget->bounds.width){
        return NOCTERM_SUCCESS;
    }

    return nocterm_widget_set_viewport(widget, (nocterm_dimension_t){widget->viewport.row, widget->viewport.col + 1, widget->viewport.height, widget->viewport.width});
}

int nocterm_widget_set_viewport_left(nocterm_widget_t* widget){

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(widget->viewport.col == 0){
        return NOCTERM_SUCCESS;
    }

    return nocterm_widget_set_viewport(widget, (nocterm_dimension_t){widget->viewport.row, widget->viewport.col - 1, widget->viewport.height, widget->viewport.width});
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

    pthread_mutex_lock(&widget->lock);

    if(widget->bounds.row != row || widget->bounds.col != col){
        widget->bounds.row = row;
        widget->bounds.col = col;

        widget->align.flags.percent_horizontal = false;
        widget->align.flags.left = false;
        widget->align.flags.right = false;

        widget->align.flags.percent_vertical = false;
        widget->align.flags.top = false;
        widget->align.flags.bottom = false;

        nocterm_widget_enforce_root_refresh(widget);        
    }

    pthread_mutex_unlock(&widget->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_set_row(nocterm_widget_t* widget, nocterm_dimension_size_t row){
    // Absolute
    
    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&widget->lock);

    if(widget->bounds.row != row){

        widget->bounds.row = row;
        widget->align.flags.percent_vertical = false;
        widget->align.flags.top = false;
        widget->align.flags.bottom = false;
        
        nocterm_widget_enforce_root_refresh(widget);
    }

    pthread_mutex_unlock(&widget->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_set_col(nocterm_widget_t* widget, nocterm_dimension_size_t col){
    // Absolute
    
    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&widget->lock);

    if(widget->bounds.col != col){
        widget->bounds.col = col;

        widget->align.flags.percent_horizontal = false;
        widget->align.flags.left = false;
        widget->align.flags.right = false;

        nocterm_screen_ownership_reset();
        nocterm_widget_enforce_root_refresh(widget);
    }

    pthread_mutex_unlock(&widget->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_align_left(nocterm_widget_t* widget){
    
    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&widget->lock);

    if(widget->parent == NULL){
        // There is no parent, so we have to look for the actual terminal

        struct winsize w = {0};
        if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1){
            pthread_mutex_unlock(&widget->lock);
            return NOCTERM_FAILURE;
        }

        if(w.ws_col == 0){
            pthread_mutex_unlock(&widget->lock);
            return NOCTERM_SUCCESS;
        }

        if(w.ws_col >= widget->viewport.width + widget->align.margin_horizontal){
            nocterm_dimension_size_t new_col = 0 + widget->align.margin_horizontal;
            widget->bounds.col = new_col;
        }
        // Else no effect

    }else{
        // There is a parent
        if(widget->parent->bounds.width != 0 && widget->parent->bounds.height != 0){
            // Parent has dimension

            if(widget->parent->viewport.width >= widget->viewport.width + widget->align.margin_horizontal){
                // We can align left because parent has enough width
                nocterm_dimension_size_t new_col = 0 + widget->align.margin_horizontal;
                widget->bounds.col = new_col;
            }
            // Else no effect
        }
        // Otherwise, no effect

    }

    nocterm_widget_enforce_root_refresh(widget);

    widget->align.flags.left = true;
    widget->align.flags.right = false;
    widget->align.flags.percent_horizontal = false;

    pthread_mutex_unlock(&widget->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_align_right(nocterm_widget_t* widget){
    
    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&widget->lock);

    if(widget->parent == NULL){
        // There is no parent, so we have to look for the actual terminal

        struct winsize w = {0};
        if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1){
            pthread_mutex_unlock(&widget->lock);
            return NOCTERM_FAILURE;
        }

        if(w.ws_col == 0){
            pthread_mutex_unlock(&widget->lock);
            return NOCTERM_SUCCESS;
        }

        if(w.ws_col >= widget->viewport.width + widget->align.margin_horizontal){
            nocterm_dimension_size_t new_col = w.ws_col - widget->viewport.width - widget->align.margin_horizontal;
            widget->bounds.col = new_col;
        }
        // Else no effect

    }else{
        // There is a parent
        if(widget->parent->bounds.width != 0 && widget->parent->bounds.height != 0){
            // Parent has dimension

            if(widget->parent->viewport.width >= widget->viewport.width + widget->align.margin_horizontal){
                // We can align right because parent has enough width
                nocterm_dimension_size_t new_col = widget->parent->viewport.width - widget->viewport.width - widget->align.margin_horizontal;
                widget->bounds.row = new_col;
            }
            // Else no effect
        }
        // Otherwise, no effect

    }

    nocterm_widget_enforce_root_refresh(widget);

    widget->align.flags.left = false;
    widget->align.flags.right = true;
    widget->align.flags.percent_horizontal = false;

    pthread_mutex_unlock(&widget->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_align_top(nocterm_widget_t* widget){
    
    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&widget->lock);

    if(widget->parent == NULL){
        // There is no parent, so we have to look for the actual terminal

        struct winsize w = {0};
        if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1){
            pthread_mutex_unlock(&widget->lock);
            return NOCTERM_FAILURE;
        }

        if(w.ws_row == 0){
            pthread_mutex_unlock(&widget->lock);
            return NOCTERM_SUCCESS;
        }

        if(w.ws_row >= widget->viewport.height + widget->align.margin_vertical){
            nocterm_dimension_size_t new_row = 0 + widget->align.margin_vertical;
            widget->bounds.row = new_row;
        }
        // Else no effect

    }else{
        // There is a parent
        if(widget->parent->bounds.width != 0 && widget->parent->bounds.height != 0){
            // Parent has dimension

            if(widget->parent->viewport.height >= widget->viewport.height + widget->align.margin_vertical){
                // We can align left because parent has enough width
                nocterm_dimension_size_t new_row = 0 + widget->align.margin_vertical;
                widget->bounds.row = new_row;
            }
            // Else no effect
        }
        // Otherwise, no effect

    }

    nocterm_widget_enforce_root_refresh(widget);

    widget->align.flags.top = true;
    widget->align.flags.bottom = false;
    widget->align.flags.percent_vertical = false;


    pthread_mutex_unlock(&widget->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_align_bottom(nocterm_widget_t* widget){
    
    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&widget->lock);

    if(widget->parent == NULL){
        // There is no parent, so we have to look for the actual terminal

        struct winsize w = {0};
        if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1){
            pthread_mutex_unlock(&widget->lock);
            return NOCTERM_FAILURE;
        }

        if(w.ws_row == 0){
            pthread_mutex_unlock(&widget->lock);
            return NOCTERM_SUCCESS;
        }

        if(w.ws_row >= widget->viewport.height + widget->align.margin_vertical){
            nocterm_dimension_size_t new_row = w.ws_row - widget->viewport.height - widget->align.margin_vertical;
            widget->bounds.row = new_row;
        }
        // Else no effect

    }else{
        // There is a parent
        if(widget->parent->bounds.width != 0 && widget->parent->bounds.height != 0){
            // Parent has dimension

            if(widget->parent->viewport.height >= widget->viewport.height + widget->align.margin_vertical){
                // We can align left because parent has enough width
                nocterm_dimension_size_t new_row = widget->parent->viewport.height - widget->viewport.height - widget->align.margin_vertical;
                widget->bounds.row = new_row;
            }
            // Else no effect
        }
        // Otherwise, no effect

    }

    nocterm_widget_enforce_root_refresh(widget);

    widget->align.flags.top = false;
    widget->align.flags.bottom = true;
    widget->align.flags.percent_vertical = false;

    pthread_mutex_unlock(&widget->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_align_percent_horizontal(nocterm_widget_t* widget, uint8_t percent){

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&widget->lock);

    nocterm_dimension_size_t control_width = 0;

    if(widget->parent == NULL){
        control_width = nocterm_screen_width;
    }else{
        control_width = widget->parent->bounds.width;
    }

    nocterm_dimension_size_t percent_left_size = (control_width * percent)/100;
    nocterm_dimension_size_t percent_right_size = control_width - percent_left_size;
    nocterm_dimension_size_t widget_left_size = widget->viewport.width/2;
    nocterm_dimension_size_t widget_right_size = widget->viewport.width - widget_left_size;


    if(control_width > 0 && control_width >= widget->viewport.width){
        // We can align left because there is enough width

        if((percent_left_size >= widget_left_size + widget->align.margin_horizontal) && (percent_right_size >= widget_right_size + widget->align.margin_horizontal)){
            // Widgets fits perfectly    
            widget->bounds.col = percent_left_size-widget_left_size;   
        }
        else if(percent_left_size < percent_right_size && widget->align.margin_horizontal + widget_left_size + widget_right_size <= control_width){
            // Left align
            widget->bounds.col = 0 + widget->align.margin_horizontal;
        }else if(percent_left_size >= percent_right_size && widget->align.margin_horizontal + widget_left_size + widget_right_size <= control_width){
            // Right align
            widget->bounds.col = control_width - widget->viewport.width - widget->align.margin_horizontal;
        }
        // Else no effect

    }

    nocterm_widget_enforce_root_refresh(widget);

    widget->align.flags.percent_horizontal = true;
    widget->align.percent_values.horizontal = percent;
    widget->align.flags.left = false;
    widget->align.flags.right = false;

    pthread_mutex_unlock(&widget->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_align_percent_vertical(nocterm_widget_t* widget, uint8_t percent){

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }
    pthread_mutex_lock(&widget->lock);

    nocterm_dimension_size_t control_height = 0;

    if(widget->parent == NULL){
        control_height = nocterm_screen_height;
    }else{
        control_height = widget->parent->bounds.height;
    }

    nocterm_dimension_size_t percent_top_size = (control_height * percent)/100;
    nocterm_dimension_size_t percent_bottom_size = control_height - percent_top_size;
    nocterm_dimension_size_t widget_top_size = widget->viewport.height/2;
    nocterm_dimension_size_t widget_bottom_size = widget->viewport.height - widget_top_size;


    if(control_height > 0 && control_height >= widget->viewport.height){
        // We can align left because there is enough width

        if((percent_top_size >= widget_top_size + widget->align.margin_vertical) && (percent_bottom_size >= widget_bottom_size + widget->align.margin_vertical)){
            // Widgets fits perfectly    
            widget->bounds.row = percent_top_size-widget_top_size;   
        }
        else if(percent_top_size < percent_bottom_size && widget->align.margin_vertical + widget_top_size + widget_bottom_size <= control_height){
            // Left align
            widget->bounds.row = 0 + widget->align.margin_vertical;
        }else if(percent_top_size >= percent_bottom_size && widget->align.margin_vertical + widget_top_size + widget_bottom_size <= control_height){
            // Right align
            widget->bounds.row = control_height - widget->viewport.height - widget->align.margin_vertical;
        }
        // Else no effect
    }

    nocterm_widget_enforce_root_refresh(widget);

    widget->align.flags.percent_vertical = true;
    widget->align.percent_values.vertical = percent;
    widget->align.flags.top = false;
    widget->align.flags.bottom = false;

    pthread_mutex_unlock(&widget->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_align_set_margin_horizontal(nocterm_widget_t* widget, nocterm_dimension_size_t margin){
    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&widget->lock);

    widget->align.margin_horizontal = margin;
    nocterm_widget_enforce_root_refresh(widget);

    pthread_mutex_unlock(&widget->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_align_set_margin_vertical(nocterm_widget_t* widget, nocterm_dimension_size_t margin){
    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&widget->lock);

    widget->align.margin_vertical = margin;
    nocterm_widget_enforce_root_refresh(widget);

    pthread_mutex_unlock(&widget->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_align_update(nocterm_widget_t* widget){

    if(widget == NULL){
        return NOCTERM_SUCCESS;
    }

    if(widget->align.flags.left){
        nocterm_widget_align_left(widget);
    }else if(widget->align.flags.right){
        nocterm_widget_align_right(widget);
    }else if(widget->align.flags.percent_horizontal){
        nocterm_widget_align_percent_horizontal(widget, widget->align.percent_values.horizontal);
    }
    
    if(widget->align.flags.top){
        nocterm_widget_align_top(widget);
    }else if(widget->align.flags.bottom){
        nocterm_widget_align_bottom(widget);
    }else if(widget->align.flags.percent_vertical){
        nocterm_widget_align_percent_vertical(widget, widget->align.percent_values.vertical);
    }

    for(uint64_t i = 0; i < widget->subwidgets_size; i++){
        nocterm_widget_align_update(widget->subwidgets[i]);
    }

    return NOCTERM_SUCCESS;
}

int nocterm_widget_get_visible(nocterm_widget_t* widget, bool* visible){
    if(widget == NULL || visible == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&widget->lock);

    *visible = widget->visible;

    pthread_mutex_unlock(&widget->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_set_visible(nocterm_widget_t* widget, bool visible){

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&widget->lock);

    if(widget->visible == visible){
        pthread_mutex_unlock(&widget->lock);        
        return NOCTERM_SUCCESS;
    }

    // Post order visibility setting
    for(uint64_t i = 0; i < widget->subwidgets_size; i++){
        nocterm_widget_set_visible(widget->subwidgets[i], visible);
    }

    nocterm_widget_enforce_root_refresh(widget);

    widget->visible = visible;

    pthread_mutex_unlock(&widget->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_set_click_activation(nocterm_widget_t* widget, bool click_activation){

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&widget->lock);

    widget->click_activation = click_activation;

    pthread_mutex_unlock(&widget->lock);

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

    pthread_mutex_lock(&widget->lock);
    
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

    pthread_mutex_unlock(&widget->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_add_key_handler(nocterm_widget_t* widget, nocterm_widget_key_handler_t key_handler){

    if(widget == NULL || key_handler == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&widget->lock);

    widget->key_handler = key_handler;

    pthread_mutex_unlock(&widget->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_add_focus_handler(nocterm_widget_t* widget, nocterm_widget_focus_handler_t focus_handler){

    if(widget == NULL || focus_handler == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&widget->lock);

    widget->focus_handler = focus_handler;

    pthread_mutex_unlock(&widget->lock);

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

    if((row * widget->bounds.width + col) >= widget->buffer_size){
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

    // POST ORDER TREE TRAVERSAL WITH PRE ORDER LOCKING

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(pthread_mutex_trylock(&widget->lock) != 0){
        return NOCTERM_SUCCESS;
    }

    if(widget->visible == false){
        pthread_mutex_unlock(&widget->lock);
        return NOCTERM_SUCCESS;
    }

    // If widget includes sub widgets, those are going to be freed recursively
    for(uint64_t i = 0; i < widget->subwidgets_size; i++){
        if(widget->subwidgets[i] != NULL && widget->subwidgets[i]->visible == true){
            if(widget->hard_refresh){
                widget->subwidgets[i]->hard_refresh = true;
            }
            nocterm_widget_refresh(widget->subwidgets[i]);
        }
    }

    // Refresh loop
    uint16_t relative_row, relative_col;
    if(nocterm_widget_get_position(widget, &relative_row, &relative_col) == NOCTERM_FAILURE){
        pthread_mutex_unlock(&widget->lock);
        return NOCTERM_FAILURE;
    }
    
    bool at_least_one_refresh_remaining = false;

    // If there is no change at all, then there is no need to perform this loop, so exhausting! :D
    if((widget->soft_refresh || widget->hard_refresh) && widget->is_virtual == false){
        
        for(nocterm_dimension_size_t row = 0; row < widget->viewport.height; row++ ){
            for(nocterm_dimension_size_t col = 0; col < widget->viewport.width; col++){
                
                uint16_t buffer_index = (widget->viewport.row + row) * widget->bounds.width + (widget->viewport.col + col);

                if(widget->hard_refresh || widget->buffer[buffer_index].refresh){
                    
                    uint64_t absolute_row = relative_row + row;
                    uint64_t absolute_col = relative_col + col;

                    uint64_t screen_index = absolute_row * nocterm_screen_width + absolute_col;
                    uint64_t screen_size = nocterm_screen_height * nocterm_screen_width;
                    nocterm_screen_ownership_t* current_ownership = NULL;

                    if (absolute_row >= nocterm_screen_height || absolute_col >= nocterm_screen_width) {
                        // Skip this cell - it's outside screen boundaries
                        continue;
                    }
                    
                    if(screen_index < screen_size){
                        current_ownership = &(nocterm_screen_ownership[screen_index]);
                    }

                    if(current_ownership && (current_ownership->owner == (void*)widget || current_ownership->owner == NULL)){

                        if(widget->buffer[buffer_index].character.bytes_size != 0){
                            if(nocterm_attribute_set(widget->buffer[buffer_index].attribute) == NOCTERM_FAILURE){
                                pthread_mutex_unlock(&widget->lock);
                                return NOCTERM_FAILURE;
                            }                        
                            if(nocterm_io_put_char_at(relative_row + row, relative_col + col, widget->buffer[buffer_index].character) == NOCTERM_FAILURE){
                                pthread_mutex_unlock(&widget->lock);
                                return NOCTERM_FAILURE;
                            }  
                            if(nocterm_attribute_clear() == NOCTERM_FAILURE){
                                pthread_mutex_unlock(&widget->lock);
                                return NOCTERM_FAILURE;
                            }    
                        }else{
                            if(nocterm_io_erase_char_at(relative_row + row, relative_col + col) == NOCTERM_FAILURE){
                                pthread_mutex_unlock(&widget->lock);
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

    if(at_least_one_refresh_remaining){
        widget->soft_refresh = true;
    }else{
        widget->soft_refresh = false;
    }

    widget->hard_refresh = false;

    pthread_mutex_unlock(&widget->lock);

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
        nocterm_widget_enforce_root_refresh(widget);
        return NOCTERM_SUCCESS;
    }

    if(width != widget->bounds.width || height != widget->bounds.height){
        // At least something is changed internally

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
    
    nocterm_widget_enforce_root_refresh(widget);

    return NOCTERM_SUCCESS;
}

bool nocterm_widget_is_focused(nocterm_widget_t* widget){
    if(widget == NULL || nocterm_widget_focused == NULL){
        return false;
    }else{
        if(widget->owner == nocterm_widget_focused){
            return true;
        }else{
            return false;
        }
    }
}
