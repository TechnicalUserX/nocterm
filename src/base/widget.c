#include <nocterm/base/widget.h>

// ====================== Internal Access ====================== //

extern nocterm_screen_ownership_t* nocterm_screen_ownership;

NOCTERM_INTERNAL

NOCTERM_INTERNAL
int nocterm_widget_flex_policy_set_fixed_horizontal(nocterm_widget_t* widget);

NOCTERM_INTERNAL
int nocterm_widget_flex_policy_set_fixed_vertical(nocterm_widget_t* widget);

NOCTERM_INTERNAL
int nocterm_widget_flex_policy_set_fill_horizontal(nocterm_widget_t* widget);

NOCTERM_INTERNAL
int nocterm_widget_flex_policy_set_fill_vertical(nocterm_widget_t* widget);

NOCTERM_INTERNAL
int nocterm_widget_flex_policy_set_fill_both(nocterm_widget_t* widget);

NOCTERM_INTERNAL
int nocterm_widget_flex_policy_set_percent_horizontal(nocterm_widget_t* widget, nocterm_percentage_t percentage);

NOCTERM_INTERNAL
int nocterm_widget_flex_policy_set_percent_vertical(nocterm_widget_t* widget, nocterm_percentage_t percentage);

NOCTERM_INTERNAL
int nocterm_widget_flex_update(nocterm_widget_t* widget);


// ====================== Internal Access ====================== //

NOCTERM_INTERNAL nocterm_widget_t* nocterm_widget_focused = NULL;

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
    widget->align_policy.flags.horizontal = false;
    widget->align_policy.flags.vertical = false;

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

    widget->flex_policy_permission = NOCTERM_WIDGET_FLEX_POLICY_PERMISSION_BOTH;
    widget->flex_policy.horizontal_mode = NOCTERM_WIDGET_FLEX_POLICY_MODE_FIXED;
    widget->flex_policy.vertical_mode = NOCTERM_WIDGET_FLEX_POLICY_MODE_FIXED;
    widget->flex_policy.horizontal_percentage = 0;
    widget->flex_policy.vertical_percentage = 0;
    widget->flex_policy_inner_padding_h = 0;
    widget->flex_policy_inner_padding_w = 0;

    widget->key_handler = NULL;
    widget->focus_handler = NULL;
    widget->internal_resize_handler = NULL;
    widget->resize_handler = NULL;

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

