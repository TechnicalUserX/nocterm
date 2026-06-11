/*
 * overlay_manager.c — nocterm advanced example: the passive overlay layer
 *
 * The overlay (nocterm_overlay_*) is a *passive* layer of widgets that floats
 * above whatever page is currently on the stack.  It is NOT a dialog or input
 * system: keystrokes always go to the focused widget of the active page, never
 * to the overlay.  The overlay just renders on top — think persistent HUDs,
 * status read-outs, watermarks, and transient toast notifications.
 *
 * This example shows the two properties that make the overlay distinctive:
 *
 *   1. It is page-independent.  We push and pop between two pages; the floating
 *      status HUD (a live clock + spinner) and any toast notifications stay put
 *      and keep updating across the page change.  The HUD's repaint timer is
 *      attached to an overlay widget, so the page machinery never stops it.
 *
 *   2. Dynamic changes recomposite correctly via nocterm_overlay_invalidate().
 *      Because the screen is arbitrated by per-cell ownership, simply toggling
 *      an overlay widget's visibility is not enough — the page may still own the
 *      cells.  Calling nocterm_overlay_invalidate() asks the overlay to reclaim
 *      (or release) its cells on the next frame.  Toasts appearing/expiring and
 *      the 'h' HUD toggle all go through it.
 *
 * Compile from the repo root:
 *
 *   gcc -Werror -Wall \
 *       -I/home/tux/nocterm/include \
 *       -I/home/tux/nocterm/build/include \
 *       examples/advanced/overlay_manager.c \
 *       /home/tux/nocterm/build/lib/libnocterm.a \
 *       -lpthread -o overlay_manager
 *
 * Keys (on either page):
 *   o   open the second page      b   go back to the first page
 *   t   fire a toast notification h   toggle the floating HUD
 *   q   quit
 */

#include <nocterm/nocterm.h>

#include <locale.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>

/* ════════════════════════════════════════════════════════════ STATE ══ */

#define MAX_TOASTS 4
#define TOAST_LEN  34
typedef struct toast_t {
    char   text[TOAST_LEN];
    int    color;
    double expires_at;
    bool   active;
} toast_t;
static toast_t g_toasts[MAX_TOASTS];

static int  g_page_id    = 0;        /* 0 = Home, 1 = Second               */
static bool g_hud_shown  = true;
static int  g_toast_seq  = 1;

/* ═══════════════════════════════════════════════════════ WIDGET REFS ══ */

/* page A */
static nocterm_widget_t *g_home_root;
static nocterm_widget_t *g_home_body;
/* page B */
static nocterm_widget_t *g_second_root;
static nocterm_widget_t *g_second_body;

/* overlay layer (page-independent) */
static nocterm_overlay_t *g_overlay;
static nocterm_widget_t  *g_hud_body;                 /* inside the HUD decorbox */
static nocterm_decorbox_t *g_hud_box;
static nocterm_widget_t  *g_toast_w[MAX_TOASTS];

static nocterm_page_t  *g_home_page;
static nocterm_page_t  *g_second_page;
static nocterm_timer_t *g_clock;

/* ════════════════════════════════════════════════════ ATTRIBUTE HELPERS ══ */

static nocterm_attribute_t a_fg(int color, bool bold)
{
    nocterm_attribute_t a = {0};
    a.color.ansi.fg = true; a.color.ansi.codes.fg = (uint8_t)color; a.bold = bold;
    return a;
}
static nocterm_attribute_t a_fg_bg(int fg, int bg, bool bold)
{
    nocterm_attribute_t a = {0};
    a.color.ansi.fg = true; a.color.ansi.codes.fg = (uint8_t)fg;
    a.color.ansi.bg = true; a.color.ansi.codes.bg = (uint8_t)bg;
    a.bold = bold;
    return a;
}

#define COL_ACCENT 6   /* cyan   */
#define COL_KEY    3   /* yellow */
#define COL_PLAIN  7   /* white  */
#define COL_DIM    8   /* grey   */
#define COL_OK     2   /* green  */
#define COL_INFO   4   /* blue   */

/* ═══════════════════════════════════════════════════ LOW-LEVEL DRAWING ══ */

static int cw(nocterm_widget_t *w) { return (int)w->bounds.width;  }
static int ch(nocterm_widget_t *w) { return (int)w->bounds.height; }

