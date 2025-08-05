#include <nocterm/widgets/listview.h>

NOCTERM_WIDGET_KEY_HANDLER(nocterm_listview_key_handler){
    switch(nocterm_key_translate(key)){

        case NOCTERM_KEY_EVENT_LEFT:{
            // Slide top
            nocterm_listview_move_top(NOCTERM_LISTVIEW(self));
        }break;

        case NOCTERM_KEY_EVENT_RIGHT:{
            // Slide bottom
            nocterm_listview_move_bottom(NOCTERM_LISTVIEW(self));
        }break;

        case NOCTERM_KEY_EVENT_UP:{
            // Slide up
            nocterm_listview_move_up(NOCTERM_LISTVIEW(self));
        }break;

        case NOCTERM_KEY_EVENT_DOWN:{
            // Slide down
            nocterm_listview_move_down(NOCTERM_LISTVIEW(self));
        }break;

        default:
    }
}

nocterm_listview_t* nocterm_listview_new(nocterm_dimension_size_t row, nocterm_dimension_size_t col, nocterm_dimension_size_t items_displayed, uint64_t items_total, nocterm_dimension_size_t item_width){

    nocterm_listview_t* new_listview = (nocterm_listview_t*)malloc(sizeof(nocterm_listview_t));

    if(new_listview == NULL){
        return NULL;
    }
    memset(new_listview, 0x0, sizeof(nocterm_listview_t));

    if(nocterm_widget_constructor(NOCTERM_WIDGET(new_listview),(nocterm_dimension_t){row, col, items_total, item_width}, true, false) == NOCTERM_FAILURE){
        return NULL;
    }

    nocterm_widget_viewport(NOCTERM_WIDGET(new_listview), (nocterm_dimension_t){0, 0, items_displayed, NOCTERM_WIDGET(new_listview)->bounds.width});
    
    new_listview->item_array = nocterm_listview_item_array_new();
    if(new_listview->item_array == NULL){
        free(new_listview);
        return NULL;
    }

    nocterm_widget_add_key_handler(NOCTERM_WIDGET(new_listview), nocterm_listview_key_handler);

    return new_listview;
}

