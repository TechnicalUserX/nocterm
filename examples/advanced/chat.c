/*
 * chat.c — nocterm advanced example: terminal chat over TCP sockets
 *
 * Two-party chat app.  Run one instance in host mode (bind + listen) and
 * another in client mode (connect).  Messages are newline-delimited over a
 * plain TCP stream.
 *
 * Compile from the repo root:
 *
 *   gcc -Werror -Wall \
 *       -I/home/tux/nocterm/include \
 *       -I/home/tux/nocterm/build/include \
 *       examples/advanced/chat.c \
 *       /home/tux/nocterm/build/lib/libnocterm.a \
 *       -lpthread -o chat
 */

#include <nocterm/nocterm.h>

#include <arpa/inet.h>
#include <errno.h>
#include <locale.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/* ── internal entry helpers (no visibility attribute in static-lib build) ─ */
int nocterm_entry_cursor_move_left(nocterm_entry_t *e);
int nocterm_entry_cursor_move_right(nocterm_entry_t *e);
int nocterm_entry_cursor_insert(nocterm_entry_t *e, nocterm_char_t ch);
int nocterm_entry_cursor_erase_right(nocterm_entry_t *e);
int nocterm_entry_cursor_erase_left(nocterm_entry_t *e);

/* ══════════════════════════════════════════════════════════════ STATE ══ */

#define CONN_IDLE      0
#define CONN_PENDING   1
#define CONN_CONNECTED 2
#define CONN_ERROR     3

#define MSG_CAP  128        /* receive ring-buffer capacity              */
#define MSG_MAX  512        /* max bytes per message (excluding newline) */

typedef enum { MODE_HOST, MODE_CLIENT } chat_mode_t;

static chat_mode_t  g_mode;
static atomic_int   g_conn_state;
static int          g_server_fd       = -1;
static int          g_conn_fd         = -1;
static pthread_t    g_net_thread;
static bool         g_net_thread_live = false;
static char         g_errmsg[256]     = {0};

/* receive ring-buffer: net thread writes, UI timer reads */
static char            g_rbuf[MSG_CAP][MSG_MAX];
static volatile int    g_rhead = 0, g_rtail = 0;
static pthread_mutex_t g_rmtx  = PTHREAD_MUTEX_INITIALIZER;

static void qpush(const char *s)
{
    pthread_mutex_lock(&g_rmtx);
    int nxt = (g_rtail + 1) % MSG_CAP;
    if (nxt != g_rhead) {
        strncpy(g_rbuf[g_rtail], s, MSG_MAX - 1);
        g_rbuf[g_rtail][MSG_MAX - 1] = '\0';
        g_rtail = nxt;
    }
    pthread_mutex_unlock(&g_rmtx);
}

static bool qpop(char *out)
{
    pthread_mutex_lock(&g_rmtx);
    if (g_rhead == g_rtail) { pthread_mutex_unlock(&g_rmtx); return false; }
    strncpy(out, g_rbuf[g_rhead], MSG_MAX - 1);
    out[MSG_MAX - 1] = '\0';
    g_rhead = (g_rhead + 1) % MSG_CAP;
    pthread_mutex_unlock(&g_rmtx);
    return true;
}

/* ═══════════════════════════════════════════════════════ WIDGET REFS ══ */

/* setup page entries */
static nocterm_entry_t *g_host_ip_e   = NULL;
static nocterm_entry_t *g_host_port_e = NULL;
static nocterm_entry_t *g_cli_ip_e    = NULL;
static nocterm_entry_t *g_cli_port_e  = NULL;

/* feedback labels on setup pages (80 chars wide so they can be updated) */
static nocterm_label_t *g_host_err    = NULL;
static nocterm_label_t *g_cli_err     = NULL;
static nocterm_label_t *g_cli_conn_st = NULL;   /* "Connecting..." status */

/* waiting page */
static nocterm_label_t      *g_wait_addr = NULL;
static nocterm_loadingbar_t *g_spinner   = NULL;

/* chat page */
static nocterm_listview_t *g_chat_list   = NULL;
static nocterm_entry_t    *g_msg_entry   = NULL;
static nocterm_label_t    *g_chat_status = NULL;

/* ════════════════════════════════════════════════════════ PAGE REFS ══ */

static nocterm_page_t *g_greeting_pg   = NULL;
static nocterm_page_t *g_host_setup_pg = NULL;
static nocterm_page_t *g_cli_setup_pg  = NULL;
static nocterm_page_t *g_waiting_pg    = NULL;
static nocterm_page_t *g_chat_pg       = NULL;

/* timers – started manually, auto-stopped by page pop */
static nocterm_timer_t *g_recv_timer = NULL;   /* chat page: drain queue    */
static nocterm_timer_t *g_wait_timer = NULL;   /* waiting page: poll accept */
static nocterm_timer_t *g_conn_timer = NULL;   /* cli setup: poll connect   */

/* ══════════════════════════════════════════════════════════ NETWORK ══ */

static void recv_loop(void)
{
    char line[MSG_MAX]; int pos = 0;
    for (;;) {
        char c; ssize_t n = read(g_conn_fd, &c, 1);
        if (n <= 0) {
            if (atomic_load(&g_conn_state) == CONN_CONNECTED) {
                snprintf(g_errmsg, sizeof g_errmsg, "Remote closed the connection.");
                atomic_store(&g_conn_state, CONN_ERROR);
            }
            return;
        }
        if (c == '\n') {
            if (pos > 0) { line[pos] = '\0'; qpush(line); }
            pos = 0;
        } else if (pos < MSG_MAX - 1) {
            line[pos++] = c;
        }
    }
}

