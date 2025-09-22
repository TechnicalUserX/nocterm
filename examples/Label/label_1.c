#include <nocterm/nocterm.h>

int main(){

    nocterm_widget_t* my_widget = nocterm_widget_new(10,10, NOCTERM_WIDGET_FOCUSABLE_YES, NOCTERM_WIDGET_TYPE_REAL);
    nocterm_page_t* main_page = nocterm_page_new("Main page", sizeof("Main page"), my_widget);

    nocterm_attribute_t attr = {
        .color.ansi.fg = true,
        .color.ansi.codes.fg = 3,
        .bold = true
    };

    nocterm_label_t* my_label = nocterm_label_new("Hello", sizeof("Hello"));
    nocterm_widget_set_position(NOCTERM_WIDGET(my_label), 2, 5);
    nocterm_widget_add_subwidget(my_widget, NOCTERM_WIDGET(my_label));

    nocterm_page_stack_push(main_page); 

    nocterm_init();
    nocterm_loop(); 
    nocterm_end();
  
    nocterm_widget_delete(my_widget);
    nocterm_page_delete(main_page); 
    nocterm_label_delete(my_label);
 
    return 0;
}