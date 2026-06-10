/*
 * music_player.c — nocterm advanced example: a terminal music player
 *
 * Yes, a music player in a TUI.  Pick an audio file from a small built-in file
 * browser, then watch it play on a dashboard built entirely from nocterm
 * widgets: a progress *levelbar*, an animated block-character spectrum, a
 * volume levelbar, and a Unicode transport control strip (⏪ ⏯ ⏩ ⏹ ⏏).
 *
 * It shows a handful of the library's facilities working together:
 *
 *   - Two focusable "virtual root" pages (browser + player) that each own
 *     every keystroke through a single key handler — no per-widget focus dance.
 *   - The levelbar widget driven from a repaint timer to show playback progress
 *     (0..1000 per-mille) and the current volume.
 *   - Manual per-cell rendering into real widgets for the animated spectrum and
 *     the transport strip (colour gradients, sub-cell block glyphs).
 *   - A repaint timer plus a playback-poll that reaps the decoder process and
 *     keeps the elapsed-time clock honest across pause/resume/seek.
 *
 * Actual decoding is delegated to whatever command-line player is installed
 * (ffplay / cvlc / sox's play / paplay).  Track duration comes from ffprobe
 * when available.  The decoder is launched in its own process group with its
 * stdio sent to /dev/null so it can never fight us for the terminal; we
 * pause/resume it with SIGSTOP/SIGCONT and seek by relaunching at an offset.
 *
 *   ╭ ♪ now playing ───────────────────────────────────────────╮
 *   │            ▁▂▄▆█▆▄▂  (animated spectrum)                  │
 *   │   ████████████████████░░░░░░░░░░░░░░░░  (progress level)  │
 *   │                01:23 / 04:10                              │
 *   │            ⏪    ⏸    ⏩    ⏹    ⏏                          │
 *   │   ▶ Playing            Vol ███████░░  80%                 │
 *   ╰───────────────────────────────────────────────────────────╯
 *
 * Compile from the repo root:
 *
 *   gcc -Wall -Wextra \
 *       -I/home/tux/nocterm/include \
 *       -I/home/tux/nocterm/build/include \
 *       examples/advanced/music_player.c \
 *       /home/tux/nocterm/build/lib/libnocterm.a \
 *       -lpthread -lm -o music_player
 *
 * Run it in the current directory or pass a starting folder:
 *
 *   ./music_player ~/Music
 */

#include <nocterm/nocterm.h>

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

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

/* 256-colour foreground (used for the spectrum / progress gradients). */
static nocterm_attribute_t a256(uint8_t fg, bool bold)
{
    nocterm_attribute_t a = {0};
    a.color.c256.fg       = true;
    a.color.c256.codes.fg = fg;
    a.bold                = bold;
    return a;
}

/* ANSI palette indices we reuse */
#define COL_DIR     4   /* blue    */
#define COL_AUDIO   6   /* cyan    */
#define COL_PLAIN   7   /* white   */
#define COL_DIM     8   /* grey    */
#define COL_KEY     3   /* yellow  */
#define COL_ACCENT  6   /* cyan    */
#define COL_HOT     5   /* magenta */

/* ═══════════════════════════════════════════════════ LOW-LEVEL DRAWING ══ */

static int cell_w(nocterm_widget_t *w) { return w ? (int)w->bounds.width  : 0; }
static int cell_h(nocterm_widget_t *w) { return w ? (int)w->bounds.height : 0; }

static void cell_put(nocterm_widget_t *w, int row, int col,
                     nocterm_char_t ch, nocterm_attribute_t a)
{
    if (row >= 0 && col >= 0 && row < cell_h(w) && col < cell_w(w))
        nocterm_widget_update(w, (nocterm_dimension_size_t)row,
                                 (nocterm_dimension_size_t)col, ch, a);
}

static void fill_row(nocterm_widget_t *w, int row, int from, nocterm_attribute_t a)
{
    for (int c = from; c < cell_w(w); c++)
        cell_put(w, row, c, nocterm_char_from_ascii(' '), a);
}

static void clear_widget(nocterm_widget_t *w, nocterm_attribute_t a)
{
    for (int r = 0; r < cell_h(w); r++)
        fill_row(w, r, 0, a);
}

/* Draw a UTF-8 string at (row, col); returns the next free column. Clips. */
static int draw_str(nocterm_widget_t *w, int row, int col,
                    const char *s, nocterm_attribute_t a)
{
    nocterm_char_t buf[1024];
    uint64_t n = nocterm_char_string_from_stream(
            buf, sizeof buf / sizeof buf[0], s, strlen(s) + 1);
    for (uint64_t i = 0; i < n; i++) {
        if (col >= cell_w(w)) break;
        cell_put(w, row, col++, buf[i], a);
    }
    return col;
}

/* Update the visible text of a pre-sized label widget (single attribute). */
static void label_set(nocterm_label_t *lbl, const char *text)
{
    nocterm_widget_t *w = NOCTERM_WIDGET(lbl);
    nocterm_widget_clear(w);
    nocterm_char_t ch[256];
    uint64_t len = nocterm_char_string_from_stream(ch, 256, text, strlen(text) + 1);
    nocterm_dimension_size_t width = w->bounds.width;
    for (uint64_t i = 0; i < len && i < width; i++)
        nocterm_widget_update(w, 0, i, ch[i], lbl->attribute);
}

static nocterm_char_t wch(wchar_t c) { return nocterm_char_from_wchar(c); }

static void fmt_time(double seconds, char *out, size_t n)
{
    if (seconds < 0) seconds = 0;
    int s = (int)(seconds + 0.5);
    snprintf(out, n, "%02d:%02d", s / 60, s % 60);
}

static uint64_t now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
}

/* ════════════════════════════════════════════════════ AUDIO BACKEND ══ */

enum { PB_NONE, PB_FFPLAY, PB_CVLC, PB_PLAY, PB_PAPLAY };

static int   g_backend      = PB_NONE;
static bool  g_seek_ok      = false;   /* backend can start at an offset      */
static bool  g_vol_ok       = false;   /* backend honours our volume          */
static bool  g_have_ffprobe = false;

