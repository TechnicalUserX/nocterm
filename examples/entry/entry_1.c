#include <nocterm/nocterm.h>


NOCTERM_BUTTON_ONPRESS_HANDLER(handler){
    char buf[100] = {0};
    if(nocterm_entry_get_text(NOCTERM_ENTRY(user_data), buf, 100, NULL) == NOCTERM_SUCCESS){
        nocterm_io_print_at(3,1,"Retrieved: %-20s", buf);
    }
}


int main(){

    nocterm_widget_t* my_widget = nocterm_widget_new(10,10 , NOCTERM_WIDGET_FOCUSABLE_YES, NOCTERM_WIDGET_TYPE_REAL);
    nocterm_page_t* main_page = nocterm_page_new("Main page", sizeof("Main page"), my_widget);
 
    nocterm_attribute_t attr = {
        .color.ansi.fg = true,
        .color.ansi.codes.fg = 2
    };

    nocterm_entry_t* my_entry = nocterm_entry_new(6);
    nocterm_widget_set_position(NOCTERM_WIDGET(my_entry), 1, 1);

    nocterm_button_t* my_button = nocterm_button_new(1,8,handler, my_entry);
    nocterm_button_set_text(my_button, "Get Text", 9);
    nocterm_widget_set_position(NOCTERM_WIDGET(my_button), 2, 1);

    nocterm_widget_add_subwidget(my_widget, NOCTERM_WIDGET(my_entry));
    nocterm_widget_add_subwidget(my_widget, NOCTERM_WIDGET(my_button));

    nocterm_page_stack_push(main_page); 
 
    nocterm_init();
    nocterm_loop(); 
    nocterm_end();
  
    nocterm_widget_delete(my_widget);
    nocterm_page_delete(main_page); 
    nocterm_entry_delete(my_entry);
    nocterm_button_delete(my_button);
 
    return 0;
}