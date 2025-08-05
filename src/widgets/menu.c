#include <nocterm/widgets/menu.h>

nocterm_menu_t* nocterm_menu_new(nocterm_dimension_size_t row, nocterm_dimension_size_t col, nocterm_attribute_t attribute_selection, nocterm_dimension_size_t items_displayed, uint64_t items_total, nocterm_dimension_size_t item_width, nocterm_menu_onselect_handler_t onselect_handler, void* user_data){
    nocterm_menu_t* new_menu = (nocterm_menu_t*)malloc(sizeof(nocterm_menu_t));

    if(new_menu == NULL){
        return NULL;
    }
    memset(new_menu, 0x0, sizeof(nocterm_menu_t));

    if(nocterm_widget_constructor(NOCTERM_WIDGET(new_menu), (nocterm_dimension_t){row, col, items_total, item_width}, true, false) == NOCTERM_FAILURE){
        return NULL;
    }

    nocterm_widget_viewport(NOCTERM_WIDGET(new_menu), (nocterm_dimension_t){0, 0, items_displayed, NOCTERM_WIDGET(new_menu)->bounds.width});
    
    new_menu->item_array = nocterm_menu_item_array_new();
    if(new_menu->item_array == NULL){
        free(new_menu);
        return NULL;
    }

    new_menu->current_item = 0;
    new_menu->selection_position = 0;
    new_menu->attribute_selection = attribute_selection;
    new_menu->onselect_handler = onselect_handler;
    new_menu->user_data = user_data;

    nocterm_widget_add_key_handler(NOCTERM_WIDGET(new_menu), nocterm_menu_key_handler);
    nocterm_widget_add_focus_handler(NOCTERM_WIDGET(new_menu), nocterm_menu_focus_handler);

    return new_menu;
}