/* playback state (all touched only from the single UI thread) */
static pid_t  g_pid       = -1;
static bool   g_loaded    = false;     /* a track is selected / active        */
static bool   g_paused    = false;
static bool   g_finished  = false;     /* reached the natural end             */
static char   g_path[PATH_MAX] = {0};
static char   g_name[256]      = {0};
static double g_duration  = 0.0;       /* seconds, 0 == unknown               */
static uint64_t g_base_ms = 0;         /* monotonic ms mapped to position 0   */
static uint64_t g_pause_ms = 0;        /* monotonic ms when pause began       */
static int    g_volume    = 80;        /* 0..100                              */
static char   g_notice[128] = {0};     /* transient status line               */

static bool have_cmd(const char *name)
{
    char cmd[128];
    snprintf(cmd, sizeof cmd, "command -v %s >/dev/null 2>&1", name);
    return system(cmd) == 0;
}

static void audio_detect_backend(void)
{
    g_have_ffprobe = have_cmd("ffprobe");

    if      (have_cmd("ffplay")) { g_backend = PB_FFPLAY; g_seek_ok = true;  g_vol_ok = true;  }
    else if (have_cmd("cvlc"))   { g_backend = PB_CVLC;   g_seek_ok = true;  g_vol_ok = false; }
    else if (have_cmd("play"))   { g_backend = PB_PLAY;   g_seek_ok = true;  g_vol_ok = false; }
    else if (have_cmd("paplay")) { g_backend = PB_PAPLAY; g_seek_ok = false; g_vol_ok = false; }
    else                         { g_backend = PB_NONE; }
}

static const char *audio_backend_name(void)
{
    switch (g_backend) {
    case PB_FFPLAY: return "ffplay";
    case PB_CVLC:   return "cvlc";
    case PB_PLAY:   return "sox/play";
    case PB_PAPLAY: return "paplay";
    default:        return "none";
    }
}

/* Build a NULL-terminated argv for the chosen backend at a start offset. */
static void audio_build_argv(const char **argv, const char *path, double start)
{
    static char seekbuf[32], volbuf[16], trimbuf[32];
    int i = 0;

    switch (g_backend) {
    case PB_FFPLAY:
        argv[i++] = "ffplay"; argv[i++] = "-nodisp"; argv[i++] = "-autoexit";
        argv[i++] = "-loglevel"; argv[i++] = "quiet";
        snprintf(volbuf, sizeof volbuf, "%d", g_volume);
        argv[i++] = "-volume"; argv[i++] = volbuf;
        if (start > 0.5) {
            snprintf(seekbuf, sizeof seekbuf, "%.2f", start);
            argv[i++] = "-ss"; argv[i++] = seekbuf;
        }
        argv[i++] = path;
        break;
    case PB_CVLC:
        argv[i++] = "cvlc"; argv[i++] = "--intf"; argv[i++] = "dummy";
        argv[i++] = "--play-and-exit"; argv[i++] = "--quiet";
        if (start > 0.5) {
            snprintf(seekbuf, sizeof seekbuf, "%.0f", start);
            argv[i++] = "--start-time"; argv[i++] = seekbuf;
        }
        argv[i++] = path;
        break;
    case PB_PLAY:
        argv[i++] = "play"; argv[i++] = "-q"; argv[i++] = path;
        if (start > 0.5) {                       /* sox: trim goes after file */
            snprintf(trimbuf, sizeof trimbuf, "%.2f", start);
            argv[i++] = "trim"; argv[i++] = trimbuf;
        }
        break;
    case PB_PAPLAY:
        argv[i++] = "paplay"; argv[i++] = path;
        break;
    default: break;
    }
    argv[i] = NULL;
}

/* Spawn the decoder for g_path starting at `start` seconds. */
static void audio_spawn(double start)
{
    const char *argv[16];
    audio_build_argv(argv, g_path, start);
    if (!argv[0]) return;

    pid_t pid = fork();
    if (pid < 0) {
        snprintf(g_notice, sizeof g_notice, "fork failed: %s", strerror(errno));
        return;
    }
    if (pid == 0) {
        /* child: detach into our own process group and mute all stdio so the
         * decoder can never read our terminal or scribble on the screen. */
        setpgid(0, 0);
        int devnull = open("/dev/null", O_RDWR);
        if (devnull >= 0) {
            dup2(devnull, STDIN_FILENO);
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            if (devnull > 2) close(devnull);
        }
        execvp(argv[0], (char *const *)argv);
        _exit(127);
    }
    setpgid(pid, pid);                 /* also set in parent to avoid the race */
    g_pid     = pid;
    g_paused  = false;
    g_base_ms = now_ms() - (uint64_t)(start * 1000.0);
}

/* Kill the decoder (if any) but leave the logical track-loaded state alone. */
static void audio_kill_child(void)
{
    if (g_pid > 0) {
        kill(-g_pid, SIGCONT);         /* in case it was paused (stopped)     */
        kill(-g_pid, SIGTERM);
        int status;
        waitpid(g_pid, &status, 0);
        g_pid = -1;
    }
}

/* Seconds of audio elapsed, honouring the current pause state. */
static double audio_elapsed(void)
{
    if (!g_loaded) return 0.0;
    uint64_t ref = g_paused ? g_pause_ms : now_ms();
    double e = (double)(ref - g_base_ms) / 1000.0;
    if (e < 0) e = 0;
    if (g_duration > 0 && e > g_duration) e = g_duration;
    return e;
}

static double audio_probe_duration(const char *path)
{
    if (!g_have_ffprobe) return 0.0;
    char cmd[PATH_MAX + 160];
    snprintf(cmd, sizeof cmd,
             "ffprobe -v error -show_entries format=duration "
             "-of default=nokey=1:noprint_wrappers=1 \"%s\" 2>/dev/null", path);
    FILE *f = popen(cmd, "r");
    if (!f) return 0.0;
    char buf[64] = {0};
    double d = 0.0;
    if (fgets(buf, sizeof buf, f)) d = atof(buf);
    pclose(f);
    return d > 0 ? d : 0.0;
}