int nocterm_listview_delete(nocterm_listview_t* listview){

    nocterm_listview_item_array_delete(listview->item_array);

    if(nocterm_widget_destructor(NOCTERM_WIDGET(listview)) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    free(listview);

    return NOCTERM_SUCCESS;
}

int nocterm_listview_push_back(nocterm_listview_t* listview, nocterm_listview_item_t item){
    
    if(listview == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(listview->item_array->size == listview->widget.bounds.height){
        errno = ENOMEM;
        return NOCTERM_FAILURE;
    }

    if(nocterm_listview_item_array_push_back(listview->item_array, item) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    // Widget Update
    if(listview->item_array->size < listview->widget.viewport.width){
        // Add items to the buffer only if the displayed viewport height is enough to cover it
        // Otherwise, the user is obliged to change the viewport by arrow keys
        for(uint64_t i = 0; i < listview->widget.bounds.width && i < item.content_length; i++){
            nocterm_widget_update(NOCTERM_WIDGET(listview), listview->item_array->size-1,i, item.content[i].character, item.content[i].attribute);
        }

    }
    if(listview->autoscroll){
        // Autoscroll downwards
        if(listview->item_array->size > NOCTERM_WIDGET(listview)->viewport.height){
            nocterm_widget_viewport(NOCTERM_WIDGET(listview), (nocterm_dimension_t){listview->item_array->size - NOCTERM_WIDGET(listview)->viewport.height, 0, NOCTERM_WIDGET(listview)->viewport.height, NOCTERM_WIDGET(listview)->viewport.width});
        }

    }

    return NOCTERM_SUCCESS;
}

int nocterm_listview_push_front(nocterm_listview_t* listview, nocterm_listview_item_t item){

    if(listview == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(listview->item_array->size == listview->widget.bounds.height){
        errno = ENOMEM;
        return NOCTERM_FAILURE;
    }

    nocterm_listview_item_array_push_front(listview->item_array, item);

    // Widget Update
    nocterm_widget_clear(NOCTERM_WIDGET(listview));
    for(uint64_t i = 0; i < listview->item_array->size; i++){
        for(uint64_t j = 0; j < listview->item_array->items[i].content_length && j < listview->widget.bounds.width ; j++){
            nocterm_widget_update(NOCTERM_WIDGET(listview), i, j, listview->item_array->items[i].content[j].character, listview->item_array->items[i].content[j].attribute);
        }
    }
    
    if(listview->autoscroll){
        // Autoscroll downwards
        if(listview->item_array->size > NOCTERM_WIDGET(listview)->viewport.height){
            nocterm_widget_viewport(NOCTERM_WIDGET(listview), (nocterm_dimension_t){0, 0, NOCTERM_WIDGET(listview)->viewport.height, NOCTERM_WIDGET(listview)->viewport.width});
        }
    }

    NOCTERM_WIDGET(listview)->hard_refresh = true;

    return NOCTERM_SUCCESS;
}

int nocterm_listview_pop_back(nocterm_listview_t* listview, nocterm_listview_item_t* item){

    if(listview == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(listview->item_array->size == 0){
        return NOCTERM_SUCCESS;
    }

    // Widget Update
    // If there are more items than viewport height, viewport must go one up after deletion and also viewport is at the end
    for(uint64_t i = 0; i < listview->item_array->items[listview->item_array->size-1].content_length && i < listview->widget.bounds.width ; i++){
        nocterm_widget_update(NOCTERM_WIDGET(listview),listview->item_array->size-1, i, NOCTERM_CHAR_EMPTY, NOCTERM_ATTRIBUTE_EMPTY);
    }

    if(NOCTERM_WIDGET(listview)->viewport.row + NOCTERM_WIDGET(listview)->viewport.height == listview->item_array->size){
        // We are at most bottom edge

        if(listview->item_array->size > NOCTERM_WIDGET(listview)->viewport.height){
            // There are items at the top
            nocterm_widget_viewport_up(NOCTERM_WIDGET(listview));
        }
    }

    if(nocterm_listview_item_array_pop_back(listview->item_array, item) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    NOCTERM_WIDGET(listview)->hard_refresh = true;

    return NOCTERM_SUCCESS;
}

int nocterm_listview_pop_front(nocterm_listview_t* listview, nocterm_listview_item_t* item){

    if(listview == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(listview->item_array->size == 0){
        return NOCTERM_SUCCESS;
    }

    if(listview->item_array->size == 1){
        nocterm_listview_item_array_pop_back(listview->item_array, item);
        nocterm_widget_clear(NOCTERM_WIDGET(listview));
        NOCTERM_WIDGET(listview)->hard_refresh = true;
        return NOCTERM_SUCCESS;
    }


    if(nocterm_listview_item_array_pop_front(listview->item_array, item) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    // Widget Update
    nocterm_widget_clear(NOCTERM_WIDGET(listview));
    for(uint64_t i = 0; i < listview->item_array->size; i++){
        for(uint64_t j = 0; j < listview->item_array->items[i].content_length && j < listview->widget.bounds.width; j++){
            nocterm_widget_update(NOCTERM_WIDGET(listview), i, j, listview->item_array->items[i].content[j].character, listview->item_array->items[i].content[j].attribute);
        }
    }

    if(NOCTERM_WIDGET(listview)->viewport.row + NOCTERM_WIDGET(listview)->viewport.height == listview->item_array->size+1){
        // We are at most bottom edge
        if(NOCTERM_WIDGET(listview)->viewport.row > 0 ){
            // There are items at the top
            nocterm_widget_viewport_up(NOCTERM_WIDGET(listview));
        }
    }

    NOCTERM_WIDGET(listview)->hard_refresh = true;

    return NOCTERM_SUCCESS;
}

int nocterm_listview_item_constructor(nocterm_listview_item_t* item, const char* text, uint64_t text_size, nocterm_attribute_t attribute){

    if(item == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    nocterm_listview_item_t new_item = {0};

    nocterm_char_t content_string[NOCTERM_LISTVIEW_ITEM_CONTENT_MAX_SIZE] = {0};

    uint64_t content_string_length = nocterm_char_string_from_stream(content_string, NOCTERM_LISTVIEW_ITEM_CONTENT_MAX_SIZE, text, text_size);

    if(content_string_length == 0){
        return NOCTERM_FAILURE;
    }

    new_item.content_length = content_string_length;
    for(uint64_t i = 0; i < content_string_length; i++){
        new_item.content[i].character = content_string[i];
        new_item.content[i].attribute = attribute;
    }

    *item = new_item;

    return NOCTERM_SUCCESS;
}

int nocterm_listview_set_autoscroll(nocterm_listview_t* listview, bool autoscroll){
    if(listview == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    listview->autoscroll = autoscroll;

    return NOCTERM_SUCCESS;
}

int nocterm_listview_clear(nocterm_listview_t* listview){

    if(listview == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(nocterm_listview_item_array_clear(listview->item_array) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    nocterm_widget_clear(NOCTERM_WIDGET(listview));

    NOCTERM_WIDGET(listview)->hard_refresh = true;
    
    return NOCTERM_SUCCESS;
}

int nocterm_listview_move_up(nocterm_listview_t* listview){
    
    if(listview == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    nocterm_widget_viewport_up(NOCTERM_WIDGET(listview));          

    return NOCTERM_SUCCESS;
}

int nocterm_listview_move_down(nocterm_listview_t* listview){
    
    if(listview == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }
    
    if(NOCTERM_WIDGET(listview)->viewport.row + NOCTERM_WIDGET(listview)->viewport.height < NOCTERM_LISTVIEW(listview)->item_array->size){
        nocterm_widget_viewport_down(NOCTERM_WIDGET(listview));
    }

    return NOCTERM_SUCCESS;
}

int nocterm_listview_move_top(nocterm_listview_t* listview){

    if(listview == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    nocterm_widget_viewport(NOCTERM_WIDGET(listview),(nocterm_dimension_t){0,0, NOCTERM_WIDGET(listview)->viewport.height, NOCTERM_WIDGET(listview)->viewport.width});

    return NOCTERM_SUCCESS;
}

int nocterm_listview_move_bottom(nocterm_listview_t* listview){

    if(listview == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(listview->item_array->size > NOCTERM_WIDGET(listview)->viewport.height){
        nocterm_widget_viewport(NOCTERM_WIDGET(listview), (nocterm_dimension_t){listview->item_array->size -  NOCTERM_WIDGET(listview)->viewport.height, 0, NOCTERM_WIDGET(listview)->viewport.height, NOCTERM_WIDGET(listview)->viewport.width});
    }

    return NOCTERM_SUCCESS;
}

// Dynamic Array Implementation

nocterm_listview_item_array_t* nocterm_listview_item_array_new(void){
    nocterm_listview_item_array_t* new_dynamic_array = (nocterm_listview_item_array_t*)malloc(sizeof(nocterm_listview_item_array_t));
    if(new_dynamic_array == NULL){
        return NULL;
    }
    new_dynamic_array->capacity = 0;
    new_dynamic_array->size = 0;
    new_dynamic_array->items = NULL;
    return new_dynamic_array;
}

void nocterm_listview_item_array_delete(nocterm_listview_item_array_t* dynamic_array){
    if(dynamic_array == NULL){
        return;
    }    
    free(dynamic_array->items);
    free(dynamic_array);
}

void nocterm_listview_item_array_increase_capacity(nocterm_listview_item_array_t* dynamic_array){
    if(dynamic_array == NULL){
        return;
    }
    if(dynamic_array->size == dynamic_array->capacity){
        uint64_t new_capacity = dynamic_array->capacity * 2 == 0 ? 1 : dynamic_array->capacity * 2;
        if(new_capacity < dynamic_array->capacity){
            exit(NOCTERM_FAILURE);
        }
        nocterm_listview_item_t* new_items = (nocterm_listview_item_t*)malloc(sizeof(nocterm_listview_item_t) * (new_capacity));
        if(new_items == NULL){
            exit(NOCTERM_FAILURE);
        }        
        if(dynamic_array->items){
            memcpy(new_items, dynamic_array->items, sizeof(nocterm_listview_item_t) * dynamic_array->size);
            free(dynamic_array->items);
        }
        dynamic_array->items = new_items;
        dynamic_array->capacity = new_capacity;
    }
}

int nocterm_listview_item_array_push_back(nocterm_listview_item_array_t* dynamic_array, nocterm_listview_item_t item){
    if(dynamic_array == NULL){
        return NOCTERM_FAILURE;
    }    
    nocterm_listview_item_array_increase_capacity(dynamic_array);    
    dynamic_array->items[dynamic_array->size] = item;
    dynamic_array->size++;
    return NOCTERM_SUCCESS;
}

int nocterm_listview_item_array_pop_back(nocterm_listview_item_array_t* dynamic_array, nocterm_listview_item_t* item){
    if(dynamic_array == NULL){
        return NOCTERM_FAILURE;
    }    
    if(dynamic_array->size == 0){
        return NOCTERM_SUCCESS;
    }    
    if(item != NULL){
        *item = dynamic_array->items[dynamic_array->size-1];
    }
    dynamic_array->size--;
    return NOCTERM_SUCCESS;
}

int nocterm_listview_item_array_push_front(nocterm_listview_item_array_t* dynamic_array, nocterm_listview_item_t item){
    if(dynamic_array == NULL){
        return NOCTERM_FAILURE;
    }    
    nocterm_listview_item_array_increase_capacity(dynamic_array);    
    memmove(&(dynamic_array->items[1]), &(dynamic_array->items[0]), sizeof(nocterm_listview_item_t) * dynamic_array->size);
    memcpy(&(dynamic_array->items[0]), &item, sizeof(nocterm_listview_item_t));
    dynamic_array->size++;
    return NOCTERM_SUCCESS;
}

int nocterm_listview_item_array_pop_front(nocterm_listview_item_array_t* dynamic_array, nocterm_listview_item_t* item){
    if(dynamic_array == NULL){
        return NOCTERM_FAILURE;
    }    
    if(dynamic_array->size == 0){
        return NOCTERM_SUCCESS;
    }    
    if(item != NULL){
        *item = dynamic_array->items[0];
    }
    memmove(&(dynamic_array->items[0]), &(dynamic_array->items[1]), sizeof(nocterm_listview_item_t) * (dynamic_array->size - 1));
    dynamic_array->size--;
    return NOCTERM_SUCCESS;
}

int nocterm_listview_item_array_insert(nocterm_listview_item_array_t* dynamic_array, nocterm_listview_item_t item, uint64_t index){
    if(dynamic_array == NULL){
        return NOCTERM_FAILURE;
    }
    if(index > dynamic_array->size){
        return NOCTERM_FAILURE;
    }
    nocterm_listview_item_array_increase_capacity(dynamic_array);
    memmove(&(dynamic_array->items[index+1]), &(dynamic_array->items[index]), sizeof(nocterm_listview_item_t) * (dynamic_array->size - index) );
    dynamic_array->items[index] = item;
    dynamic_array->size++;
    return NOCTERM_SUCCESS;
}

int nocterm_listview_item_array_remove(nocterm_listview_item_array_t* dynamic_array,  nocterm_listview_item_t* item, uint64_t index){
    if(dynamic_array == NULL){
        return NOCTERM_FAILURE;
    }
    if(dynamic_array->size == 0){
        return NOCTERM_FAILURE;
    }
    if(index >= dynamic_array->size){
        return NOCTERM_FAILURE;
    }
    if(item != NULL){
        *item = dynamic_array->items[index];
    }
    memmove(&(dynamic_array->items[index]), &(dynamic_array->items[index+1]), (dynamic_array->size - index - 1) * sizeof(nocterm_listview_item_t));
    dynamic_array->size--;
    return NOCTERM_SUCCESS;
}

int nocterm_listview_item_array_clear(nocterm_listview_item_array_t* dynamic_array){
    if(dynamic_array == NULL){
        return NOCTERM_FAILURE;
    }
    dynamic_array->size = 0;
    return NOCTERM_SUCCESS;
}

int nocterm_listview_item_array_shrink_to_fit(nocterm_listview_item_array_t* dynamic_array){
    if(dynamic_array == NULL){
        return NOCTERM_FAILURE;
    }
    if (dynamic_array->size == dynamic_array->capacity){
        return NOCTERM_SUCCESS;
    }

    nocterm_listview_item_t* new_items = (nocterm_listview_item_t*)malloc(sizeof(nocterm_listview_item_t) * (dynamic_array->size));
    if(new_items == NULL){
        exit(NOCTERM_FAILURE);
    }
    memcpy(new_items, dynamic_array->items, sizeof(nocterm_listview_item_t) * dynamic_array->size);
    free(dynamic_array->items);
    dynamic_array->items = new_items;
    dynamic_array->capacity = dynamic_array->size;
    return NOCTERM_SUCCESS;
}
