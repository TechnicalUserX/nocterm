#include <nocterm/nocterm.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>

/* --- Grid & road geometry ------------------------------------------------- */
#define GRID_H      80
#define GRID_W      60

#define ROAD_L      12
#define ROAD_R      47
#define CENTER_X    29

/* --- Car shape ------------------------------------------------------------ */
#define CAR_W        6
#define CAR_H       10
#define PLAYER_Y    (GRID_H - CAR_H - 4)   /* 66 */
#define PLAYER_MIN  (ROAD_L + 2)
#define PLAYER_MAX  (ROAD_R - CAR_W - 1)

#define MAX_ENEMIES  4

/* -------------------------------------------------------------------------- */
typedef struct {
    int x, y;
    bool active;
    uint8_t r, g, b;
} enemy_t;

typedef struct {
    int player_x;
    int score;
    int speed;      /* 2-8 pixels/tick */
    int scroll;     /* total road scroll, mod 10000 */
    int ticks;
    bool game_over;
    enemy_t  enemies[MAX_ENEMIES];
    nocterm_pixelgrid_t* grid;
    nocterm_widget_t*    hud;
} race_t;

static race_t game;

/* -------------------------------------------------------------------------- */
static void dot(int row, int col, uint8_t r, uint8_t g, uint8_t b) {
    if (row < 0 || row >= GRID_H || col < 0 || col >= GRID_W) return;
    nocterm_pixelgrid_set_pixel(game.grid, (uint32_t)row, (uint16_t)col, r, g, b);
}

/*
 * Car sprite (6 wide × 10 tall):
 *   row 0: headlights at col+1 and col+4
 *   rows 1-2: full-width body
 *   rows 3-4: side pillars + darker windshield
 *   rows 5-6: full-width body
 *   row 7: tires at corners
 *   row 8: tail lights at col+1 and col+4
 *   row 9: half-width rear bumper
 */
static void draw_car(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    uint8_t dr = r / 4, dg = g / 4, db = b / 4;

    dot(y,   x+1, 255,255,180); dot(y,   x+4, 255,255,180);
    for (int c = 0; c < CAR_W; c++) { dot(y+1,x+c,r,g,b); dot(y+2,x+c,r,g,b); }
    dot(y+3,x,r,g,b); dot(y+3,x+CAR_W-1,r,g,b);
    dot(y+4,x,r,g,b); dot(y+4,x+CAR_W-1,r,g,b);
    for (int c = 1; c < CAR_W-1; c++) { dot(y+3,x+c,dr,dg,db); dot(y+4,x+c,dr,dg,db); }
    for (int c = 0; c < CAR_W; c++) { dot(y+5,x+c,r,g,b); dot(y+6,x+c,r,g,b); }
    dot(y+7,x,  22,22,22); dot(y+7,x+1,22,22,22);
    dot(y+7,x+2,r,g,b);    dot(y+7,x+3,r,g,b);
    dot(y+7,x+4,22,22,22); dot(y+7,x+5,22,22,22);
    dot(y+8,x+1,200,0,0);  dot(y+8,x+4,200,0,0);
    for (int c = 1; c < CAR_W-1; c++) dot(y+9,x+c, r/2,g/2,b/2);
}