static void audio_play(const char *path, const char *display_name)
{
    audio_kill_child();
    strncpy(g_path, path, sizeof g_path - 1);
    g_path[sizeof g_path - 1] = '\0';
    strncpy(g_name, display_name, sizeof g_name - 1);
    g_name[sizeof g_name - 1] = '\0';

    g_duration = audio_probe_duration(path);
    g_loaded   = true;
    g_paused   = false;
    g_finished = false;
    g_notice[0] = '\0';
    audio_spawn(0.0);
}

static void audio_stop(void)
{
    audio_kill_child();
    g_loaded   = false;
    g_paused   = false;
    g_finished = false;
}

static void audio_toggle_pause(void)
{
    if (!g_loaded || g_pid <= 0) return;
    if (g_finished) {                          /* restart a finished track    */
        g_finished = false;
        audio_spawn(0.0);
        return;
    }
    if (g_paused) {
        kill(-g_pid, SIGCONT);
        g_base_ms += now_ms() - g_pause_ms;    /* discount paused interval    */
        g_paused = false;
    } else {
        kill(-g_pid, SIGSTOP);
        g_pause_ms = now_ms();
        g_paused = true;
    }
}

static void audio_seek(double delta)
{
    if (!g_loaded) return;
    if (!g_seek_ok) {
        snprintf(g_notice, sizeof g_notice,
                 "%s cannot seek", audio_backend_name());
        return;
    }
    double target = audio_elapsed() + delta;
    if (target < 0) target = 0;
    if (g_duration > 0 && target > g_duration - 1) target = g_duration - 1;
    if (target < 0) target = 0;
    audio_kill_child();
    g_finished = false;
    audio_spawn(target);
}

static void audio_restart(void)
{
    if (!g_loaded) return;
    audio_kill_child();
    g_finished = false;
    audio_spawn(0.0);
}

static void audio_change_volume(int delta)
{
    int v = g_volume + delta;
    if (v < 0)   v = 0;
    if (v > 100) v = 100;
    if (v == g_volume) return;
    g_volume = v;
    if (!g_vol_ok) {
        snprintf(g_notice, sizeof g_notice,
                 "%s ignores volume changes", audio_backend_name());
        return;
    }
    if (g_loaded && g_pid > 0 && !g_finished) {   /* relaunch at same offset  */
        double at = audio_elapsed();
        bool was_paused = g_paused;
        audio_kill_child();
        audio_spawn(at);
        if (was_paused) {                          /* preserve paused state   */
            kill(-g_pid, SIGSTOP);
            g_pause_ms = now_ms();
            g_paused = true;
        }
    }
}

/* Reap the decoder if it exited on its own; detect a natural end-of-track. */
static void audio_poll(void)
{
    if (g_pid <= 0) return;
    int status;
    pid_t r = waitpid(g_pid, &status, WNOHANG);
    if (r == g_pid) {
        g_pid = -1;
        if (!g_paused) {
            g_finished = true;
            g_paused   = false;
        }
    }
}

/* ══════════════════════════════════════════════ FILE-BROWSER MODEL ══ */

#define FE_NAME_MAX 512

typedef struct { char name[FE_NAME_MAX]; bool is_dir; } fb_entry_t;

static fb_entry_t *g_entries  = NULL;
static uint64_t    g_count    = 0;
static uint64_t    g_capacity = 0;
static int64_t     g_sel      = 0;
static int64_t     g_top      = 0;
static char        g_cwd[PATH_MAX] = {0};
static int         g_list_rows = 1;

static bool is_audio_file(const char *name)
{
    const char *ext = strrchr(name, '.');
    if (!ext) return false;
    static const char *exts[] = {
        ".mp3", ".flac", ".wav", ".ogg", ".oga", ".opus",
        ".m4a", ".aac", ".wma", ".mp2", ".aiff", NULL
    };
    for (int i = 0; exts[i]; i++)
        if (strcasecmp(ext, exts[i]) == 0) return true;
    return false;
}

static void entries_push(const fb_entry_t *e)
{
    if (g_count == g_capacity) {
        uint64_t nc = g_capacity ? g_capacity * 2 : 64;
        fb_entry_t *n = realloc(g_entries, nc * sizeof *n);
        if (!n) return;
        g_entries = n; g_capacity = nc;
    }
    g_entries[g_count++] = *e;
}

static int entry_cmp(const void *pa, const void *pb)
{
    const fb_entry_t *a = pa, *b = pb;
    if (strcmp(a->name, "..") == 0) return -1;
    if (strcmp(b->name, "..") == 0) return  1;
    if (a->is_dir != b->is_dir) return a->is_dir ? -1 : 1;
    return strcasecmp(a->name, b->name);
}

/* Read the current directory, keeping directories and audio files only. */
static void load_dir(void)
{
    DIR *d = opendir(".");
    g_count = 0;
    if (!d) {
        if (!getcwd(g_cwd, sizeof g_cwd)) strncpy(g_cwd, "?", sizeof g_cwd - 1);
        return;
    }
    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (strcmp(de->d_name, ".") == 0) continue;
        if (de->d_name[0] == '.' && strcmp(de->d_name, "..") != 0) continue;

        struct stat st;
        bool is_dir = (stat(de->d_name, &st) == 0) && S_ISDIR(st.st_mode);
        if (!is_dir && !is_audio_file(de->d_name)) continue;

        fb_entry_t e = {0};
        strncpy(e.name, de->d_name, FE_NAME_MAX - 1);
        e.is_dir = is_dir;
        entries_push(&e);
    }
    closedir(d);
    qsort(g_entries, g_count, sizeof *g_entries, entry_cmp);
    if (!getcwd(g_cwd, sizeof g_cwd)) strncpy(g_cwd, "?", sizeof g_cwd - 1);
    if (g_sel >= (int64_t)g_count) g_sel = (int64_t)g_count - 1;
    if (g_sel < 0) g_sel = 0;
    g_top = 0;
}

static void browser_change_dir(const char *path)
{
    if (chdir(path) == 0) { g_sel = 0; g_top = 0; load_dir(); }
}

