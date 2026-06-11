/*
 * file_explorer.c — nocterm advanced example: a two-pane terminal file explorer
 *
 * A keyboard-driven file browser built entirely on the nocterm widget tree.
 * It demonstrates a number of the library's "advanced" facilities working
 * together:
 *
 *   - A custom focusable widget that owns the whole UI and receives every
 *     keystroke through a single key handler (no per-button focus dance).
 *   - Manual rendering into a real widget's cell buffer with per-cell colour,
 *     Unicode type glyphs and a highlighted selection bar (the listview widget
 *     is display-only and has no selection cursor, so we draw our own).
 *   - A flex layout (a full-width file-list decorbox pane between a one-row
 *     title bar and a one-row status bar) that re-flows on terminal resize,
 *     with a resize handler that recomputes the pane's vertical extent.
 *   - A repaint timer driven by a dirty flag, so the screen is only re-rendered
 *     when something actually changed.
 *   - A second page (the help overlay) pushed onto the page stack.
 *
 * Layout:
 *
 *   ┌ path bar ─────────────────────────────────────────────┐  (title, row 0)
 *   │ ╭ Files ──────────────────────────────────────────────╮│
 *   │ │   ↑ ..                                               ││
 *   │ │   ◆ src/                                             ││
 *   │ │ ▶ • report.txt                                       ││
 *   │ ╰──────────────────────────────────────────────────────╯│
 *   └ status / key hints ───────────────────────────────────┘  (status, last)
 *
 * Compile from the repo root:
 *
 *   gcc -Wall -Wextra \
 *       -I/home/tux/nocterm/include \
 *       -I/home/tux/nocterm/build/include \
 *       examples/advanced/file_explorer.c \
 *       /home/tux/nocterm/build/lib/libnocterm.a \
 *       -lpthread -o file_explorer
 *
 * Run it with no arguments (starts in the current directory) or pass a path:
 *
 *   ./file_explorer /etc
 */

#include <nocterm/nocterm.h>

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>

/* ════════════════════════════════════════════════════════════ MODEL ══ */

#define FE_NAME_MAX 512

typedef struct fe_entry_t {
    char    name[FE_NAME_MAX];
    bool    is_dir;   /* resolved through symlinks */
    bool    is_link;
    bool    is_exec;
    bool    stat_ok;
    off_t   size;
    mode_t  mode;
    time_t  mtime;
} fe_entry_t;

static fe_entry_t *g_entries     = NULL;   /* dynamic array of directory entries */
static uint64_t    g_count       = 0;
static uint64_t    g_capacity    = 0;
static int64_t     g_sel         = 0;      /* selected index                     */
static int64_t     g_top         = 0;      /* first visible index (scroll)       */
static bool        g_show_hidden = false;
static char        g_cwd[PATH_MAX] = {0};
static char        g_message[256]  = {0};  /* transient status message           */

static atomic_bool g_dirty = true;         /* set when a repaint is needed        */
static int         g_list_rows = 1;        /* visible rows in the list pane       */

/* ═══════════════════════════════════════════════════════ WIDGET REFS ══ */

static nocterm_widget_t *g_app      = NULL;  /* focusable virtual root            */
static nocterm_widget_t *g_list     = NULL;  /* file list (we render into this)   */
static nocterm_widget_t *g_title    = NULL;  /* top path bar                      */
static nocterm_widget_t *g_status   = NULL;  /* bottom hint bar                   */
static nocterm_decorbox_t *g_list_box    = NULL;

static nocterm_page_t *g_main_page = NULL;
static nocterm_page_t *g_help_page = NULL;
static nocterm_timer_t *g_paint_timer = NULL;

/* ════════════════════════════════════════════════════ ATTRIBUTE HELPERS ══ */

static nocterm_attribute_t a_fg(int color, bool bold)
{
    nocterm_attribute_t a = {0};
    a.color.ansi.fg       = true;
    a.color.ansi.codes.fg = (uint8_t)color;
    a.bold                = bold;
    return a;
}

static nocterm_attribute_t a_fg_bg(int fg, int bg, bool bold)
{
    nocterm_attribute_t a = {0};
    a.color.ansi.fg       = true;
    a.color.ansi.codes.fg = (uint8_t)fg;
    a.color.ansi.bg       = true;
    a.color.ansi.codes.bg = (uint8_t)bg;
    a.bold                = bold;
    return a;
}

