#include <nocterm/nocterm.h>
#include <term.h>

struct add_item_handler_arg_t{
    void* entry;
    void* listview;
};

nocterm_attribute_t generic_widget_attribute_normal = {
    .color.ansi.fg = true,
    .color.ansi.codes.fg = 7
};

nocterm_attribute_t generic_widget_attribute_focused = {
    .color.ansi.fg = true,
    .color.ansi.codes.fg = 4
};

nocterm_attribute_t generic_widget_attribute_focused_2 = {
    .color.ansi.fg = true,
    .color.ansi.codes.fg = 5
};


NOCTERM_CHECKBOX_ONCHECK_HANDLER(make_menu_visible_handler){

    static bool state = true;
    if(state){
        state = false;
        nocterm_widget_set_visible(NOCTERM_WIDGET(user_data),false);
    }else{
        state = true;
        nocterm_widget_set_visible(NOCTERM_WIDGET(user_data),true);

    }
    
}

NOCTERM_MENU_ONSELECT_HANDLER(menu_select_handler){


    switch(selected_item){
        case 0:
            nocterm_textview_set_text(NOCTERM_TEXTVIEW(user_data), "First item selected", 20);
            break;
        case 1:
            nocterm_textview_set_text(NOCTERM_TEXTVIEW(user_data), "Second item selected", 21);
            break;            
        case 2:
            nocterm_textview_set_text(NOCTERM_TEXTVIEW(user_data), "Did you like this demo application?", 36);
            break; 
        case 3:
            nocterm_textview_set_text(NOCTERM_TEXTVIEW(user_data), "Never gonna give you up, never gonna let you down", 50);
            break; 
        case 4:
            nocterm_textview_set_text(NOCTERM_TEXTVIEW(user_data), "Last item selected", 19);
            break;
        default:
            break;

    }
}

NOCTERM_BUTTON_ONPRESS_HANDLER(add_item_handler){

    char buf[100];

    nocterm_entry_t* entry = ((struct add_item_handler_arg_t*)user_data)->entry;
    nocterm_listview_t* listview = ((struct add_item_handler_arg_t*)user_data)->listview;

    uint64_t text_length = 0;
    nocterm_entry_get_text(entry, buf, 100, &text_length);


    if(text_length > 0){
        nocterm_listview_item_t new_item = {0};
        nocterm_listview_item_constructor(&new_item, buf, 100, NOCTERM_ATTRIBUTE_EMPTY);
        nocterm_listview_push_back(listview, new_item);
    }

}

NOCTERM_TIMER_CALLBACK(levelbar_timer_callback){

    static uint64_t level = 0;

    nocterm_levelbar_set_value(NOCTERM_LEVELBAR(widget),level);

    level++;
    if(level == 101){
        level = 0;
    }
}

NOCTERM_TIMER_CALLBACK(menu_colorful_selection){
    static int color = 0;

    nocterm_attribute_t attr = {
        .color.ansi.fg = true,
        .color.ansi.codes.fg = color
    };

    nocterm_menu_set_selection_attribute(NOCTERM_MENU(widget), attr);

    color = color == 7 ? 0 : color + 1;
}