static void browser_go_parent(void)
{
    char base[FE_NAME_MAX] = {0};
    const char *slash = strrchr(g_cwd, '/');
    if (slash && slash[1]) strncpy(base, slash + 1, sizeof base - 1);
    if (chdir("..") != 0) return;
    g_sel = 0; g_top = 0; load_dir();
    for (uint64_t i = 0; i < g_count; i++)
        if (strcmp(g_entries[i].name, base) == 0) { g_sel = (int64_t)i; break; }
}

/* ══════════════════════════════════════════════════════ WIDGET REFS ══ */

/* browser page */
static nocterm_widget_t *g_browser_root = NULL;
static nocterm_widget_t *g_browser_list = NULL;   /* we render into this      */
static nocterm_widget_t *g_browser_bar  = NULL;   /* top path bar             */
static nocterm_widget_t *g_browser_hint = NULL;   /* bottom hint bar          */
static nocterm_decorbox_t *g_browser_box = NULL;
static nocterm_page_t   *g_browser_page = NULL;
static nocterm_timer_t  *g_browser_timer = NULL;

/* player page */
static nocterm_widget_t *g_player_root = NULL;
static nocterm_label_t  *g_np_label    = NULL;    /* track name               */
static nocterm_widget_t *g_spectrum    = NULL;    /* animated bars            */
static nocterm_levelbar_t *g_progress  = NULL;    /* playback progress        */
static nocterm_label_t  *g_time_label  = NULL;    /* mm:ss / mm:ss            */
static nocterm_widget_t *g_transport   = NULL;    /* ⏪ ⏯ ⏩ ⏹ ⏏ strip        */
static nocterm_widget_t *g_statusrow   = NULL;    /* state + volume           */
static nocterm_levelbar_t *g_volbar    = NULL;
static nocterm_page_t   *g_player_page = NULL;
static nocterm_timer_t  *g_player_timer = NULL;

#define PROGRESS_RES 1000          /* per-mille resolution for the levelbar    */
#define VOL_LEN      10            /* cells in the volume levelbar             */

/* ════════════════════════════════════════════════════ BROWSER RENDER ══ */

static void browser_select_move(int64_t delta)
{
    if (g_count == 0) return;
    g_sel += delta;
    if (g_sel < 0) g_sel = 0;
    if (g_sel >= (int64_t)g_count) g_sel = (int64_t)g_count - 1;
}

static void browser_render(void)
{
    /* path bar */
    if (cell_w(g_browser_bar) > 0) {
        nocterm_attribute_t bar = a_fg_bg(0, COL_ACCENT, true);
        clear_widget(g_browser_bar, bar);
        char path[PATH_MAX + 32];
        snprintf(path, sizeof path, " ♪ choose a track   %s", g_cwd);
        int avail = cell_w(g_browser_bar);
        if ((int)strlen(path) > avail && avail > 4) {
            char tmp[PATH_MAX + 32];
            snprintf(tmp, sizeof tmp, " ...%s",
                     path + strlen(path) - (avail - 4));
            draw_str(g_browser_bar, 0, 0, tmp, bar);
        } else {
            draw_str(g_browser_bar, 0, 0, path, bar);
        }
    }

    /* file list */
    nocterm_widget_t *w = g_browser_list;
    int H = cell_h(w), W = cell_w(w);
    if (H > 0 && W > 0) {
        g_list_rows = H;
        if (g_sel < g_top)      g_top = g_sel;
        if (g_sel >= g_top + H) g_top = g_sel - H + 1;
        if (g_top < 0)          g_top = 0;

        nocterm_attribute_t blank = {0};
        for (int r = 0; r < H; r++) {
            int64_t idx = g_top + r;
            if (idx >= (int64_t)g_count) { fill_row(w, r, 0, blank); continue; }

            fb_entry_t *e = &g_entries[idx];
            bool selected = (idx == g_sel);
            int  color = e->is_dir ? COL_DIR : COL_AUDIO;

            nocterm_attribute_t attr = selected ? a_fg_bg(0, COL_ACCENT, true)
                                                : a_fg(color, e->is_dir);
            fill_row(w, r, 0, attr);

            int col = 0;
            cell_put(w, r, col++, nocterm_char_from_ascii(' '), attr);
            /* a little glyph: 📁-ish folder vs ♪ note */
            cell_put(w, r, col++, e->is_dir ? wch(L'▸') : wch(L'♪'), attr);
            cell_put(w, r, col++, nocterm_char_from_ascii(' '), attr);

            char label[FE_NAME_MAX + 2];
            if (e->is_dir) snprintf(label, sizeof label, "%s/", e->name);
            else           snprintf(label, sizeof label, "%s",  e->name);
            draw_str(w, r, col, label, attr);
        }
    }

    /* hint bar */
    if (cell_w(g_browser_hint) > 0) {
        nocterm_attribute_t st = a_fg(COL_DIM, false);
        clear_widget(g_browser_hint, st);
        char info[160];
        snprintf(info, sizeof info,
                 " %lld/%llu   [↑↓] move  [Enter] open/play  [Bksp] up  [q] quit"
                 "    backend: %s",
                 (long long)(g_count ? g_sel + 1 : 0),
                 (unsigned long long)g_count, audio_backend_name());
        draw_str(g_browser_hint, 0, 0, info, st);
    }
}

NOCTERM_TIMER_CALLBACK(browser_paint_cb)
{
    (void)widget; (void)user_data;
    browser_render();
}

static void browser_enter(void)
{
    if (g_count == 0) return;
    fb_entry_t *e = &g_entries[g_sel];
    if (e->is_dir) {
        char name[FE_NAME_MAX];
        strncpy(name, e->name, sizeof name - 1);
        name[sizeof name - 1] = '\0';
        browser_change_dir(name);
        return;
    }
    if (g_backend == PB_NONE) {
        /* nothing can decode audio; stay put (hint bar shows "backend: none") */
        return;
    }
    /* build an absolute-ish path and hand off to the player page */
    char full[PATH_MAX + FE_NAME_MAX + 2];
    snprintf(full, sizeof full, "%s/%s", g_cwd, e->name);
    audio_play(full, e->name);

    label_set(g_np_label, e->name);
    nocterm_timer_start(g_player_timer);
    nocterm_page_stack_push(g_player_page);
}