int nocterm_menu_delete(nocterm_menu_t* menu){

    nocterm_menu_item_array_delete(menu->item_array);

    if(nocterm_widget_destructor(NOCTERM_WIDGET(menu)) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    free(menu);

    return NOCTERM_SUCCESS;
}

int nocterm_menu_add_item(nocterm_menu_t* menu, nocterm_menu_item_t item){

    if(menu == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(menu->item_array->size == menu->widget.bounds.height){
        errno = ENOMEM;
        return NOCTERM_FAILURE;
    }

    if(nocterm_menu_item_array_push_back(menu->item_array, item) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    // Widget Update
    if(menu->item_array->size < menu->widget.viewport.width){
        // Add items to the buffer only if the displayed viewport height is enough to cover it
        // Otherwise, the user is obliged to change the viewport by arrow keys
        for(uint64_t i = 0; i < menu->widget.bounds.width && i < item.content_length; i++){
            nocterm_widget_update(NOCTERM_WIDGET(menu), menu->item_array->size-1,i, item.content[i].character, item.content[i].attribute);
        }

    }

    return NOCTERM_SUCCESS;
}

int nocterm_menu_add_item_multiple(nocterm_menu_t* menu, nocterm_menu_item_t* items, uint64_t items_size){
    if(menu == NULL || items == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(items_size == 0){
        return NOCTERM_SUCCESS;
    }

    for(uint64_t i = 0; i < items_size; i++){
        if(nocterm_menu_add_item(menu, items[i]) == NOCTERM_FAILURE){
            return NOCTERM_FAILURE;
        }
    }

    return NOCTERM_SUCCESS;
}

int nocterm_menu_item_constructor(nocterm_menu_item_t* item, const char* text, uint64_t text_size, nocterm_attribute_t attribute){

    if(item == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    nocterm_menu_item_t new_item = {0};

    nocterm_char_t content_string[NOCTERM_MENU_ITEM_CONTENT_MAX_SIZE] = {0};

    uint64_t content_string_length = nocterm_char_string_from_stream(content_string, NOCTERM_MENU_ITEM_CONTENT_MAX_SIZE, text, text_size);

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

int nocterm_menu_clear(nocterm_menu_t* menu){

    if(menu == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    if(nocterm_menu_item_array_clear(menu->item_array) == NOCTERM_FAILURE){
        return NOCTERM_FAILURE;
    }

    nocterm_widget_clear(NOCTERM_WIDGET(menu));

    NOCTERM_WIDGET(menu)->hard_refresh = true;

    return NOCTERM_SUCCESS;
}

int nocterm_menu_get_selection(nocterm_menu_t* menu, uint64_t* selection){

    return NOCTERM_SUCCESS;
}

int nocterm_menu_selection_move_up(nocterm_menu_t* menu){

    if(menu == NULL){
        return EINVAL;
        return NOCTERM_FAILURE;
    }

    if(menu->selection_position == 0 && menu->current_item == 0){
        // We are at the topmost possible with the selection
        return NOCTERM_SUCCESS;
    }

    // If selection position is greater than 0, then it is guaranteed that we can move up
    // And also current_item position is guaranteed to be greater than zero, so...
    if(menu->selection_position > 0){

        // Setting current item to it's original attribute
        for(uint64_t i = 0; i < menu->item_array->items[menu->current_item].content_length && i < NOCTERM_WIDGET(menu)->bounds.width; i++){
            nocterm_widget_update(NOCTERM_WIDGET(menu), menu->current_item, i, menu->item_array->items[menu->current_item].content[i].character , menu->item_array->items[menu->current_item].content[i].attribute);
        }
        for(uint64_t i = 0; i < menu->item_array->items[menu->current_item-1].content_length && i < NOCTERM_WIDGET(menu)->bounds.width; i++){
            nocterm_widget_update(NOCTERM_WIDGET(menu), menu->current_item-1, i, menu->item_array->items[menu->current_item-1].content[i].character , menu->attribute_selection);
        }

        menu->selection_position--;
        menu->current_item--;

    }else{
        // Selection is at the topmost position, but let's see whether there are more items on the top side?
        if(menu->current_item > 0){
            for(uint64_t i = 0; i < menu->item_array->items[menu->current_item].content_length && i < NOCTERM_WIDGET(menu)->bounds.width; i++){
                nocterm_widget_update(NOCTERM_WIDGET(menu), menu->current_item, i, menu->item_array->items[menu->current_item].content[i].character , menu->item_array->items[menu->current_item].content[i].attribute);
            }
            for(uint64_t i = 0; i < menu->item_array->items[menu->current_item-1].content_length && i < NOCTERM_WIDGET(menu)->bounds.width; i++){
                nocterm_widget_update(NOCTERM_WIDGET(menu), menu->current_item-1, i, menu->item_array->items[menu->current_item-1].content[i].character , menu->attribute_selection);
            }
            menu->current_item--;
            nocterm_widget_viewport_up(NOCTERM_WIDGET(menu));

        }

    }
    
    return NOCTERM_SUCCESS;
}

int nocterm_menu_selection_move_down(nocterm_menu_t* menu){
    if(menu == NULL){
        return EINVAL;
        return NOCTERM_FAILURE;
    }

    if(menu->current_item < menu->item_array->size - 1){
        // At least 1 character to go down

        for(uint64_t i = 0; i < menu->item_array->items[menu->current_item].content_length && i < NOCTERM_WIDGET(menu)->bounds.width; i++){
            nocterm_widget_update(NOCTERM_WIDGET(menu), menu->current_item, i, menu->item_array->items[menu->current_item].content[i].character , menu->item_array->items[menu->current_item].content[i].attribute);
        }
        for(uint64_t i = 0; i < menu->item_array->items[menu->current_item+1].content_length && i < NOCTERM_WIDGET(menu)->bounds.width; i++){
            nocterm_widget_update(NOCTERM_WIDGET(menu), menu->current_item+1, i, menu->item_array->items[menu->current_item+1].content[i].character , menu->attribute_selection);
        }

        if(menu->selection_position == NOCTERM_WIDGET(menu)->viewport.height - 1){
            nocterm_widget_viewport_down(NOCTERM_WIDGET(menu));

        }else{
            menu->selection_position++;
        }
        menu->current_item++;
    }

    return NOCTERM_SUCCESS;
}

int nocterm_menu_selection_move_top(nocterm_menu_t* menu){
    if(menu == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    for(uint64_t i = 0; i < menu->item_array->items[menu->current_item].content_length && i < NOCTERM_WIDGET(menu)->bounds.width; i++){
        nocterm_widget_update(NOCTERM_WIDGET(menu), menu->current_item, i, menu->item_array->items[menu->current_item].content[i].character , menu->item_array->items[menu->current_item].content[i].attribute);
    }
    for(uint64_t i = 0; i < menu->item_array->items[0].content_length && i < NOCTERM_WIDGET(menu)->bounds.width; i++){
        nocterm_widget_update(NOCTERM_WIDGET(menu), 0, i, menu->item_array->items[0].content[i].character , menu->attribute_selection);
    }
    
    menu->current_item = 0;
    menu->selection_position = 0;

    nocterm_widget_viewport(NOCTERM_WIDGET(menu), (nocterm_dimension_t){0,0, NOCTERM_WIDGET(menu)->viewport.height, NOCTERM_WIDGET(menu)->viewport.width});

    return NOCTERM_SUCCESS;
}

int nocterm_menu_selection_move_bottom(nocterm_menu_t* menu){
    if(menu == NULL){
        errno = EINVAL;
        return NOCTERM_FAILURE;
    }

    for(uint64_t i = 0; i < menu->item_array->items[menu->current_item].content_length && i < NOCTERM_WIDGET(menu)->bounds.width; i++){
        nocterm_widget_update(NOCTERM_WIDGET(menu), menu->current_item, i, menu->item_array->items[menu->current_item].content[i].character , menu->item_array->items[menu->current_item].content[i].attribute);
    }
    for(uint64_t i = 0; i < menu->item_array->items[menu->item_array->size-1].content_length && i < NOCTERM_WIDGET(menu)->bounds.width; i++){
        nocterm_widget_update(NOCTERM_WIDGET(menu), menu->item_array->size-1, i, menu->item_array->items[menu->item_array->size-1].content[i].character , menu->attribute_selection);
    }

    if(menu->item_array->size > NOCTERM_WIDGET(menu)->viewport.height){
        // There are items more than the viewport height
        // So the last item is OK but we need to find the viewport

        nocterm_widget_viewport(NOCTERM_WIDGET(menu), (nocterm_dimension_t){menu->item_array->size -  NOCTERM_WIDGET(menu)->viewport.height, 0, NOCTERM_WIDGET(menu)->viewport.height, NOCTERM_WIDGET(menu)->viewport.width});
        menu->selection_position = NOCTERM_WIDGET(menu)->viewport.height-1;
        menu->current_item = menu->item_array->size-1;
    }else{
        // There are no more items than the viewport height
        menu->current_item = menu->item_array->size-1;
        menu->selection_position = menu->item_array->size-1;
    }


    return NOCTERM_SUCCESS;
}

NOCTERM_WIDGET_KEY_HANDLER(nocterm_menu_key_handler){
    
    if(NOCTERM_MENU(self)->item_array->size == 0){
        return;
    }

    switch(nocterm_key_translate(key)){

        case NOCTERM_KEY_EVENT_RIGHT:{
            nocterm_menu_selection_move_bottom(NOCTERM_MENU(self));
        }break;

        case NOCTERM_KEY_EVENT_LEFT:{
            nocterm_menu_selection_move_top(NOCTERM_MENU(self));
        }break;

        case NOCTERM_KEY_EVENT_UP:{
            nocterm_menu_selection_move_up(NOCTERM_MENU(self));
        }break;

        case NOCTERM_KEY_EVENT_DOWN:{
            nocterm_menu_selection_move_down(NOCTERM_MENU(self));
        }break;

        case NOCTERM_KEY_EVENT_ENTER:{
            if(NOCTERM_MENU(self)->onselect_handler){
                NOCTERM_MENU(self)->onselect_handler(self, NOCTERM_MENU(self)->current_item, NOCTERM_MENU(self)->user_data);
            }
        }break;

        default:
    }


}

NOCTERM_WIDGET_FOCUS_HANDLER(nocterm_menu_focus_handler){
    if(NOCTERM_MENU(self)->item_array->size == 0){
        return;
    }
    
    switch(focus){

        case NOCTERM_WIDGET_FOCUS_ENTER:{
                for(uint64_t i = 0; i < NOCTERM_MENU(self)->item_array->items[NOCTERM_MENU(self)->current_item].content_length && i < self->bounds.width; i++){
                    nocterm_widget_update(self, NOCTERM_MENU(self)->current_item, i, NOCTERM_MENU(self)->item_array->items[NOCTERM_MENU(self)->current_item].content[i].character , NOCTERM_MENU(self)->attribute_selection);
                }
        }break;

        case NOCTERM_WIDGET_FOCUS_LEAVE:{
            for(uint64_t i = 0; i < NOCTERM_MENU(self)->item_array->items[NOCTERM_MENU(self)->current_item].content_length && i < self->bounds.width; i++){
                nocterm_widget_update(self, NOCTERM_MENU(self)->current_item, i, NOCTERM_MENU(self)->item_array->items[NOCTERM_MENU(self)->current_item].content[i].character , NOCTERM_MENU(self)->item_array->items[NOCTERM_MENU(self)->current_item].content[i].attribute);
            }
        }break;
    }
}

// Dynamic Array Implemenation

nocterm_menu_item_array_t* nocterm_menu_item_array_new(void){
    nocterm_menu_item_array_t* new_nocterm_menu_item_array = (nocterm_menu_item_array_t*)malloc(sizeof(nocterm_menu_item_array_t));
    if(new_nocterm_menu_item_array == NULL){
        return NULL;
    }
    new_nocterm_menu_item_array->capacity = 0;
    new_nocterm_menu_item_array->size = 0;
    new_nocterm_menu_item_array->items = NULL;
    return new_nocterm_menu_item_array;
}

void nocterm_menu_item_array_delete(nocterm_menu_item_array_t* nocterm_menu_item_array){
    if(nocterm_menu_item_array == NULL){
        return;
    }    
    free(nocterm_menu_item_array->items);
    free(nocterm_menu_item_array);
}

void nocterm_menu_item_array_increase_capacity(nocterm_menu_item_array_t* nocterm_menu_item_array){
    if(nocterm_menu_item_array == NULL){
        return;
    }
    if(nocterm_menu_item_array->size == nocterm_menu_item_array->capacity){
        uint64_t new_capacity = nocterm_menu_item_array->capacity * 2 == 0 ? 1 : nocterm_menu_item_array->capacity * 2;
        if(new_capacity < nocterm_menu_item_array->capacity){
            exit(EXIT_FAILURE);
        }
        nocterm_menu_item_t* new_items = (nocterm_menu_item_t*)malloc(sizeof(nocterm_menu_item_t) * (new_capacity));
        if(new_items == NULL){
            exit(EXIT_FAILURE);
        }        
        if(nocterm_menu_item_array->items){
            memcpy(new_items, nocterm_menu_item_array->items, sizeof(nocterm_menu_item_t) * nocterm_menu_item_array->size);
            free(nocterm_menu_item_array->items);
        }
        nocterm_menu_item_array->items = new_items;
        nocterm_menu_item_array->capacity = new_capacity;
    }
}

int nocterm_menu_item_array_push_back(nocterm_menu_item_array_t* nocterm_menu_item_array, nocterm_menu_item_t item){
    if(nocterm_menu_item_array == NULL){
        return EXIT_FAILURE;
    }    
    nocterm_menu_item_array_increase_capacity(nocterm_menu_item_array);    
    nocterm_menu_item_array->items[nocterm_menu_item_array->size] = item;
    nocterm_menu_item_array->size++;
    return EXIT_SUCCESS;
}

int nocterm_menu_item_array_pop_back(nocterm_menu_item_array_t* nocterm_menu_item_array, nocterm_menu_item_t* item){
    if(nocterm_menu_item_array == NULL){
        return EXIT_FAILURE;
    }    
    if(nocterm_menu_item_array->size == 0){
        return EXIT_SUCCESS;
    }    
    if(item != NULL){
        *item = nocterm_menu_item_array->items[nocterm_menu_item_array->size-1];
    }
    nocterm_menu_item_array->size--;
    return EXIT_SUCCESS;
}

int nocterm_menu_item_array_push_front(nocterm_menu_item_array_t* nocterm_menu_item_array, nocterm_menu_item_t item){
    if(nocterm_menu_item_array == NULL){
        return EXIT_FAILURE;
    }    
    nocterm_menu_item_array_increase_capacity(nocterm_menu_item_array);    
    memmove(&(nocterm_menu_item_array->items[1]), &(nocterm_menu_item_array->items[0]), sizeof(nocterm_menu_item_t) * nocterm_menu_item_array->size);
    memcpy(&(nocterm_menu_item_array->items[0]), &item, sizeof(nocterm_menu_item_t));
    nocterm_menu_item_array->size++;
    return EXIT_SUCCESS;
}

int nocterm_menu_item_array_pop_front(nocterm_menu_item_array_t* nocterm_menu_item_array, nocterm_menu_item_t* item){
    if(nocterm_menu_item_array == NULL){
        return EXIT_FAILURE;
    }    
    if(nocterm_menu_item_array->size == 0){
        return EXIT_SUCCESS;
    }    
    if(item != NULL){
        *item = nocterm_menu_item_array->items[0];
    }
    memmove(&(nocterm_menu_item_array->items[0]), &(nocterm_menu_item_array->items[1]), sizeof(nocterm_menu_item_t) * (nocterm_menu_item_array->size - 1));
    nocterm_menu_item_array->size--;
    return EXIT_SUCCESS;
}

int nocterm_menu_item_array_insert(nocterm_menu_item_array_t* nocterm_menu_item_array, nocterm_menu_item_t item, uint64_t index){
    if(nocterm_menu_item_array == NULL){
        return EXIT_FAILURE;
    }
    if(index > nocterm_menu_item_array->size){
        return EXIT_FAILURE;
    }
    nocterm_menu_item_array_increase_capacity(nocterm_menu_item_array);
    memmove(&(nocterm_menu_item_array->items[index+1]), &(nocterm_menu_item_array->items[index]), sizeof(nocterm_menu_item_t) * (nocterm_menu_item_array->size - index) );
    nocterm_menu_item_array->items[index] = item;
    nocterm_menu_item_array->size++;
    return EXIT_SUCCESS;
}

int nocterm_menu_item_array_remove(nocterm_menu_item_array_t* nocterm_menu_item_array,  nocterm_menu_item_t* item, uint64_t index){
    if(nocterm_menu_item_array == NULL){
        return EXIT_FAILURE;
    }
    if(nocterm_menu_item_array->size == 0){
        return EXIT_FAILURE;
    }
    if(index >= nocterm_menu_item_array->size){
        return EXIT_FAILURE;
    }
    if(item != NULL){
        *item = nocterm_menu_item_array->items[index];
    }
    memmove(&(nocterm_menu_item_array->items[index]), &(nocterm_menu_item_array->items[index+1]), (nocterm_menu_item_array->size - index - 1) * sizeof(nocterm_menu_item_t));
    nocterm_menu_item_array->size--;
    return EXIT_SUCCESS;
}

int nocterm_menu_item_array_clear(nocterm_menu_item_array_t* nocterm_menu_item_array){
    if(nocterm_menu_item_array == NULL){
        return EXIT_FAILURE;
    }
    nocterm_menu_item_array->size = 0;
    return EXIT_SUCCESS;
}

int nocterm_menu_item_array_shrink_to_fit(nocterm_menu_item_array_t* nocterm_menu_item_array){
    if(nocterm_menu_item_array == NULL){
        return EXIT_FAILURE;
    }
    if (nocterm_menu_item_array->size == nocterm_menu_item_array->capacity){
        return EXIT_SUCCESS;
    }

    nocterm_menu_item_t* new_items = (nocterm_menu_item_t*)malloc(sizeof(nocterm_menu_item_t) * (nocterm_menu_item_array->size));
    if(new_items == NULL){
        exit(EXIT_FAILURE);
    }
    memcpy(new_items, nocterm_menu_item_array->items, sizeof(nocterm_menu_item_t) * nocterm_menu_item_array->size);
    free(nocterm_menu_item_array->items);
    nocterm_menu_item_array->items = new_items;
    nocterm_menu_item_array->capacity = nocterm_menu_item_array->size;
    return EXIT_SUCCESS;
}
