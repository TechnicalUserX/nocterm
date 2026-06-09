#include <nocterm/nocterm.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>

/*
 * 9 lanes × 8 px = 72 px tall, 72 px wide
 *
 *  lane 8  y= 0- 7   goal  (5 lily pads)
 *  lane 7  y= 8-15   water (logs →, slow)
 *  lane 6  y=16-23   water (logs ←, fast)
 *  lane 5  y=24-31   water (logs →, medium)
 *  lane 4  y=32-39   safe  (grass median)
 *  lane 3  y=40-47   road  (cars ←, fast)
 *  lane 2  y=48-55   road  (cars →, medium)
 *  lane 1  y=56-63   road  (cars ←, slow)
 *  lane 0  y=64-71   start (safe grass)
 */

#define GRID_W    72
#define GRID_H    72
#define LANE_H     8
#define NUM_LANES  9
#define GOAL_LANE  8
#define START_LANE 0

#define FROG_W     6
#define FROG_H     6
#define FROG_PAD   ((LANE_H - FROG_H) / 2)   /* vertical centering within lane */

#define NUM_PADS   5
#define PAD_W      8
#define PAD_H      6
static const int PAD_X[NUM_PADS] = { 4, 17, 30, 43, 56 };

#define MAX_OBJS   4

typedef enum { LANE_SAFE, LANE_ROAD, LANE_WATER } lane_type_t;

typedef struct {
    lane_type_t type;
    int         dir;    /* +1 right, -1 left */
    int         speed;  /* px per tick at level 1 */
    int         obj_w;  /* obstacle width in pixels */
    int         n_objs;
    uint8_t     r, g, b;
} lane_cfg_t;

static const lane_cfg_t CFG[NUM_LANES] = {
    /* 0 start  */ { LANE_SAFE,  0,  0,  0, 0,   0,  0,  0 },
    /* 1 road   */ { LANE_ROAD, -1,  1, 10, 3, 200,200, 50 },
    /* 2 road   */ { LANE_ROAD, +1,  2,  9, 3, 210, 90, 40 },
    /* 3 road   */ { LANE_ROAD, -1,  3,  8, 2, 210, 55, 55 },
    /* 4 median */ { LANE_SAFE,  0,  0,  0, 0,   0,  0,  0 },
    /* 5 water  */ { LANE_WATER,+1,  1, 16, 2, 120, 80, 30 },
    /* 6 water  */ { LANE_WATER,-1,  2, 10, 3,  90, 60, 20 },
    /* 7 water  */ { LANE_WATER,+1,  1, 14, 2, 140, 95, 25 },
    /* 8 goal   */ { LANE_SAFE,  0,  0,  0, 0,   0,  0,  0 },
};

typedef struct { int x; } obj_t;

typedef struct {
    int  frog_lane;
    int  frog_x;
    int  lives;
    int  score;
    int  level;
    bool dead;
    int  dead_ticks;
    bool game_over;
    bool pad_filled[NUM_PADS];
    obj_t objs[NUM_LANES][MAX_OBJS];
    nocterm_pixelgrid_t* grid;
    nocterm_widget_t*    hud;
} state_t;

static state_t S;

static int lane_top(int lane) { return (GOAL_LANE - lane) * LANE_H; }

/* ---------- drawing ------------------------------------------------------- */

static void dot(int row, int col, uint8_t r, uint8_t g, uint8_t b) {
    if (row < 0 || row >= GRID_H || col < 0 || col >= GRID_W) return;
    nocterm_pixelgrid_set_pixel(S.grid, (uint32_t)row, (uint16_t)col, r, g, b);
}

static void fill_rect(int y, int x, int h, int w, uint8_t r, uint8_t g, uint8_t b) {
    for (int dy = 0; dy < h; dy++)
        for (int dx = 0; dx < w; dx++)
            dot(y+dy, x+dx, r, g, b);
}

/*
 * Frog sprite (6×6):
 *   row 0: yellow eyes at col+1, col+4
 *   rows 1-4: green body (4×4 interior)
 *   rows 4-5: green legs at col 0 and col 5
 */
