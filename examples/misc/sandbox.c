/**
 * @file sandbox.c
 *
 * A large "everything" sandbox for the Nocterm library.  It exercises every
 * widget and the major subsystems together in one program:
 *
 *   - tabs        : four tab pages, switched by buttons (no built-in navbar)
 *   - overlay     : a page-independent floating clock/HUD on top of everything
 *   - timers      : animate the level bars, the pixel grid and the clock
 *   - pages       : a second "Help" page pushed/popped on the page stack
 *   - mouse       : advanced mouse support (click + hover focus)
 *   - widgets     : button, checkbox, radiobutton, decorbox, entry, label,
 *                   levelbar, listview, loadingbar, menu, pixelgrid, textbox,
 *                   textview, tabs
 *
 * Controls:
 *   Tab / Shift-Tab : move focus       Enter / Space : activate focused widget
 *   Mouse           : click + hover     Arrow keys     : within menus / entries
 *   ESC             : quit
 *
 * Build (from the repo root):
 *   gcc -Iinclude -Ibuild/include examples/misc/sandbox.c \
 *       -Lbuild/lib -lnocterm -lpthread -o sandbox
 */

#include <nocterm/nocterm.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <time.h>

/* ───────────────────────────── cleanup registry ───────────────────────── */
/* Every allocated object is registered with its deleter so teardown is simple
 * and leak-free regardless of how deeply widgets are nested. */
typedef enum {
    D_WIDGET, D_DECORBOX, D_BUTTON, D_LABEL, D_ENTRY, D_TEXTBOX, D_TEXTVIEW,
    D_LISTVIEW, D_MENU, D_LEVELBAR, D_LOADINGBAR, D_PIXELGRID, D_CHECKBOX,
    D_RADIO, D_RADIOGROUP, D_TABS
} del_kind_t;

static struct { void* p; del_kind_t k; } g_cleanup[300];
static int g_cleanup_n = 0;

static void* track(void* p, del_kind_t k){
    if(p && g_cleanup_n < (int)(sizeof(g_cleanup)/sizeof(g_cleanup[0]))){
        g_cleanup[g_cleanup_n].p = p;
        g_cleanup[g_cleanup_n].k = k;
        g_cleanup_n++;
    }
    return p;
}

static void cleanup_all(void){
    for(int i = g_cleanup_n - 1; i >= 0; i--){
        void* p = g_cleanup[i].p;
        switch(g_cleanup[i].k){
            case D_WIDGET:     nocterm_widget_delete(p);            break;
            case D_DECORBOX:   nocterm_decorbox_delete(p);          break;
            case D_BUTTON:     nocterm_button_delete(p);            break;
            case D_LABEL:      nocterm_label_delete(p);             break;
            case D_ENTRY:      nocterm_entry_delete(p);             break;
            case D_TEXTBOX:    nocterm_textbox_delete(p);           break;
            case D_TEXTVIEW:   nocterm_textview_delete(p);          break;
            case D_LISTVIEW:   nocterm_listview_delete(p);          break;
            case D_MENU:       nocterm_menu_delete(p);              break;
            case D_LEVELBAR:   nocterm_levelbar_delete(p);          break;
            case D_LOADINGBAR: nocterm_loadingbar_delete(p);        break;
            case D_PIXELGRID:  nocterm_pixelgrid_delete(p);         break;
            case D_CHECKBOX:   nocterm_checkbox_delete(p);          break;
            case D_RADIO:      nocterm_radiobutton_delete(p);       break;
            case D_RADIOGROUP: nocterm_radiobutton_group_delete(p); break;
            case D_TABS:       nocterm_tabs_delete(p);              break;
        }
    }
}

/* ───────────────────────────── attribute helpers ──────────────────────── */
static nocterm_attribute_t A(int fg, int bold){
    nocterm_attribute_t a = {0};
    a.color.ansi.fg = true; a.color.ansi.codes.fg = (uint8_t)fg; a.bold = bold ? 1 : 0;
    return a;
}