NOCTERM_WIDGET_KEY_HANDLER(browser_kh)
{
    (void)self;
    switch (nocterm_key_translate(key)) {
    case NOCTERM_KEY_EVENT_UP:        browser_select_move(-1);          break;
    case NOCTERM_KEY_EVENT_DOWN:      browser_select_move(+1);          break;
    case NOCTERM_KEY_EVENT_LEFT:
    case NOCTERM_KEY_EVENT_BACKSPACE: browser_go_parent();              break;
    case NOCTERM_KEY_EVENT_RIGHT:
    case NOCTERM_KEY_EVENT_ENTER:     browser_enter();                  break;
    case NOCTERM_KEY_EVENT_BREAK:
    case NOCTERM_KEY_EVENT_EOF:
    case NOCTERM_KEY_EVENT_ESCAPE:    nocterm_page_stack_pop();         break;
    case NOCTERM_KEY_EVENT_PRINTABLE:
        switch (key->buffer[0]) {
        case 'k': browser_select_move(-1);              break;
        case 'j': browser_select_move(+1);              break;
        case 'h': browser_go_parent();                  break;
        case 'l': browser_enter();                      break;
        case 'g': g_sel = 0;                            break;
        case 'G': g_sel = (int64_t)g_count - 1;         break;
        case 'u': browser_select_move(-(g_list_rows/2));break;
        case 'd': browser_select_move(+(g_list_rows/2));break;
        case 'q': nocterm_page_stack_pop();             break;
        default: break;
        }
        break;
    default: break;
    }
}

/* ════════════════════════════════════════════════════ PLAYER RENDER ══ */

/* animated spectrum bar heights (one float per column) */
#define SPEC_MAXW 512
static double g_levels[SPEC_MAXW];

static void spectrum_update(void)
{
    int W = cell_w(g_spectrum);
    int H = cell_h(g_spectrum);
    if (W <= 0 || H <= 0) return;
    if (W > SPEC_MAXW) W = SPEC_MAXW;

    double t = (double)now_ms() * 0.004;
    for (int c = 0; c < W; c++) {
        if (g_loaded && !g_paused && !g_finished) {
            /* a smooth travelling wave mixed with a little noise per bar */
            double wave  = (sin(c * 0.45 + t) + sin(c * 0.13 - t * 1.7)) * 0.25 + 0.5;
            double noise = (double)(rand() % 1000) / 1000.0;
            double target = (0.55 * wave + 0.45 * noise) * H;
            if (target > g_levels[c]) g_levels[c] += (target - g_levels[c]) * 0.55;
            else                      g_levels[c] -= 0.6;   /* gravity fall    */
        } else {
            g_levels[c] -= 0.5;                              /* settle to floor */
        }
        if (g_levels[c] < 0) g_levels[c] = 0;
        if (g_levels[c] > H) g_levels[c] = H;
    }
}

static void spectrum_render(void)
{
    nocterm_widget_t *w = g_spectrum;
    int W = cell_w(w), H = cell_h(w);
    if (W <= 0 || H <= 0) return;
    if (W > SPEC_MAXW) W = SPEC_MAXW;

    /* sub-cell vertical block glyphs ▁..█ for a smooth top edge */
    static const wchar_t blocks[] = {
        L' ', L'▁', L'▂', L'▃', L'▄', L'▅', L'▆', L'▇', L'█'
    };
    /* 256-colour gradient by height: green → yellow → orange → red */
    static const uint8_t grad[] = { 46, 47, 82, 154, 226, 220, 208, 202, 196 };

    nocterm_attribute_t blank = {0};
    for (int c = 0; c < W; c++) {
        double hf  = g_levels[c];
        int    full = (int)hf;
        double frac = hf - full;
        for (int r = 0; r < H; r++) {
            int from_bottom = H - 1 - r;          /* 0 == bottom row          */
            uint8_t color = grad[(from_bottom * 8) / (H > 1 ? H - 1 : 1)];
            nocterm_attribute_t a = a256(color, true);
            if (from_bottom < full) {
                cell_put(w, r, c, wch(L'█'), a);
            } else if (from_bottom == full && frac > 0.10) {
                int lvl = (int)(frac * 8.0);
                if (lvl < 1) lvl = 1;
                if (lvl > 8) lvl = 8;
                cell_put(w, r, c, wch(blocks[lvl]), a);
            } else {
                cell_put(w, r, c, nocterm_char_from_ascii(' '), blank);
            }
        }
    }
}

static void transport_render(void)
{
    nocterm_widget_t *w = g_transport;
    int W = cell_w(w);
    if (W <= 0) return;

    nocterm_attribute_t blank = {0};
    fill_row(w, 0, 0, blank);

    /* glyphs:  ⏪  ⏯(▶/⏸)  ⏩  ⏹  ⏏   ; centre glyph reflects play state */
    wchar_t play_glyph = (g_loaded && !g_paused && !g_finished) ? L'⏸' : L'▶';
    wchar_t glyphs[5]  = { L'⏪', play_glyph, L'⏩', L'⏹', L'⏏' };

    int gap = 4;
    int total = 5 + 4 * gap;                 /* 5 glyphs + 4 gaps             */
    int col = (W - total) / 2;
    if (col < 0) col = 0;

    for (int i = 0; i < 5; i++) {
        nocterm_attribute_t a;
        if (i == 1)                          /* highlight play/pause          */
            a = a_fg(g_paused ? COL_KEY : COL_AUDIO, true);
        else
            a = a_fg(COL_PLAIN, false);
        cell_put(w, 0, col, wch(glyphs[i]), a);
        col += 1 + gap;
    }
}