static void *host_thread_fn(void *arg)
{
    (void)arg;
    struct sockaddr_in ca; socklen_t cl = sizeof ca;
    int fd = accept(g_server_fd, (struct sockaddr *)&ca, &cl);
    if (fd < 0) {
        if (atomic_load(&g_conn_state) == CONN_PENDING) {
            snprintf(g_errmsg, sizeof g_errmsg,
                     "accept(): %s", strerror(errno));
            atomic_store(&g_conn_state, CONN_ERROR);
        }
        return NULL;
    }
    g_conn_fd = fd;
    atomic_store(&g_conn_state, CONN_CONNECTED);
    recv_loop();
    return NULL;
}

typedef struct { char ip[64]; int port; } net_arg_t;
static net_arg_t g_narg;

static void *client_thread_fn(void *arg)
{
    net_arg_t *a = (net_arg_t *)arg;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        snprintf(g_errmsg, sizeof g_errmsg,
                 "socket(): %s", strerror(errno));
        atomic_store(&g_conn_state, CONN_ERROR);
        return NULL;
    }
    struct sockaddr_in sa = {0};
    sa.sin_family = AF_INET;
    sa.sin_port   = htons((uint16_t)a->port);
    if (inet_pton(AF_INET, a->ip, &sa.sin_addr) <= 0) {
        snprintf(g_errmsg, sizeof g_errmsg, "Bad address: %s", a->ip);
        close(fd); atomic_store(&g_conn_state, CONN_ERROR); return NULL;
    }
    if (connect(fd, (struct sockaddr *)&sa, sizeof sa) < 0) {
        snprintf(g_errmsg, sizeof g_errmsg,
                 "connect(): %s", strerror(errno));
        close(fd); atomic_store(&g_conn_state, CONN_ERROR); return NULL;
    }
    g_conn_fd = fd;
    atomic_store(&g_conn_state, CONN_CONNECTED);
    recv_loop();
    return NULL;
}

/* ════════════════════════════════════════════════════════ HELPERS ══ */

static void cleanup_conn(void)
{
    atomic_store(&g_conn_state, CONN_IDLE);
    /* shutdown() before close(): tears down the TCP connection so any
     * blocking read() or accept() in the network thread returns immediately.
     * close() alone does not interrupt a blocking syscall in another thread. */
    if (g_conn_fd >= 0) {
        shutdown(g_conn_fd, SHUT_RDWR);
        close(g_conn_fd);
        g_conn_fd = -1;
    }
    if (g_server_fd >= 0) {
        shutdown(g_server_fd, SHUT_RDWR);
        close(g_server_fd);
        g_server_fd = -1;
    }
    if (g_net_thread_live) {
        g_net_thread_live = false;
        pthread_join(g_net_thread, NULL);
    }
    char tmp[MSG_MAX];
    while (qpop(tmp)) {}
}

/* update the visible text of a pre-allocated label buffer */
static void label_update(nocterm_label_t *lbl, const char *text)
{
    nocterm_widget_clear(NOCTERM_WIDGET(lbl));
    nocterm_char_t ch[256];
    uint64_t len = nocterm_char_string_from_stream(
            ch, 256, text, strlen(text) + 1);
    nocterm_dimension_size_t w = NOCTERM_WIDGET(lbl)->bounds.width;
    for (uint64_t i = 0; i < len && i < w; i++)
        nocterm_widget_update(NOCTERM_WIDGET(lbl), 0, i, ch[i], lbl->attribute);
}

static void entry_clear(nocterm_entry_t *e)
{
    nocterm_widget_clear(NOCTERM_WIDGET(e));
    memset(e->text_store, 0, sizeof e->text_store);
    e->current_length  = 0;
    e->buffer_position = 0;
    e->cursor_position = 0;
    nocterm_widget_set_viewport(NOCTERM_WIDGET(e),
        (nocterm_dimension_t){0, 0, 1, NOCTERM_WIDGET(e)->viewport.width});
    nocterm_widget_update(NOCTERM_WIDGET(e), 0, 0,
        nocterm_char_from_ascii(' '), e->cursor_attribute);
}

static void chat_append(const char *prefix, const char *text,
                        nocterm_attribute_t attr)
{
    char line[MSG_MAX + 16];
    snprintf(line, sizeof line, "%s %s", prefix, text);
    nocterm_listview_item_t item = {0};
    nocterm_listview_item_constructor(&item, line, strlen(line) + 1, attr);
    nocterm_listview_push_back(g_chat_list, item);
}

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

static void do_send(void)
{
    if (atomic_load(&g_conn_state) != CONN_CONNECTED) return;
    char text[MSG_MAX] = {0}; uint64_t len = 0;
    nocterm_entry_get_text(g_msg_entry, text, sizeof text, &len);
    if (len == 0) return;
    char buf[MSG_MAX + 2];
    int  n = snprintf(buf, sizeof buf, "%s\n", text);
    if (n > 0 && g_conn_fd >= 0) write(g_conn_fd, buf, (size_t)n);
    chat_append("[You]", text, a_fg(2, true));
    entry_clear(g_msg_entry);
}

/* ══════════════════════════════════════════════════ KEY HANDLERS ══ */

/* Shared entry handler for setup pages: text editing only */
NOCTERM_WIDGET_KEY_HANDLER(setup_entry_kh)
{
    switch (nocterm_key_translate(key)) {
    case NOCTERM_KEY_EVENT_RIGHT:
        nocterm_entry_cursor_move_right(NOCTERM_ENTRY(self)); break;
    case NOCTERM_KEY_EVENT_LEFT:
        nocterm_entry_cursor_move_left(NOCTERM_ENTRY(self)); break;
    case NOCTERM_KEY_EVENT_PRINTABLE: {
        nocterm_char_t ch = {0};
        memcpy(ch.bytes, key->buffer, key->buffer_length);
        ch.bytes_size = key->buffer_length;
        ch.is_utf8    = (key->buffer_length > 1);
        nocterm_entry_cursor_insert(NOCTERM_ENTRY(self), ch); break;
    }
    case NOCTERM_KEY_EVENT_BACKSPACE:
        nocterm_entry_cursor_erase_left(NOCTERM_ENTRY(self)); break;
    case NOCTERM_KEY_EVENT_DELETE:
        nocterm_entry_cursor_erase_right(NOCTERM_ENTRY(self)); break;
    default: break;
    }
}