static nocterm_decorbox_border_t g_border;
static nocterm_attribute_t g_bnormal, g_bfocus, g_blabel;

/* Wrap a widget in a labelled, bordered box placed at (row,col) in its parent. */
static nocterm_decorbox_t* boxed(nocterm_widget_t* w, int row, int col, const char* label){
    nocterm_decorbox_t* b = nocterm_decorbox_new(w);
    track(b, D_DECORBOX);
    nocterm_widget_set_position(NOCTERM_WIDGET(b), row, col);
    nocterm_decorbox_set_border(b, g_border, g_bnormal, g_bfocus);
    if(label) nocterm_decorbox_set_label(b, label, strlen(label)+1, g_blabel, 1);
    return b;
}

static void draw_text(nocterm_widget_t* w, int row, int col, const char* s, nocterm_attribute_t a){
    nocterm_char_t buf[160];
    uint64_t n = nocterm_char_string_from_stream(buf, 160, s, strlen(s));
    for(uint64_t i = 0; i < n; i++) nocterm_widget_update(w, row, col+(int)i, buf[i], a);
}

/* ───────────────────────────── global state ───────────────────────────── */
static struct {
    nocterm_tabs_t*     tabs;

    /* Inputs tab */
    nocterm_entry_t*    echo_entry;
    nocterm_textview_t* echo_out;
    nocterm_textbox_t*  notes;
    bool                bold;
    int                 color;          /* ansi code */

    /* Lists tab */
    nocterm_entry_t*    add_entry;
    nocterm_listview_t* list;
    nocterm_textview_t* menu_out;

    /* Display tab */
    nocterm_levelbar_t* lvl_h;
    nocterm_levelbar_t* lvl_v;
    nocterm_pixelgrid_t* grid;
    bool                paused;
    nocterm_timer_t*    t_lvlh;
    nocterm_timer_t*    t_lvlv;
    nocterm_timer_t*    t_grid;

    /* overlay HUD */
    nocterm_overlay_t*  overlay;
    nocterm_widget_t*   hud_body;
    nocterm_decorbox_t* hud_box;
    bool                hud_shown;

    /* help page */
    nocterm_page_t*     help_page;
} g;

/* ───────────────────────────── handlers ───────────────────────────────── */

NOCTERM_BUTTON_ONPRESS_HANDLER(on_tab_btn){
    (void)self;
    nocterm_tabs_navigate(g.tabs, (uint64_t)(intptr_t)user_data);
}

NOCTERM_BUTTON_ONPRESS_HANDLER(on_echo){
    (void)self; (void)user_data;
    char buf[128] = {0};
    uint64_t len = 0;
    nocterm_entry_get_text(g.echo_entry, buf, sizeof(buf), &len);
    nocterm_textview_set_attribute(g.echo_out, A(g.color, g.bold));
    if(len > 0) nocterm_textview_set_text(g.echo_out, buf, len+1);
    else        nocterm_textview_set_text(g.echo_out, "(type something then Echo)", sizeof("(type something then Echo)"));
}

NOCTERM_CHECKBOX_ONCHECK_HANDLER(on_bold){
    (void)self; (void)user_data;
    g.bold = (action == NOCTERM_CHECKBOX_ACTION_CHECK);
}

NOCTERM_RADIOBUTTON_ONSELECT_HANDLER(on_color){
    (void)self;
    if(action == NOCTERM_RADIOBUTTON_ACTION_SELECT) g.color = (int)(intptr_t)user_data;
}

NOCTERM_MENU_ONSELECT_HANDLER(on_menu){
    (void)self; (void)user_data;
    static const char* names[] = {
        "New file","Open file","Save","Save As","Preferences","Quit"
    };
    if(selected_item < 6){
        char msg[64];
        snprintf(msg, sizeof(msg), "Selected: %s", names[selected_item]);
        nocterm_textview_set_text(g.menu_out, msg, strlen(msg)+1);
    }
}