static void draw_frog(int y, int x) {
    fill_rect(y+1, x+1, 4, 4, 50, 200, 50);
    dot(y,   x+1, 255,255, 0); dot(y,   x+4, 255,255, 0);
    dot(y+4, x,    50,200,50); dot(y+4, x+5,  50,200, 50);
    dot(y+5, x,    50,200,50); dot(y+5, x+5,  50,200, 50);
}

static void draw_dead_frog(int y, int x) {
    dot(y,   x,   200, 0,0); dot(y,   x+5, 200,0,0);
    dot(y+1, x+1, 200, 0,0); dot(y+1, x+4, 200,0,0);
    dot(y+2, x+2, 200, 0,0); dot(y+2, x+3, 200,0,0);
    dot(y+3, x+2, 200, 0,0); dot(y+3, x+3, 200,0,0);
    dot(y+4, x+1, 200, 0,0); dot(y+4, x+4, 200,0,0);
    dot(y+5, x,   200, 0,0); dot(y+5, x+5, 200,0,0);
}

static void draw_bg(void) {
    for (int lane = 0; lane < NUM_LANES; lane++) {
        int y = lane_top(lane);
        uint8_t r, g, b;
        switch (CFG[lane].type) {
            case LANE_SAFE:  r=30;  g=90;  b=30;  break;
            case LANE_ROAD:  r=50;  g=50;  b=50;  break;
            case LANE_WATER: r=20;  g=55;  b=155; break;
            default:         r=0;   g=0;   b=0;   break;
        }
        fill_rect(y, 0, LANE_H, GRID_W, r, g, b);
    }

    /* Dashed white dividers between road lanes */
    for (int lane = 1; lane <= 3; lane++) {
        int y = lane_top(lane);
        for (int x = 0; x < GRID_W; x += 8)
            dot(y, x, 160, 160, 160);
    }

    /* Goal zone: water background + lily pads */
    int gy = lane_top(GOAL_LANE);
    fill_rect(gy, 0, LANE_H, GRID_W, 20, 55, 155);
    for (int i = 0; i < NUM_PADS; i++) {
        uint8_t pr = S.pad_filled[i] ? 50  : 30;
        uint8_t pg = S.pad_filled[i] ? 220 : 130;
        uint8_t pb = S.pad_filled[i] ? 50  : 30;
        fill_rect(gy+1, PAD_X[i], PAD_H, PAD_W, pr, pg, pb);
    }
}

static void draw_obstacles(void) {
    for (int lane = 1; lane <= 7; lane++) {
        if (CFG[lane].type == LANE_SAFE) continue;
        int ly = lane_top(lane) + 1;
        int lh = LANE_H - 2;
        for (int i = 0; i < CFG[lane].n_objs; i++) {
            int ox = S.objs[lane][i].x;
            int ow = CFG[lane].obj_w;
            int x0 = ox < 0      ? 0      : ox;
            int x1 = ox+ow > GRID_W ? GRID_W : ox+ow;
            if (x1 > x0)
                fill_rect(ly, x0, lh, x1-x0, CFG[lane].r, CFG[lane].g, CFG[lane].b);
        }
    }
}

static void render_frame(bool show_frog, bool dead) {
    NOCTERM_WIDGET(S.grid)->hard_refresh = true;
    nocterm_pixelgrid_clear(S.grid);
    draw_bg();
    draw_obstacles();
    if (show_frog) {
        int fy = lane_top(S.frog_lane) + FROG_PAD;
        if (dead) draw_dead_frog(fy, S.frog_x);
        else      draw_frog(fy, S.frog_x);
    }
}

/* ---------- collision helpers --------------------------------------------- */

/* 1-px shrink on the frog for fair leniency */
static bool overlaps(int frog_x, int obj_x, int obj_w) {
    return (frog_x+1) < (obj_x+obj_w) && (frog_x+FROG_W-1) > obj_x;
}

static bool on_any_log(void) {
    int lane = S.frog_lane;
    if (CFG[lane].type != LANE_WATER) return false;
    for (int i = 0; i < CFG[lane].n_objs; i++)
        if (overlaps(S.frog_x, S.objs[lane][i].x, CFG[lane].obj_w)) return true;
    return false;
}