/* ANSI palette indices we reuse */
#define COL_DIR     4   /* blue    */
#define COL_EXEC    2   /* green   */
#define COL_LINK    6   /* cyan    */
#define COL_PLAIN   7   /* white   */
#define COL_DIM     8   /* grey    */
#define COL_KEY     3   /* yellow  */
#define COL_ACCENT  6   /* cyan    */

/* ═══════════════════════════════════════════════════ LOW-LEVEL DRAWING ══ */

static int cell_w(nocterm_widget_t *w) { return (int)w->bounds.width;  }
static int cell_h(nocterm_widget_t *w) { return (int)w->bounds.height; }

static void cell_put(nocterm_widget_t *w, int row, int col,
                     nocterm_char_t ch, nocterm_attribute_t a)
{
    if (row >= 0 && col >= 0 && row < cell_h(w) && col < cell_w(w))
        nocterm_widget_update(w, (nocterm_dimension_size_t)row,
                                 (nocterm_dimension_size_t)col, ch, a);
}

/* Fill row [from .. width) with spaces of attribute a (paints a background). */
static void fill_row(nocterm_widget_t *w, int row, int from, nocterm_attribute_t a)
{
    for (int c = from; c < cell_w(w); c++)
        cell_put(w, row, c, nocterm_char_from_ascii(' '), a);
}

/* Draw a UTF-8 string at (row, col); returns the next free column. Clips. */
static int draw_str(nocterm_widget_t *w, int row, int col,
                    const char *s, nocterm_attribute_t a)
{
    nocterm_char_t buf[FE_NAME_MAX + 64];
    uint64_t n = nocterm_char_string_from_stream(
            buf, sizeof buf / sizeof buf[0], s, strlen(s) + 1);
    for (uint64_t i = 0; i < n; i++) {
        if (col >= cell_w(w)) break;
        cell_put(w, row, col++, buf[i], a);
    }
    return col;
}

/* Blank every cell of a widget with attribute a (so no stale cells linger). */
static void clear_widget(nocterm_widget_t *w, nocterm_attribute_t a)
{
    for (int r = 0; r < cell_h(w); r++)
        fill_row(w, r, 0, a);
}

/* ════════════════════════════════════════════════════ DIRECTORY MODEL ══ */

static int entry_cmp(const void *pa, const void *pb)
{
    const fe_entry_t *a = pa, *b = pb;
    if (strcmp(a->name, "..") == 0) return -1;
    if (strcmp(b->name, "..") == 0) return  1;
    if (a->is_dir != b->is_dir) return a->is_dir ? -1 : 1;
    return strcasecmp(a->name, b->name);
}

static void entries_push(const fe_entry_t *e)
{
    if (g_count == g_capacity) {
        uint64_t nc = g_capacity ? g_capacity * 2 : 64;
        fe_entry_t *n = realloc(g_entries, nc * sizeof *n);
        if (!n) return;
        g_entries  = n;
        g_capacity = nc;
    }
    g_entries[g_count++] = *e;
}

/* Read the current working directory into g_entries. Returns 0 on success. */
static int load_dir(void)
{
    DIR *d = opendir(".");
    if (!d) {
        snprintf(g_message, sizeof g_message,
                 "Cannot open directory: %s", strerror(errno));
        return -1;
    }

    g_count = 0;

    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (strcmp(de->d_name, ".") == 0) continue;
        if (de->d_name[0] == '.' &&
            strcmp(de->d_name, "..") != 0 && !g_show_hidden)
            continue;

        fe_entry_t e = {0};
        strncpy(e.name, de->d_name, FE_NAME_MAX - 1);

        struct stat st, lst;
        e.is_link = (lstat(de->d_name, &lst) == 0) && S_ISLNK(lst.st_mode);

        if (stat(de->d_name, &st) == 0) {       /* follows symlinks */
            e.stat_ok = true;
            e.mode    = st.st_mode;
            e.size    = st.st_size;
            e.mtime   = st.st_mtime;
            e.is_dir  = S_ISDIR(st.st_mode);
            e.is_exec = !e.is_dir && (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH));
        } else if (e.is_link) {                 /* broken symlink */
            e.stat_ok = true;
            e.mode    = lst.st_mode;
            e.size    = lst.st_size;
            e.mtime   = lst.st_mtime;
        }
        entries_push(&e);
    }
    closedir(d);

    qsort(g_entries, g_count, sizeof *g_entries, entry_cmp);

    if (!getcwd(g_cwd, sizeof g_cwd))
        strncpy(g_cwd, "?", sizeof g_cwd - 1);

    if (g_sel >= (int64_t)g_count) g_sel = (int64_t)g_count - 1;
    if (g_sel < 0)                 g_sel = 0;
    g_top = 0;
    return 0;
}