/* Chat entry handler: text editing + ENTER = send */
NOCTERM_WIDGET_KEY_HANDLER(chat_entry_kh)
{
    switch (nocterm_key_translate(key)) {
    case NOCTERM_KEY_EVENT_RIGHT:
        nocterm_entry_cursor_move_right(NOCTERM_ENTRY(self)); break;
    case NOCTERM_KEY_EVENT_LEFT:
        nocterm_entry_cursor_move_left(NOCTERM_ENTRY(self)); break;
    case NOCTERM_KEY_EVENT_PRINTABLE: {
        nocterm_char_t ch = {0};
        memcpy(ch.bytes, key->buffer, key->buffer_length);
        ch.bytes_size = key->buffer_length;
        ch.is_utf8    = (key->buffer_length > 1);
        nocterm_entry_cursor_insert(NOCTERM_ENTRY(self), ch); break;
    }
    case NOCTERM_KEY_EVENT_BACKSPACE:
        nocterm_entry_cursor_erase_left(NOCTERM_ENTRY(self)); break;
    case NOCTERM_KEY_EVENT_DELETE:
        nocterm_entry_cursor_erase_right(NOCTERM_ENTRY(self)); break;
    case NOCTERM_KEY_EVENT_ENTER:
        do_send(); break;
    default: break;
    }
}

/* ═══════════════════════════════════════════════ BUTTON HANDLERS ══ */

NOCTERM_BUTTON_ONPRESS_HANDLER(greet_host_press)
{
    (void)self; (void)user_data;
    g_mode = MODE_HOST;
    label_update(g_host_err, "");
    nocterm_page_stack_push(g_host_setup_pg);
}

NOCTERM_BUTTON_ONPRESS_HANDLER(greet_client_press)
{
    (void)self; (void)user_data;
    g_mode = MODE_CLIENT;
    label_update(g_cli_err, "");
    label_update(g_cli_conn_st, "");
    nocterm_page_stack_push(g_cli_setup_pg);
}

NOCTERM_BUTTON_ONPRESS_HANDLER(greet_quit_press)
{
    (void)self; (void)user_data;
    nocterm_page_stack_pop();
}

NOCTERM_BUTTON_ONPRESS_HANDLER(setup_back_press)
{
    (void)self; (void)user_data;
    nocterm_page_stack_pop();
}

NOCTERM_BUTTON_ONPRESS_HANDLER(disconnect_press)
{
    (void)self; (void)user_data;
    cleanup_conn();
    nocterm_page_stack_pop();
}

NOCTERM_BUTTON_ONPRESS_HANDLER(send_press)
{
    (void)self; (void)user_data;
    do_send();
}

NOCTERM_BUTTON_ONPRESS_HANDLER(cancel_wait_press)
{
    (void)self; (void)user_data;
    nocterm_loadingbar_disable(g_spinner);
    cleanup_conn();
    nocterm_page_stack_pop();
}

NOCTERM_BUTTON_ONPRESS_HANDLER(host_listen_press)
{
    (void)self; (void)user_data;
    char ip[64] = {0}, ps[16] = {0};
    uint64_t il = 0, pl = 0;
    nocterm_entry_get_text(g_host_ip_e,   ip, sizeof ip, &il);
    nocterm_entry_get_text(g_host_port_e, ps, sizeof ps, &pl);
    if (il == 0 || pl == 0) {
        label_update(g_host_err, " Please fill in both fields."); return;
    }
    int port = atoi(ps);
    if (port <= 0 || port > 65535) {
        label_update(g_host_err, " Invalid port (1-65535)."); return;
    }
    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) {
        char m[128]; snprintf(m, sizeof m, " socket(): %s", strerror(errno));
        label_update(g_host_err, m); return;
    }
    int one = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    struct sockaddr_in sa = {0};
    sa.sin_family = AF_INET; sa.sin_port = htons((uint16_t)port);
    if (inet_pton(AF_INET, ip, &sa.sin_addr) <= 0) {
        char m[128]; snprintf(m, sizeof m, " Bad address: %s", ip);
        label_update(g_host_err, m); close(sfd); return;
    }
    if (bind(sfd, (struct sockaddr *)&sa, sizeof sa) < 0) {
        char m[128]; snprintf(m, sizeof m, " bind(): %s", strerror(errno));
        label_update(g_host_err, m); close(sfd); return;
    }
    if (listen(sfd, 1) < 0) {
        char m[128]; snprintf(m, sizeof m, " listen(): %s", strerror(errno));
        label_update(g_host_err, m); close(sfd); return;
    }
    g_server_fd = sfd;
    atomic_store(&g_conn_state, CONN_PENDING);
    char wm[128]; snprintf(wm, sizeof wm, " Listening on %s : %d ...", ip, port);
    label_update(g_wait_addr, wm);
    nocterm_page_stack_push(g_waiting_pg);
    nocterm_loadingbar_enable(g_spinner);
    pthread_create(&g_net_thread, NULL, host_thread_fn, NULL);
    g_net_thread_live = true;
    nocterm_timer_start(g_wait_timer);
}