static void put(nocterm_widget_t *w, int r, int c, nocterm_char_t k, nocterm_attribute_t a)
{
    if (r >= 0 && c >= 0 && r < ch(w) && c < cw(w))
        nocterm_widget_update(w, (nocterm_dimension_size_t)r, (nocterm_dimension_size_t)c, k, a);
}
static void fill_row(nocterm_widget_t *w, int r, int from, nocterm_attribute_t a)
{
    for (int c = from; c < cw(w); c++) put(w, r, c, nocterm_char_from_ascii(' '), a);
}
static void clear_w(nocterm_widget_t *w, nocterm_attribute_t a)
{
    for (int r = 0; r < ch(w); r++) fill_row(w, r, 0, a);
}
static int draw(nocterm_widget_t *w, int r, int c, const char *s, nocterm_attribute_t a)
{
    nocterm_char_t buf[256];
    uint64_t n = nocterm_char_string_from_stream(buf, 256, s, strlen(s) + 1);
    for (uint64_t i = 0; i < n; i++) { if (c >= cw(w)) break; put(w, r, c++, buf[i], a); }
    return c;
}

/* ════════════════════════════════════════════════════════════ TOASTS ══ */

static void toast_push(const char *text, int color)
{
    int slot = -1;
    for (int i = 0; i < MAX_TOASTS; i++) if (!g_toasts[i].active) { slot = i; break; }
    if (slot < 0) {                                  /* recycle the oldest */
        double oldest = 1e18;
        for (int i = 0; i < MAX_TOASTS; i++)
            if (g_toasts[i].expires_at < oldest) { oldest = g_toasts[i].expires_at; slot = i; }
    }
    snprintf(g_toasts[slot].text, TOAST_LEN, "%s", text);
    g_toasts[slot].color      = color;
    g_toasts[slot].expires_at = (double)time(NULL) + 3.0;
    g_toasts[slot].active     = true;

    /* composition changed → let the overlay claim cells next frame */
    nocterm_overlay_invalidate(g_overlay);
}

/* ════════════════════════════════════════════════════════════ RENDER ══ */

static const wchar_t SPINNER[] = { L'⠋', L'⠙', L'⠹', L'⠸', L'⠼', L'⠴', L'⠦', L'⠧', L'⠇', L'⠏' };

static void render_hud(unsigned tick)
{
    nocterm_widget_t *w = g_hud_body;
    if (!w || cw(w) <= 0) return;

    nocterm_attribute_t bg = a_fg_bg(COL_PLAIN, 0, false);
    clear_w(w, bg);

    time_t t = time(NULL);
    struct tm tm; localtime_r(&t, &tm);
    char clock[16];
    strftime(clock, sizeof clock, "%H:%M:%S", &tm);

    int active = 0;
    for (int i = 0; i < MAX_TOASTS; i++) if (g_toasts[i].active) active++;

    int c = 1;
    c = draw(w, 0, c, clock, a_fg(COL_ACCENT, true));
    put(w, 0, c + 1, nocterm_char_from_wchar(SPINNER[tick % 10]), a_fg(COL_OK, true));
    c += 3;
    c = draw(w, 0, c, g_page_id == 0 ? "Home" : "Second", a_fg(COL_KEY, true));

    char tc[16]; snprintf(tc, sizeof tc, "toasts:%d", active);
    draw(w, 0, cw(w) - (int)strlen(tc) - 1, tc, a_fg(COL_DIM, false));
}

static void render_toasts(void)
{
    for (int i = 0; i < MAX_TOASTS; i++) {
        nocterm_widget_t *w = g_toast_w[i];
        if (!w) continue;

        bool want = g_toasts[i].active;
        bool have; nocterm_widget_get_visible(w, &have);
        if (want != have) {
            nocterm_widget_set_visible(w, want);
            nocterm_overlay_invalidate(g_overlay);   /* appeared or vanished */
        }
        if (!want) continue;

        nocterm_attribute_t body = a_fg_bg(COL_PLAIN, 0, false);
        clear_w(w, body);
        for (int r = 0; r < ch(w); r++)              /* accent bar */
            put(w, r, 0, nocterm_char_from_wchar(L'▌'), a_fg(g_toasts[i].color, true));
        draw(w, 0, 2, g_toasts[i].text, a_fg(COL_PLAIN, true));
    }
}

/* the only place the overlay is repainted; runs across both pages */
NOCTERM_TIMER_CALLBACK(clock_cb)
{
    (void)widget; (void)user_data;
    static unsigned tick = 0;
    tick++;

    /* expire toasts */
    double now = (double)time(NULL);
    for (int i = 0; i < MAX_TOASTS; i++)
        if (g_toasts[i].active && now >= g_toasts[i].expires_at) {
            g_toasts[i].active = false;              /* render_toasts hides + invalidates */
        }

    if (g_hud_shown) render_hud(tick);
    render_toasts();
}