/* chdir + reload, reverting cleanly on failure. */
static bool change_dir(const char *path)
{
    char saved[PATH_MAX];
    if (!getcwd(saved, sizeof saved)) saved[0] = '\0';

    if (chdir(path) != 0) {
        snprintf(g_message, sizeof g_message,
                 "%s: %s", path, strerror(errno));
        return false;
    }
    if (load_dir() != 0) {
        if (saved[0]) { (void)!chdir(saved); load_dir(); }
        return false;
    }
    g_message[0] = '\0';
    return true;
}

static void enter_selected(void)
{
    if (g_count == 0) return;
    fe_entry_t *s = &g_entries[g_sel];
    if (!s->is_dir) {
        snprintf(g_message, sizeof g_message,
                 "'%.200s' is not a directory", s->name);
        return;
    }
    char name[FE_NAME_MAX];
    strncpy(name, s->name, sizeof name - 1);
    name[sizeof name - 1] = '\0';
    if (change_dir(name)) { g_sel = 0; g_top = 0; }
}

static void go_parent(void)
{
    /* remember where we came from so we can re-select it after going up */
    char base[FE_NAME_MAX] = {0};
    const char *slash = strrchr(g_cwd, '/');
    if (slash && slash[1]) strncpy(base, slash + 1, sizeof base - 1);

    if (!change_dir("..")) return;

    g_sel = 0; g_top = 0;
    if (base[0]) {
        for (uint64_t i = 0; i < g_count; i++)
            if (strcmp(g_entries[i].name, base) == 0) { g_sel = (int64_t)i; break; }
    }
}

static void select_move(int64_t delta)
{
    if (g_count == 0) return;
    g_sel += delta;
    if (g_sel < 0)                 g_sel = 0;
    if (g_sel >= (int64_t)g_count) g_sel = (int64_t)g_count - 1;
}

/* ════════════════════════════════════════════════════════ RENDERING ══ */

static void render_list(void)
{
    nocterm_widget_t *w = g_list;
    int H = cell_h(w), W = cell_w(w);
    if (H <= 0 || W <= 0) return;
    g_list_rows = H;

    /* keep the selection inside the visible window */
    if (g_sel < g_top)          g_top = g_sel;
    if (g_sel >= g_top + H)     g_top = g_sel - H + 1;
    if (g_top < 0)              g_top = 0;

    nocterm_attribute_t blank = {0};

    for (int r = 0; r < H; r++) {
        int64_t idx = g_top + r;
        bool selected = (idx == g_sel);

        if (idx >= (int64_t)g_count) {            /* empty row */
            fill_row(w, r, 0, blank);
            continue;
        }

        fe_entry_t *e = &g_entries[idx];
        bool is_parent = (strcmp(e->name, "..") == 0);

        int     color  = COL_PLAIN;
        bool    bold   = false;
        wchar_t glyph  = L'•';     /* type icon: regular file */
        wchar_t suffix = L'\0';
        if      (is_parent)  { color = COL_DIR;  bold = true; glyph = L'↑'; }
        else if (e->is_dir)  { color = COL_DIR;  bold = true; glyph = L'◆'; suffix = L'/'; }
        else if (e->is_link) { color = COL_LINK;              glyph = L'↪'; suffix = L'@'; }
        else if (e->is_exec) { color = COL_EXEC; bold = true; glyph = L'★'; suffix = L'*'; }
        if (e->name[0] == '.' && !is_parent)
            color = COL_DIM;                       /* dim hidden entries */

        nocterm_attribute_t attr = selected ? a_fg_bg(0, COL_ACCENT, true)
                                            : a_fg(color, bold);

        /* paint the whole row first (so the highlight spans full width) */
        fill_row(w, r, 0, attr);

        int col = 0;
        /* selection pointer, then the type glyph, then the name */
        cell_put(w, r, col++, nocterm_char_from_wchar(selected ? L'▶' : L' '), attr);
        cell_put(w, r, col++, nocterm_char_from_ascii(' '), attr);
        cell_put(w, r, col++, nocterm_char_from_wchar(glyph), attr);
        cell_put(w, r, col++, nocterm_char_from_ascii(' '), attr);

        col = draw_str(w, r, col, e->name, attr);
        if (suffix && col < cell_w(w))
            cell_put(w, r, col, nocterm_char_from_wchar(suffix), attr);
    }
}