NOCTERM_BUTTON_ONPRESS_HANDLER(client_connect_press)
{
    (void)self; (void)user_data;
    if (atomic_load(&g_conn_state) == CONN_PENDING) return;
    char ip[64] = {0}, ps[16] = {0};
    uint64_t il = 0, pl = 0;
    nocterm_entry_get_text(g_cli_ip_e,   ip, sizeof ip, &il);
    nocterm_entry_get_text(g_cli_port_e, ps, sizeof ps, &pl);
    if (il == 0 || pl == 0) {
        label_update(g_cli_err, " Please fill in both fields."); return;
    }
    int port = atoi(ps);
    if (port <= 0 || port > 65535) {
        label_update(g_cli_err, " Invalid port (1-65535)."); return;
    }
    strncpy(g_narg.ip, ip, sizeof g_narg.ip - 1); g_narg.port = port;
    atomic_store(&g_conn_state, CONN_PENDING);
    label_update(g_cli_conn_st, " Connecting...");
    label_update(g_cli_err, "");
    pthread_create(&g_net_thread, NULL, client_thread_fn, &g_narg);
    g_net_thread_live = true;
    nocterm_timer_start(g_conn_timer);
}

/* ════════════════════════════════════════════ TIMER CALLBACKS ══ */

NOCTERM_TIMER_CALLBACK(wait_poll_cb)
{
    (void)widget; (void)user_data;
    int s = atomic_load(&g_conn_state);
    if (s == CONN_CONNECTED) {
        nocterm_loadingbar_disable(g_spinner);
        /* thread continues running recv_loop, keep g_net_thread_live = true */
        nocterm_page_stack_pop();     /* pop waiting   */
        nocterm_page_stack_pop();     /* pop host_setup */
        nocterm_page_stack_push(g_chat_pg);
        nocterm_timer_start(g_recv_timer);
        label_update(g_chat_status,
            " HOST MODE  |  Enter to send  |  Tab to Disconnect button");
    } else if (s == CONN_ERROR) {
        nocterm_loadingbar_disable(g_spinner);
        label_update(g_host_err, g_errmsg);
        atomic_store(&g_conn_state, CONN_IDLE);
        if (g_net_thread_live) {
            g_net_thread_live = false;
            pthread_join(g_net_thread, NULL);
        }
        nocterm_page_stack_pop();     /* pop waiting -> host_setup visible */
    }
}

NOCTERM_TIMER_CALLBACK(conn_poll_cb)
{
    (void)widget; (void)user_data;
    int s = atomic_load(&g_conn_state);
    if (s == CONN_CONNECTED) {
        /* thread continues running recv_loop, keep g_net_thread_live = true */
        label_update(g_cli_conn_st, "");
        label_update(g_cli_err, "");
        nocterm_page_stack_pop();     /* pop cli_setup */
        nocterm_page_stack_push(g_chat_pg);
        nocterm_timer_start(g_recv_timer);
        label_update(g_chat_status,
            " CLIENT MODE  |  Enter to send  |  Tab to Disconnect button");
    } else if (s == CONN_ERROR) {
        label_update(g_cli_err, g_errmsg);
        label_update(g_cli_conn_st, "");
        atomic_store(&g_conn_state, CONN_IDLE);
        if (g_net_thread_live) {
            g_net_thread_live = false;
            pthread_join(g_net_thread, NULL);
        }
        nocterm_timer_stop(g_conn_timer);
    }
}

NOCTERM_TIMER_CALLBACK(recv_poll_cb)
{
    (void)widget; (void)user_data;
    char msg[MSG_MAX];
    while (qpop(msg))
        chat_append("[Peer]", msg, a_fg(6, false));
    if (atomic_load(&g_conn_state) == CONN_ERROR) {
        chat_append("[--]", g_errmsg, a_fg(1, true));
        atomic_store(&g_conn_state, CONN_IDLE);
    }
}

/* ══════════════════════════════════════════════ PAGE BUILDERS ══ */

/* ── shared helpers ── */

static nocterm_decorbox_t *make_decorbox(nocterm_widget_t *inner,
                                          nocterm_attribute_t normal,
                                          nocterm_attribute_t focused,
                                          const char *lbl, int lbl_off)
{
    nocterm_decorbox_t *db = nocterm_decorbox_new(inner);
    nocterm_decorbox_set_border(db,
        nocterm_decorbox_border_from_shape(NOCTERM_DECORBOX_BORDER_SHAPE_UNICODE_ROUND),
        normal, focused);
    if (lbl)
        nocterm_decorbox_set_label(db, lbl, strlen(lbl) + 1,
                                   focused, (nocterm_dimension_size_t)lbl_off);
    return db;
}

/*
 * Add a button wrapped in a decorbox to parent, centered horizontally,
 * with its top edge at pct_v% of parent height from the top.
 */
static void add_btn(nocterm_widget_t *parent,
                    nocterm_button_t *btn,
                    nocterm_attribute_t bn, nocterm_attribute_t bf,
                    int pct_v)
{
    nocterm_button_set_attribute(btn, bn, bf);
    nocterm_decorbox_t *db = make_decorbox(NOCTERM_WIDGET(btn), bn, bf, NULL, 0);
    nocterm_widget_add_subwidget(parent, NOCTERM_WIDGET(db));
    nocterm_widget_align(NOCTERM_WIDGET(db), NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL);
    nocterm_widget_align(NOCTERM_WIDGET(db), NOCTERM_WIDGET_ALIGN_TOP);
    nocterm_widget_align(NOCTERM_WIDGET(db), NOCTERM_WIDGET_ALIGN_PERCENT_VERTICAL, (uint8_t)pct_v);
}

/* ─────────────────────────────── GREETING PAGE ─── */

