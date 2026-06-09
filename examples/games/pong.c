#include <nocterm/nocterm.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>

/*
 * Two-player Pong.
 *   P1 (left)  — W / S
 *   P2 (right) — Up / Down arrows
 *   Space to serve, R to restart, Q to quit.
 *   First to 7 wins.
 */

#define GRID_W     80
#define GRID_H     50

#define PADDLE_W    3
#define PADDLE_H    9
#define BALL_SZ     2

#define P1_X        2
#define P2_X       (GRID_W - PADDLE_W - 2)   /* 75 */

#define STEP        4   /* paddle pixels per key press */
#define SCORE_WIN   7

/* 3×5 pixel-art digit bitmaps (bit 2 = left col, bit 0 = right col) */
static const uint8_t DIGIT[10][5] = {
    {7,5,5,5,7}, /* 0 */
    {2,2,2,2,2}, /* 1 */
    {7,1,7,4,7}, /* 2 */
    {7,1,7,1,7}, /* 3 */
    {5,5,7,1,1}, /* 4 */
    {7,4,7,1,7}, /* 5 */
    {7,4,7,5,7}, /* 6 */
    {7,1,1,1,1}, /* 7 */
    {7,5,7,5,7}, /* 8 */
    {7,5,7,1,7}, /* 9 */
};

typedef struct {
    float bx, by;   /* ball position (float for sub-pixel accuracy) */
    float bdx, bdy; /* ball velocity */
    int   p1_y;     /* left paddle top row */
    int   p2_y;     /* right paddle top row */
    int   p1_score;
    int   p2_score;
    int   serve_dir; /* +1 toward P2, -1 toward P1 */
    bool  waiting;   /* waiting for serve */
    bool  game_over;
    int   winner;
    nocterm_pixelgrid_t* grid;
    nocterm_widget_t*    hud;
} pong_t;

static pong_t G;

/* ---------- drawing ------------------------------------------------------- */

static void dot(int row, int col, uint8_t r, uint8_t g, uint8_t b) {
    if (row < 0 || row >= GRID_H || col < 0 || col >= GRID_W) return;
    nocterm_pixelgrid_set_pixel(G.grid, (uint32_t)row, (uint16_t)col, r, g, b);
}

static void fill_rect(int y, int x, int h, int w, uint8_t r, uint8_t g, uint8_t b) {
    for (int dy = 0; dy < h; dy++)
        for (int dx = 0; dx < w; dx++)
            dot(y+dy, x+dx, r, g, b);
}

static void draw_digit(int y, int x, int d) {
    for (int row = 0; row < 5; row++) {
        uint8_t bits = DIGIT[d][row];
        for (int col = 0; col < 3; col++)
            if (bits & (4 >> col))
                dot(y+row, x+col, 160, 160, 160);
    }
}

static void render(void) {
    NOCTERM_WIDGET(G.grid)->hard_refresh = true;
    nocterm_pixelgrid_clear(G.grid);

    /* Center dashed line */
    for (int y = 0; y < GRID_H; y += 4)
        dot(y, GRID_W/2, 55, 55, 55);

    /* Score digits near top of court */
    draw_digit(3, GRID_W/4 - 1,  G.p1_score);
    draw_digit(3, 3*GRID_W/4 - 1, G.p2_score);

    /* Paddles */
    fill_rect(G.p1_y, P1_X, PADDLE_H, PADDLE_W, 255, 255, 255);
    fill_rect(G.p2_y, P2_X, PADDLE_H, PADDLE_W, 255, 255, 255);

    /* Ball */
    fill_rect((int)G.by, (int)G.bx, BALL_SZ, BALL_SZ, 255, 230, 50);
}

/* ---------- HUD ----------------------------------------------------------- */

