/*
 * video_player.c — nocterm advanced example: play an .mp4 in the terminal
 *
 * Yes, a video player in a TUI.  Decoded frames are rendered as truecolor
 * "pixels" into a nocterm *pixelgrid* widget (each character cell carries two
 * vertical pixels via the ▀/▄ half-block glyphs), and the passive *overlay*
 * layer is used for transient toast notifications — "Video started", "Paused",
 * "Video stopped", "Playback finished".
 *
 * Decoding is delegated to the ffmpeg toolchain (the same philosophy as the
 * music_player example):
 *
 *   - VIDEO: `ffmpeg -re` decodes the file to raw rgb24 frames, scaled and
 *     letter-boxed to the pixelgrid size, piped to us in realtime.  A reader
 *     thread pulls whole frames off the pipe into a shared latest-frame buffer;
 *     the render timer (on the nocterm loop thread, where widget access is
 *     safe) blits that buffer into the pixelgrid.
 *   - AUDIO: a parallel `ffplay -nodisp` process plays the soundtrack.  Both
 *     children run in their own process groups with stdio sent to /dev/null so
 *     they can never fight us for the terminal; pause/resume is SIGSTOP/SIGCONT
 *     on both groups, which keeps them roughly in sync.
 *
 * The overlay is used exactly as it is meant to be — a passive notification
 * layer floating above the page, never capturing input.
 *
 * Compile from the repo root:
 *
 *   gcc -Werror -Wall \
 *       -I/home/tux/nocterm/include \
 *       -I/home/tux/nocterm/build/include \
 *       examples/advanced/video_player.c \
 *       /home/tux/nocterm/build/lib/libnocterm.a \
 *       -lpthread -o video_player
 *
 * Run:  ./video_player path/to/clip.mp4
 *
 * Keys:  space pause/resume   p (re)start   s stop   q quit
 */

#include <nocterm/nocterm.h>

#include <fcntl.h>
#include <limits.h>
#include <locale.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* ════════════════════════════════════════════════════════════ STATE ══ */

typedef enum { VP_STOPPED, VP_PLAYING, VP_PAUSED } vp_state_t;

static char       g_path[PATH_MAX];
static char       g_name[256];
static vp_state_t g_state = VP_STOPPED;

static int        g_fps      = 24;
static double     g_duration = 0.0;          /* seconds, 0 if unknown        */

/* video frame geometry */
static int        g_pixw = 0, g_pixh = 0;    /* pixels (cols, rows)          */
static size_t     g_fsz  = 0;                /* bytes per frame = w*h*3       */

/* decode children */
static pid_t      g_vpid = -1;               /* ffmpeg (video)               */
static pid_t      g_apid = -1;               /* ffplay (audio)               */
static int        g_video_fd = -1;           /* read end of ffmpeg stdout    */

/* reader thread ↔ render timer hand-off */
static pthread_t        g_reader;
static bool             g_reader_running = false;
static pthread_mutex_t  g_frame_lock = PTHREAD_MUTEX_INITIALIZER;
static uint8_t         *g_frame_buf  = NULL; /* latest decoded frame         */
static uint8_t         *g_scratch    = NULL; /* render-side copy             */
static uint64_t         g_frame_seq  = 0;    /* bumped on each new frame      */
static uint64_t         g_last_seq   = 0;    /* last frame we rendered        */
static atomic_bool      g_reader_exit = false;
static atomic_bool      g_eof = false;

/* timing */
static uint64_t   g_base_ms  = 0;            /* play start, shifted on resume */
static uint64_t   g_pause_ms = 0;

/* ═══════════════════════════════════════════════════════ WIDGET REFS ══ */

static nocterm_widget_t   *g_root;
static nocterm_widget_t   *g_title;
static nocterm_widget_t   *g_status;
static nocterm_pixelgrid_t *g_grid;
static nocterm_decorbox_t  *g_grid_box;

static nocterm_page_t   *g_page;
static nocterm_timer_t  *g_timer;

/* overlay toast layer */
#define MAX_TOASTS 4
#define TOAST_LEN  36
typedef struct { char text[TOAST_LEN]; int color; double expires; bool active; } toast_t;
static toast_t           g_toasts[MAX_TOASTS];
static nocterm_overlay_t *g_overlay;
static nocterm_widget_t  *g_toast_w[MAX_TOASTS];

/* ════════════════════════════════════════════════════ SMALL HELPERS ══ */