NOCTERM_BUTTON_ONPRESS_HANDLER(on_add){
    (void)self; (void)user_data;
    char buf[128] = {0};
    uint64_t len = 0;
    nocterm_entry_get_text(g.add_entry, buf, sizeof(buf), &len);
    if(len > 0){
        nocterm_listview_item_t item = {0};
        nocterm_listview_item_constructor(&item, buf, len+1, A(6, 0));
        nocterm_listview_push_back(g.list, item);
    }
}

NOCTERM_BUTTON_ONPRESS_HANDLER(on_clear_list){
    (void)self; (void)user_data;
    nocterm_listview_clear(g.list);
}

NOCTERM_CHECKBOX_ONCHECK_HANDLER(on_toggle_hud){
    (void)self; (void)user_data;
    g.hud_shown = (action == NOCTERM_CHECKBOX_ACTION_CHECK);
    nocterm_widget_set_visible(NOCTERM_WIDGET(g.hud_box), g.hud_shown);
    nocterm_overlay_invalidate(g.overlay);
}

NOCTERM_BUTTON_ONPRESS_HANDLER(on_pause){
    (void)self; (void)user_data;
    g.paused = !g.paused;
    if(g.paused){
        nocterm_timer_stop(g.t_lvlh); nocterm_timer_stop(g.t_lvlv); nocterm_timer_stop(g.t_grid);
    }else{
        nocterm_timer_start(g.t_lvlh); nocterm_timer_start(g.t_lvlv); nocterm_timer_start(g.t_grid);
    }
}

NOCTERM_BUTTON_ONPRESS_HANDLER(on_help){
    (void)self; (void)user_data;
    nocterm_page_stack_push(g.help_page);
}

NOCTERM_BUTTON_ONPRESS_HANDLER(on_back){
    (void)self; (void)user_data;
    nocterm_page_stack_pop();
}

/* ───────────────────────────── timers ─────────────────────────────────── */

NOCTERM_TIMER_CALLBACK(tick_lvlh){
    static uint64_t v = 0;
    nocterm_levelbar_set_value(NOCTERM_LEVELBAR(widget), v);
    v = (v + 2) % 101;
    (void)user_data;
}

NOCTERM_TIMER_CALLBACK(tick_lvlv){
    static uint64_t v = 0;
    nocterm_levelbar_set_value(NOCTERM_LEVELBAR(widget), v);
    v = (v + 3) % 101;
    (void)user_data;
}

NOCTERM_TIMER_CALLBACK(tick_grid){
    static int f = 0;
    for(uint16_t r = 0; r < 8; r++){
        for(uint16_t c = 0; c < 16; c++){
            uint8_t red   = (uint8_t)((r*16 + f) & 0xff);
            uint8_t green = (uint8_t)((c*8  + f*2) & 0xff);
            uint8_t blue  = (uint8_t)((r*c  + f*3) & 0xff);
            nocterm_pixelgrid_set_pixel(g.grid, r, c, red, green, blue);
        }
    }
    f = (f + 4) & 0xff;
    (void)user_data;
}

NOCTERM_TIMER_CALLBACK(tick_clock){
    (void)user_data;
    static const char spin[] = "|/-\\";
    static int s = 0;
    time_t now = time(NULL);
    struct tm* lt = localtime(&now);
    char line[24];
    snprintf(line, sizeof(line), "%c %02d:%02d:%02d", spin[s], lt->tm_hour, lt->tm_min, lt->tm_sec);
    s = (s + 1) & 3;
    nocterm_widget_clear(widget);
    draw_text(widget, 0, 0, line, A(3, 1));
}

/* ───────────────────────────── tab builders ───────────────────────────── */
#define TAB_H 17
#define TAB_W 72