static nocterm_page_t *build_greeting_page(void)
{
    nocterm_attribute_t bdr  = a_fg(4, false);
    nocterm_attribute_t ttl  = a_fg(3, true);
    nocterm_attribute_t sub  = a_fg(7, false);
    nocterm_attribute_t hint = a_fg(8, false);
    nocterm_attribute_t bn   = a_fg(7, false);
    nocterm_attribute_t bf   = a_fg_bg(0, 6, true);

    nocterm_widget_t *root = nocterm_widget_new(
        1, 1, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_VIRTUAL);
    nocterm_widget_flex(root, NOCTERM_WIDGET_FLEX_FILL_BOTH);

    nocterm_decorbox_t *outer = make_decorbox(root, bdr, bdr, " nocterm chat ", 2);
    nocterm_widget_flex(NOCTERM_WIDGET(outer), NOCTERM_WIDGET_FLEX_FILL_BOTH);

    /* title */
    nocterm_label_t *title = nocterm_label_new("Terminal Chat", 14);
    nocterm_label_set_attribute(title, ttl);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(title));
    nocterm_widget_align(NOCTERM_WIDGET(title), NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL);
    nocterm_widget_align(NOCTERM_WIDGET(title), NOCTERM_WIDGET_ALIGN_TOP);
    nocterm_widget_align(NOCTERM_WIDGET(title), NOCTERM_WIDGET_ALIGN_PERCENT_VERTICAL, 12);
    

    /* subtitle */
    nocterm_label_t *sub_lbl = nocterm_label_new(
        "Choose your role to begin a conversation", 41);
    nocterm_label_set_attribute(sub_lbl, sub);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(sub_lbl));
    nocterm_widget_align(NOCTERM_WIDGET(sub_lbl), NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL);
    nocterm_widget_align(NOCTERM_WIDGET(sub_lbl), NOCTERM_WIDGET_ALIGN_TOP);
    nocterm_widget_align(NOCTERM_WIDGET(sub_lbl), NOCTERM_WIDGET_ALIGN_PERCENT_VERTICAL, 22);

    /* Host Mode button */
    nocterm_button_t *host_btn = nocterm_button_new(1, 22, greet_host_press, NULL);
    nocterm_button_set_text(host_btn, "Host Mode", 10);
    add_btn(root, host_btn, bn, bf, 38);

    /* Client Mode button */
    nocterm_button_t *cli_btn = nocterm_button_new(1, 22, greet_client_press, NULL);
    nocterm_button_set_text(cli_btn, "Client Mode", 12);
    add_btn(root, cli_btn, bn, bf, 55);

    /* Quit button */
    nocterm_button_t *quit_btn = nocterm_button_new(1, 10, greet_quit_press, NULL);
    nocterm_button_set_text(quit_btn, "Quit", 5);
    add_btn(root, quit_btn, bn, bf, 72);

    /* hint */
    nocterm_label_t *hint_lbl = nocterm_label_new(
        "Tab  next field    Enter  select    ESC  quit", 46);
    nocterm_label_set_attribute(hint_lbl, hint);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(hint_lbl));
    nocterm_widget_align(NOCTERM_WIDGET(hint_lbl), NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL);
    nocterm_widget_align(NOCTERM_WIDGET(hint_lbl), NOCTERM_WIDGET_ALIGN_BOTTOM);
    nocterm_widget_align(NOCTERM_WIDGET(hint_lbl), NOCTERM_WIDGET_ALIGN_MARGIN_VERTICAL, 1);

    return nocterm_page_new("Greeting", 9, NOCTERM_WIDGET(outer));
}

/* ─────────────────────────── SHARED SETUP PAGE BUILDER ─── */