static uint64_t now_ms(void)
{
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

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

#define COL_ACCENT 6
#define COL_KEY    3
#define COL_PLAIN  7
#define COL_DIM    8
#define COL_OK     2
#define COL_WARN   1

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
static void fmt_time(double s, char *out, size_t n)
{
    if (s < 0) s = 0;
    int t = (int)s;
    snprintf(out, n, "%02d:%02d", t / 60, t % 60);
}

/* ════════════════════════════════════════════════════════════ TOASTS ══ */

static void toast(const char *text, int color)
{
    int slot = -1;
    for (int i = 0; i < MAX_TOASTS; i++) if (!g_toasts[i].active) { slot = i; break; }
    if (slot < 0) {
        double oldest = 1e18;
        for (int i = 0; i < MAX_TOASTS; i++)
            if (g_toasts[i].expires < oldest) { oldest = g_toasts[i].expires; slot = i; }
    }
    snprintf(g_toasts[slot].text, TOAST_LEN, "%s", text);
    g_toasts[slot].color   = color;
    g_toasts[slot].expires = (double)time(NULL) + 2.5;
    g_toasts[slot].active  = true;
    nocterm_overlay_invalidate(g_overlay);
}

static void toasts_render(void)
{
    double now = (double)time(NULL);
    for (int i = 0; i < MAX_TOASTS; i++) {
        nocterm_widget_t *w = g_toast_w[i];
        if (g_toasts[i].active && now >= g_toasts[i].expires) g_toasts[i].active = false;

        bool want = g_toasts[i].active, have;
        nocterm_widget_get_visible(w, &have);
        if (want != have) { nocterm_widget_set_visible(w, want); nocterm_overlay_invalidate(g_overlay); }
        if (!want) continue;

        nocterm_attribute_t body = a_fg_bg(COL_PLAIN, 0, false);
        clear_w(w, body);
        for (int r = 0; r < ch(w); r++)
            put(w, r, 0, nocterm_char_from_wchar(L'▌'), a_fg(g_toasts[i].color, true));
        draw(w, 0, 2, g_toasts[i].text, a_fg(COL_PLAIN, true));
    }
}

/* ════════════════════════════════════════════════════ FFMPEG PROBING ══ */

static bool have_cmd(const char *name)
{
    char cmd[64];
    snprintf(cmd, sizeof cmd, "command -v %s >/dev/null 2>&1", name);
    return system(cmd) == 0;
}

static void probe_media(void)
{
    char cmd[PATH_MAX + 200];

    /* frame rate, e.g. "30000/1001" */
    snprintf(cmd, sizeof cmd,
        "ffprobe -v error -select_streams v:0 -show_entries stream=r_frame_rate "
        "-of default=nokey=1:noprint_wrappers=1 \"%s\" 2>/dev/null", g_path);
    FILE *f = popen(cmd, "r");
    if (f) {
        char buf[64] = {0};
        if (fgets(buf, sizeof buf, f)) {
            long num = 0, den = 1;
            if (sscanf(buf, "%ld/%ld", &num, &den) >= 1 && den != 0 && num > 0) {
                int fps = (int)((num + den / 2) / den);
                if (fps >= 1 && fps <= 60) g_fps = fps > 30 ? 30 : fps;
            }
        }
        pclose(f);
    }

    /* duration in seconds */
    snprintf(cmd, sizeof cmd,
        "ffprobe -v error -show_entries format=duration "
        "-of default=nokey=1:noprint_wrappers=1 \"%s\" 2>/dev/null", g_path);
    f = popen(cmd, "r");
    if (f) {
        char buf[64] = {0};
        if (fgets(buf, sizeof buf, f)) g_duration = atof(buf);
        pclose(f);
    }
}

/* ════════════════════════════════════════════════════ DECODE CONTROL ══ */

/* Reader thread: pull whole rgb24 frames off ffmpeg's pipe into g_frame_buf. */
static void *reader_main(void *arg)
{
    (void)arg;
    uint8_t *tmp = malloc(g_fsz);
    if (!tmp) return NULL;

    while (!atomic_load(&g_reader_exit)) {
        size_t got = 0;
        while (got < g_fsz) {
            ssize_t n = read(g_video_fd, tmp + got, g_fsz - got);
            if (n > 0)      { got += (size_t)n; }
            else            { atomic_store(&g_eof, true); free(tmp); return NULL; }
        }
        pthread_mutex_lock(&g_frame_lock);
        memcpy(g_frame_buf, tmp, g_fsz);
        g_frame_seq++;
        pthread_mutex_unlock(&g_frame_lock);
    }
    free(tmp);
    return NULL;
}

static void reap_children(void)
{
    if (g_vpid > 0) { kill(-g_vpid, SIGCONT); kill(-g_vpid, SIGTERM);
                      waitpid(g_vpid, NULL, 0); g_vpid = -1; }
    if (g_apid > 0) { kill(-g_apid, SIGCONT); kill(-g_apid, SIGTERM);
                      waitpid(g_apid, NULL, 0); g_apid = -1; }
}

static void video_stop(bool notify)
{
    if (g_state == VP_STOPPED && g_vpid < 0) return;

    atomic_store(&g_reader_exit, true);
    reap_children();                         /* closing the pipe unblocks read() */
    if (g_reader_running) { pthread_join(g_reader, NULL); g_reader_running = false; }
    if (g_video_fd >= 0)  { close(g_video_fd); g_video_fd = -1; }

    nocterm_pixelgrid_clear(g_grid);
    g_state = VP_STOPPED;
    if (notify) toast("Video stopped", COL_WARN);
}

static void spawn_audio(void)
{
    if (!have_cmd("ffplay")) return;
    pid_t pid = fork();
    if (pid < 0) return;
    if (pid == 0) {
        setpgid(0, 0);
        int dn = open("/dev/null", O_RDWR);
        if (dn >= 0) { dup2(dn, 0); dup2(dn, 1); dup2(dn, 2); if (dn > 2) close(dn); }
        execlp("ffplay", "ffplay", "-nodisp", "-vn", "-autoexit",
               "-loglevel", "quiet", "-i", g_path, (char *)NULL);
        _exit(127);
    }
    setpgid(pid, pid);
    g_apid = pid;
}

static void video_play(void)
{
    video_stop(false);

    int pfd[2];
    if (pipe(pfd) != 0) { toast("pipe() failed", COL_WARN); return; }

    char vf[256], r_fps[16];
    snprintf(vf, sizeof vf,
        "scale=%d:%d:force_original_aspect_ratio=decrease,"
        "pad=%d:%d:(ow-iw)/2:(oh-ih)/2:color=black",
        g_pixw, g_pixh, g_pixw, g_pixh);
    snprintf(r_fps, sizeof r_fps, "%d", g_fps);

    pid_t pid = fork();
    if (pid < 0) { close(pfd[0]); close(pfd[1]); toast("fork() failed", COL_WARN); return; }
    if (pid == 0) {
        setpgid(0, 0);
        dup2(pfd[1], STDOUT_FILENO);
        int dn = open("/dev/null", O_RDWR);
        if (dn >= 0) { dup2(dn, STDIN_FILENO); dup2(dn, STDERR_FILENO); if (dn > 2) close(dn); }
        close(pfd[0]); close(pfd[1]);
        execlp("ffmpeg", "ffmpeg", "-hide_banner", "-loglevel", "quiet", "-re",
               "-i", g_path, "-an", "-f", "rawvideo", "-pix_fmt", "rgb24",
               "-vf", vf, "-r", r_fps, "pipe:1", (char *)NULL);
        _exit(127);
    }
    setpgid(pid, pid);
    close(pfd[1]);
    g_vpid     = pid;
    g_video_fd = pfd[0];

    atomic_store(&g_reader_exit, false);
    atomic_store(&g_eof, false);
    g_frame_seq = g_last_seq = 0;
    if (pthread_create(&g_reader, NULL, reader_main, NULL) == 0) g_reader_running = true;

    spawn_audio();

    g_base_ms = now_ms();
    g_state   = VP_PLAYING;
    toast("Video started", COL_OK);
}

static void video_toggle_pause(void)
{
    if (g_state == VP_PLAYING) {
        if (g_vpid > 0) kill(-g_vpid, SIGSTOP);
        if (g_apid > 0) kill(-g_apid, SIGSTOP);
        g_pause_ms = now_ms();
        g_state = VP_PAUSED;
        toast("Paused", COL_KEY);
    } else if (g_state == VP_PAUSED) {
        if (g_vpid > 0) kill(-g_vpid, SIGCONT);
        if (g_apid > 0) kill(-g_apid, SIGCONT);
        g_base_ms += now_ms() - g_pause_ms;   /* don't count paused time */
        g_state = VP_PLAYING;
        toast("Resumed", COL_OK);
    }
}

static double elapsed_seconds(void)
{
    if (g_state == VP_STOPPED) return 0.0;
    uint64_t ref = (g_state == VP_PAUSED) ? g_pause_ms : now_ms();
    double e = (double)(ref - g_base_ms) / 1000.0;
    return e < 0 ? 0 : e;
}

/* ════════════════════════════════════════════════════════════ RENDER ══ */

static void blit_frame(const uint8_t *rgb)
{
    for (int y = 0; y < g_pixh; y++) {
        const uint8_t *row = rgb + (size_t)y * g_pixw * 3;
        for (int x = 0; x < g_pixw; x++) {
            const uint8_t *p = row + (size_t)x * 3;
            nocterm_pixelgrid_set_pixel(g_grid, (uint32_t)y, (uint16_t)x, p[0], p[1], p[2]);
        }
    }
}

static void render_bars(void)
{
    if (cw(g_title) > 0) {
        nocterm_attribute_t bar = a_fg_bg(0, COL_ACCENT, true);
        clear_w(g_title, bar);
        char line[320];
        snprintf(line, sizeof line, " ▶ nocterm video player  —  %.200s", g_name);
        draw(g_title, 0, 0, line, bar);
    }

    if (cw(g_status) > 0) {
        nocterm_attribute_t st = a_fg_bg(COL_PLAIN, 0, false);
        clear_w(g_status, st);
        const char *tag = g_state == VP_PLAYING ? "PLAYING"
                        : g_state == VP_PAUSED  ? "PAUSED " : "STOPPED";
        int tagcol = g_state == VP_PLAYING ? COL_OK
                   : g_state == VP_PAUSED  ? COL_KEY : COL_DIM;
        char cur[8], tot[8];
        fmt_time(elapsed_seconds(), cur, sizeof cur);
        fmt_time(g_duration, tot, sizeof tot);
        char info[160];
        snprintf(info, sizeof info, " %s/%s  %dfps   ", cur, tot, g_fps);
        int c = draw(g_status, 0, 1, tag, a_fg(tagcol, true));
        c = draw(g_status, 0, c, info, st);
        draw(g_status, 0, c, "[space] pause  [p] play  [s] stop  [q] quit", a_fg(COL_DIM, false));
    }
}

NOCTERM_TIMER_CALLBACK(tick_cb)
{
    (void)widget; (void)user_data;

    /* grab the freshest decoded frame, if any */
    bool have = false;
    pthread_mutex_lock(&g_frame_lock);
    if (g_frame_seq != g_last_seq) {
        memcpy(g_scratch, g_frame_buf, g_fsz);
        g_last_seq = g_frame_seq;
        have = true;
    }
    pthread_mutex_unlock(&g_frame_lock);

    if (have) blit_frame(g_scratch);

    /* end of stream: reader hit EOF and we've shown the last frame */
    if (g_state != VP_STOPPED && atomic_load(&g_eof) && !have &&
        g_frame_seq == g_last_seq) {
        video_stop(false);
        toast("Playback finished", COL_ACCENT);
    }

    toasts_render();
    render_bars();
}

/* ════════════════════════════════════════════════════════════ INPUT ══ */

NOCTERM_WIDGET_KEY_HANDLER(key_handler)
{
    (void)self;
    switch (nocterm_key_translate(key)) {
    case NOCTERM_KEY_EVENT_PRINTABLE:
        switch (key->buffer[0]) {
        case ' ': video_toggle_pause(); break;
        case 'p': video_play();         break;
        case 's': video_stop(true);     break;
        case 'q': video_stop(false); nocterm_page_stack_pop(); break;
        default: break;
        }
        break;
    default: break;
    }
}

/* ════════════════════════════════════════════════════════════ BUILD ══ */

static void compute_geometry(void)
{
    struct winsize ws = {0};
    int cols = 80, rows = 24;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col && ws.ws_row) {
        cols = ws.ws_col; rows = ws.ws_row;
    }
    int cell_w = cols - 2;             /* decorbox border    */
    int cell_h = rows - 2 - 2;         /* title+status, border */
    if (cell_w > 160) cell_w = 160;    /* cap the per-frame work */
    if (cell_h > 60)  cell_h = 60;
    if (cell_w < 16)  cell_w = 16;
    if (cell_h < 8)   cell_h = 8;

    g_pixw = cell_w;
    g_pixh = cell_h * 2;               /* two pixels per character row */
    g_fsz  = (size_t)g_pixw * g_pixh * 3;
}