static void draw_road(void) {
    for (int y = 0; y < GRID_H; y++) {
        int phase = y + game.scroll;

        /* Grass: alternating light/dark green stripes */
        uint8_t gg = ((phase / 5) & 1) ? 110 : 60;
        for (int x = 0;        x < ROAD_L;  x++) dot(y,x, 0,gg,0);
        for (int x = ROAD_R+1; x < GRID_W;  x++) dot(y,x, 0,gg,0);

        /* Roadside poles on the grass (every 20 pixels, red-top / white-base) */
        int pole_row = phase % 20;
        if (pole_row < 5) {
            uint8_t pr = pole_row < 3 ? 210 : 255;
            uint8_t pg = pole_row < 3 ? 25  : 255;
            uint8_t pb = pole_row < 3 ? 25  : 255;
            dot(y, 6,  pr, pg, pb);
            dot(y, 53, pr, pg, pb);
        }

        /* Road surface: subtle alternating depth bands */
        uint8_t rv = ((phase / 8) & 1) ? 54 : 44;
        for (int x = ROAD_L; x <= ROAD_R; x++) dot(y,x, rv,rv,rv);

        /* Kerb (road edge): alternating red / white chevrons */
        uint8_t kr = ((phase / 4) & 1) ? 215 : 255;
        uint8_t kg = ((phase / 4) & 1) ? 25  : 255;
        uint8_t kb = ((phase / 4) & 1) ? 25  : 255;
        dot(y, ROAD_L,   kr,kg,kb); dot(y, ROAD_L+1, kr,kg,kb);
        dot(y, ROAD_R,   kr,kg,kb); dot(y, ROAD_R-1, kr,kg,kb);

        /* Center dashes (yellow) */
        if ((phase % 12) < 6) dot(y, CENTER_X, 220,200,0);
    }
}

static bool cars_collide(int ax, int ay, int bx, int by) {
    return ax < bx+CAR_W && ax+CAR_W > bx && ay < by+CAR_H && ay+CAR_H > by;
}

static void spawn_enemy(void) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (game.enemies[i].active) continue;
        int lane = rand() % 2;
        /* left lane: x ≈ 16-22, right lane: x ≈ 32-38 */
        game.enemies[i].x = lane
            ? ROAD_L + 4 + rand() % 7
            : CENTER_X + 3 + rand() % 7;
        game.enemies[i].y = -CAR_H;
        game.enemies[i].active = true;
        switch (rand() % 4) {
            case 0: game.enemies[i].r=210; game.enemies[i].g=55;  game.enemies[i].b=55;  break;
            case 1: game.enemies[i].r=210; game.enemies[i].g=200; game.enemies[i].b=45;  break;
            case 2: game.enemies[i].r=55;  game.enemies[i].g=200; game.enemies[i].b=210; break;
            default:game.enemies[i].r=200; game.enemies[i].g=75;  game.enemies[i].b=215; break;
        }
        return;
    }
}

static void update_hud(void) {
    char buf[128];
    if (game.game_over)
        snprintf(buf, sizeof(buf),
            "CRASH!  Score: %d  |  Speed: %d  |  R to restart  |  Q to quit",
            game.score, game.speed);
    else
        snprintf(buf, sizeof(buf),
            "Score: %-6d  Speed: %d  |  Left / Right to steer  |  Q to quit",
            game.score, game.speed);

    nocterm_widget_clear(game.hud);
    nocterm_char_t cs[128];
    uint64_t n = nocterm_char_string_from_stream(cs, 128, buf, strlen(buf));
    for (uint64_t i = 0; i < n; i++)
        nocterm_widget_update(game.hud, 0, (uint32_t)i, cs[i], NOCTERM_ATTRIBUTE_EMPTY);
    game.hud->hard_refresh = true;
}

static void init_game(void) {
    game.player_x  = (ROAD_L + ROAD_R) / 2 - CAR_W / 2;
    game.score     = 0;
    game.speed     = 2;
    game.scroll    = 0;
    game.ticks     = 0;
    game.game_over = false;
    for (int i = 0; i < MAX_ENEMIES; i++) game.enemies[i].active = false;
    update_hud();
    NOCTERM_WIDGET(game.grid)->hard_refresh = true;
    nocterm_pixelgrid_clear(game.grid);
    draw_road();
    draw_car(game.player_x, PLAYER_Y, 30, 120, 255);
}