static void update_hud(void) {
    char buf[128];
    if (G.game_over)
        snprintf(buf, sizeof(buf),
            "PLAYER %d WINS!  %d - %d  |  R to restart  |  Q to quit",
            G.winner, G.p1_score, G.p2_score);
    else if (G.waiting)
        snprintf(buf, sizeof(buf),
            "P1: %d  P2: %d  |  SPACE to serve  |  W/S  vs  Up/Down  |  Q to quit",
            G.p1_score, G.p2_score);
    else
        snprintf(buf, sizeof(buf),
            "P1: %d  P2: %d  |  W/S  vs  Up/Down  |  Q to quit",
            G.p1_score, G.p2_score);

    nocterm_widget_clear(G.hud);
    nocterm_char_t cs[128];
    uint64_t n = nocterm_char_string_from_stream(cs, 128, buf, strlen(buf));
    for (uint64_t i = 0; i < n; i++)
        nocterm_widget_update(G.hud, 0, (uint32_t)i, cs[i], NOCTERM_ATTRIBUTE_EMPTY);
    G.hud->hard_refresh = true;
}

/* ---------- game logic ---------------------------------------------------- */

static void serve(int dir) {
    G.bx  = (GRID_W - BALL_SZ) / 2.0f;
    G.by  = (GRID_H - BALL_SZ) / 2.0f;
    G.bdx = dir * 2.5f;
    /* Random launch angle: avoid near-zero dy */
    int r = rand() % 9 - 4;          /* -4 .. 4 */
    G.bdy = r * 0.25f;
    if (G.bdy > -0.4f && G.bdy < 0.4f) G.bdy = (r >= 0) ? 0.4f : -0.4f;
    G.waiting = false;
}

static void point_to(int scorer) {
    if (scorer == 1) G.p1_score++; else G.p2_score++;
    if (G.p1_score >= SCORE_WIN) { G.game_over = true; G.winner = 1; }
    else if (G.p2_score >= SCORE_WIN) { G.game_over = true; G.winner = 2; }
    else {
        /* Loser serves next */
        G.serve_dir = (scorer == 1) ? 1 : -1;
        G.waiting = true;
    }
    update_hud();
}

static void init_game(void) {
    G.p1_y      = (GRID_H - PADDLE_H) / 2;
    G.p2_y      = (GRID_H - PADDLE_H) / 2;
    G.p1_score  = 0;
    G.p2_score  = 0;
    G.serve_dir = 1;
    G.waiting   = true;
    G.game_over = false;
    G.winner    = 0;
    G.bx  = (GRID_W - BALL_SZ) / 2.0f;
    G.by  = (GRID_H - BALL_SZ) / 2.0f;
    G.bdx = 0; G.bdy = 0;
}

/* ---------- timer --------------------------------------------------------- */

NOCTERM_TIMER_CALLBACK(game_tick) {
    if (G.game_over || G.waiting) { render(); return; }

    /* Move ball */
    G.bx += G.bdx;
    G.by += G.bdy;

    /* Top / bottom wall bounce */
    if (G.by <= 0.0f) {
        G.by  = 0.0f;
        if (G.bdy < 0) G.bdy = -G.bdy;
    }
    if (G.by + BALL_SZ >= GRID_H) {
        G.by  = GRID_H - BALL_SZ;
        if (G.bdy > 0) G.bdy = -G.bdy;
    }

    /* Left paddle hit */
    if (G.bdx < 0 &&
        (int)G.bx        <= P1_X + PADDLE_W &&
        (int)G.bx + BALL_SZ > P1_X          &&
        (int)G.by + BALL_SZ > G.p1_y        &&
        (int)G.by          < G.p1_y + PADDLE_H) {
        G.bx  = P1_X + PADDLE_W;
        float rel = (G.by + BALL_SZ/2.0f - (G.p1_y + PADDLE_H/2.0f)) / (PADDLE_H/2.0f);
        G.bdy = rel * 3.5f;
        G.bdx = -(G.bdx * 1.06f);         /* reverse + speed up */
        if (G.bdx >  5.5f) G.bdx =  5.5f;
        if (G.bdy >  4.0f) G.bdy =  4.0f;
        if (G.bdy < -4.0f) G.bdy = -4.0f;
    }

    /* Right paddle hit */
    if (G.bdx > 0 &&
        (int)G.bx + BALL_SZ >= P2_X         &&
        (int)G.bx             < P2_X + PADDLE_W &&
        (int)G.by + BALL_SZ > G.p2_y        &&
        (int)G.by             < G.p2_y + PADDLE_H) {
        G.bx  = P2_X - BALL_SZ;
        float rel = (G.by + BALL_SZ/2.0f - (G.p2_y + PADDLE_H/2.0f)) / (PADDLE_H/2.0f);
        G.bdy = rel * 3.5f;
        G.bdx = -(G.bdx * 1.06f);
        if (G.bdx < -5.5f) G.bdx = -5.5f;
        if (G.bdy >  4.0f) G.bdy =  4.0f;
        if (G.bdy < -4.0f) G.bdy = -4.0f;
    }

    /* Scoring */
    if (G.bx + BALL_SZ <= 0)   { point_to(2); return; }
    if (G.bx >= GRID_W)        { point_to(1); return; }

    render();
}