static bool hit_any_car(void) {
    int lane = S.frog_lane;
    if (CFG[lane].type != LANE_ROAD) return false;
    for (int i = 0; i < CFG[lane].n_objs; i++)
        if (overlaps(S.frog_x, S.objs[lane][i].x, CFG[lane].obj_w)) return true;
    return false;
}

/* ---------- game logic ---------------------------------------------------- */

static void kill_frog(void) {
    S.dead       = true;
    S.dead_ticks = 0;
    S.lives--;
    if (S.lives <= 0) S.game_over = true;
}

static void reset_frog(void) {
    S.frog_lane  = START_LANE;
    S.frog_x     = (GRID_W - FROG_W) / 2;
    S.dead       = false;
    S.dead_ticks = 0;
}

static bool all_pads_filled(void) {
    for (int i = 0; i < NUM_PADS; i++) if (!S.pad_filled[i]) return false;
    return true;
}

static void init_objs(void) {
    for (int lane = 1; lane <= 7; lane++) {
        if (CFG[lane].type == LANE_SAFE || CFG[lane].n_objs == 0) continue;
        int sp = GRID_W / CFG[lane].n_objs;
        for (int i = 0; i < CFG[lane].n_objs; i++)
            S.objs[lane][i].x = i * sp + rand() % (sp / 2 + 1);
    }
}

static void init_game(void) {
    S.lives      = 3;
    S.score      = 0;
    S.level      = 1;
    S.game_over  = false;
    memset(S.pad_filled, 0, sizeof(S.pad_filled));
    init_objs();
    reset_frog();
}

static int lane_speed(int lane) {
    int s = CFG[lane].speed + S.level - 1;
    return s > 5 ? 5 : s;
}

static void move_obstacles(void) {
    for (int lane = 1; lane <= 7; lane++) {
        if (CFG[lane].type == LANE_SAFE) continue;
        int spd = lane_speed(lane);
        for (int i = 0; i < CFG[lane].n_objs; i++) {
            S.objs[lane][i].x += CFG[lane].dir * spd;
            if (CFG[lane].dir > 0 && S.objs[lane][i].x >= GRID_W)
                S.objs[lane][i].x = -CFG[lane].obj_w;
            if (CFG[lane].dir < 0 && S.objs[lane][i].x + CFG[lane].obj_w <= 0)
                S.objs[lane][i].x = GRID_W;
        }
    }
}

/* ---------- HUD ----------------------------------------------------------- */

static void update_hud(void) {
    char buf[128];
    if (S.game_over)
        snprintf(buf, sizeof(buf),
            "GAME OVER  Score: %d  |  R to restart  |  Q to quit", S.score);
    else
        snprintf(buf, sizeof(buf),
            "Score: %-5d  Lives: %d  Level: %d  |  Arrow keys  |  Q to quit",
            S.score, S.lives, S.level);
    nocterm_widget_clear(S.hud);
    nocterm_char_t cs[128];
    uint64_t n = nocterm_char_string_from_stream(cs, 128, buf, strlen(buf));
    for (uint64_t i = 0; i < n; i++)
        nocterm_widget_update(S.hud, 0, (uint32_t)i, cs[i], NOCTERM_ATTRIBUTE_EMPTY);
    S.hud->hard_refresh = true;
}

/* ---------- timer --------------------------------------------------------- */

NOCTERM_TIMER_CALLBACK(game_tick) {
    if (S.game_over) return;

    move_obstacles();

    /* Death animation: flash for ~1 second then respawn */
    if (S.dead) {
        S.dead_ticks++;
        if (S.dead_ticks >= 20) {
            if (!S.game_over) { reset_frog(); update_hud(); }
            else render_frame(false, false);
            return;
        }
        render_frame((S.dead_ticks / 3) % 2 == 0, true);
        return;
    }

    /* Ride log: frog x drifts with the log; fall off edge = death */
    if (CFG[S.frog_lane].type == LANE_WATER) {
        bool riding = false;
        for (int i = 0; i < CFG[S.frog_lane].n_objs; i++) {
            if (overlaps(S.frog_x, S.objs[S.frog_lane][i].x, CFG[S.frog_lane].obj_w)) {
                S.frog_x += CFG[S.frog_lane].dir * lane_speed(S.frog_lane);
                riding = true;
                break;
            }
        }
        int cx = S.frog_x + FROG_W / 2;
        if (!riding || cx < 0 || cx >= GRID_W) {
            kill_frog(); update_hud();
            render_frame(false, false);
            return;
        }
    }

    /* Car collision */
    if (hit_any_car()) { kill_frog(); update_hud(); }

    render_frame(!S.dead, false);
}

