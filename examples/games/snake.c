#include <nocterm/nocterm.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>

#define GRID_WIDTH 40
#define GRID_HEIGHT 20
#define MAX_SNAKE_LENGTH (GRID_WIDTH * GRID_HEIGHT)

typedef struct {
    int x, y;
} point_t;

typedef enum {
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} direction_t;

typedef struct {
    point_t body[MAX_SNAKE_LENGTH];
    int length;
    direction_t dir;
    point_t food;
    int score;
    bool game_over;
    nocterm_pixelgrid_t* grid;
    nocterm_widget_t* score_widget;
} snake_game_t;

static snake_game_t game;

void spawn_food();
void update_score_display();
void reset_game();
void initial_draw();
NOCTERM_TIMER_CALLBACK(game_tick);
NOCTERM_WIDGET_KEY_HANDLER(handle_key);


void spawn_food() {
    bool on_snake;
    do {
        on_snake = false;
        game.food.x = rand() % GRID_WIDTH;
        game.food.y = rand() % GRID_HEIGHT;
        for (int i = 0; i < game.length; i++) {
            if (game.body[i].x == game.food.x && game.body[i].y == game.food.y) {
                on_snake = true;
                break;
            }
        }
    } while (on_snake);
}

void update_score_display() {
    char score_str[128];
    if (game.game_over) {
        sprintf(score_str, "GAME OVER! Score: %d | Press 'R' to restart | Press 'Q' to quit", game.score);
    } else {
        sprintf(score_str, "Score: %d | Controls: Arrow Keys | Press 'Q' to quit", game.score);
    }
    
    nocterm_widget_clear(game.score_widget);
    nocterm_char_t ch_str[128];
    uint64_t len = nocterm_char_string_from_stream(ch_str, 128, score_str, strlen(score_str));
    for (uint64_t i = 0; i < len; i++) {
        nocterm_widget_update(game.score_widget, 0, i, ch_str[i], NOCTERM_ATTRIBUTE_EMPTY);
    }
}

void initial_draw() {
    nocterm_pixelgrid_clear(game.grid);
    NOCTERM_WIDGET(game.grid)->hard_refresh = true; 
    
    nocterm_pixelgrid_set_pixel(game.grid, game.food.y, game.food.x, 255, 0, 0);
    for (int i = 0; i < game.length; i++) {
        nocterm_pixelgrid_set_pixel(game.grid, game.body[i].y, game.body[i].x, 0, 255, 0);
    }
}

void reset_game() {
    game.length = 5;
    game.body[0] = (point_t){10, 10};
    game.body[1] = (point_t){9, 10};
    game.body[2] = (point_t){8, 10};
    game.body[3] = (point_t){7, 10};
    game.body[4] = (point_t){6, 10};
    game.dir = DIR_RIGHT;
    game.score = 0;
    game.game_over = false;
    spawn_food();
    update_score_display();
    initial_draw();
}

NOCTERM_TIMER_CALLBACK(game_tick) {
    if (game.game_over) return;

    point_t old_tail = game.body[game.length - 1];
    bool ate_food = false;

    point_t next_head = game.body[0];
    switch (game.dir) {
        case DIR_UP:    next_head.y--; break;
        case DIR_DOWN:  next_head.y++; break;
        case DIR_LEFT:  next_head.x--; break;
        case DIR_RIGHT: next_head.x++; break;
    }

    if (next_head.x < 0 || next_head.x >= GRID_WIDTH || next_head.y < 0 || next_head.y >= GRID_HEIGHT) {
        game.game_over = true;
        update_score_display();
        return;
    }

    for (int i = 0; i < game.length; i++) {
        if (next_head.x == game.body[i].x && next_head.y == game.body[i].y) {
            game.game_over = true;
            update_score_display();
            return;
        }
    }

    if (next_head.x == game.food.x && next_head.y == game.food.y) {
        ate_food = true;
        if (game.length < MAX_SNAKE_LENGTH) game.length++;
        game.score += 10;
        update_score_display();
        spawn_food();
        nocterm_pixelgrid_set_pixel(game.grid, game.food.y, game.food.x, 255, 0, 0);
    }

    for (int i = game.length - 1; i > 0; i--) {
        game.body[i] = game.body[i - 1];
    }
    game.body[0] = next_head;

    if (!ate_food) {
        nocterm_pixelgrid_erase(game.grid, old_tail.y, old_tail.x);
    }

    nocterm_pixelgrid_set_pixel(game.grid, game.body[0].y, game.body[0].x, 0, 255, 0);
}

NOCTERM_WIDGET_KEY_HANDLER(handle_key) {
    nocterm_key_event_t event = nocterm_key_translate(key);
    
    if (key->buffer_length == 1) {
        char c = key->buffer[0];
        if (c == 'q' || c == 'Q') {
            nocterm_page_stack_pop();
            return;
        }
        if ((c == 'r' || c == 'R') && game.game_over) {
            reset_game();
            return;
        }
    }

    switch (event) {
        case NOCTERM_KEY_EVENT_UP:    if (game.dir != DIR_DOWN) game.dir = DIR_UP; break;
        case NOCTERM_KEY_EVENT_DOWN:  if (game.dir != DIR_UP) game.dir = DIR_DOWN; break;
        case NOCTERM_KEY_EVENT_LEFT:  if (game.dir != DIR_RIGHT) game.dir = DIR_LEFT; break;
        case NOCTERM_KEY_EVENT_RIGHT: if (game.dir != DIR_LEFT) game.dir = DIR_RIGHT; break;
        default: break;
    }
}

int main() {
    setlocale(LC_ALL, "");
    srand(time(NULL));

    game.grid = nocterm_pixelgrid_new(GRID_HEIGHT, GRID_WIDTH);
    nocterm_decorbox_t* box = nocterm_decorbox_new(NOCTERM_WIDGET(game.grid));
    nocterm_decorbox_set_border(box, nocterm_decorbox_border_from_shape(NOCTERM_DECORBOX_BORDER_SHAPE_UNICODE_ROUND), NOCTERM_ATTRIBUTE_EMPTY, NOCTERM_ATTRIBUTE_EMPTY);

    game.score_widget = nocterm_widget_new(1, 80, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_REAL);

    nocterm_widget_t* container = nocterm_widget_new(GRID_HEIGHT/2 + 5, GRID_WIDTH + 4, NOCTERM_WIDGET_FOCUSABLE_YES, NOCTERM_WIDGET_TYPE_VIRTUAL);
    nocterm_widget_add_subwidget(container, NOCTERM_WIDGET(box));
    nocterm_widget_add_subwidget(container, NOCTERM_WIDGET(game.score_widget));
    
    nocterm_widget_set_position(NOCTERM_WIDGET(box), 1, 2);
    nocterm_widget_set_position(NOCTERM_WIDGET(game.score_widget), GRID_HEIGHT/2 + 3, 2);

    nocterm_widget_set_key_handler(container, handle_key);

    nocterm_page_t* page = nocterm_page_new("Snake Game", 10, container);
    nocterm_page_stack_push(page);

    reset_game();

    nocterm_timer_t* timer = nocterm_timer_create(container, 100, game_tick, NULL);
    nocterm_timer_start(timer);

    nocterm_init();
    nocterm_loop();
    nocterm_end();

    nocterm_timer_delete(timer);
    nocterm_page_delete(page);
    nocterm_decorbox_delete(box);
    nocterm_pixelgrid_delete(game.grid);
    nocterm_widget_delete(game.score_widget);
    nocterm_widget_delete(container);

    return 0;
}
