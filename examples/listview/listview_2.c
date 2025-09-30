#include <nocterm/nocterm.h>

nocterm_entry_t* input = NULL;
nocterm_entry_t* list_index = NULL;

NOCTERM_BUTTON_ONPRESS_HANDLER(button0_handler){
    char buf[1000] = {0};
    uint64_t len = 0;
    nocterm_entry_get_text(input, buf, 1000, &len);

    if(len == 0){
        return;
    }
    
    nocterm_listview_item_t my_item = {0};
    nocterm_listview_item_constructor(&my_item, buf, strlen(buf)+1, NOCTERM_ATTRIBUTE_EMPTY);

    nocterm_listview_push_back(NOCTERM_LISTVIEW(user_data), my_item);

}

NOCTERM_BUTTON_ONPRESS_HANDLER(button1_handler){
    char buf[1000] = {0};
    uint64_t len = 0;
    nocterm_entry_get_text(input, buf, 1000, &len);

    if(len == 0){
        return;
    }
    
    nocterm_listview_item_t my_item = {0};
    nocterm_listview_item_constructor(&my_item, buf, strlen(buf)+1, NOCTERM_ATTRIBUTE_EMPTY);

    nocterm_listview_push_front(NOCTERM_LISTVIEW(user_data), my_item);
}

NOCTERM_BUTTON_ONPRESS_HANDLER(button2_handler){

    nocterm_listview_pop_back(NOCTERM_LISTVIEW(user_data), NULL);
}

NOCTERM_BUTTON_ONPRESS_HANDLER(button3_handler){
    nocterm_listview_pop_front(NOCTERM_LISTVIEW(user_data), NULL);

}

NOCTERM_BUTTON_ONPRESS_HANDLER(button4_handler){

    char buf[1000] = {0};
    uint64_t len = 0;
    nocterm_entry_get_text(list_index, buf, 1000, &len);
    if(len == 0){
        return;
    }

    int x = atoi(buf);

    nocterm_entry_get_text(input, buf, 1000, &len);

    if(len == 0){
        return;
    }

    nocterm_listview_item_t my_item = {0};
    nocterm_listview_item_constructor(&my_item, buf, strlen(buf)+1, NOCTERM_ATTRIBUTE_EMPTY);

    nocterm_listview_insert(NOCTERM_LISTVIEW(user_data), my_item, x);
}

NOCTERM_BUTTON_ONPRESS_HANDLER(button5_handler){
    char buf[1000] = {0};
    uint64_t len = 0;
    nocterm_entry_get_text(list_index, buf, 1000, &len);
    if(len == 0){
        return;
    }

    int x = atoi(buf);

    nocterm_listview_remove(NOCTERM_LISTVIEW(user_data), NULL, x);

}