static void statusrow_render(void)
{
    nocterm_widget_t *w = g_statusrow;
    int W = cell_w(w);
    if (W <= 0) return;

    nocterm_attribute_t blank = {0};
    fill_row(w, 0, 0, blank);

    const char *state;
    nocterm_attribute_t sa;
    if (g_finished)       { state = "■ Finished"; sa = a_fg(COL_DIM, false); }
    else if (g_paused)    { state = "⏸ Paused";   sa = a_fg(COL_KEY, true);  }
    else if (g_loaded)    { state = "▶ Playing";  sa = a_fg(2, true);        }
    else                  { state = "⏹ Stopped";  sa = a_fg(COL_DIM, false); }

    if (g_notice[0]) { state = g_notice; sa = a_fg(1, true); }

    draw_str(w, 0, 1, state, sa);

    /* volume readout on the right (the levelbar itself sits beside it) */
    char vol[24];
    snprintf(vol, sizeof vol, "%3d%%", g_volume);
    nocterm_attribute_t va = a_fg(COL_PLAIN, false);
    draw_str(w, 0, W - (int)strlen(vol) - 1, vol, va);
    const char *lbl = "Vol";
    draw_str(w, 0, W - VOL_LEN - (int)strlen(vol) - 7, lbl, a_fg(COL_DIM, false));
}

static void player_render(void)
{
    spectrum_update();
    spectrum_render();
    transport_render();
    statusrow_render();

    /* progress levelbar (per-mille of duration; animated when length unknown) */
    if (g_progress) {
        uint64_t value = 0;
        if (g_duration > 0) {
            double frac = audio_elapsed() / g_duration;
            if (frac < 0) frac = 0;
            if (frac > 1) frac = 1;
            value = (uint64_t)(frac * PROGRESS_RES);
        } else if (g_loaded && !g_paused && !g_finished) {
            /* unknown duration: sweep a marker so the bar still feels alive */
            value = (now_ms() / 30) % PROGRESS_RES;
        }
        nocterm_levelbar_set_value(g_progress, value);
    }

    /* time readout */
    if (g_time_label) {
        char e[16], d[16], line[40];
        fmt_time(audio_elapsed(), e, sizeof e);
        if (g_duration > 0) { fmt_time(g_duration, d, sizeof d);
                              snprintf(line, sizeof line, "%s / %s", e, d); }
        else                  snprintf(line, sizeof line, "%s / --:--", e);
        /* centre the time text inside its (wide) label widget */
        int width = NOCTERM_WIDGET(g_time_label)->bounds.width;
        int pad = (width - (int)strlen(line)) / 2;
        if (pad < 0) pad = 0;
        char padded[64];
        snprintf(padded, sizeof padded, "%*s%s", pad, "", line);
        label_set(g_time_label, padded);
    }

    /* volume levelbar */
    if (g_volbar) {
        uint64_t v = (uint64_t)(g_volume * VOL_LEN / 100);
        if (v > VOL_LEN) v = VOL_LEN;
        nocterm_levelbar_set_value(g_volbar, v);
    }
}

NOCTERM_TIMER_CALLBACK(player_paint_cb)
{
    (void)widget; (void)user_data;
    audio_poll();
    player_render();
}

static void player_leave_to_browser(void)
{
    audio_stop();
    nocterm_timer_stop(g_player_timer);
    nocterm_page_stack_pop();              /* back to the browser             */
}

NOCTERM_WIDGET_KEY_HANDLER(player_kh)
{
    (void)self;
    g_notice[0] = '\0';
    switch (nocterm_key_translate(key)) {
    case NOCTERM_KEY_EVENT_LEFT:   audio_seek(-10);            break;
    case NOCTERM_KEY_EVENT_RIGHT:  audio_seek(+10);            break;
    case NOCTERM_KEY_EVENT_UP:     audio_change_volume(+5);    break;
    case NOCTERM_KEY_EVENT_DOWN:   audio_change_volume(-5);    break;
    case NOCTERM_KEY_EVENT_ENTER:  audio_toggle_pause();       break;
    case NOCTERM_KEY_EVENT_BACKSPACE:
    case NOCTERM_KEY_EVENT_ESCAPE: player_leave_to_browser();  break;
    case NOCTERM_KEY_EVENT_BREAK:
    case NOCTERM_KEY_EVENT_EOF:
        audio_stop();
        nocterm_timer_stop(g_player_timer);
        nocterm_page_stack_pop();          /* pop player                      */
        nocterm_page_stack_pop();          /* pop browser → quit              */
        break;
    case NOCTERM_KEY_EVENT_PRINTABLE:
        switch (key->buffer[0]) {
        case ' ': audio_toggle_pause();       break;
        case ',': audio_seek(-10);            break;
        case '.': audio_seek(+10);            break;
        case '<': audio_restart();            break;
        case '>': audio_seek(+30);            break;
        case 's': audio_stop();               break;
        case '+': case '=': audio_change_volume(+5); break;
        case '-': case '_': audio_change_volume(-5); break;
        case 'o': player_leave_to_browser();  break;
        case 'q':
            audio_stop();
            nocterm_timer_stop(g_player_timer);
            nocterm_page_stack_pop();
            nocterm_page_stack_pop();
            break;
        default: break;
        }
        break;
    default: break;
    }
}

NOCTERM_WIDGET_RESIZE_HANDLER(player_resize)
{
    (void)self; (void)bounds; (void)viewport;
    /* nothing special: flex re-flows children; the paint timer redraws. */
}

/* ══════════════════════════════════════════════════════ PAGE BUILDERS ══ */

static nocterm_decorbox_t *make_box(nocterm_widget_t *inner, const char *label)
{
    nocterm_decorbox_t *db = nocterm_decorbox_new(inner);
    nocterm_decorbox_set_border(db,
        nocterm_decorbox_border_from_shape(NOCTERM_DECORBOX_BORDER_SHAPE_UNICODE_ROUND),
        a_fg(COL_DIM, false), a_fg(COL_ACCENT, true));
    if (label)
        nocterm_decorbox_set_label(db, label, strlen(label) + 1,
                                   a_fg(COL_ACCENT, true), 2);
    return db;
}

static nocterm_label_t *spaced_label(int width, nocterm_attribute_t attr)
{
    char *sp = malloc(width + 1);
    memset(sp, ' ', width);
    sp[width] = '\0';
    nocterm_label_t *l = nocterm_label_new(sp, width + 1);
    nocterm_label_set_attribute(l, attr);
    free(sp);
    return l;
}

