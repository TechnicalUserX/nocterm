#include <nocterm/nocterm.h>

int main(){

    nocterm_widget_t* my_widget = nocterm_widget_new(10,10, NOCTERM_WIDGET_FOCUSABLE_YES, NOCTERM_WIDGET_TYPE_REAL);
    nocterm_page_t* main_page = nocterm_page_new("Main page", sizeof("Main page"), my_widget);
 
    nocterm_page_stack_push(main_page); 
 
    nocterm_init();
    nocterm_loop(); 
    nocterm_end();
  
    nocterm_widget_delete(my_widget);
    nocterm_page_delete(main_page); 
 
    return 0;
}