/* Tab 1: text input widgets. */
static nocterm_widget_t* build_inputs_tab(void){
    nocterm_widget_t* root = track(nocterm_widget_new(TAB_H, TAB_W, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_REAL), D_WIDGET);
    
    g.echo_entry = track(nocterm_entry_new(28), D_ENTRY);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(boxed(NOCTERM_WIDGET(g.echo_entry), 0, 0, "Entry")));

    g.echo_out = track(nocterm_textview_new(5, 38), D_TEXTVIEW);
    nocterm_textview_set_text(g.echo_out, "Echoed text appears here.", sizeof("Echoed text appears here."));
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(boxed(NOCTERM_WIDGET(g.echo_out), 4, 0, "Echo output")));

    g.notes = track(nocterm_textbox_new(3, 38), D_TEXTBOX);
    nocterm_textbox_set_text(g.notes, "A multi-line textbox.\nEdit me freely.", sizeof("A multi-line textbox.\nEdit me freely."));
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(boxed(NOCTERM_WIDGET(g.notes), 11, 0, "Notes (textbox)")));

    /* right column */
    nocterm_button_t* echo = track(nocterm_button_new(1, 8, on_echo, NULL), D_BUTTON);
    nocterm_button_set_text(echo, "Echo", sizeof("Echo"));
    nocterm_button_set_attribute(echo, A(7,0), A(2,1));
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(boxed(NOCTERM_WIDGET(echo), 0, 44, NULL)));

    nocterm_checkbox_t* bold = track(nocterm_checkbox_new(on_bold, false, NULL), D_CHECKBOX);
    nocterm_widget_set_position(NOCTERM_WIDGET(bold), 4, 44);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(bold));
    nocterm_label_t* bold_l = track(nocterm_label_new("Bold output", sizeof("Bold output")), D_LABEL);
    nocterm_widget_set_position(NOCTERM_WIDGET(bold_l), 4, 48);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(bold_l));

    draw_text(root, 6, 44, "Text colour:", A(7,1));
    nocterm_radiobutton_group_t* grp = track(nocterm_radiobutton_group_new(), D_RADIOGROUP);
    const char* cnames[3] = {"Red","Green","Blue"};
    int ccodes[3] = {1,2,4};
    for(int i = 0; i < 3; i++){
        nocterm_radiobutton_t* rb = track(nocterm_radiobutton_new(grp, on_color, i==0, (void*)(intptr_t)ccodes[i]), D_RADIO);
        nocterm_widget_set_position(NOCTERM_WIDGET(rb), 7+i, 44);
        nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(rb));
        nocterm_label_t* l = track(nocterm_label_new(cnames[i], strlen(cnames[i])+1), D_LABEL);
        nocterm_widget_set_position(NOCTERM_WIDGET(l), 7+i, 48);
        nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(l));
    }
    g.color = 1;

    draw_text(root, 16, 0, "Type in the entry, pick a colour/bold, press Echo.", A(8,0));
    return root;
}