static nocterm_page_t *build_browser_page(void)
{
    g_browser_root = nocterm_widget_new(1, 1, NOCTERM_WIDGET_FOCUSABLE_YES,
                                        NOCTERM_WIDGET_TYPE_VIRTUAL);
    nocterm_widget_flex(g_browser_root, NOCTERM_WIDGET_FLEX_FILL_BOTH);
    nocterm_widget_set_key_handler(g_browser_root, browser_kh);

    /* top path bar */
    g_browser_bar = nocterm_widget_new(1, 1, NOCTERM_WIDGET_FOCUSABLE_NO,
                                       NOCTERM_WIDGET_TYPE_REAL);
    nocterm_widget_flex(g_browser_bar, NOCTERM_WIDGET_FLEX_FILL_HORIZONTAL);
    nocterm_widget_add_subwidget(g_browser_root, g_browser_bar);
    nocterm_widget_align(g_browser_bar, NOCTERM_WIDGET_ALIGN_TOP);
    nocterm_widget_align(g_browser_bar, NOCTERM_WIDGET_ALIGN_LEFT);

    /* file list inside a pane */
    g_browser_list = nocterm_widget_new(1, 1, NOCTERM_WIDGET_FOCUSABLE_NO,
                                        NOCTERM_WIDGET_TYPE_REAL);
    nocterm_widget_flex(g_browser_list, NOCTERM_WIDGET_FLEX_FILL_BOTH);
    g_browser_box = make_box(g_browser_list, " ♪ library ");
    nocterm_widget_flex(NOCTERM_WIDGET(g_browser_box),
                        NOCTERM_WIDGET_FLEX_FILL_HORIZONTAL);
    nocterm_widget_flex(NOCTERM_WIDGET(g_browser_box),
                        NOCTERM_WIDGET_FLEX_PERCENT_VERTICAL, 85);
    nocterm_widget_add_subwidget(g_browser_root, NOCTERM_WIDGET(g_browser_box));
    nocterm_widget_align(NOCTERM_WIDGET(g_browser_box), NOCTERM_WIDGET_ALIGN_LEFT);
    nocterm_widget_align(NOCTERM_WIDGET(g_browser_box), NOCTERM_WIDGET_ALIGN_TOP);
    nocterm_widget_align(NOCTERM_WIDGET(g_browser_box),
                         NOCTERM_WIDGET_ALIGN_MARGIN_VERTICAL, 1);

    /* bottom hint bar */
    g_browser_hint = nocterm_widget_new(1, 1, NOCTERM_WIDGET_FOCUSABLE_NO,
                                        NOCTERM_WIDGET_TYPE_REAL);
    nocterm_widget_flex(g_browser_hint, NOCTERM_WIDGET_FLEX_FILL_HORIZONTAL);
    nocterm_widget_add_subwidget(g_browser_root, g_browser_hint);
    nocterm_widget_align(g_browser_hint, NOCTERM_WIDGET_ALIGN_BOTTOM);
    nocterm_widget_align(g_browser_hint, NOCTERM_WIDGET_ALIGN_LEFT);

    g_browser_timer = nocterm_timer_create(g_browser_root, 60, browser_paint_cb, NULL);

    return nocterm_page_new("Library", sizeof "Library", g_browser_root);
}