static nocterm_page_t *build_setup_page(
    const char *page_title,
    const char *mode_label,
    const char *ip_lbl_text,   const char *ip_default,
    const char *port_lbl_text, const char *port_default,
    const char *action_label,
    nocterm_button_onpress_handler_t action_press,
    nocterm_entry_t **ip_entry_ref,
    nocterm_entry_t **port_entry_ref,
    nocterm_label_t **err_ref,
    nocterm_label_t **conn_st_ref)
{
    nocterm_attribute_t bdr    = a_fg(4, false);
    nocterm_attribute_t bdr_f  = a_fg(6, true);
    nocterm_attribute_t ttl    = a_fg(3, true);
    nocterm_attribute_t lbl    = a_fg(7, false);
    nocterm_attribute_t en     = a_fg(7, false);
    nocterm_attribute_t ef     = a_fg(6, true);
    nocterm_attribute_t ec     = a_fg_bg(0, 6, false);
    nocterm_attribute_t bn     = a_fg(7, false);
    nocterm_attribute_t bf     = a_fg_bg(0, 6, true);
    nocterm_attribute_t err_a  = a_fg(1, true);
    nocterm_attribute_t hint_a = a_fg(8, false);

    nocterm_widget_t *root = nocterm_widget_new(
        1, 1, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_VIRTUAL);
    nocterm_widget_flex(root, NOCTERM_WIDGET_FLEX_FILL_BOTH);

    char pg_lbl[64]; snprintf(pg_lbl, sizeof pg_lbl, " %s ", mode_label);
    nocterm_decorbox_t *outer = make_decorbox(root, bdr, bdr, pg_lbl, 2);
    nocterm_widget_flex(NOCTERM_WIDGET(outer), NOCTERM_WIDGET_FLEX_FILL_BOTH);

    /* mode title */
    nocterm_label_t *title_lbl = nocterm_label_new(mode_label, strlen(mode_label) + 1);
    nocterm_label_set_attribute(title_lbl, ttl);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(title_lbl));
    nocterm_widget_align(NOCTERM_WIDGET(title_lbl), NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL);
    nocterm_widget_align(NOCTERM_WIDGET(title_lbl), NOCTERM_WIDGET_ALIGN_TOP);
    nocterm_widget_align(NOCTERM_WIDGET(title_lbl), NOCTERM_WIDGET_ALIGN_PERCENT_VERTICAL, 8);

    /* IP address label */
    nocterm_label_t *ip_lbl = nocterm_label_new(ip_lbl_text, strlen(ip_lbl_text) + 1);
    nocterm_label_set_attribute(ip_lbl, lbl);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(ip_lbl));
    nocterm_widget_align(NOCTERM_WIDGET(ip_lbl), NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL);
    nocterm_widget_align(NOCTERM_WIDGET(ip_lbl), NOCTERM_WIDGET_ALIGN_TOP);
    nocterm_widget_align(NOCTERM_WIDGET(ip_lbl), NOCTERM_WIDGET_ALIGN_PERCENT_VERTICAL, 23);

    /* IP entry */
    nocterm_entry_t *ip_e = nocterm_entry_new(28);
    nocterm_entry_set_attribute(ip_e, en, ec);
    nocterm_entry_set_text(ip_e, (char *)ip_default, strlen(ip_default) + 1);
    nocterm_widget_set_key_handler(NOCTERM_WIDGET(ip_e), setup_entry_kh);
    nocterm_decorbox_t *ip_db = make_decorbox(NOCTERM_WIDGET(ip_e), en, ef, NULL, 0);
    nocterm_widget_flex(NOCTERM_WIDGET(ip_db), NOCTERM_WIDGET_FLEX_PERCENT_HORIZONTAL, 45);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(ip_db));
    nocterm_widget_align(NOCTERM_WIDGET(ip_db), NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL);
    nocterm_widget_align(NOCTERM_WIDGET(ip_db), NOCTERM_WIDGET_ALIGN_TOP);
    nocterm_widget_align(NOCTERM_WIDGET(ip_db), NOCTERM_WIDGET_ALIGN_PERCENT_VERTICAL, 30);
    *ip_entry_ref = ip_e;

    /* port label */
    nocterm_label_t *port_lbl = nocterm_label_new(port_lbl_text, strlen(port_lbl_text) + 1);
    nocterm_label_set_attribute(port_lbl, lbl);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(port_lbl));
    nocterm_widget_align(NOCTERM_WIDGET(port_lbl), NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL);
    nocterm_widget_align(NOCTERM_WIDGET(port_lbl), NOCTERM_WIDGET_ALIGN_TOP);
    nocterm_widget_align(NOCTERM_WIDGET(port_lbl), NOCTERM_WIDGET_ALIGN_PERCENT_VERTICAL, 44);

    /* port entry */
    nocterm_entry_t *port_e = nocterm_entry_new(10);
    nocterm_entry_set_attribute(port_e, en, ec);
    nocterm_entry_set_text(port_e, (char *)port_default, strlen(port_default) + 1);
    nocterm_widget_set_key_handler(NOCTERM_WIDGET(port_e), setup_entry_kh);
    nocterm_decorbox_t *port_db = make_decorbox(NOCTERM_WIDGET(port_e), en, ef, NULL, 0);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(port_db));
    nocterm_widget_align(NOCTERM_WIDGET(port_db), NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL);
    nocterm_widget_align(NOCTERM_WIDGET(port_db), NOCTERM_WIDGET_ALIGN_TOP);
    nocterm_widget_align(NOCTERM_WIDGET(port_db), NOCTERM_WIDGET_ALIGN_PERCENT_VERTICAL, 51);
    *port_entry_ref = port_e;

    /* action button */
    nocterm_button_t *act_btn = nocterm_button_new(1, 22, action_press, NULL);
    nocterm_button_set_text(act_btn, action_label, strlen(action_label) + 1);
    add_btn(root, act_btn, bn, bf, 63);

    /* back button */
    nocterm_button_t *back_btn = nocterm_button_new(1, 10, setup_back_press, NULL);
    nocterm_button_set_text(back_btn, "Back", 5);
    add_btn(root, back_btn, bn, bf, 74);

    /* optional "Connecting..." status line (client only) */
    if (conn_st_ref) {
        char spaces[64]; memset(spaces, ' ', 63); spaces[63] = '\0';
        nocterm_label_t *cs = nocterm_label_new(spaces, 64);
        nocterm_label_set_attribute(cs, a_fg(3, true));
        nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(cs));
        nocterm_widget_align(NOCTERM_WIDGET(cs), NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL);
        nocterm_widget_align(NOCTERM_WIDGET(cs), NOCTERM_WIDGET_ALIGN_TOP);
        nocterm_widget_align(NOCTERM_WIDGET(cs), NOCTERM_WIDGET_ALIGN_PERCENT_VERTICAL, 83);
        *conn_st_ref = cs;
    }

    /* error label (80-wide, pre-allocated, cleared on entry) */
    char sp80[81]; memset(sp80, ' ', 80); sp80[80] = '\0';
    nocterm_label_t *err_lbl = nocterm_label_new(sp80, 81);
    nocterm_label_set_attribute(err_lbl, err_a);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(err_lbl));
    nocterm_widget_align(NOCTERM_WIDGET(err_lbl), NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL);
    nocterm_widget_align(NOCTERM_WIDGET(err_lbl), NOCTERM_WIDGET_ALIGN_BOTTOM);
    nocterm_widget_align(NOCTERM_WIDGET(err_lbl), NOCTERM_WIDGET_ALIGN_MARGIN_VERTICAL, 2);
    *err_ref = err_lbl;

    /* hint */
    nocterm_label_t *hint_lbl = nocterm_label_new(
        "Tab  next field    ESC  back", 29);
    nocterm_label_set_attribute(hint_lbl, hint_a);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(hint_lbl));
    nocterm_widget_align(NOCTERM_WIDGET(hint_lbl), NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL);
    nocterm_widget_align(NOCTERM_WIDGET(hint_lbl), NOCTERM_WIDGET_ALIGN_BOTTOM);
    nocterm_widget_align(NOCTERM_WIDGET(hint_lbl), NOCTERM_WIDGET_ALIGN_MARGIN_VERTICAL, 1);

    return nocterm_page_new(page_title, strlen(page_title) + 1,
                            NOCTERM_WIDGET(outer));
}

/* ─────────────────────────────── WAITING PAGE ─── */

