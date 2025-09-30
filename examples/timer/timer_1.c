#include <nocterm/nocterm.h>
 
NOCTERM_TIMER_CALLBACK(callback){
 
    static int color = 0;
 
    nocterm_attribute_t attr = {
        .color.ansi.fg = true,
        .color.ansi.codes.fg = color
    };
 
    nocterm_char_t ch = {
        .bytes = {'A'},
        .bytes_size = 1,
        .is_utf8 = false
    };
    nocterm_widget_update(widget, 4, 4,ch,attr);
    color++;
 
    if(color % 7 == 0){
        color = 0;
    }
    
}
 
int main(){
 
    nocterm_dimension_size_t height = 10, width = 10;
 
    nocterm_widget_focusable_t focusable = NOCTERM_WIDGET_FOCUSABLE_YES;
    nocterm_widget_type_t type = NOCTERM_WIDGET_TYPE_REAL;
 
    nocterm_widget_t* my_widget = nocterm_widget_new(height, width, focusable, type);
 
    nocterm_page_t* main_page = nocterm_page_new("Main page", sizeof("Main page"), my_widget);
 
    nocterm_timer_t* timer = nocterm_timer_create(my_widget, 500, callback, NULL);
    nocterm_timer_start(timer);
 
    nocterm_page_stack_push(main_page); 
 
    nocterm_init();
    nocterm_loop();
    nocterm_end(); 
 
    nocterm_widget_delete(my_widget);
    nocterm_page_delete(main_page); 
 
    return 0;
}