static void render_bars(void)
{
    /* title / path bar */
    if (g_title && cell_w(g_title) > 0) {
        nocterm_attribute_t bar = a_fg_bg(0, COL_ACCENT, true);
        clear_widget(g_title, bar);
        char path[PATH_MAX + 32];
        snprintf(path, sizeof path, " nocterm explorer  ▸  %s", g_cwd);
        /* if the path is too long, keep the tail (most relevant part) */
        int avail = cell_w(g_title);
        if ((int)strlen(path) > avail && avail > 4) {
            const char *tail = path + strlen(path) - (avail - 4);
            char tmp[PATH_MAX + 32];
            snprintf(tmp, sizeof tmp, " ...%s", tail);
            draw_str(g_title, 0, 0, tmp, bar);
        } else {
            draw_str(g_title, 0, 0, path, bar);
        }
    }

    /* status / hint bar */
    if (g_status && cell_w(g_status) > 0) {
        nocterm_attribute_t st = a_fg_bg(7, 0, false);
        clear_widget(g_status, st);
        char info[sizeof g_message + 64];
        if (g_message[0]) {
            snprintf(info, sizeof info, " %s", g_message);
            draw_str(g_status, 0, 0, info, a_fg(1, true));
        } else {
            snprintf(info, sizeof info,
                     " %lld/%llu  •  hidden:%s   "
                     "↑↓ move   → open   ← up   . hidden   r refresh   ? help   q quit",
                     (long long)(g_count ? g_sel + 1 : 0),
                     (unsigned long long)g_count,
                     g_show_hidden ? "on" : "off");
            draw_str(g_status, 0, 0, info, st);
        }
    }
}

static void render_all(void)
{
    /* reflect the focused selection in the pane title */
    if (g_list_box)
        nocterm_decorbox_set_label(g_list_box, " Files ", sizeof " Files ",
                                   a_fg(COL_ACCENT, true), 2);
    render_list();
    render_bars();
}

static void mark_dirty(void) { atomic_store(&g_dirty, true); }

/* repaint timer: only touches the cell buffers when something changed */
NOCTERM_TIMER_CALLBACK(paint_cb)
{
    (void)widget; (void)user_data;
    if (atomic_exchange(&g_dirty, false))
        render_all();
}

/* ════════════════════════════════════════════════════ RESIZE HANDLER ══ */

/*
 * Called while the root widget is being flex-resized.  The list pane occupies
 * the vertical space between the one-row title bar and the one-row status bar,
 * so we recompute its height as a percentage of the (new) screen height before
 * the flex pass recurses into it.
 */
NOCTERM_WIDGET_RESIZE_HANDLER(app_resize)
{
    (void)self; (void)viewport;
    int H = (int)bounds.height;
    int desired = H - 3;                       /* title + gap + status */
    if (desired < 3) desired = 3;
    int pct = (desired * 100 + H / 2) / (H ? H : 1);
    if (pct < 10) pct = 10;
    if (pct > 95) pct = 95;

    if (g_list_box)
        nocterm_widget_flex(NOCTERM_WIDGET(g_list_box),
                            NOCTERM_WIDGET_FLEX_PERCENT_VERTICAL, pct);

    mark_dirty();
}

/* ════════════════════════════════════════════════════════ KEY INPUT ══ */

NOCTERM_WIDGET_KEY_HANDLER(app_kh)
{
    (void)self;
    g_message[0] = '\0';

    switch (nocterm_key_translate(key)) {
    case NOCTERM_KEY_EVENT_UP:        select_move(-1);            break;
    case NOCTERM_KEY_EVENT_DOWN:      select_move(+1);            break;
    case NOCTERM_KEY_EVENT_LEFT:      go_parent();                break;
    case NOCTERM_KEY_EVENT_BACKSPACE: go_parent();                break;
    case NOCTERM_KEY_EVENT_RIGHT:     enter_selected();           break;
    case NOCTERM_KEY_EVENT_ENTER:     enter_selected();           break;
    case NOCTERM_KEY_EVENT_BREAK:                                  /* Ctrl-C */
    case NOCTERM_KEY_EVENT_EOF:       nocterm_page_stack_pop();   break;
    case NOCTERM_KEY_EVENT_PRINTABLE: {
        switch (key->buffer[0]) {
        case 'k': select_move(-1);                       break;
        case 'j': select_move(+1);                       break;
        case 'h': go_parent();                           break;
        case 'l': enter_selected();                      break;
        case 'g': g_sel = 0;                             break;
        case 'G': g_sel = (int64_t)g_count - 1;          break;
        case 'u': select_move(-(g_list_rows / 2));       break;
        case 'd': select_move(+(g_list_rows / 2));       break;
        case '.': g_show_hidden = !g_show_hidden; load_dir(); break;
        case 'r': load_dir();                            break;
        case 'H': { const char *home = getenv("HOME");
                    if (home) { if (change_dir(home)) { g_sel = 0; g_top = 0; } } } break;
        case '?': nocterm_page_stack_push(g_help_page);  break;
        case 'q': nocterm_page_stack_pop();              break;
        default:  break;
        }
        break;
    }
    default: break;
    }

    if (g_sel < 0) g_sel = 0;
    if (g_count && g_sel >= (int64_t)g_count) g_sel = (int64_t)g_count - 1;
    mark_dirty();
}