/* ---------- key handler --------------------------------------------------- */

NOCTERM_WIDGET_KEY_HANDLER(handle_key) {
    if (key->buffer_length == 1) {
        char c = key->buffer[0];
        if (c == 'q' || c == 'Q') { nocterm_page_stack_pop(); return; }
        if ((c == 'r' || c == 'R') && (S.game_over || S.dead)) {
            init_game(); update_hud(); return;
        }
    }
    if (S.dead || S.game_over) return;

    nocterm_key_event_t ev = nocterm_key_translate(key);
    int new_lane = S.frog_lane;
    int new_x    = S.frog_x;

    switch (ev) {
        case NOCTERM_KEY_EVENT_UP:    new_lane++;        break;
        case NOCTERM_KEY_EVENT_DOWN:  new_lane--;        break;
        case NOCTERM_KEY_EVENT_LEFT:  new_x -= FROG_W;  break;
        case NOCTERM_KEY_EVENT_RIGHT: new_x += FROG_W;  break;
        default: return;
    }

    if (new_lane < START_LANE)  new_lane = START_LANE;
    if (new_lane > GOAL_LANE)   new_lane = GOAL_LANE;
    if (new_x < 0)              new_x = 0;
    if (new_x > GRID_W-FROG_W)  new_x = GRID_W - FROG_W;

    S.frog_lane = new_lane;
    S.frog_x    = new_x;

    /* Landed on goal row */
    if (S.frog_lane == GOAL_LANE) {
        bool on_pad = false;
        for (int i = 0; i < NUM_PADS; i++) {
            if (!S.pad_filled[i] && overlaps(S.frog_x, PAD_X[i], PAD_W)) {
                S.pad_filled[i] = true;
                S.score += 50;
                on_pad = true;
                break;
            }
        }
        if (!on_pad) {
            kill_frog();
        } else {
            reset_frog();
            if (all_pads_filled()) {
                S.level++;
                S.score += 200;
                memset(S.pad_filled, 0, sizeof(S.pad_filled));
            }
        }
        update_hud();
        return;
    }

    /* Immediate checks for the new lane */
    if (CFG[S.frog_lane].type == LANE_WATER && !on_any_log()) {
        kill_frog(); update_hud(); return;
    }
    if (hit_any_car()) { kill_frog(); update_hud(); }
}

/* ---------- main ---------------------------------------------------------- */

int main(void) {
    setlocale(LC_ALL, "");
    srand((unsigned)time(NULL));

    S.grid = nocterm_pixelgrid_new(GRID_H, GRID_W);

    nocterm_decorbox_t* box = nocterm_decorbox_new(NOCTERM_WIDGET(S.grid));
    nocterm_decorbox_set_border(box,
        nocterm_decorbox_border_shape(NOCTERM_DECORBOX_BORDER_SHAPE_UNICODE_ROUND),
        NOCTERM_ATTRIBUTE_EMPTY, NOCTERM_ATTRIBUTE_EMPTY);

    S.hud = nocterm_widget_new(1, 80, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_REAL);

    nocterm_widget_t* root = nocterm_widget_new(
        GRID_H/2 + 5, GRID_W + 4,
        NOCTERM_WIDGET_FOCUSABLE_YES, NOCTERM_WIDGET_TYPE_VIRTUAL);

    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(box));
    nocterm_widget_add_subwidget(root, S.hud);
    nocterm_widget_set_position(NOCTERM_WIDGET(box), 1, 2);
    nocterm_widget_set_position(S.hud, GRID_H/2 + 3, 2);
    nocterm_widget_add_key_handler(root, handle_key);

    nocterm_page_t* page = nocterm_page_new("Frogger", 10, root);
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
    nocterm_pixelgrid_delete(S.grid);
    nocterm_widget_delete(S.hud);
    nocterm_widget_delete(root);
    return 0;
}