static void build_page(void)
{
    g_root = nocterm_widget_new(1, 1, NOCTERM_WIDGET_FOCUSABLE_YES, NOCTERM_WIDGET_TYPE_VIRTUAL);
    nocterm_widget_flex(g_root, NOCTERM_WIDGET_FLEX_FILL_BOTH);
    nocterm_widget_set_key_handler(g_root, key_handler);

    g_title = nocterm_widget_new(1, 1, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_REAL);
    nocterm_widget_flex(g_title, NOCTERM_WIDGET_FLEX_FILL_HORIZONTAL);
    nocterm_widget_add_subwidget(g_root, g_title);
    nocterm_widget_align(g_title, NOCTERM_WIDGET_ALIGN_TOP);
    nocterm_widget_align(g_title, NOCTERM_WIDGET_ALIGN_LEFT);

    /* video surface: a fixed-size pixelgrid inside a decorbox, centered */
    g_grid     = nocterm_pixelgrid_new((uint32_t)g_pixh, (uint16_t)g_pixw);
    g_grid_box = nocterm_decorbox_new(NOCTERM_WIDGET(g_grid));
    nocterm_decorbox_set_border(g_grid_box,
        nocterm_decorbox_border_from_shape(NOCTERM_DECORBOX_BORDER_SHAPE_UNICODE_ROUND),
        a_fg(COL_DIM, false), a_fg(COL_ACCENT, true));
    nocterm_widget_add_subwidget(g_root, NOCTERM_WIDGET(g_grid_box));
    nocterm_widget_align(NOCTERM_WIDGET(g_grid_box), NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL);
    nocterm_widget_align(NOCTERM_WIDGET(g_grid_box), NOCTERM_WIDGET_ALIGN_PERCENT_HORIZONTAL, 50);
    nocterm_widget_align(NOCTERM_WIDGET(g_grid_box), NOCTERM_WIDGET_ALIGN_TOP);
    nocterm_widget_align(NOCTERM_WIDGET(g_grid_box), NOCTERM_WIDGET_ALIGN_MARGIN_VERTICAL, 1);

    g_status = nocterm_widget_new(1, 1, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_REAL);
    nocterm_widget_flex(g_status, NOCTERM_WIDGET_FLEX_FILL_HORIZONTAL);
    nocterm_widget_add_subwidget(g_root, g_status);
    nocterm_widget_align(g_status, NOCTERM_WIDGET_ALIGN_BOTTOM);
    nocterm_widget_align(g_status, NOCTERM_WIDGET_ALIGN_LEFT);

    g_timer = nocterm_timer_create(g_root, (uint64_t)(1000 / g_fps), tick_cb, NULL);

    g_page = nocterm_page_new("Video", sizeof "Video", g_root);
}