static nocterm_page_t *build_waiting_page(void)
{
    nocterm_attribute_t bdr    = a_fg(4, false);
    nocterm_attribute_t addr_a = a_fg(6, false);
    nocterm_attribute_t bn     = a_fg(7, false);
    nocterm_attribute_t bf     = a_fg_bg(0, 1, true);
    nocterm_attribute_t hint_a = a_fg(8, false);

    nocterm_widget_t *root = nocterm_widget_new(
        1, 1, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_VIRTUAL);
    nocterm_widget_flex(root, NOCTERM_WIDGET_FLEX_FILL_BOTH);

    nocterm_decorbox_t *outer = make_decorbox(root, bdr, bdr, " Waiting ", 2);
    nocterm_widget_flex(NOCTERM_WIDGET(outer), NOCTERM_WIDGET_FLEX_FILL_BOTH);

    /* heading */
    nocterm_label_t *heading = nocterm_label_new(
        "Waiting for incoming connection", 32);
    nocterm_label_set_attribute(heading, a_fg(3, true));
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(heading));
    nocterm_widget_align(NOCTERM_WIDGET(heading), NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL);
    nocterm_widget_align(NOCTERM_WIDGET(heading), NOCTERM_WIDGET_ALIGN_TOP);
    nocterm_widget_align(NOCTERM_WIDGET(heading), NOCTERM_WIDGET_ALIGN_PERCENT_VERTICAL, 22);

    /* address label (filled before push) */
    char sp79[80]; memset(sp79, ' ', 79); sp79[79] = '\0';
    g_wait_addr = nocterm_label_new(sp79, 80);
    nocterm_label_set_attribute(g_wait_addr, addr_a);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(g_wait_addr));
    nocterm_widget_align(NOCTERM_WIDGET(g_wait_addr), NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL);
    nocterm_widget_align(NOCTERM_WIDGET(g_wait_addr), NOCTERM_WIDGET_ALIGN_TOP);
    nocterm_widget_align(NOCTERM_WIDGET(g_wait_addr), NOCTERM_WIDGET_ALIGN_PERCENT_VERTICAL, 33);

    /* spinner */
    g_spinner = nocterm_loadingbar_new(220);
    nocterm_loadingbar_set_attribute(g_spinner, a_fg(6, true));
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(g_spinner));
    nocterm_widget_align(NOCTERM_WIDGET(g_spinner), NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL);
    nocterm_widget_align(NOCTERM_WIDGET(g_spinner), NOCTERM_WIDGET_ALIGN_TOP);
    nocterm_widget_align(NOCTERM_WIDGET(g_spinner), NOCTERM_WIDGET_ALIGN_PERCENT_VERTICAL, 47);

    /* cancel button */
    nocterm_button_t *cancel_btn = nocterm_button_new(1, 14, cancel_wait_press, NULL);
    nocterm_button_set_text(cancel_btn, "Cancel", 7);
    add_btn(root, cancel_btn, bn, bf, 63);

    /* hint */
    nocterm_label_t *hint_lbl = nocterm_label_new(
        "Enter  cancel    ESC  cancel", 29);
    nocterm_label_set_attribute(hint_lbl, hint_a);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(hint_lbl));
    nocterm_widget_align(NOCTERM_WIDGET(hint_lbl), NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL);
    nocterm_widget_align(NOCTERM_WIDGET(hint_lbl), NOCTERM_WIDGET_ALIGN_BOTTOM);
    nocterm_widget_align(NOCTERM_WIDGET(hint_lbl), NOCTERM_WIDGET_ALIGN_MARGIN_VERTICAL, 1);

    /* wait_timer attached to root widget (auto-stopped on page pop) */
    g_wait_timer = nocterm_timer_create(root, 220, wait_poll_cb, NULL);

    return nocterm_page_new("Waiting", 8, NOCTERM_WIDGET(outer));
}

/* ─────────────────────────────────── CHAT PAGE ─── */

