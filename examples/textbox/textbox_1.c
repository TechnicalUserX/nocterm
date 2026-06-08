#include <nocterm/nocterm.h>


NOCTERM_BUTTON_ONPRESS_HANDLER(on_get_text){
    char buf[NOCTERM_TEXTBOX_BUFFER_MAX_SIZE] = {0};
    uint64_t len = 0;
    if(nocterm_textbox_get_text(NOCTERM_TEXTBOX(user_data), buf, sizeof(buf), &len) == NOCTERM_SUCCESS && len > 0){
        nocterm_io_print_at(11, 1, "Got %lu char(s): %-40s", len, buf);
    }else{
        nocterm_io_print_at(11, 1, "(textbox is empty)                          ");
    }
}


int main(){

    // Root widget: 12 rows x 44 cols
    nocterm_widget_t* root = nocterm_widget_new(12, 44, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_REAL);
    nocterm_page_t* main_page = nocterm_page_new("Textbox Demo", sizeof("Textbox Demo"), root);

    // Title label
    nocterm_label_t* title = nocterm_label_new("Textbox Demo", sizeof("Textbox Demo"));
    nocterm_widget_set_position(NOCTERM_WIDGET(title), 1, 1);

    // Editable textbox: 7 rows x 40 cols
    nocterm_textbox_t* my_textbox = nocterm_textbox_new(7, 40);
    nocterm_widget_set_position(NOCTERM_WIDGET(my_textbox), 2, 2);

    // "Get Text" button beneath the textbox
    nocterm_button_t* get_btn = nocterm_button_new(1, 10, on_get_text, my_textbox);
    nocterm_button_set_text(get_btn, "Get Text", sizeof("Get Text"));
    nocterm_widget_set_position(NOCTERM_WIDGET(get_btn), 10, 2);

    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(title));
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(my_textbox));
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(get_btn));

    nocterm_page_stack_push(main_page);

    nocterm_init();
    nocterm_loop();
    nocterm_end();

    nocterm_widget_delete(root);
    nocterm_page_delete(main_page);
    nocterm_label_delete(title);
    nocterm_textbox_delete(my_textbox);
    nocterm_button_delete(get_btn);

    return 0;
}
