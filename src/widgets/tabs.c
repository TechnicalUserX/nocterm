#include <nocterm/widgets/tabs.h>

nocterm_tabs_t* nocterm_tabs_new(nocterm_dimension_size_t height, nocterm_dimension_size_t width){

    nocterm_tabs_t* new_tabs = (nocterm_tabs_t*)malloc(sizeof(nocterm_tabs_t));

    if(new_tabs == NULL){
        return NULL;
    }

    memset(new_tabs, 0x0, sizeof(nocterm_tabs_t));

    if(nocterm_tabs_constructor(new_tabs, height, width) == NOCTERM_FAILURE){
        free(new_tabs);
        return NULL;
    }

    return new_tabs;
}

int nocterm_tabs_constructor(nocterm_tabs_t* tabs, nocterm_dimension_size_t height, nocterm_dimension_size_t width){

    if(tabs == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    // The tabs widget is a structural container: it draws nothing of its own and
    // stays out of the focus order, so its tab roots' focusable descendants are
    // reached directly.  A virtual widget claims no screen cells.
    if(nocterm_widget_constructor(NOCTERM_WIDGET(tabs), height, width, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_VIRTUAL) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    tabs->roots = NULL;
    tabs->roots_size = 0;
    tabs->active_index = 0;

    return NOCTERM_SUCCESS;
}

int nocterm_tabs_destructor(nocterm_tabs_t* tabs){

    if(tabs == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    // Only the bookkeeping array is freed; the tab root widgets are owned by the
    // caller.
    free(tabs->roots);
    tabs->roots = NULL;
    tabs->roots_size = 0;

    if(nocterm_widget_destructor(NOCTERM_WIDGET(tabs)) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    return NOCTERM_SUCCESS;
}

int nocterm_tabs_delete(nocterm_tabs_t* tabs){

    if(nocterm_tabs_destructor(tabs) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    free(tabs);

    return NOCTERM_SUCCESS;
}

int nocterm_tabs_add(nocterm_tabs_t* tabs, nocterm_widget_t* root){

    if(tabs == NULL || root == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(nocterm_widget_add_subwidget(NOCTERM_WIDGET(tabs), root) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    nocterm_widget_t** grown = (nocterm_widget_t**)realloc(tabs->roots, sizeof(nocterm_widget_t*) * (tabs->roots_size + 1));

    if(grown == NULL){
        nocterm_widget_remove_subwidget(NOCTERM_WIDGET(tabs), root);
        return NOCTERM_FAILURE;
    }

    tabs->roots = grown;
    tabs->roots[tabs->roots_size] = root;

    // The first tab is shown; every later tab starts hidden.
    nocterm_widget_set_visible(root, tabs->roots_size == 0);
    if(tabs->roots_size == 0){
        tabs->active_index = 0;
    }

    tabs->roots_size++;

    return NOCTERM_SUCCESS;
}

int nocterm_tabs_navigate(nocterm_tabs_t* tabs, uint64_t index){

    if(tabs == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(index >= tabs->roots_size){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(tabs->active_index != index && tabs->active_index < tabs->roots_size){
        nocterm_widget_set_visible(tabs->roots[tabs->active_index], false);
    }

    nocterm_widget_set_visible(tabs->roots[index], true);
    tabs->active_index = index;

    return NOCTERM_SUCCESS;
}

uint64_t nocterm_tabs_get_count(nocterm_tabs_t* tabs){

    if(tabs == NULL){
        errno = EINVAL;
        return 0;
    }

    return tabs->roots_size;
}

int64_t nocterm_tabs_get_active_index(nocterm_tabs_t* tabs){

    if(tabs == NULL){
        errno = EINVAL;
        return -1;
    }

    if(tabs->roots_size == 0){
        return -1;
    }

    return (int64_t)tabs->active_index;
}

nocterm_widget_t* nocterm_tabs_get_active(nocterm_tabs_t* tabs){

    if(tabs == NULL){
        errno = EINVAL;
        return NULL;
    }

    if(tabs->roots_size == 0){
        return NULL;
    }

    return tabs->roots[tabs->active_index];
}

nocterm_widget_t* nocterm_tabs_get_root(nocterm_tabs_t* tabs, uint64_t index){

    if(tabs == NULL){
        errno = EINVAL;
        return NULL;
    }

    if(index >= tabs->roots_size){
        errno = EINVAL;
        return NULL;
    }

    return tabs->roots[index];
}
