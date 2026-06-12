#include <nocterm/nocterm.h>

// Each radio button carries the option name as its user data so the handler can
// report which choice is now active.  Because the buttons share a single group,
// selecting one automatically deselects whichever was previously chosen.
NOCTERM_RADIOBUTTON_ONSELECT_HANDLER(handler){

    const char* name = (const char*)user_data;

    switch(action){
        case NOCTERM_RADIOBUTTON_ACTION_SELECT:
            nocterm_io_print_at(7, 1, "Selected: %-12s", name);
            break;

        case NOCTERM_RADIOBUTTON_ACTION_DESELECT:
            // Nothing to do; the newly selected button will overwrite the line.
            break;
    }
}

int main(){

    nocterm_widget_t* my_widget = nocterm_widget_new(10, 24, NOCTERM_WIDGET_FOCUSABLE_YES, NOCTERM_WIDGET_TYPE_REAL);
    nocterm_page_t* main_page = nocterm_page_new("Main page", sizeof("Main page"), my_widget);

    nocterm_label_t* title = nocterm_label_new("Pick a difficulty:", sizeof("Pick a difficulty:"));
    nocterm_widget_set_position(NOCTERM_WIDGET(title), 0, 1);
    nocterm_widget_add_subwidget(my_widget, NOCTERM_WIDGET(title));

    // A single group ties the three radio buttons together so only one of them
    // can be selected at any moment.
    nocterm_radiobutton_group_t* group = nocterm_radiobutton_group_new();

    // "Normal" starts selected, demonstrating an initial selection.
    nocterm_radiobutton_t* easy   = nocterm_radiobutton_new(group, handler, false, "Easy");
    nocterm_radiobutton_t* normal = nocterm_radiobutton_new(group, handler, true,  "Normal");
    nocterm_radiobutton_t* hard   = nocterm_radiobutton_new(group, handler, false, "Hard");

    nocterm_widget_set_position(NOCTERM_WIDGET(easy),   2, 1);
    nocterm_widget_set_position(NOCTERM_WIDGET(normal), 3, 1);
    nocterm_widget_set_position(NOCTERM_WIDGET(hard),   4, 1);

    nocterm_widget_add_subwidget(my_widget, NOCTERM_WIDGET(easy));
    nocterm_widget_add_subwidget(my_widget, NOCTERM_WIDGET(normal));
    nocterm_widget_add_subwidget(my_widget, NOCTERM_WIDGET(hard));

    // Labels printed to the right of each radio button (each button is 3 cells
    // wide, so the text starts at column 5).
    nocterm_label_t* easy_label   = nocterm_label_new("Easy",   sizeof("Easy"));
    nocterm_label_t* normal_label = nocterm_label_new("Normal", sizeof("Normal"));
    nocterm_label_t* hard_label   = nocterm_label_new("Hard",   sizeof("Hard"));

    nocterm_widget_set_position(NOCTERM_WIDGET(easy_label),   2, 5);
    nocterm_widget_set_position(NOCTERM_WIDGET(normal_label), 3, 5);
    nocterm_widget_set_position(NOCTERM_WIDGET(hard_label),   4, 5);

    nocterm_widget_add_subwidget(my_widget, NOCTERM_WIDGET(easy_label));
    nocterm_widget_add_subwidget(my_widget, NOCTERM_WIDGET(normal_label));
    nocterm_widget_add_subwidget(my_widget, NOCTERM_WIDGET(hard_label));

    // Reflect the initial selection on screen before the loop starts.
    nocterm_io_print_at(7, 1, "Selected: %-12s", "Normal");

    nocterm_page_stack_push(main_page);

    nocterm_init();
    nocterm_loop();
    nocterm_end();

    nocterm_widget_delete(my_widget);
    nocterm_page_delete(main_page);

    nocterm_radiobutton_delete(easy);
    nocterm_radiobutton_delete(normal);
    nocterm_radiobutton_delete(hard);
    nocterm_radiobutton_group_delete(group);

    nocterm_label_delete(title);
    nocterm_label_delete(easy_label);
    nocterm_label_delete(normal_label);
    nocterm_label_delete(hard_label);

    return 0;
}