/* -------------------------------------------------------------------------- */
NOCTERM_TIMER_CALLBACK(game_tick) {
    if (game.game_over) return;

    game.ticks++;
    game.scroll = (game.scroll + game.speed) % 10000;
    game.score++;

    /* Bump speed every 200 points, cap at 8 */
    if (game.score % 200 == 0 && game.speed < 8) game.speed++;

    /* Spawn an enemy car at a rate that increases with speed */
    int period = 30 / game.speed;
    if (period < 8) period = 8;
    if (game.ticks % period == 0) spawn_enemy();

    /* Advance enemies and check for collisions */
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!game.enemies[i].active) continue;
        game.enemies[i].y += game.speed + 1;
        if (game.enemies[i].y > GRID_H) { game.enemies[i].active = false; continue; }
        if (cars_collide(game.player_x, PLAYER_Y, game.enemies[i].x, game.enemies[i].y)) {
            game.game_over = true;
            update_hud();
            return;
        }
    }

    if (game.ticks % 8 == 0) update_hud();

    /* Render frame */
    NOCTERM_WIDGET(game.grid)->hard_refresh = true;
    nocterm_pixelgrid_clear(game.grid);
    draw_road();
    for (int i = 0; i < MAX_ENEMIES; i++)
        if (game.enemies[i].active)
            draw_car(game.enemies[i].x, game.enemies[i].y,
                     game.enemies[i].r, game.enemies[i].g, game.enemies[i].b);
    draw_car(game.player_x, PLAYER_Y, 30, 120, 255);
}

NOCTERM_WIDGET_KEY_HANDLER(handle_key) {
    if (key->buffer_length == 1) {
        char c = key->buffer[0];
        if (c == 'q' || c == 'Q') { nocterm_page_stack_pop(); return; }
        if ((c == 'r' || c == 'R') && game.game_over) { init_game(); return; }
    }
    if (game.game_over) return;

    nocterm_key_event_t ev = nocterm_key_translate(key);
    int step = 2 + game.speed / 2;
    if (ev == NOCTERM_KEY_EVENT_LEFT)  { game.player_x -= step; if (game.player_x < PLAYER_MIN) game.player_x = PLAYER_MIN; }
    if (ev == NOCTERM_KEY_EVENT_RIGHT) { game.player_x += step; if (game.player_x > PLAYER_MAX) game.player_x = PLAYER_MAX; }
}

/* -------------------------------------------------------------------------- */
int main(void) {
    setlocale(LC_ALL, "");
    srand((unsigned)time(NULL));

    game.grid = nocterm_pixelgrid_new(GRID_H, GRID_W);

    nocterm_decorbox_t* box = nocterm_decorbox_new(NOCTERM_WIDGET(game.grid));
    nocterm_decorbox_set_border(box,
        nocterm_decorbox_border_shape(NOCTERM_DECORBOX_BORDER_SHAPE_UNICODE_ROUND),
        NOCTERM_ATTRIBUTE_EMPTY, NOCTERM_ATTRIBUTE_EMPTY);

    game.hud = nocterm_widget_new(1, 80, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_REAL);

    nocterm_widget_t* root = nocterm_widget_new(
        GRID_H/2 + 5, GRID_W + 4, NOCTERM_WIDGET_FOCUSABLE_YES, NOCTERM_WIDGET_TYPE_VIRTUAL);

    nocterm_widget_add_subwidget(root, NOCTERM_WIDGET(box));
    nocterm_widget_add_subwidget(root, game.hud);
    nocterm_widget_set_position(NOCTERM_WIDGET(box), 1, 2);
    nocterm_widget_set_position(game.hud, GRID_H/2 + 3, 2);
    nocterm_widget_add_key_handler(root, handle_key);

    nocterm_page_t* page = nocterm_page_new("Turbo Racer", 10, root);
    nocterm_page_stack_push(page);

    init_game();

    nocterm_timer_t* timer = nocterm_timer_create(root, 50, game_tick, NULL);
    nocterm_timer_start(timer);

    nocterm_init();
    nocterm_loop();
    nocterm_end();

    nocterm_timer_delete(timer);
    nocterm_page_delete(page);
    nocterm_decorbox_delete(box);
    nocterm_pixelgrid_delete(game.grid);
    nocterm_widget_delete(game.hud);
    nocterm_widget_delete(root);
    return 0;
}