int main(){

    setlocale(LC_ALL, "en_US.UTF-8");

    nocterm_decorbox_border_t generic_widget_border_shape = nocterm_decorbox_border_from_shape(NOCTERM_DECORBOX_BORDER_SHAPE_UNICODE_ROUND);

    nocterm_widget_t* main_widget = nocterm_widget_new(13, 61, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_VIRTUAL);
    nocterm_decorbox_t* main_widget_decorbox = nocterm_decorbox_new(main_widget);
    nocterm_widget_set_position(NOCTERM_WIDGET(main_widget_decorbox), 1, 1);
    nocterm_decorbox_set_border(main_widget_decorbox, generic_widget_border_shape, generic_widget_attribute_normal, generic_widget_attribute_focused);


    nocterm_textview_t* textview = nocterm_textview_new(8, 20);
    nocterm_decorbox_t* textview_decorbox = nocterm_decorbox_new(NOCTERM_WIDGET(textview));
    nocterm_widget_set_position(NOCTERM_WIDGET(textview_decorbox), 0, 38);
    nocterm_decorbox_set_border(textview_decorbox, generic_widget_border_shape, generic_widget_attribute_normal, generic_widget_attribute_focused); 

    // Menu
    nocterm_menu_t* menu = nocterm_menu_new(3, 10, 16, menu_select_handler, textview);
    nocterm_menu_item_t menu_items[5] = {0};
    nocterm_menu_item_constructor(&menu_items[0], "Item 1", 7, NOCTERM_ATTRIBUTE_EMPTY);
    nocterm_menu_item_constructor(&menu_items[1], "Item 2", 7, NOCTERM_ATTRIBUTE_EMPTY);
    nocterm_menu_item_constructor(&menu_items[2], "Item 3", 7, NOCTERM_ATTRIBUTE_EMPTY);
    nocterm_menu_item_constructor(&menu_items[3], "Item 4", 7, NOCTERM_ATTRIBUTE_EMPTY);
    nocterm_menu_item_constructor(&menu_items[4], "Item 5", 7, NOCTERM_ATTRIBUTE_EMPTY);
    nocterm_menu_add_item_multiple(menu, menu_items, 5);
    nocterm_decorbox_t* menu_decorbox = nocterm_decorbox_new(NOCTERM_WIDGET(menu));
    nocterm_widget_set_position(NOCTERM_WIDGET(menu_decorbox), 5,1);
    nocterm_decorbox_set_border(menu_decorbox, generic_widget_border_shape, generic_widget_attribute_normal, generic_widget_attribute_focused);

    nocterm_timer_t* menu_selection_timer = nocterm_timer_create(NOCTERM_WIDGET(menu), 500, menu_colorful_selection, NULL);
    nocterm_timer_start(menu_selection_timer);

    // Checkbox that enables menu
    nocterm_checkbox_t* make_menu_visible = nocterm_checkbox_new(make_menu_visible_handler, false, menu_decorbox);
    nocterm_widget_set_position(NOCTERM_WIDGET(make_menu_visible), 1, 1);

    nocterm_entry_t* entry = nocterm_entry_new(16);

    nocterm_decorbox_t* entry_decorbox = nocterm_decorbox_new(NOCTERM_WIDGET(entry));
    nocterm_widget_set_position(NOCTERM_WIDGET(entry_decorbox), 2,1);
    nocterm_decorbox_set_border(entry_decorbox, generic_widget_border_shape, generic_widget_attribute_normal, generic_widget_attribute_focused); 
    
    nocterm_label_t* enable_menu_label = nocterm_label_new("Show/Hide Menu", 15);
    nocterm_widget_set_position(NOCTERM_WIDGET(enable_menu_label), 1, 5);

    nocterm_listview_t* item_list = nocterm_listview_new(3, 20, 16);
    nocterm_listview_set_autoscroll(item_list, NOCTERM_LISTVIEW_AUTOSCROLL_DOWN);
    nocterm_decorbox_t* item_list_decorbox = nocterm_decorbox_new(NOCTERM_WIDGET(item_list));
    nocterm_widget_set_position(NOCTERM_WIDGET(item_list_decorbox), 5, 20);
    nocterm_decorbox_set_border(item_list_decorbox, generic_widget_border_shape, generic_widget_attribute_normal, generic_widget_attribute_focused); 


    struct add_item_handler_arg_t* add_item_handler_arg = (struct add_item_handler_arg_t*)malloc(sizeof(struct add_item_handler_arg_t));
    add_item_handler_arg->entry = entry;
    add_item_handler_arg->listview = item_list;

    nocterm_button_t* add_items_to_list = nocterm_button_new(1, 8, add_item_handler, add_item_handler_arg);
    nocterm_button_set_text(add_items_to_list, "Add Item", 9);
    nocterm_decorbox_t* add_items_to_list_decorbox = nocterm_decorbox_new(NOCTERM_WIDGET(add_items_to_list));
    nocterm_widget_set_position(NOCTERM_WIDGET(add_items_to_list_decorbox), 2, 20);
    nocterm_decorbox_set_border(add_items_to_list_decorbox, generic_widget_border_shape, generic_widget_attribute_normal, generic_widget_attribute_focused); 


    nocterm_pixelgrid_t* pixelgrid = nocterm_pixelgrid_new(6, 6);
    nocterm_decorbox_t* pixelgrid_decorbox = nocterm_decorbox_new(NOCTERM_WIDGET(pixelgrid));
    nocterm_widget_set_position(NOCTERM_WIDGET(pixelgrid_decorbox), 0, 30);
    nocterm_decorbox_set_border(pixelgrid_decorbox, generic_widget_border_shape, generic_widget_attribute_normal, generic_widget_attribute_focused); 

    for(uint16_t i = 0; i < 10; i++){
        for(uint16_t j = 0; j < 10; j++){
            nocterm_pixelgrid_set_pixel(pixelgrid, i,j, i*10 + j*10, i*10 + j*10, i*10 + j*10);
        }
    }

    nocterm_loadingbar_t* loadingbar = nocterm_loadingbar_new(200);
    nocterm_widget_set_position(NOCTERM_WIDGET(loadingbar), 1, 24);

    nocterm_levelbar_t* levelbar = nocterm_levelbar_new(57,0,100,NOCTERM_LEVELBAR_TYPE_HORIZONTAL, false);
    nocterm_decorbox_t* levelbar_decorbox = nocterm_decorbox_new(NOCTERM_WIDGET(levelbar));
    nocterm_widget_set_position(NOCTERM_WIDGET(levelbar_decorbox), 10, 1);

    nocterm_decorbox_set_border(levelbar_decorbox, generic_widget_border_shape, generic_widget_attribute_normal, generic_widget_attribute_focused); 

    nocterm_timer_t* levelbar_timer = nocterm_timer_create(NOCTERM_WIDGET(levelbar), 20, levelbar_timer_callback, NULL);
    
    nocterm_timer_start(levelbar_timer);

    nocterm_widget_add_subwidget(main_widget, NOCTERM_WIDGET(enable_menu_label));
    nocterm_widget_add_subwidget(main_widget, NOCTERM_WIDGET(make_menu_visible));
    nocterm_widget_add_subwidget(main_widget, NOCTERM_WIDGET(entry_decorbox));
    nocterm_widget_add_subwidget(main_widget, NOCTERM_WIDGET(add_items_to_list_decorbox));
    nocterm_widget_add_subwidget(main_widget, NOCTERM_WIDGET(menu_decorbox));
    nocterm_widget_add_subwidget(main_widget, NOCTERM_WIDGET(item_list_decorbox));
    nocterm_widget_add_subwidget(main_widget, NOCTERM_WIDGET(textview_decorbox));
    nocterm_widget_add_subwidget(main_widget, NOCTERM_WIDGET(pixelgrid_decorbox));
    nocterm_widget_add_subwidget(main_widget, NOCTERM_WIDGET(loadingbar));
    nocterm_widget_add_subwidget(main_widget, NOCTERM_WIDGET(levelbar_decorbox));

    nocterm_loadingbar_enable(loadingbar);

    nocterm_widget_align(NOCTERM_WIDGET(main_widget_decorbox), NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL);

    nocterm_widget_align(NOCTERM_WIDGET(main_widget_decorbox), NOCTERM_WIDGET_ALIGN_PERCENT_HORIZONTAL, 50);

    nocterm_widget_align(NOCTERM_WIDGET(main_widget_decorbox), NOCTERM_WIDGET_ALIGN_CENTER_VERTICAL);

    nocterm_widget_align(NOCTERM_WIDGET(main_widget_decorbox), NOCTERM_WIDGET_ALIGN_PERCENT_VERTICAL, 50);


    nocterm_page_t* main_page = nocterm_page_new("Main Page", 10, NOCTERM_WIDGET(main_widget_decorbox));
    nocterm_page_stack_push(main_page);

    // Enable mouse support
    // Use on supporting temrinals
    nocterm_mouse_set_support(NOCTERM_MOUSE_SUPPORT_SIMPLE);

    // Main Loop
    nocterm_init();
    nocterm_loop(); 
    nocterm_end();
 
    // Delete Listview
    nocterm_decorbox_delete(add_items_to_list_decorbox);
    nocterm_button_delete(add_items_to_list);

    // Delete Pixelgrid
    nocterm_pixelgrid_delete(pixelgrid);
    nocterm_decorbox_delete(pixelgrid_decorbox);

    // Delete Main Widget
    nocterm_widget_delete(main_widget);
    nocterm_decorbox_delete(main_widget_decorbox);

    // Delete Menu
    nocterm_menu_delete(menu);
    nocterm_decorbox_delete(menu_decorbox);

    // Delete Entry
    nocterm_decorbox_delete(entry_decorbox);
    nocterm_entry_delete(entry);

    // Delete Textview
    nocterm_textview_delete(textview);
    nocterm_decorbox_delete(textview_decorbox);

    // Delete Listview
    nocterm_listview_delete(item_list);
    nocterm_decorbox_delete(item_list_decorbox);

    // Delete Levelbar
    nocterm_levelbar_delete(levelbar);
    nocterm_decorbox_delete(levelbar_decorbox);

    // Delete Checkbox
    nocterm_checkbox_delete(make_menu_visible);

    // Delete Loadingbar
    nocterm_loadingbar_delete(loadingbar);

    // Delete Label
    nocterm_label_delete(enable_menu_label);

    // Delete Page
    nocterm_page_delete(main_page);

    free(add_item_handler_arg);
    
    return 0;
}