/* page bodies are static; drawn once at build time */
static void render_home(void)
{
    nocterm_widget_t *w = g_home_body;
    clear_w(w, a_fg_bg(COL_PLAIN, 0, false));
    draw(w, 1, 2, "HOME PAGE", a_fg(COL_ACCENT, true));
    draw(w, 3, 2, "The status HUD (top-right) and toasts float above this page", a_fg(COL_PLAIN, false));
    draw(w, 4, 2, "from the overlay layer — and stay live when you switch pages.", a_fg(COL_PLAIN, false));
    draw(w, 6, 2, "o", a_fg(COL_KEY, true)); draw(w, 6, 6, "open the second page", a_fg(COL_DIM, false));
    draw(w, 7, 2, "t", a_fg(COL_KEY, true)); draw(w, 7, 6, "fire a toast notification", a_fg(COL_DIM, false));
    draw(w, 8, 2, "h", a_fg(COL_KEY, true)); draw(w, 8, 6, "toggle the HUD", a_fg(COL_DIM, false));
    draw(w, 9, 2, "q", a_fg(COL_KEY, true)); draw(w, 9, 6, "quit", a_fg(COL_DIM, false));
}

static void render_second(void)
{
    nocterm_widget_t *w = g_second_body;
    clear_w(w, a_fg_bg(COL_PLAIN, COL_INFO, false));   /* blue field to make the page change obvious */
    draw(w, 1, 2, "SECOND PAGE", a_fg_bg(COL_KEY, COL_INFO, true));
    draw(w, 3, 2, "Different page, same overlay on top. Watch the HUD clock keep", a_fg_bg(COL_PLAIN, COL_INFO, false));
    draw(w, 4, 2, "ticking and any toasts persist — the overlay is page-independent.", a_fg_bg(COL_PLAIN, COL_INFO, false));
    draw(w, 6, 2, "b", a_fg_bg(COL_KEY, COL_INFO, true)); draw(w, 6, 6, "back to the first page", a_fg_bg(COL_PLAIN, COL_INFO, false));
    draw(w, 7, 2, "t", a_fg_bg(COL_KEY, COL_INFO, true)); draw(w, 7, 6, "fire a toast notification", a_fg_bg(COL_PLAIN, COL_INFO, false));
    draw(w, 8, 2, "h", a_fg_bg(COL_KEY, COL_INFO, true)); draw(w, 8, 6, "toggle the HUD", a_fg_bg(COL_PLAIN, COL_INFO, false));
}

/* ════════════════════════════════════════════════════════════ INPUT ══ */

static void hud_toggle(void)
{
    g_hud_shown = !g_hud_shown;
    nocterm_widget_set_visible(NOCTERM_WIDGET(g_hud_box), g_hud_shown);
    nocterm_overlay_invalidate(g_overlay);
    if (!g_hud_shown) toast_push("HUD hidden (press h)", COL_DIM);
}

static void fire_toast(void)
{
    char msg[TOAST_LEN];
    snprintf(msg, sizeof msg, "Notification #%d", g_toast_seq++);
    toast_push(msg, COL_OK);
}

NOCTERM_WIDGET_KEY_HANDLER(home_kh)
{
    (void)self;
    if (nocterm_key_translate(key) != NOCTERM_KEY_EVENT_PRINTABLE) return;
    switch (key->buffer[0]) {
    case 'o': g_page_id = 1; nocterm_page_stack_push(g_second_page); break;
    case 't': fire_toast(); break;
    case 'h': hud_toggle();  break;
    case 'q': nocterm_page_stack_pop(); break;
    default: break;
    }
}

NOCTERM_WIDGET_KEY_HANDLER(second_kh)
{
    (void)self;
    if (nocterm_key_translate(key) != NOCTERM_KEY_EVENT_PRINTABLE) return;
    switch (key->buffer[0]) {
    case 'b': g_page_id = 0; nocterm_page_stack_pop(); break;
    case 't': fire_toast(); break;
    case 'h': hud_toggle();  break;
    default: break;
    }
}

/* ════════════════════════════════════════════════════════════ BUILD ══ */

