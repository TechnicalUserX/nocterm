#include <nocterm/base/widget.h>
#include <nocterm/nocterm.h>


NOCTERM_TIMER_CALLBACK(my_callback){

    static int i = 1;

    if(i){
        i = 0;

        nocterm_widget_add_subwidget(widget, NOCTERM_WIDGET(user_data));
        
        nocterm_widget_align(NOCTERM_WIDGET(user_data), NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL);
        nocterm_widget_align(NOCTERM_WIDGET(user_data), NOCTERM_WIDGET_ALIGN_CENTER_VERTICAL);

    }else{
        i = 1;
        nocterm_widget_remove_subwidget(widget, NOCTERM_WIDGET(user_data));

    }

}

int main(){

    nocterm_widget_t* my_widget = nocterm_widget_new(11,10, NOCTERM_WIDGET_FOCUSABLE_YES, NOCTERM_WIDGET_TYPE_REAL);

    nocterm_decorbox_t* my_decorbox = nocterm_decorbox_new(my_widget);
    
    nocterm_label_t* my_label = nocterm_label_new("HI", 3);

 
    nocterm_decorbox_border_t border = nocterm_decorbox_border_from_shape(NOCTERM_DECORBOX_BORDER_SHAPE_UNICODE_ROUND);

    nocterm_attribute_t attr = {
        .color.ansi.fg = true,
        .color.ansi.codes.fg = 5
    };

    nocterm_decorbox_set_border(my_decorbox, border, NOCTERM_ATTRIBUTE_EMPTY, attr);
    
    nocterm_page_t* main_page = nocterm_page_new("Main page", sizeof("Main page"), NOCTERM_WIDGET(my_decorbox));

    nocterm_widget_align(NOCTERM_WIDGET(my_decorbox), NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL);
    nocterm_widget_align(NOCTERM_WIDGET(my_decorbox), NOCTERM_WIDGET_ALIGN_CENTER_VERTICAL);

    nocterm_timer_t* my_timer = nocterm_timer_create(my_widget, 500, my_callback, my_label);

    nocterm_timer_start(my_timer);

    nocterm_page_stack_push(main_page); 
 
    nocterm_init();
    nocterm_loop(); 
    nocterm_end();

    nocterm_widget_delete(my_widget);
    nocterm_page_delete(main_page); 
    nocterm_decorbox_delete(my_decorbox);
    nocterm_label_delete(my_label);
 
    return 0;
}