/* ---------- key handler --------------------------------------------------- */

NOCTERM_WIDGET_KEY_HANDLER(handle_key) {
    if (key->buffer_length == 1) {
        char c = key->buffer[0];
        if (c == 'q' || c == 'Q') { nocterm_page_stack_pop(); return; }
        if ((c == 'r' || c == 'R') && G.game_over) { init_game(); update_hud(); return; }
        if (c == ' ' && G.waiting && !G.game_over) { serve(G.serve_dir); update_hud(); return; }

        if (!G.game_over) {
            if (c == 'w' || c == 'W') { G.p1_y -= STEP; if (G.p1_y < 0) G.p1_y = 0; }
            if (c == 's' || c == 'S') { G.p1_y += STEP; if (G.p1_y + PADDLE_H > GRID_H) G.p1_y = GRID_H - PADDLE_H; }
        }
    }

    if (!G.game_over) {
        nocterm_key_event_t ev = nocterm_key_translate(key);
        if (ev == NOCTERM_KEY_EVENT_UP)   { G.p2_y -= STEP; if (G.p2_y < 0) G.p2_y = 0; }
        if (ev == NOCTERM_KEY_EVENT_DOWN) { G.p2_y += STEP; if (G.p2_y + PADDLE_H > GRID_H) G.p2_y = GRID_H - PADDLE_H; }
    }
}

/* ---------- main ---------------------------------------------------------- */

int main(void) {
    setlocale(LC_ALL, "");
    srand((unsigned)time(NULL));

    G.grid = nocterm_pixelgrid_new(GRID_H, GRID_W);

    nocterm_decorbox_t* box = nocterm_decorbox_new(NOCTERM_WIDGET(G.grid));
    nocterm_decorbox_set_border(box,
        nocterm_decorbox_border_shape(NOCTERM_DECORBOX_BORDER_SHAPE_UNICODE_ROUND),
        NOCTERM_ATTRIBUTE_EMPTY, NOCTERM_ATTRIBUTE_EMPTY);

    G.hud = nocterm_widget_new(1, 80, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_REAL);

    nocterm_widget_t* root = nocterm_widget_new(
        GRID_H/2 + 5, GRID_W + 4,
        NOCTERM_WIDGET_FOCUSABLE_YES, NOCTERM_WIDGET_TYPE_VIRTUAL);

    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(box));
    nocterm_widget_add_subwidget(root, G.hud);
    nocterm_widget_set_position(NOCTERM_WIDGET(box), 1, 2);
    nocterm_widget_set_position(G.hud, GRID_H/2 + 3, 2);
    nocterm_widget_add_key_handler(root, handle_key);

    nocterm_page_t* page = nocterm_page_new("Pong", 10, root);
    nocterm_page_stack_push(page);

    init_game();
    update_hud();

    nocterm_timer_t* timer = nocterm_timer_create(root, 50, game_tick, NULL);
    nocterm_timer_start(timer);

    nocterm_init();
    nocterm_loop();
    nocterm_end();

    nocterm_timer_delete(timer);
    nocterm_page_delete(page);
    nocterm_decorbox_delete(box);
    nocterm_pixelgrid_delete(G.grid);
    nocterm_widget_delete(G.hud);
    nocterm_widget_delete(root);
    return 0;
}