/* ═══════════════════════════════════════════════════════ HELP PAGE ══ */

NOCTERM_WIDGET_RESIZE_HANDLER(help_resize)
{
    nocterm_widget_t *w = self;
    (void)bounds; (void)viewport;
    nocterm_attribute_t blank = {0};
    nocterm_attribute_t key   = a_fg(COL_KEY, true);
    nocterm_attribute_t txt   = a_fg(COL_PLAIN, false);

    static const struct { const char *k, *d; } rows[] = {
        { "Up / k",        "move selection up"            },
        { "Down / j",      "move selection down"          },
        { "u / d",         "half page up / down"          },
        { "g / G",         "jump to top / bottom"         },
        { "Enter / l / >", "open the selected directory"  },
        { "Bksp / h / <",  "go to the parent directory"   },
        { "H",             "go to your home directory"    },
        { ".",             "toggle hidden files"          },
        { "r",             "refresh the listing"          },
        { "?",             "show / hide this help"        },
        { "q / Esc",       "quit the explorer"            },
    };

    clear_widget(w, blank);
    draw_str(w, 0, 0, "Keyboard shortcuts", a_fg(COL_ACCENT, true));
    int row = 2;
    for (size_t i = 0; i < sizeof rows / sizeof rows[0]; i++, row++) {
        int c = draw_str(w, row, 0, rows[i].k, key);
        draw_str(w, row, (c < 18 ? 18 : c + 1), rows[i].d, txt);
    }
    row += 1;
    draw_str(w, row, 0, "Press q or Enter to return.", a_fg(COL_DIM, false));
}

NOCTERM_WIDGET_KEY_HANDLER(help_kh)
{
    (void)self;
    switch (nocterm_key_translate(key)) {
    case NOCTERM_KEY_EVENT_ENTER:
    case NOCTERM_KEY_EVENT_BACKSPACE:
        nocterm_page_stack_pop();
        break;
    case NOCTERM_KEY_EVENT_PRINTABLE:
        if (key->buffer[0] == 'q' || key->buffer[0] == '?')
            nocterm_page_stack_pop();
        break;
    default:
        break;
    }
    mark_dirty();
}

/* ═══════════════════════════════════════════════════════ PAGE BUILD ══ */

static nocterm_decorbox_t *make_pane(nocterm_widget_t *inner, const char *label)
{
    nocterm_attribute_t bdr = a_fg(COL_DIM, false);
    nocterm_decorbox_t *db = nocterm_decorbox_new(inner);
    nocterm_decorbox_set_border(db,
        nocterm_decorbox_border_from_shape(NOCTERM_DECORBOX_BORDER_SHAPE_UNICODE_ROUND),
        bdr, a_fg(COL_ACCENT, true));
    if (label)
        nocterm_decorbox_set_label(db, label, strlen(label) + 1,
                                   a_fg(COL_ACCENT, true), 2);
    return db;
}

