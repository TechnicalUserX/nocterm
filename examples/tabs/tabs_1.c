/**
 * @file tabs_1.c
 *
 * Demonstrates the minimal tabs widget.  The tabs widget shows one tab root at
 * a time and draws no navigation bar of its own — switching tabs is entirely up
 * to the application.  Here three buttons drive nocterm_tabs_navigate().
 *
 * Use Tab / Shift-Tab to move focus between the three buttons, and Enter (or a
 * mouse click) to press one and switch tabs.
 */

#include <nocterm/nocterm.h>
#include <stdint.h>
#include <string.h>

static nocterm_tabs_t* tabs;

/* The target tab index is carried in the button's user_data. */
NOCTERM_BUTTON_ONPRESS_HANDLER(on_tab_button){
    (void)self;
    nocterm_tabs_navigate(tabs, (uint64_t)(intptr_t)user_data);
}

static void draw_text(nocterm_widget_t* w, int row, int col, const char* s){
    nocterm_char_t buf[128];
    uint64_t n = nocterm_char_string_from_stream(buf, 128, s, strlen(s));
    for(uint64_t i=0;i<n;i++) nocterm_widget_update(w, row, col+(int)i, buf[i], NOCTERM_ATTRIBUTE_EMPTY);
}

/* A tab is just a widget the caller populates; here we draw straight onto it. */
static nocterm_widget_t* make_tab(const char* title, const char* body){
    nocterm_widget_t* root = nocterm_widget_new(6, 44, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_REAL);
    draw_text(root, 0, 0, title);
    draw_text(root, 2, 0, body);
    return root;
}

int main(void){

    nocterm_widget_t* container = nocterm_widget_new(12, 44, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_VIRTUAL);

    /* Navigation buttons across the top. */
    nocterm_button_t* b1 = nocterm_button_new(1, 8, on_tab_button, (void*)(intptr_t)0);
    nocterm_button_t* b2 = nocterm_button_new(1, 8, on_tab_button, (void*)(intptr_t)1);
    nocterm_button_t* b3 = nocterm_button_new(1, 8, on_tab_button, (void*)(intptr_t)2);
    nocterm_button_set_text(b1, "Tab 1", sizeof("Tab 1"));
    nocterm_button_set_text(b2, "Tab 2", sizeof("Tab 2"));
    nocterm_button_set_text(b3, "Tab 3", sizeof("Tab 3"));
    /* A clear focused colour so hovering/focusing a button is obvious. */
    nocterm_attribute_t normal = {0};
    nocterm_attribute_t focused = {0};
    focused.bold = true;
    focused.color.rgb.bg = true; focused.color.rgb.codes.bg.red=200; focused.color.rgb.codes.bg.green=50; focused.color.rgb.codes.bg.blue=50;
    focused.color.rgb.fg = true; focused.color.rgb.codes.fg.red=255; focused.color.rgb.codes.fg.green=255; focused.color.rgb.codes.fg.blue=255;
    nocterm_button_set_attribute(b1, normal, focused);
    nocterm_button_set_attribute(b2, normal, focused);
    nocterm_button_set_attribute(b3, normal, focused);
    nocterm_widget_set_position(NOCTERM_WIDGET(b1), 0, 0);
    nocterm_widget_set_position(NOCTERM_WIDGET(b2), 0, 10);
    nocterm_widget_set_position(NOCTERM_WIDGET(b3), 0, 20);
    nocterm_widget_add_subwidget(container, NOCTERM_WIDGET(b1));
    nocterm_widget_add_subwidget(container, NOCTERM_WIDGET(b2));
    nocterm_widget_add_subwidget(container, NOCTERM_WIDGET(b3));

    /* The tabs widget sits below the buttons and holds the three tab roots. */
    tabs = nocterm_tabs_new(8, 44);
    nocterm_widget_set_position(NOCTERM_WIDGET(tabs), 2, 0);
    nocterm_widget_add_subwidget(container, NOCTERM_WIDGET(tabs));

    nocterm_widget_t* page1 = make_tab("== Tab 1 ==", "The first tab is shown by default.");
    nocterm_widget_t* page2 = make_tab("== Tab 2 ==", "Press the buttons to switch tabs.");
    nocterm_widget_t* page3 = make_tab("== Tab 3 ==", "No navbar is built in - you drive it.");
    nocterm_tabs_add(tabs, page1);
    nocterm_tabs_add(tabs, page2);
    nocterm_tabs_add(tabs, page3);

    nocterm_page_t* main_page = nocterm_page_new("Tabs", sizeof("Tabs"), container);
    nocterm_page_stack_push(main_page);

    nocterm_init();
    nocterm_loop();
    nocterm_end();

    nocterm_page_delete(main_page);
    nocterm_tabs_delete(tabs);            /* does not delete the tab roots ... */
    nocterm_widget_delete(page1);         /* ... so the caller deletes them    */
    nocterm_widget_delete(page2);
    nocterm_widget_delete(page3);
    nocterm_button_delete(b1);
    nocterm_button_delete(b2);
    nocterm_button_delete(b3);
    nocterm_widget_delete(container);

    return 0;
}