static nocterm_page_t *build_chat_page(void)
{
    nocterm_attribute_t bdr    = a_fg(4, false);
    nocterm_attribute_t bdr_f  = a_fg(6, true);
    nocterm_attribute_t st_a   = a_fg(7, false);
    nocterm_attribute_t en     = a_fg(7, false);
    nocterm_attribute_t ef     = a_fg(6, true);
    nocterm_attribute_t ec     = a_fg_bg(0, 6, false);
    nocterm_attribute_t bn     = a_fg(7, false);
    nocterm_attribute_t bf     = a_fg_bg(0, 2, true);

    nocterm_widget_t *root = nocterm_widget_new(
        1, 1, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_VIRTUAL);
    nocterm_widget_flex(root, NOCTERM_WIDGET_FLEX_FILL_BOTH);

    nocterm_decorbox_t *outer = make_decorbox(root, bdr, bdr, " Chat ", 2);
    nocterm_widget_flex(NOCTERM_WIDGET(outer), NOCTERM_WIDGET_FLEX_FILL_BOTH);

    /* status bar: top row, full width */
    char sp80[81]; memset(sp80, ' ', 80); sp80[80] = '\0';
    g_chat_status = nocterm_label_new(sp80, 81);
    nocterm_label_set_attribute(g_chat_status, st_a);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(g_chat_status));
    nocterm_widget_flex(NOCTERM_WIDGET(g_chat_status), NOCTERM_WIDGET_FLEX_FILL_HORIZONTAL);
    nocterm_widget_align(NOCTERM_WIDGET(g_chat_status), NOCTERM_WIDGET_ALIGN_TOP);

    /* messages listview: fills 78% of height, 1 row margin from top */
    g_chat_list = nocterm_listview_new(20, 2048, 120);
    nocterm_listview_set_autoscroll(g_chat_list, NOCTERM_LISTVIEW_AUTOSCROLL_DOWN);
    nocterm_decorbox_t *list_box = make_decorbox(
        NOCTERM_WIDGET(g_chat_list), bdr, bdr, " Messages ", 2);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(list_box));
    nocterm_widget_flex(NOCTERM_WIDGET(list_box),    NOCTERM_WIDGET_FLEX_FILL_HORIZONTAL);
    nocterm_widget_flex(NOCTERM_WIDGET(g_chat_list), NOCTERM_WIDGET_FLEX_FILL_HORIZONTAL);
    nocterm_widget_flex(NOCTERM_WIDGET(list_box),    NOCTERM_WIDGET_FLEX_PERCENT_VERTICAL, 78);
    nocterm_widget_align(NOCTERM_WIDGET(list_box), NOCTERM_WIDGET_ALIGN_TOP);
    nocterm_widget_align(NOCTERM_WIDGET(list_box), NOCTERM_WIDGET_ALIGN_MARGIN_VERTICAL, 1);

    /* message entry: bottom-left, 75% wide */
    g_msg_entry = nocterm_entry_new(64);
    nocterm_entry_set_attribute(g_msg_entry, en, ec);
    nocterm_widget_set_key_handler(NOCTERM_WIDGET(g_msg_entry), chat_entry_kh);
    nocterm_decorbox_t *entry_box = make_decorbox(
        NOCTERM_WIDGET(g_msg_entry), en, ef, " Message ", 2);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(entry_box));
    nocterm_widget_flex(NOCTERM_WIDGET(entry_box), NOCTERM_WIDGET_FLEX_PERCENT_HORIZONTAL, 75);
    nocterm_widget_align(NOCTERM_WIDGET(entry_box), NOCTERM_WIDGET_ALIGN_LEFT);
    nocterm_widget_align(NOCTERM_WIDGET(entry_box), NOCTERM_WIDGET_ALIGN_BOTTOM);

    /* send button: bottom, right of entry (at 75% from left), fixed width */
    nocterm_button_t *send_btn = nocterm_button_new(1, 8, send_press, NULL);
    nocterm_button_set_text(send_btn, "Send", 5);
    nocterm_button_set_attribute(send_btn, bn, bf);
    nocterm_decorbox_t *send_box = make_decorbox(
        NOCTERM_WIDGET(send_btn), bn, bf, NULL, 0);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(send_box));
    nocterm_widget_flex(NOCTERM_WIDGET(send_box), NOCTERM_WIDGET_FLEX_PERCENT_HORIZONTAL, 12);
    nocterm_widget_align(NOCTERM_WIDGET(send_box), NOCTERM_WIDGET_ALIGN_LEFT);
    nocterm_widget_align(NOCTERM_WIDGET(send_box), NOCTERM_WIDGET_ALIGN_PERCENT_HORIZONTAL, 75);
    nocterm_widget_align(NOCTERM_WIDGET(send_box), NOCTERM_WIDGET_ALIGN_BOTTOM);

    /* disconnect button: bottom-right, fixed width */
    nocterm_attribute_t db_n = a_fg(7, false);
    nocterm_attribute_t db_f = a_fg_bg(0, 1, true);
    nocterm_button_t *disc_btn = nocterm_button_new(1, 12, disconnect_press, NULL);
    nocterm_button_set_text(disc_btn, "Disconnect", 11);
    nocterm_button_set_attribute(disc_btn, db_n, db_f);
    nocterm_decorbox_t *disc_box = make_decorbox(
        NOCTERM_WIDGET(disc_btn), db_n, db_f, NULL, 0);
    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(disc_box));
    nocterm_widget_flex(NOCTERM_WIDGET(disc_box), NOCTERM_WIDGET_FLEX_PERCENT_HORIZONTAL, 11);
    nocterm_widget_align(NOCTERM_WIDGET(disc_box), NOCTERM_WIDGET_ALIGN_RIGHT);
    nocterm_widget_align(NOCTERM_WIDGET(disc_box), NOCTERM_WIDGET_ALIGN_BOTTOM);

    /* recv_timer attached to root widget (auto-stopped on page pop) */
    g_recv_timer = nocterm_timer_create(root, 100, recv_poll_cb, NULL);

    return nocterm_page_new("Chat", 5, NOCTERM_WIDGET(outer));
}

/* ══════════════════════════════════════════════════════════ MAIN ══ */

int main(void)
{
    setlocale(LC_ALL, "en_US.UTF-8");
    atomic_store(&g_conn_state, CONN_IDLE);

    g_greeting_pg = build_greeting_page();

    g_host_setup_pg = build_setup_page(
        "Host Setup",  "Host Mode",
        "Bind Address", "0.0.0.0",
        "Port",         "8080",
        "Start Listening", host_listen_press,
        &g_host_ip_e, &g_host_port_e,
        &g_host_err, NULL);

    g_cli_setup_pg = build_setup_page(
        "Client Setup", "Client Mode",
        "Remote Address", "127.0.0.1",
        "Port",           "8080",
        "Connect", client_connect_press,
        &g_cli_ip_e, &g_cli_port_e,
        &g_cli_err, &g_cli_conn_st);

    g_waiting_pg = build_waiting_page();
    g_chat_pg    = build_chat_page();

    /* conn_timer is attached to the cli_setup page root so it stops on pop */
    g_conn_timer = nocterm_timer_create(
        g_cli_setup_pg->root_widget, 200, conn_poll_cb, NULL);

    nocterm_mouse_set_support(NOCTERM_MOUSE_SUPPORT_ADVANCED);
    nocterm_page_stack_push(g_greeting_pg);

    nocterm_init();
    nocterm_loop();
    nocterm_end();

    cleanup_conn();

    nocterm_timer_delete_all();
    nocterm_page_delete(g_greeting_pg);
    nocterm_page_delete(g_host_setup_pg);
    nocterm_page_delete(g_cli_setup_pg);
    nocterm_page_delete(g_waiting_pg);
    nocterm_page_delete(g_chat_pg);

    return 0;
}