static nocterm_page_t *build_main_page(void)
{
    /* focusable virtual root: owns every keystroke and the resize handler */
    g_app = nocterm_widget_new(1, 1, NOCTERM_WIDGET_FOCUSABLE_YES,
                               NOCTERM_WIDGET_TYPE_VIRTUAL);
    nocterm_widget_flex(g_app, NOCTERM_WIDGET_FLEX_FILL_BOTH);
    nocterm_widget_set_key_handler(g_app, app_kh);
    nocterm_widget_set_resize_handler(g_app, app_resize);

    /* title / path bar: full-width real widget, one row, top */
    g_title = nocterm_widget_new(1, 1, NOCTERM_WIDGET_FOCUSABLE_NO,
                                 NOCTERM_WIDGET_TYPE_REAL);
    nocterm_widget_flex(g_title, NOCTERM_WIDGET_FLEX_FILL_HORIZONTAL);
    nocterm_widget_add_subwidget(g_app, g_title);
    nocterm_widget_align(g_title, NOCTERM_WIDGET_ALIGN_TOP);
    nocterm_widget_align(g_title, NOCTERM_WIDGET_ALIGN_LEFT);

    /* file list: full-width pane between the title and status bars */
    g_list = nocterm_widget_new(1, 1, NOCTERM_WIDGET_FOCUSABLE_NO,
                                NOCTERM_WIDGET_TYPE_REAL);
    nocterm_widget_flex(g_list, NOCTERM_WIDGET_FLEX_FILL_BOTH);
    g_list_box = make_pane(g_list, " Files ");
    nocterm_widget_flex(NOCTERM_WIDGET(g_list_box),
                        NOCTERM_WIDGET_FLEX_PERCENT_HORIZONTAL, 100);
    nocterm_widget_flex(NOCTERM_WIDGET(g_list_box),
                        NOCTERM_WIDGET_FLEX_PERCENT_VERTICAL, 80);
    nocterm_widget_add_subwidget(g_app, NOCTERM_WIDGET(g_list_box));
    nocterm_widget_align(NOCTERM_WIDGET(g_list_box), NOCTERM_WIDGET_ALIGN_LEFT);
    nocterm_widget_align(NOCTERM_WIDGET(g_list_box), NOCTERM_WIDGET_ALIGN_TOP);
    nocterm_widget_align(NOCTERM_WIDGET(g_list_box),
                         NOCTERM_WIDGET_ALIGN_MARGIN_VERTICAL, 1);

    /* status / hint bar: full-width real widget, one row, bottom */
    g_status = nocterm_widget_new(1, 1, NOCTERM_WIDGET_FOCUSABLE_NO,
                                  NOCTERM_WIDGET_TYPE_REAL);
    nocterm_widget_flex(g_status, NOCTERM_WIDGET_FLEX_FILL_HORIZONTAL);
    nocterm_widget_add_subwidget(g_app, g_status);
    nocterm_widget_align(g_status, NOCTERM_WIDGET_ALIGN_BOTTOM);
    nocterm_widget_align(g_status, NOCTERM_WIDGET_ALIGN_LEFT);

    /* repaint timer lives on the root widget (auto-stopped on page pop) */
    g_paint_timer = nocterm_timer_create(g_app, 33, paint_cb, NULL);

    return nocterm_page_new("Explorer", sizeof "Explorer", g_app);
}

static nocterm_page_t *build_help_page(void)
{
    nocterm_widget_t *root = nocterm_widget_new(1, 1, NOCTERM_WIDGET_FOCUSABLE_YES,
                                                NOCTERM_WIDGET_TYPE_VIRTUAL);
    nocterm_widget_flex(root, NOCTERM_WIDGET_FLEX_FILL_BOTH);
    nocterm_widget_set_key_handler(root, help_kh);

    nocterm_widget_t *body = nocterm_widget_new(1, 1, NOCTERM_WIDGET_FOCUSABLE_NO,
                                                NOCTERM_WIDGET_TYPE_REAL);
    nocterm_widget_flex(body, NOCTERM_WIDGET_FLEX_FILL_BOTH);
    nocterm_widget_set_resize_handler(body, help_resize);

    nocterm_decorbox_t *box = make_pane(body, " Help ");
    nocterm_widget_flex(NOCTERM_WIDGET(box), NOCTERM_WIDGET_FLEX_FILL_BOTH);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(box));

    return nocterm_page_new("Help", sizeof "Help", root);
}

/* ══════════════════════════════════════════════════════════════ MAIN ══ */

int main(int argc, char **argv)
{
    setlocale(LC_ALL, "");

    if (argc > 1 && chdir(argv[1]) != 0) {
        fprintf(stderr, "file_explorer: %s: %s\n", argv[1], strerror(errno));
        return 1;
    }

    if (load_dir() != 0) {
        fprintf(stderr, "file_explorer: %s\n", g_message);
        return 1;
    }

    g_main_page = build_main_page();
    g_help_page = build_help_page();

    nocterm_page_stack_push(g_main_page);
    nocterm_timer_start(g_paint_timer);

    nocterm_init();
    nocterm_loop();
    nocterm_end();

    nocterm_timer_delete_all();
    nocterm_page_delete(g_main_page);
    nocterm_page_delete(g_help_page);
    free(g_entries);

    return 0;
}