int main(){

    setlocale(LC_ALL, "en_US.UTF-8");

    nocterm_widget_t* my_widget = nocterm_widget_new(10,10, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_VIRTUAL);
    nocterm_page_t* main_page = nocterm_page_new("Main page", sizeof("Main page"), my_widget);

    nocterm_decorbox_border_shape_t border_shape = nocterm_decorbox_border_shape(NOCTERM_DECORBOX_BORDER_SHAPE_TYPE_UNICODE_SHARP);

    nocterm_attribute_t attr = {
        .color.ansi.fg = true,
        .color.ansi.codes.fg = 2
    };

    nocterm_listview_t* my_listview = nocterm_listview_new(5,100,20);
    nocterm_decorbox_t* my_listview_db = nocterm_decorbox_new(NOCTERM_WIDGET(my_listview));
    nocterm_decorbox_set_border(my_listview_db, border_shape, NOCTERM_ATTRIBUTE_EMPTY, attr);
    nocterm_widget_add_subwidget(my_widget, NOCTERM_WIDGET(my_listview_db));
    nocterm_widget_set_position(NOCTERM_WIDGET(my_listview_db), 1, 1);

    nocterm_label_t* my_listview_label = nocterm_label_new("List", 5);
    nocterm_widget_add_subwidget(my_widget, NOCTERM_WIDGET(my_listview_label));
    nocterm_widget_set_position(NOCTERM_WIDGET(my_listview_label), 0,1);


    input = nocterm_entry_new(20);
    nocterm_decorbox_t* input_db = nocterm_decorbox_new(NOCTERM_WIDGET(input));
    nocterm_widget_add_subwidget(my_widget, NOCTERM_WIDGET(input_db));
    nocterm_decorbox_set_border(NOCTERM_DECORBOX(input_db), border_shape, NOCTERM_ATTRIBUTE_EMPTY, attr);
    nocterm_widget_set_position(NOCTERM_WIDGET(input_db), 10,1);

    nocterm_label_t* input_label = nocterm_label_new("Input", 6);
    nocterm_widget_add_subwidget(my_widget, NOCTERM_WIDGET(input_label));
    nocterm_widget_set_position(NOCTERM_WIDGET(input_label), 9,1);

    list_index = nocterm_entry_new(4);
    nocterm_decorbox_t* list_index_db = nocterm_decorbox_new(NOCTERM_WIDGET(list_index));
    nocterm_decorbox_set_border(list_index_db, border_shape, NOCTERM_ATTRIBUTE_EMPTY, attr);
    nocterm_widget_add_subwidget(my_widget, NOCTERM_WIDGET(list_index_db));
    nocterm_widget_set_position(NOCTERM_WIDGET(list_index_db), 10, 30);

    nocterm_label_t* list_index_label = nocterm_label_new("Index", 6);
    nocterm_widget_add_subwidget(my_widget, NOCTERM_WIDGET(list_index_label));
    nocterm_widget_set_position(NOCTERM_WIDGET(list_index_label), 9,30);

    nocterm_button_t* buttons[6];
    buttons[0] = nocterm_button_new(1, 9, button0_handler, my_listview);
    buttons[1] = nocterm_button_new(1, 10, button1_handler, my_listview);
    buttons[2] = nocterm_button_new(1, 8, button2_handler, my_listview);
    buttons[3] = nocterm_button_new(1, 9, button3_handler, my_listview);
    buttons[4] = nocterm_button_new(1, 6, button4_handler, my_listview);
    buttons[5] = nocterm_button_new(1, 6, button5_handler, my_listview);

    nocterm_widget_add_subwidget(my_widget, NOCTERM_WIDGET(buttons[0]));
    nocterm_widget_add_subwidget(my_widget, NOCTERM_WIDGET(buttons[1]));
    nocterm_widget_add_subwidget(my_widget, NOCTERM_WIDGET(buttons[2]));
    nocterm_widget_add_subwidget(my_widget, NOCTERM_WIDGET(buttons[3]));
    nocterm_widget_add_subwidget(my_widget, NOCTERM_WIDGET(buttons[4]));
    nocterm_widget_add_subwidget(my_widget, NOCTERM_WIDGET(buttons[5]));

    nocterm_widget_set_position(NOCTERM_WIDGET(buttons[0]), 2, 30);
    nocterm_widget_set_position(NOCTERM_WIDGET(buttons[1]), 3, 30);
    nocterm_widget_set_position(NOCTERM_WIDGET(buttons[2]), 4, 30);
    nocterm_widget_set_position(NOCTERM_WIDGET(buttons[3]), 5, 30);
    nocterm_widget_set_position(NOCTERM_WIDGET(buttons[4]), 6, 30);
    nocterm_widget_set_position(NOCTERM_WIDGET(buttons[5]), 7, 30);

    nocterm_button_set_text(buttons[0], "Push Back", 10);
    nocterm_button_set_text(buttons[1], "Push Front", 11);
    nocterm_button_set_text(buttons[2], "Pop Back", 9);
    nocterm_button_set_text(buttons[3], "Pop Front", 10);
    nocterm_button_set_text(buttons[4], "Insert", 7);
    nocterm_button_set_text(buttons[5], "Remove", 7);

    nocterm_page_stack_push(main_page); 
 
    nocterm_mouse_support(NOCTERM_MOUSE_SUPPORT_ADVANCED);

    nocterm_init();
    nocterm_loop(); 
    nocterm_end();
  
    nocterm_widget_delete(my_widget);
    nocterm_page_delete(main_page); 
    return 0;
}