/* Tab 2: list-oriented widgets. */
static nocterm_widget_t* build_lists_tab(void){
    nocterm_widget_t* root = track(nocterm_widget_new(TAB_H, TAB_W, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_REAL), D_WIDGET);

    nocterm_menu_t* menu = track(nocterm_menu_new(6, 6, 16, on_menu, NULL), D_MENU);
    nocterm_menu_item_t items[6] = {0};
    const char* labels[6] = {"New file","Open file","Save","Save As","Preferences","Quit"};
    for(int i = 0; i < 6; i++) nocterm_menu_item_constructor(&items[i], labels[i], strlen(labels[i])+1, A(7,0));
    nocterm_menu_add_item_multiple(menu, items, 6);
    nocterm_menu_set_selection_attribute(menu, A(2,1));
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(boxed(NOCTERM_WIDGET(menu), 0, 0, "Menu")));

    g.menu_out = track(nocterm_textview_new(6, 22), D_TEXTVIEW);
    nocterm_textview_set_text(g.menu_out, "Pick a menu item.", sizeof("Pick a menu item."));
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(boxed(NOCTERM_WIDGET(g.menu_out), 0, 20, "Selected")));

    g.list = track(nocterm_listview_new(8, 16, 24), D_LISTVIEW);
    nocterm_listview_set_autoscroll(g.list, NOCTERM_LISTVIEW_AUTOSCROLL_DOWN);
    for(int i = 1; i <= 3; i++){
        char s[24]; snprintf(s, sizeof(s), "Item %d", i);
        nocterm_listview_item_t it = {0};
        nocterm_listview_item_constructor(&it, s, strlen(s)+1, A(7,0));
        nocterm_listview_push_back(g.list, it);
    }
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(boxed(NOCTERM_WIDGET(g.list), 0, 46, "Listview")));

    g.add_entry = track(nocterm_entry_new(20), D_ENTRY);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(boxed(NOCTERM_WIDGET(g.add_entry), 11, 0, "Add to list")));

    nocterm_button_t* add = track(nocterm_button_new(1, 8, on_add, NULL), D_BUTTON);
    nocterm_button_set_text(add, "Add", sizeof("Add"));
    nocterm_button_set_attribute(add, A(7,0), A(2,1));
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(boxed(NOCTERM_WIDGET(add), 11, 24, NULL)));

    nocterm_button_t* clr = track(nocterm_button_new(1, 10, on_clear_list, NULL), D_BUTTON);
    nocterm_button_set_text(clr, "Clear", sizeof("Clear"));
    nocterm_button_set_attribute(clr, A(7,0), A(1,1));
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(boxed(NOCTERM_WIDGET(clr), 11, 36, NULL)));

    return root;
}

/* Tab 3: display + animated widgets driven by timers. */
static nocterm_widget_t* build_display_tab(void){
    nocterm_widget_t* root = track(nocterm_widget_new(TAB_H, TAB_W, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_REAL), D_WIDGET);

    g.lvl_h = track(nocterm_levelbar_new(46, 0, 100, NOCTERM_LEVELBAR_TYPE_HORIZONTAL, false), D_LEVELBAR);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(boxed(NOCTERM_WIDGET(g.lvl_h), 0, 0, "Levelbar (horizontal)")));

    g.lvl_v = track(nocterm_levelbar_new(10, 0, 100, NOCTERM_LEVELBAR_TYPE_VERTICAL, false), D_LEVELBAR);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(boxed(NOCTERM_WIDGET(g.lvl_v), 0, 64, "V")));

    nocterm_loadingbar_t* load = track(nocterm_loadingbar_new(120), D_LOADINGBAR);
    nocterm_widget_set_position(NOCTERM_WIDGET(load), 4, 2);
    nocterm_loadingbar_enable(load);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(load));
    draw_text(root, 4, 5, "<- loadingbar (built-in spinner)", A(8,0));

    g.grid = track(nocterm_pixelgrid_new(8, 16), D_PIXELGRID);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(boxed(NOCTERM_WIDGET(g.grid), 6, 0, "Pixelgrid")));

    nocterm_checkbox_t* hud = track(nocterm_checkbox_new(on_toggle_hud, true, NULL), D_CHECKBOX);
    nocterm_widget_set_position(NOCTERM_WIDGET(hud), 7, 40);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(hud));
    nocterm_label_t* hud_l = track(nocterm_label_new("Show overlay HUD", sizeof("Show overlay HUD")), D_LABEL);
    nocterm_widget_set_position(NOCTERM_WIDGET(hud_l), 7, 44);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(hud_l));

    nocterm_button_t* pause = track(nocterm_button_new(1, 18, on_pause, NULL), D_BUTTON);
    nocterm_button_set_text(pause, "Pause/Resume anim", sizeof("Pause/Resume anim"));
    nocterm_button_set_attribute(pause, A(7,0), A(5,1));
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(boxed(NOCTERM_WIDGET(pause), 9, 40, NULL)));

    return root;
}