static void build_overlay(void)
{
    g_overlay = nocterm_overlay_new();
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
}

/* ════════════════════════════════════════════════════════════ MAIN ══ */

int main(int argc, char **argv)
{
    setlocale(LC_ALL, "");

    if (argc < 2) {
        fprintf(stderr, "usage: %s <file.mp4>\n", argv[0]);
        return 1;
    }
    if (access(argv[1], R_OK) != 0) {
        fprintf(stderr, "video_player: cannot read '%s'\n", argv[1]);
        return 1;
    }
    if (!have_cmd("ffmpeg") || !have_cmd("ffprobe")) {
        fprintf(stderr, "video_player: ffmpeg and ffprobe are required\n");
        return 1;
    }

    snprintf(g_path, sizeof g_path, "%s", argv[1]);
    const char *base = strrchr(argv[1], '/');
    snprintf(g_name, sizeof g_name, "%s", base ? base + 1 : argv[1]);

    probe_media();
    compute_geometry();

    g_frame_buf = malloc(g_fsz);
    g_scratch   = malloc(g_fsz);
    if (!g_frame_buf || !g_scratch) {
        fprintf(stderr, "video_player: out of memory\n");
        return 1;
    }

    build_page();
    build_overlay();

    nocterm_page_stack_push(g_page);
    nocterm_timer_start(g_timer);

    video_play();                        /* start immediately */

    nocterm_init();
    nocterm_loop();
    nocterm_end();

    video_stop(false);
    nocterm_timer_delete_all();
    nocterm_overlay_unset();
    nocterm_overlay_delete(g_overlay);
    nocterm_page_delete(g_page);
    nocterm_decorbox_delete(g_grid_box);
    nocterm_pixelgrid_delete(g_grid);
    for (int i = 0; i < MAX_TOASTS; i++) nocterm_widget_delete(g_toast_w[i]);
    free(g_frame_buf);
    free(g_scratch);

    return 0;
}