int nocterm_widget_get_viewport(nocterm_widget_t* widget, nocterm_dimension_t* viewport){
    if(widget == NULL || viewport == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&widget->lock);

    *viewport = widget->viewport;

    pthread_mutex_unlock(&widget->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_set_viewport(nocterm_widget_t* widget, nocterm_dimension_t viewport){

    if(widget == NULL){
        errno = EINVAL;
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

int nocterm_widget_scroll_viewport_up(nocterm_widget_t* widget){

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(widget->viewport.row == 0){
        return NOCTERM_SUCCESS;
    }

    return nocterm_widget_set_viewport(widget, (nocterm_dimension_t){widget->viewport.row - 1, widget->viewport.col, widget->viewport.height, widget->viewport.width});
}

int nocterm_widget_scroll_viewport_down(nocterm_widget_t* widget){

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    return nocterm_widget_set_viewport(widget, (nocterm_dimension_t){widget->viewport.row + 1, widget->viewport.col, widget->viewport.height, widget->viewport.width});
}

int nocterm_widget_scroll_viewport_right(nocterm_widget_t* widget){

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }
    
    if(widget->viewport.col + widget->viewport.width >= widget->bounds.width){
        return NOCTERM_SUCCESS;
    }

    return nocterm_widget_set_viewport(widget, (nocterm_dimension_t){widget->viewport.row, widget->viewport.col + 1, widget->viewport.height, widget->viewport.width});
}

int nocterm_widget_scroll_viewport_left(nocterm_widget_t* widget){

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

        widget->align_policy.flags.horizontal = false;
        widget->align_policy.flags.vertical = false;

        nocterm_widget_request_root_refresh(widget);        
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
        widget->align_policy.flags.vertical = false;
        
        nocterm_widget_request_root_refresh(widget);
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

        widget->align_policy.flags.horizontal = false;

        nocterm_widget_request_root_refresh(widget);
    }

    pthread_mutex_unlock(&widget->lock);

    return NOCTERM_SUCCESS;
}

/* Apply stored horizontal alignment + modifiers to widget->bounds.col.
   Reads horizontal_flags.{left,center,right,percent,margin} and the stored
   percentages/margins — does NOT acquire the widget lock.
   CENTER semantics: percent shifts the widget's center point to p% of
   parent_width; margin has no effect on centered widgets. */
static void nocterm_widget_apply_horizontal(nocterm_widget_t* widget,
                                            nocterm_dimension_size_t parent_width) {
    nocterm_dimension_size_t pd = widget->align_policy.flags.horizontal_flags.percent
        ? (parent_width * (nocterm_dimension_size_t)widget->align_policy.percentages.horizontal) / 100
        : 0;
    nocterm_dimension_size_t mg = widget->align_policy.flags.horizontal_flags.margin
        ? widget->align_policy.margins.horizontal
        : 0;

    if (widget->align_policy.flags.horizontal_flags.left) {
        if (parent_width >= pd + widget->viewport.width + mg)
            widget->bounds.col = pd + mg;
    } else if (widget->align_policy.flags.horizontal_flags.center) {
        /* Place widget's center at pd (p% of parent) when percent is set,
           otherwise use the parent's midpoint. Margin is ignored for center. */
        if (widget->align_policy.flags.horizontal_flags.percent) {
            if (pd >= widget->viewport.width / 2)
                widget->bounds.col = pd - widget->viewport.width / 2;
        } else {
            if (parent_width >= widget->viewport.width)
                widget->bounds.col = (parent_width - widget->viewport.width) / 2;
        }
    } else if (widget->align_policy.flags.horizontal_flags.right) {
        if (parent_width >= pd + widget->viewport.width + mg)
            widget->bounds.col = parent_width - widget->viewport.width - mg - pd;
    }
}

/* Apply stored vertical alignment + modifiers to widget->bounds.row.
   CENTER semantics: percent shifts the widget's center point to p% of
   parent_height; margin has no effect on centered widgets. */
static void nocterm_widget_apply_vertical(nocterm_widget_t* widget,
                                          nocterm_dimension_size_t parent_height) {
    nocterm_dimension_size_t pd = widget->align_policy.flags.vertical_flags.percent
        ? (parent_height * (nocterm_dimension_size_t)widget->align_policy.percentages.vertical) / 100
        : 0;
    nocterm_dimension_size_t mg = widget->align_policy.flags.vertical_flags.margin
        ? widget->align_policy.margins.vertical
        : 0;

    if (widget->align_policy.flags.vertical_flags.top) {
        if (parent_height >= pd + widget->viewport.height + mg)
            widget->bounds.row = pd + mg;
    } else if (widget->align_policy.flags.vertical_flags.center) {
        /* Place widget's center at pd (p% of parent) when percent is set,
           otherwise use the parent's midpoint. Margin is ignored for center. */
        if (widget->align_policy.flags.vertical_flags.percent) {
            if (pd >= widget->viewport.height / 2)
                widget->bounds.row = pd - widget->viewport.height / 2;
        } else {
            if (parent_height >= widget->viewport.height)
                widget->bounds.row = (parent_height - widget->viewport.height) / 2;
        }
    } else if (widget->align_policy.flags.vertical_flags.bottom) {
        if (parent_height >= pd + widget->viewport.height + mg)
            widget->bounds.row = parent_height - widget->viewport.height - mg - pd;
    }
}

int nocterm_widget_align(nocterm_widget_t* widget, nocterm_widget_align_t align, ...){
    // Idempotent Function

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    int ret = NOCTERM_SUCCESS;

    pthread_mutex_lock(&widget->lock);

    nocterm_dimension_size_t parent_height, parent_width;

    if(widget->parent == NULL){
        // Terminal is the parent
        parent_height = nocterm_screen_height;
        parent_width = nocterm_screen_width;        
    }else{
        // Widget has a legit parent
        if(widget->parent->viewport.height * widget->parent->viewport.width == 0){
            // Parent has no dimension visible
            pthread_mutex_unlock(&widget->lock);
            return NOCTERM_FAILURE;            
        }
        parent_height = widget->parent->viewport.height;
        parent_width = widget->parent->viewport.width;
    }


    switch(align){

        case NOCTERM_WIDGET_ALIGN_NONE:{

            widget->align_policy.flags.horizontal = false;
            widget->align_policy.flags.horizontal_flags.left = false;
            widget->align_policy.flags.horizontal_flags.center = false;
            widget->align_policy.flags.horizontal_flags.right = false;
            widget->align_policy.flags.horizontal_flags.percent = false;
            widget->align_policy.flags.horizontal_flags.margin = false;

            widget->align_policy.flags.vertical = false;
            widget->align_policy.flags.vertical_flags.top = false;
            widget->align_policy.flags.vertical_flags.center = false;
            widget->align_policy.flags.vertical_flags.bottom = false;
            widget->align_policy.flags.vertical_flags.percent = false;
            widget->align_policy.flags.vertical_flags.margin = false;

        }break;

        case NOCTERM_WIDGET_ALIGN_LEFT:{

            widget->align_policy.flags.horizontal = true;
            widget->align_policy.flags.horizontal_flags.left = true;
            widget->align_policy.flags.horizontal_flags.center = false;
            widget->align_policy.flags.horizontal_flags.right = false;
            nocterm_widget_apply_horizontal(widget, parent_width);

        }break;

        case NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL:{

            widget->align_policy.flags.horizontal = true;
            widget->align_policy.flags.horizontal_flags.left = false;
            widget->align_policy.flags.horizontal_flags.center = true;
            widget->align_policy.flags.horizontal_flags.right = false;
            nocterm_widget_apply_horizontal(widget, parent_width);

        }break;

        case NOCTERM_WIDGET_ALIGN_RIGHT:{

            widget->align_policy.flags.horizontal = true;
            widget->align_policy.flags.horizontal_flags.left = false;
            widget->align_policy.flags.horizontal_flags.center = false;
            widget->align_policy.flags.horizontal_flags.right = true;
            nocterm_widget_apply_horizontal(widget, parent_width);

        }break;

        case NOCTERM_WIDGET_ALIGN_TOP:{

            widget->align_policy.flags.vertical = true;
            widget->align_policy.flags.vertical_flags.top = true;
            widget->align_policy.flags.vertical_flags.center = false;
            widget->align_policy.flags.vertical_flags.bottom = false;
            nocterm_widget_apply_vertical(widget, parent_height);

        }break;

        case NOCTERM_WIDGET_ALIGN_CENTER_VERTICAL:{

            widget->align_policy.flags.vertical = true;
            widget->align_policy.flags.vertical_flags.top = false;
            widget->align_policy.flags.vertical_flags.center = true;
            widget->align_policy.flags.vertical_flags.bottom = false;
            nocterm_widget_apply_vertical(widget, parent_height);

        }break;

        case NOCTERM_WIDGET_ALIGN_BOTTOM:{

            widget->align_policy.flags.vertical = true;
            widget->align_policy.flags.vertical_flags.top = false;
            widget->align_policy.flags.vertical_flags.center = false;
            widget->align_policy.flags.vertical_flags.bottom = true;
            nocterm_widget_apply_vertical(widget, parent_height);

        }break;

        case NOCTERM_WIDGET_ALIGN_PERCENT_HORIZONTAL:{

            va_list vl = {0};
            va_start(vl, align);
            int percentage = va_arg(vl, int);
            va_end(vl);

            if(percentage >= 0 && percentage <= 100){
                widget->align_policy.percentages.horizontal = percentage;
                widget->align_policy.flags.horizontal_flags.percent = true;
                if(widget->align_policy.flags.horizontal)
                    nocterm_widget_apply_horizontal(widget, parent_width);
            }else{
                ret = NOCTERM_FAILURE;
            }

        }break;

        case NOCTERM_WIDGET_ALIGN_PERCENT_VERTICAL:{

            va_list vl = {0};
            va_start(vl, align);
            int percentage = va_arg(vl, int);
            va_end(vl);

            if(percentage >= 0 && percentage <= 100){
                widget->align_policy.percentages.vertical = percentage;
                widget->align_policy.flags.vertical_flags.percent = true;
                if(widget->align_policy.flags.vertical)
                    nocterm_widget_apply_vertical(widget, parent_height);
            }else{
                ret = NOCTERM_FAILURE;
            }

        }break;

        case NOCTERM_WIDGET_ALIGN_MARGIN_HORIZONTAL:{

            va_list vl = {0};
            va_start(vl, align);
            int margin = va_arg(vl, int);
            va_end(vl);

            if(margin >= 0){
                widget->align_policy.margins.horizontal = margin;
                widget->align_policy.flags.horizontal_flags.margin = true;
                if(widget->align_policy.flags.horizontal)
                    nocterm_widget_apply_horizontal(widget, parent_width);
            }else{
                ret = NOCTERM_FAILURE;
            }

        }break;

        case NOCTERM_WIDGET_ALIGN_MARGIN_VERTICAL:{

            va_list vl = {0};
            va_start(vl, align);
            int margin = va_arg(vl, int);
            va_end(vl);

            if(margin >= 0){
                widget->align_policy.margins.vertical = margin;
                widget->align_policy.flags.vertical_flags.margin = true;
                if(widget->align_policy.flags.vertical)
                    nocterm_widget_apply_vertical(widget, parent_height);
            }else{
                ret = NOCTERM_FAILURE;
            }

        }break;

        default:{
            ret = NOCTERM_FAILURE;
        }break;

    }
    
    nocterm_widget_request_root_refresh(widget);

    pthread_mutex_unlock(&widget->lock);

    return ret;
}

int nocterm_widget_align_update(nocterm_widget_t* widget){

    if(widget == NULL){
        return NOCTERM_SUCCESS;
    }

    if(widget->align_policy.flags.horizontal){

        if(widget->align_policy.flags.horizontal_flags.left){
            nocterm_widget_align(widget, NOCTERM_WIDGET_ALIGN_LEFT);
        }else if(widget->align_policy.flags.horizontal_flags.center){
            nocterm_widget_align(widget, NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL);
        }else if(widget->align_policy.flags.horizontal_flags.right){
            nocterm_widget_align(widget, NOCTERM_WIDGET_ALIGN_RIGHT);

        }

    }

    if(widget->align_policy.flags.vertical){

        if(widget->align_policy.flags.vertical_flags.top){
            nocterm_widget_align(widget, NOCTERM_WIDGET_ALIGN_TOP);
        }else if(widget->align_policy.flags.vertical_flags.center){
            nocterm_widget_align(widget, NOCTERM_WIDGET_ALIGN_CENTER_VERTICAL);
        }else if(widget->align_policy.flags.vertical_flags.bottom){
            nocterm_widget_align(widget, NOCTERM_WIDGET_ALIGN_BOTTOM);
        }

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

    nocterm_widget_request_root_refresh(widget);

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
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&widget->lock);
    
    nocterm_widget_t** old_subwidgets = widget->subwidgets;
    nocterm_widget_t** new_subwidgets = (nocterm_widget_t**)malloc(sizeof(nocterm_widget_t*)*(widget->subwidgets_size+1));
    
    if(new_subwidgets == NULL){
        pthread_mutex_unlock(&widget->lock);
        return NOCTERM_FAILURE;
    }

    for(uint64_t i = 0; i < widget->subwidgets_size; i++){
        new_subwidgets[i] = old_subwidgets[i];
    }
    new_subwidgets[widget->subwidgets_size] = subwidget;
    
    subwidget->parent = widget;

    widget->subwidgets_size++;
    widget->subwidgets = new_subwidgets;

    free(old_subwidgets);

    nocterm_widget_request_root_refresh(widget);

    pthread_mutex_unlock(&widget->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_remove_subwidget(nocterm_widget_t* widget, nocterm_widget_t* subwidget){
    
    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(subwidget == NULL){
        return NOCTERM_SUCCESS;
    }

    pthread_mutex_lock(&widget->lock);

    if(widget->subwidgets_size == 0){
        pthread_mutex_unlock(&widget->lock);
        return NOCTERM_FAILURE;
    }

    // Check membership first, before locking subwidget
    if(!nocterm_widget_contains_subwidget(widget, subwidget)){
        pthread_mutex_unlock(&widget->lock);
        return NOCTERM_FAILURE;
    }

    // Now we know it's a child, safe to lock both
    pthread_mutex_lock(&subwidget->lock);
    
    if(widget->subwidgets_size == 1 && widget->subwidgets[0] == subwidget){
        free(widget->subwidgets);
        widget->subwidgets = NULL;
        widget->subwidgets_size = 0;
    }else{
        nocterm_widget_t** old_subwidgets = widget->subwidgets;
        nocterm_widget_t** new_subwidgets = (nocterm_widget_t**)malloc(sizeof(nocterm_widget_t*)*(widget->subwidgets_size-1));

        if(new_subwidgets == NULL){
            pthread_mutex_unlock(&subwidget->lock);
            pthread_mutex_unlock(&widget->lock);
            return NOCTERM_FAILURE;
        }

        for(uint64_t i = 0, j = 0; i < widget->subwidgets_size; i++){
            if(widget->subwidgets[i] != subwidget){
                new_subwidgets[j] = widget->subwidgets[i];
                j++;
            }
        }
        
        widget->subwidgets = new_subwidgets;
        free(old_subwidgets);
        widget->subwidgets_size--;
    }
    
    subwidget->parent = NULL;
    nocterm_widget_request_root_refresh(widget);
    
    pthread_mutex_unlock(&subwidget->lock);
    pthread_mutex_unlock(&widget->lock);
    
    return NOCTERM_SUCCESS;
}

bool nocterm_widget_contains_subwidget(nocterm_widget_t* widget, nocterm_widget_t* subwidget){
    if(widget == NULL || subwidget == NULL){
        return false;
    }

    for(uint64_t i = 0; i < widget->subwidgets_size; i++){
        if(widget->subwidgets[i] == subwidget){
            return true;
        }
    }

    return false;
}

int nocterm_widget_set_floating_subwidgets(nocterm_widget_t* widget, bool floating_subwidgets){
    
    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&widget->lock);

    widget->floating_subwidgets = floating_subwidgets;
    nocterm_widget_request_root_refresh(widget);
    
    pthread_mutex_unlock(&widget->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_set_key_handler(nocterm_widget_t* widget, nocterm_widget_key_handler_t key_handler){

    if(widget == NULL || key_handler == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&widget->lock);

    widget->key_handler = key_handler;

    pthread_mutex_unlock(&widget->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_set_focus_handler(nocterm_widget_t* widget, nocterm_widget_focus_handler_t focus_handler){

    if(widget == NULL || focus_handler == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&widget->lock);

    widget->focus_handler = focus_handler;

    pthread_mutex_unlock(&widget->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_set_resize_handler(nocterm_widget_t* widget, nocterm_widget_resize_handler_t resize_handler){

    if(widget == NULL || resize_handler == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&widget->lock);

    widget->resize_handler = resize_handler;

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

int nocterm_widget_request_root_refresh(nocterm_widget_t* widget){
    
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

NOCTERM_INTERNAL
int nocterm_widget_buffer_resize(nocterm_widget_t* widget, nocterm_dimension_size_t height, nocterm_dimension_size_t width){
    
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
        nocterm_widget_request_root_refresh(widget);

        return NOCTERM_SUCCESS;
    }

    if(width != widget->bounds.width || height != widget->bounds.height){
        // At least something is changed internally

        uint32_t new_buffer_size = width*height;

        if(new_buffer_size > 0){
            nocterm_widget_cell_t* new_buffer = (nocterm_widget_cell_t*)malloc(sizeof(nocterm_widget_cell_t) * new_buffer_size);
            
            if(new_buffer == NULL){
                return NOCTERM_FAILURE;
            }
            memset(new_buffer, 0x0, sizeof(nocterm_widget_cell_t) * new_buffer_size);

            // Preserve contents as much as possible if said so


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
    
    nocterm_widget_request_root_refresh(widget);

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

int nocterm_widget_flex_policy_set_permission(nocterm_widget_t* widget, nocterm_widget_flex_policy_permission_t permission){

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    widget->flex_policy_permission = permission;

    return NOCTERM_SUCCESS;
}

int nocterm_widget_flex_policy_set_fixed_both(nocterm_widget_t* widget){

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&widget->lock);

    widget->flex_policy.horizontal_mode = NOCTERM_WIDGET_FLEX_POLICY_MODE_FIXED;
    widget->flex_policy.vertical_mode = NOCTERM_WIDGET_FLEX_POLICY_MODE_FIXED;
    widget->flex_policy.horizontal_percentage = 0;
    widget->flex_policy.vertical_percentage = 0;

    pthread_mutex_unlock(&widget->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_flex_policy_set_fill_both(nocterm_widget_t* widget){

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if((widget->flex_policy_permission & NOCTERM_WIDGET_FLEX_POLICY_PERMISSION_BOTH) != NOCTERM_WIDGET_FLEX_POLICY_PERMISSION_BOTH){
        errno = EPERM;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&widget->lock);
    widget->flex_policy.horizontal_mode = NOCTERM_WIDGET_FLEX_POLICY_MODE_FILL;
    widget->flex_policy.vertical_mode = NOCTERM_WIDGET_FLEX_POLICY_MODE_FILL;
    widget->flex_policy.horizontal_percentage = 0;
    widget->flex_policy.vertical_percentage = 0;
    pthread_mutex_unlock(&widget->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_flex_policy_set_fixed_horizontal(nocterm_widget_t* widget){

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&widget->lock);
    widget->flex_policy.horizontal_mode = NOCTERM_WIDGET_FLEX_POLICY_MODE_FIXED;
    widget->flex_policy.horizontal_percentage = 0;
    pthread_mutex_unlock(&widget->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_flex_policy_set_fill_horizontal(nocterm_widget_t* widget){

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(!(widget->flex_policy_permission & NOCTERM_WIDGET_FLEX_POLICY_PERMISSION_HORIZONTAL)){
        errno = EPERM;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&widget->lock);
    widget->flex_policy.horizontal_mode = NOCTERM_WIDGET_FLEX_POLICY_MODE_FILL;
    widget->flex_policy.horizontal_percentage = 0;
    pthread_mutex_unlock(&widget->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_flex_policy_set_percent_horizontal(nocterm_widget_t* widget, nocterm_percentage_t percentage){

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(!(widget->flex_policy_permission & NOCTERM_WIDGET_FLEX_POLICY_PERMISSION_HORIZONTAL)){
        errno = EPERM;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&widget->lock);
    widget->flex_policy.horizontal_mode = NOCTERM_WIDGET_FLEX_POLICY_MODE_PERCENT;
    widget->flex_policy.horizontal_percentage = percentage;
    pthread_mutex_unlock(&widget->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_flex_policy_set_fixed_vertical(nocterm_widget_t* widget){

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&widget->lock);
    widget->flex_policy.vertical_mode = NOCTERM_WIDGET_FLEX_POLICY_MODE_FIXED;
    widget->flex_policy.vertical_percentage = 0;
    pthread_mutex_unlock(&widget->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_flex_policy_set_fill_vertical(nocterm_widget_t* widget){

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(!(widget->flex_policy_permission & NOCTERM_WIDGET_FLEX_POLICY_PERMISSION_VERTICAL)){
        errno = EPERM;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&widget->lock);
    widget->flex_policy.vertical_mode = NOCTERM_WIDGET_FLEX_POLICY_MODE_FILL;
    widget->flex_policy.vertical_percentage = 0;
    pthread_mutex_unlock(&widget->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_flex_policy_set_percent_vertical(nocterm_widget_t* widget, nocterm_percentage_t percentage){

    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(!(widget->flex_policy_permission & NOCTERM_WIDGET_FLEX_POLICY_PERMISSION_VERTICAL)){
        errno = EPERM;
        return NOCTERM_FAILURE;
    }

    pthread_mutex_lock(&widget->lock);
    widget->flex_policy.vertical_mode = NOCTERM_WIDGET_FLEX_POLICY_MODE_PERCENT;
    widget->flex_policy.vertical_percentage = percentage;
    pthread_mutex_unlock(&widget->lock);

    return NOCTERM_SUCCESS;
}

int nocterm_widget_flex_update(nocterm_widget_t* widget){

    if(widget == NULL){
        return NOCTERM_SUCCESS;
    }

    if(widget->flex_policy.horizontal_mode != NOCTERM_WIDGET_FLEX_POLICY_MODE_FIXED ||
       widget->flex_policy.vertical_mode   != NOCTERM_WIDGET_FLEX_POLICY_MODE_FIXED){

        nocterm_dimension_size_t parent_height, parent_width;

        if(widget->parent == NULL){
            parent_height = nocterm_screen_height;
            parent_width = nocterm_screen_width;
        }else{
            nocterm_dimension_size_t ph = widget->parent->viewport.height;
            nocterm_dimension_size_t pw = widget->parent->viewport.width;
            nocterm_dimension_size_t pad_h = 2 * (nocterm_dimension_size_t)widget->parent->flex_policy_inner_padding_h;
            nocterm_dimension_size_t pad_w = 2 * (nocterm_dimension_size_t)widget->parent->flex_policy_inner_padding_w;
            parent_height = (ph > pad_h) ? ph - pad_h : 0;
            parent_width  = (pw > pad_w) ? pw - pad_w : 0;
        }

        nocterm_dimension_size_t new_height = widget->bounds.height;
        nocterm_dimension_size_t new_width  = widget->bounds.width;

        if(widget->flex_policy.horizontal_mode == NOCTERM_WIDGET_FLEX_POLICY_MODE_FILL){
            new_width = parent_width;
        }else if(widget->flex_policy.horizontal_mode == NOCTERM_WIDGET_FLEX_POLICY_MODE_PERCENT){
            new_width = (parent_width * widget->flex_policy.horizontal_percentage) / 100;
        }

        if(widget->flex_policy.vertical_mode == NOCTERM_WIDGET_FLEX_POLICY_MODE_FILL){
            new_height = parent_height;
        }else if(widget->flex_policy.vertical_mode == NOCTERM_WIDGET_FLEX_POLICY_MODE_PERCENT){
            new_height = (parent_height * widget->flex_policy.vertical_percentage) / 100;
        }

        if(new_height != widget->bounds.height || new_width != widget->bounds.width){

            pthread_mutex_lock(&widget->lock);

            nocterm_widget_buffer_resize(widget, new_height, new_width);

            if(widget->internal_resize_handler){
                widget->internal_resize_handler(widget, widget->bounds, widget->viewport);
            }

            if(widget->resize_handler){
                widget->resize_handler(widget, widget->bounds, widget->viewport);
            }

            pthread_mutex_unlock(&widget->lock);
        }
    }

    for(uint64_t i = 0; i < widget->subwidgets_size; i++){
        nocterm_widget_flex_update(widget->subwidgets[i]);
    }

    return NOCTERM_SUCCESS;
}

int nocterm_widget_flex(nocterm_widget_t* widget, nocterm_widget_flex_t flex, ...){
    
    if(widget == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    switch(flex){

        case NOCTERM_WIDGET_FLEX_FIXED_BOTH:{
            return nocterm_widget_flex_policy_set_fixed_both(widget);
        }break;

        case NOCTERM_WIDGET_FLEX_FIXED_HORIZONTAL:{
            return nocterm_widget_flex_policy_set_fixed_horizontal(widget);
        }break;

        case NOCTERM_WIDGET_FLEX_FIXED_VERTICAL:{
            return nocterm_widget_flex_policy_set_fixed_vertical(widget);
        }break;
        
        case NOCTERM_WIDGET_FLEX_FILL_HORIZONTAL:{
            return nocterm_widget_flex_policy_set_fill_horizontal(widget);
        }break;
   
        case NOCTERM_WIDGET_FLEX_FILL_VERTICAL:{
            return nocterm_widget_flex_policy_set_fill_vertical(widget);
        }break;

        case NOCTERM_WIDGET_FLEX_FILL_BOTH:{
            return nocterm_widget_flex_policy_set_fill_both(widget);
        }break;

        case NOCTERM_WIDGET_FLEX_PERCENT_HORIZONTAL:{
            va_list vl = {0};
            va_start(vl, flex);
            int percentage = va_arg(vl, int);
            va_end(vl);

            if(percentage >= 0 && percentage <= 100){
                return nocterm_widget_flex_policy_set_percent_horizontal(widget, percentage);
            }else{
                return NOCTERM_FAILURE;
            }

        }break;

        case NOCTERM_WIDGET_FLEX_PERCENT_VERTICAL:{
            va_list vl = {0};
            va_start(vl, flex);
            int percentage = va_arg(vl, int);
            va_end(vl);

            if(percentage >= 0 && percentage <= 100){
                return nocterm_widget_flex_policy_set_percent_vertical(widget, percentage);
            }else{
                return NOCTERM_FAILURE;
            }
        }break;

        default:{
            return NOCTERM_FAILURE;
        }break;
        
    }
}

NOCTERM_INTERNAL
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
                
                nocterm_widget_buffer_size_t buffer_index = (nocterm_widget_buffer_size_t)(widget->viewport.row + row) * widget->bounds.width + (widget->viewport.col + col);

                if(widget->hard_refresh || widget->buffer[buffer_index].refresh){
                    
                    uint64_t absolute_row = relative_row + row;
                    uint64_t absolute_col = relative_col + col;

                    uint64_t screen_index = absolute_row * nocterm_screen_width + absolute_col;
                    uint64_t screen_size = nocterm_screen_height * nocterm_screen_width;
                    nocterm_screen_ownership_t* current_ownership = NULL;

                    if(absolute_row >= nocterm_screen_height || absolute_col >= nocterm_screen_width) {
                        // Skip this cell - it's outside screen boundaries
                        continue;
                    }

                    // Check widget positional cell access restriction
                    nocterm_widget_t* parent_iterator = widget->parent;
                    bool has_cell_access = true;
                    while(parent_iterator != NULL){

                        nocterm_dimension_size_t parent_absolute_row, parent_absolute_col;
                        nocterm_widget_get_position(parent_iterator, &parent_absolute_row, &parent_absolute_col);

                        if(parent_iterator->floating_subwidgets == false && parent_iterator->bounds.width * parent_iterator->bounds.height > 0 && (
                            (absolute_row < parent_absolute_row) ||
                            (absolute_row >= parent_absolute_row + parent_iterator->viewport.height) ||
                            (absolute_col < parent_absolute_col) ||
                            (absolute_col >= parent_absolute_col + parent_iterator->viewport.width)
                        )){
                            // Skip this cell, floating_widgets is not allowed and cell position is not inside the bounds of its parent.
                            has_cell_access = false;
                            break;
                        }else if(parent_iterator->floating_subwidgets == true || parent_iterator->bounds.width * parent_iterator->bounds.height == 0){
                            has_cell_access = true;
                            break;
                        }

                        parent_iterator = parent_iterator->parent;
                    }
                    if(has_cell_access == false){
                        continue;
                    }
                    // If widget has no parent, then this check is unnecessary

                    if(screen_index < screen_size){
                        current_ownership = &(nocterm_screen_ownership[screen_index]);
                    }

                    if(current_ownership && (current_ownership->owner == (void*)widget || current_ownership->owner == NULL)){

                        if(widget->buffer[buffer_index].character.bytes_size != 0){
                            
                            if(nocterm_attribute_apply(widget->buffer[buffer_index].attribute) == NOCTERM_FAILURE){
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
