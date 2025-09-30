#include <nocterm/nocterm.h>
 
int main(){
 
    nocterm_widget_t* my_widget = nocterm_widget_new(10,10, NOCTERM_WIDGET_FOCUSABLE_YES, NOCTERM_WIDGET_TYPE_REAL);
    nocterm_page_t* main_page = nocterm_page_new("Main page", sizeof("Main page"), my_widget);
 
    nocterm_levelbar_t* my_levelbar = nocterm_levelbar_new(20, 0, 20, NOCTERM_LEVELBAR_TYPE_HORIZONTAL, false);


    nocterm_decorbox_t* my_levelbar_dbox = nocterm_decorbox_new(NOCTERM_WIDGET(my_levelbar));
    nocterm_widget_set_position(NOCTERM_WIDGET(my_levelbar_dbox), 1, 1);

    nocterm_decorbox_border_shape_t border_shape = nocterm_decorbox_border_shape(NOCTERM_DECORBOX_BORDER_SHAPE_TYPE_UNICODE_ROUND);

    nocterm_decorbox_set_border(my_levelbar_dbox, border_shape, NOCTERM_ATTRIBUTE_EMPTY, NOCTERM_ATTRIBUTE_EMPTY);

    nocterm_char_t levelbar_char = {
        .bytes = {'='},
        .bytes_size = 1,
        .is_utf8 = false
    };

    nocterm_levelbar_set_character(my_levelbar, levelbar_char);

    nocterm_widget_add_subwidget(my_widget, NOCTERM_WIDGET(my_levelbar_dbox));

    nocterm_levelbar_set_value(my_levelbar, 12);

    nocterm_page_stack_push(main_page); 
 
    nocterm_init();
    nocterm_loop(); 
    nocterm_end();
  
    nocterm_widget_delete(my_widget);
    nocterm_page_delete(main_page); 
    nocterm_levelbar_delete(my_levelbar);
    nocterm_decorbox_delete(my_levelbar_dbox);

    return 0;
}