static nocterm_page_t *build_player_page(void)
{
    g_player_root = nocterm_widget_new(1, 1, NOCTERM_WIDGET_FOCUSABLE_YES,
                                       NOCTERM_WIDGET_TYPE_VIRTUAL);
    nocterm_widget_flex(g_player_root, NOCTERM_WIDGET_FLEX_FILL_BOTH);
    nocterm_widget_set_key_handler(g_player_root, player_kh);
    nocterm_widget_set_resize_handler(g_player_root, player_resize);

    /* decorative outer frame holding a content holder */
    nocterm_widget_t *content = nocterm_widget_new(1, 1, NOCTERM_WIDGET_FOCUSABLE_NO,
                                                   NOCTERM_WIDGET_TYPE_VIRTUAL);
    nocterm_widget_flex(content, NOCTERM_WIDGET_FLEX_FILL_BOTH);
    nocterm_decorbox_t *frame = make_box(content, " ♪ nocterm music player ");
    nocterm_widget_flex(NOCTERM_WIDGET(frame), NOCTERM_WIDGET_FLEX_FILL_BOTH);
    nocterm_widget_add_subwidget(g_player_root, NOCTERM_WIDGET(frame));

    /* now-playing name */
    g_np_label = spaced_label(60, a_fg(COL_HOT, true));
    nocterm_widget_add_subwidget(content, NOCTERM_WIDGET(g_np_label));
    nocterm_widget_align(NOCTERM_WIDGET(g_np_label), NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL);
    nocterm_widget_align(NOCTERM_WIDGET(g_np_label), NOCTERM_WIDGET_ALIGN_TOP);
    nocterm_widget_align(NOCTERM_WIDGET(g_np_label), NOCTERM_WIDGET_ALIGN_PERCENT_VERTICAL, 8);

    /* spectrum visualizer (fixed height, full width) */
    g_spectrum = nocterm_widget_new(7, 1, NOCTERM_WIDGET_FOCUSABLE_NO,
                                    NOCTERM_WIDGET_TYPE_REAL);
    nocterm_widget_flex(g_spectrum, NOCTERM_WIDGET_FLEX_PERCENT_HORIZONTAL, 80);
    nocterm_widget_add_subwidget(content, g_spectrum);
    nocterm_widget_align(g_spectrum, NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL);
    nocterm_widget_align(g_spectrum, NOCTERM_WIDGET_ALIGN_TOP);
    nocterm_widget_align(g_spectrum, NOCTERM_WIDGET_ALIGN_PERCENT_VERTICAL, 22);

    /* progress levelbar */
    g_progress = nocterm_levelbar_new(1, 0, PROGRESS_RES,
                                      NOCTERM_LEVELBAR_TYPE_HORIZONTAL, false);
    nocterm_levelbar_set_character(g_progress, wch(L'█'));
    nocterm_levelbar_set_attribute(g_progress, a256(45, true));   /* bright cyan */
    nocterm_widget_flex(NOCTERM_WIDGET(g_progress), NOCTERM_WIDGET_FLEX_PERCENT_HORIZONTAL, 80);
    nocterm_widget_add_subwidget(content, NOCTERM_WIDGET(g_progress));
    nocterm_widget_align(NOCTERM_WIDGET(g_progress), NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL);
    nocterm_widget_align(NOCTERM_WIDGET(g_progress), NOCTERM_WIDGET_ALIGN_TOP);
    nocterm_widget_align(NOCTERM_WIDGET(g_progress), NOCTERM_WIDGET_ALIGN_PERCENT_VERTICAL, 58);

    /* time readout */
    g_time_label = spaced_label(40, a_fg(COL_PLAIN, true));
    nocterm_widget_add_subwidget(content, NOCTERM_WIDGET(g_time_label));
    nocterm_widget_align(NOCTERM_WIDGET(g_time_label), NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL);
    nocterm_widget_align(NOCTERM_WIDGET(g_time_label), NOCTERM_WIDGET_ALIGN_TOP);
    nocterm_widget_align(NOCTERM_WIDGET(g_time_label), NOCTERM_WIDGET_ALIGN_PERCENT_VERTICAL, 65);

    /* transport strip */
    g_transport = nocterm_widget_new(1, 1, NOCTERM_WIDGET_FOCUSABLE_NO,
                                     NOCTERM_WIDGET_TYPE_REAL);
    nocterm_widget_flex(g_transport, NOCTERM_WIDGET_FLEX_PERCENT_HORIZONTAL, 80);
    nocterm_widget_add_subwidget(content, g_transport);
    nocterm_widget_align(g_transport, NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL);
    nocterm_widget_align(g_transport, NOCTERM_WIDGET_ALIGN_TOP);
    nocterm_widget_align(g_transport, NOCTERM_WIDGET_ALIGN_PERCENT_VERTICAL, 74);

    /* status row (state text + volume readout) */
    g_statusrow = nocterm_widget_new(1, 1, NOCTERM_WIDGET_FOCUSABLE_NO,
                                     NOCTERM_WIDGET_TYPE_REAL);
    nocterm_widget_flex(g_statusrow, NOCTERM_WIDGET_FLEX_PERCENT_HORIZONTAL, 80);
    nocterm_widget_add_subwidget(content, g_statusrow);
    nocterm_widget_align(g_statusrow, NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL);
    nocterm_widget_align(g_statusrow, NOCTERM_WIDGET_ALIGN_TOP);
    nocterm_widget_align(g_statusrow, NOCTERM_WIDGET_ALIGN_PERCENT_VERTICAL, 83);

    /* volume levelbar (small, fixed length) — sits just left of the % readout */
    g_volbar = nocterm_levelbar_new(VOL_LEN, 0, VOL_LEN,
                                    NOCTERM_LEVELBAR_TYPE_HORIZONTAL, false);
    nocterm_levelbar_set_character(g_volbar, wch(L'▮'));
    nocterm_levelbar_set_attribute(g_volbar, a_fg(2, true));
    nocterm_widget_add_subwidget(content, NOCTERM_WIDGET(g_volbar));
    nocterm_widget_align(NOCTERM_WIDGET(g_volbar), NOCTERM_WIDGET_ALIGN_RIGHT);
    nocterm_widget_align(NOCTERM_WIDGET(g_volbar), NOCTERM_WIDGET_ALIGN_MARGIN_HORIZONTAL, 8);
    nocterm_widget_align(NOCTERM_WIDGET(g_volbar), NOCTERM_WIDGET_ALIGN_TOP);
    nocterm_widget_align(NOCTERM_WIDGET(g_volbar), NOCTERM_WIDGET_ALIGN_PERCENT_VERTICAL, 83);

    /* hints */
    nocterm_label_t *hints = nocterm_label_new(
        "Space play/pause   ←/→ seek   ↑/↓ volume   < restart   o back   q quit",
        72);
    nocterm_label_set_attribute(hints, a_fg(COL_DIM, false));
    nocterm_widget_add_subwidget(content, NOCTERM_WIDGET(hints));
    nocterm_widget_align(NOCTERM_WIDGET(hints), NOCTERM_WIDGET_ALIGN_CENTER_HORIZONTAL);
    nocterm_widget_align(NOCTERM_WIDGET(hints), NOCTERM_WIDGET_ALIGN_BOTTOM);
    nocterm_widget_align(NOCTERM_WIDGET(hints), NOCTERM_WIDGET_ALIGN_MARGIN_VERTICAL, 1);

    g_player_timer = nocterm_timer_create(g_player_root, 80, player_paint_cb, NULL);

    return nocterm_page_new("Player", sizeof "Player", g_player_root);
}

/* ══════════════════════════════════════════════════════════════ MAIN ══ */

int main(int argc, char **argv)
{
    setlocale(LC_ALL, "");
    /* Keep the UTF-8 ctype locale for our wide glyphs, but force C numeric
     * formatting: under locales whose decimal separator is ',' (e.g. tr_TR,
     * de_DE) snprintf("%.2f", ...) would emit "11,19", which ffplay's -ss and
     * sox's trim reject — and atof() of ffprobe's "180.5" would stop at the
     * dot.  LC_NUMERIC=C makes both round-trip correctly. */
    setlocale(LC_NUMERIC, "C");
    srand((unsigned)time(NULL));

    if (argc > 1 && chdir(argv[1]) != 0) {
        fprintf(stderr, "music_player: %s: %s\n", argv[1], strerror(errno));
        return 1;
    }

    audio_detect_backend();
    load_dir();

    g_browser_page = build_browser_page();
    g_player_page  = build_player_page();

    nocterm_page_stack_push(g_browser_page);
    nocterm_timer_start(g_browser_timer);

    nocterm_init();
    nocterm_loop();
    nocterm_end();

    audio_stop();                          /* make sure no decoder lingers    */

    nocterm_timer_delete_all();
    nocterm_page_delete(g_browser_page);
    nocterm_page_delete(g_player_page);
    free(g_entries);

    return 0;
}