static nocterm_page_t *build_home(void)
{
    g_home_root = nocterm_widget_new(1, 1, NOCTERM_WIDGET_FOCUSABLE_YES, NOCTERM_WIDGET_TYPE_VIRTUAL);
    nocterm_widget_flex(g_home_root, NOCTERM_WIDGET_FLEX_FILL_BOTH);
    nocterm_widget_set_key_handler(g_home_root, home_kh);

    g_home_body = nocterm_widget_new(1, 1, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_REAL);
    nocterm_widget_flex(g_home_body, NOCTERM_WIDGET_FLEX_FILL_BOTH);
    nocterm_widget_add_subwidget(g_home_root, g_home_body);

    render_home();
    return nocterm_page_new("Home", sizeof "Home", g_home_root);
}

static nocterm_page_t *build_second(void)
{
    g_second_root = nocterm_widget_new(1, 1, NOCTERM_WIDGET_FOCUSABLE_YES, NOCTERM_WIDGET_TYPE_VIRTUAL);
    nocterm_widget_flex(g_second_root, NOCTERM_WIDGET_FLEX_FILL_BOTH);
    nocterm_widget_set_key_handler(g_second_root, second_kh);

    g_second_body = nocterm_widget_new(1, 1, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_REAL);
    nocterm_widget_flex(g_second_body, NOCTERM_WIDGET_FLEX_FILL_BOTH);
    nocterm_widget_add_subwidget(g_second_root, g_second_body);

    render_second();
    return nocterm_page_new("Second", sizeof "Second", g_second_root);
}

static void build_overlay(void)
{
    g_overlay = nocterm_overlay_new();

    /* ── floating status HUD, anchored top-right, kept on top of every page ── */
    g_hud_body = nocterm_widget_new(1, 30, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_REAL);
    g_hud_box  = nocterm_decorbox_new(g_hud_body);
    nocterm_decorbox_set_border(g_hud_box,
        nocterm_decorbox_border_from_shape(NOCTERM_DECORBOX_BORDER_SHAPE_UNICODE_ROUND),
        a_fg(COL_ACCENT, true), a_fg(COL_ACCENT, true));
    nocterm_decorbox_set_label(g_hud_box, " status ", sizeof " status ", a_fg(COL_ACCENT, true), 2);
    nocterm_widget_align(NOCTERM_WIDGET(g_hud_box), NOCTERM_WIDGET_ALIGN_TOP);
    nocterm_widget_align(NOCTERM_WIDGET(g_hud_box), NOCTERM_WIDGET_ALIGN_RIGHT);
    nocterm_widget_align(NOCTERM_WIDGET(g_hud_box), NOCTERM_WIDGET_ALIGN_MARGIN_HORIZONTAL, 1);
    nocterm_overlay_add_widget(g_overlay, NOCTERM_WIDGET(g_hud_box));

    /* ── toast widgets, each anchored bottom-right at a fixed slot ── */
    for (int i = 0; i < MAX_TOASTS; i++) {
        g_toast_w[i] = nocterm_widget_new(1, TOAST_LEN, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_REAL);
        nocterm_widget_set_visible(g_toast_w[i], false);
        nocterm_widget_align(g_toast_w[i], NOCTERM_WIDGET_ALIGN_BOTTOM);
        nocterm_widget_align(g_toast_w[i], NOCTERM_WIDGET_ALIGN_RIGHT);
        nocterm_widget_align(g_toast_w[i], NOCTERM_WIDGET_ALIGN_MARGIN_VERTICAL, i + 1);
        nocterm_widget_align(g_toast_w[i], NOCTERM_WIDGET_ALIGN_MARGIN_HORIZONTAL, 2);
        nocterm_overlay_add_widget(g_overlay, g_toast_w[i]);
    }

    nocterm_overlay_set(g_overlay);

    /* the overlay's repaint timer lives on an overlay widget, so the page
     * stack never stops it — the HUD keeps ticking across page changes. */
    g_clock = nocterm_timer_create(g_hud_body, 100, clock_cb, NULL);
}

/* ════════════════════════════════════════════════════════════ MAIN ══ */

int main(void)
{
    setlocale(LC_ALL, "");

    g_home_page   = build_home();
    g_second_page = build_second();
    build_overlay();

    nocterm_page_stack_push(g_home_page);
    nocterm_timer_start(g_clock);

    nocterm_init();
    nocterm_loop();
    nocterm_end();

    nocterm_timer_delete_all();
    nocterm_overlay_unset();
    nocterm_overlay_delete(g_overlay);
    nocterm_page_delete(g_home_page);
    nocterm_page_delete(g_second_page);
    nocterm_decorbox_delete(g_hud_box);
    for (int i = 0; i < MAX_TOASTS; i++) nocterm_widget_delete(g_toast_w[i]);

    return 0;
}