/* Tab 4: an "about" text page. */
static nocterm_widget_t* build_about_tab(void){
    nocterm_widget_t* root = track(nocterm_widget_new(TAB_H, TAB_W, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_REAL), D_WIDGET);

    nocterm_textview_t* tv = track(nocterm_textview_new(13, 68), D_TEXTVIEW);
    const char* about =
        "NOCTERM SANDBOX\n"
        "\n"
        "This program exercises the whole library at once: tabs, an overlay "
        "HUD, timers, a second page, mouse support and every widget "
        "(button, checkbox, radiobutton, decorbox, entry, label, levelbar, "
        "listview, loadingbar, menu, pixelgrid, textbox, textview, tabs).\n"
        "\n"
        "Controls:  Tab/Shift-Tab move focus, Enter/Space activate, the mouse "
        "clicks and hover-highlights, ESC quits.  The floating clock is an "
        "overlay and keeps ticking even on the Help page.";
    nocterm_textview_set_text(tv, about, strlen(about)+1);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(boxed(NOCTERM_WIDGET(tv), 0, 0, "About")));

    return root;
}

/* ───────────────────────────── main ───────────────────────────────────── */
int main(void){
    setlocale(LC_ALL, "");

    g_border  = nocterm_decorbox_border_from_shape(NOCTERM_DECORBOX_BORDER_SHAPE_UNICODE_ROUND);
    g_bnormal = A(7, 0);
    g_bfocus  = A(4, 1);
    g_blabel  = A(6, 1);
    g.hud_shown = true;

    /* Root container holds the tab buttons and the tabs widget. */
    nocterm_widget_t* root = track(nocterm_widget_new(TAB_H + 4, TAB_W, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_VIRTUAL), D_WIDGET);

    /* Tab navigation buttons across the top (tabs has no built-in navbar). */
    const char* tab_names[4] = {"1 Inputs","2 Lists","3 Display","4 About"};
    for(int i = 0; i < 4; i++){
        nocterm_button_t* tb = track(nocterm_button_new(1, 11, on_tab_btn, (void*)(intptr_t)i), D_BUTTON);
        nocterm_button_set_text(tb, tab_names[i], strlen(tab_names[i])+1);
        nocterm_button_set_attribute(tb, A(7,0), A(2,1));
        nocterm_widget_set_position(NOCTERM_WIDGET(tb), 0, i*13);
        nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(tb));
    }
    nocterm_button_t* help = track(nocterm_button_new(1, 8, on_help, NULL), D_BUTTON);
    nocterm_button_set_text(help, "Help", sizeof("Help"));
    nocterm_button_set_attribute(help, A(7,0), A(3,1));
    nocterm_widget_set_position(NOCTERM_WIDGET(help), 0, 56);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(help));

    /* The tabs widget with four tab pages. */
    g.tabs = track(nocterm_tabs_new(TAB_H, TAB_W), D_TABS);
    nocterm_widget_set_position(NOCTERM_WIDGET(g.tabs), 2, 0);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(g.tabs));
    nocterm_tabs_add(g.tabs, build_inputs_tab());
    nocterm_tabs_add(g.tabs, build_lists_tab());
    nocterm_tabs_add(g.tabs, build_display_tab());
    nocterm_tabs_add(g.tabs, build_about_tab());

    /* Wrap the whole thing in a titled border and centre it. */
    nocterm_decorbox_t* frame = boxed(root, 0, 0, "Nocterm Sandbox  (ESC quits)");
    nocterm_widget_align(NOCTERM_WIDGET(frame), NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL);
    nocterm_widget_align(NOCTERM_WIDGET(frame), NOCTERM_WIDGET_ALIGN_PERCENT_HORIZONTAL, 50);
    nocterm_widget_align(NOCTERM_WIDGET(frame), NOCTERM_WIDGET_ALIGN_CENTER_VERTICAL);
    nocterm_widget_align(NOCTERM_WIDGET(frame), NOCTERM_WIDGET_ALIGN_PERCENT_VERTICAL, 55);

    nocterm_page_t* main_page = nocterm_page_new("Sandbox", sizeof("Sandbox"), NOCTERM_WIDGET(frame));
    nocterm_page_stack_push(main_page);

    /* A second page reachable via the Help button (demonstrates the page stack
     * and that the overlay survives a page change). */
    nocterm_widget_t* help_root = track(nocterm_widget_new(11, 46, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_VIRTUAL), D_WIDGET);
    nocterm_textview_t* help_tv = track(nocterm_textview_new(6, 44), D_TEXTVIEW);
    nocterm_textview_set_text(help_tv,
        "HELP PAGE\n\nThis is a second page on the page stack. "
        "Notice the overlay clock keeps ticking. Press Back to return.",
        sizeof("HELP PAGE\n\nThis is a second page on the page stack. "
        "Notice the overlay clock keeps ticking. Press Back to return."));
    nocterm_widget_add_subwidget(help_root, NOCTERM_WIDGET(boxed(NOCTERM_WIDGET(help_tv), 0, 0, "Help")));
    nocterm_button_t* back = track(nocterm_button_new(1, 8, on_back, NULL), D_BUTTON);
    nocterm_button_set_text(back, "Back", sizeof("Back"));
    nocterm_button_set_attribute(back, A(7,0), A(2,1));
    nocterm_widget_add_subwidget(help_root, NOCTERM_WIDGET(boxed(NOCTERM_WIDGET(back), 8, 0, NULL)));
    nocterm_decorbox_t* help_frame = boxed(help_root, 0, 0, "Second Page");
    nocterm_widget_align(NOCTERM_WIDGET(help_frame), NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL);
    nocterm_widget_align(NOCTERM_WIDGET(help_frame), NOCTERM_WIDGET_ALIGN_PERCENT_HORIZONTAL, 50);
    nocterm_widget_align(NOCTERM_WIDGET(help_frame), NOCTERM_WIDGET_ALIGN_CENTER_VERTICAL);
    nocterm_widget_align(NOCTERM_WIDGET(help_frame), NOCTERM_WIDGET_ALIGN_PERCENT_VERTICAL, 50);
    g.help_page = nocterm_page_new("Help", sizeof("Help"), NOCTERM_WIDGET(help_frame));

    /* Overlay HUD: a floating clock, page-independent, top-left. */
    g.overlay = nocterm_overlay_new();
    g.hud_body = track(nocterm_widget_new(1, 12, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_REAL), D_WIDGET);
    g.hud_box = nocterm_decorbox_new(g.hud_body);
    track(g.hud_box, D_DECORBOX);
    nocterm_decorbox_set_border(g.hud_box, g_border, A(3,1), A(3,1));
    nocterm_decorbox_set_label(g.hud_box, "clock", sizeof("clock"), A(3,1), 1);
    nocterm_widget_set_position(NOCTERM_WIDGET(g.hud_box), 0, 1);
    nocterm_overlay_add_widget(g.overlay, NOCTERM_WIDGET(g.hud_box));
    nocterm_overlay_set(g.overlay);

    /* Timers: clock, the two level bars and the pixel grid. */
    nocterm_timer_t* t_clock = nocterm_timer_create(g.hud_body, 250, tick_clock, NULL);
    g.t_lvlh = nocterm_timer_create(NOCTERM_WIDGET(g.lvl_h), 60, tick_lvlh, NULL);
    g.t_lvlv = nocterm_timer_create(NOCTERM_WIDGET(g.lvl_v), 80, tick_lvlv, NULL);
    g.t_grid = nocterm_timer_create(NOCTERM_WIDGET(g.grid), 120, tick_grid, NULL);
    nocterm_timer_start(t_clock);
    nocterm_timer_start(g.t_lvlh);
    nocterm_timer_start(g.t_lvlv);
    nocterm_timer_start(g.t_grid);

    nocterm_mouse_set_support(NOCTERM_MOUSE_SUPPORT_ADVANCED);

    nocterm_init();
    nocterm_loop();
    nocterm_end();

    nocterm_timer_delete_all();
    nocterm_overlay_unset();
    nocterm_overlay_delete(g.overlay);
    nocterm_page_delete(main_page);
    nocterm_page_delete(g.help_page);
    cleanup_all();

    return 0;
}
