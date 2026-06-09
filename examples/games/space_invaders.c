#include <nocterm/nocterm.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>

#define GRID_WIDTH 70
#define GRID_HEIGHT 50
#define PLAYER_WIDTH 5
#define ALIEN_WIDTH 3
#define ALIEN_HEIGHT 2
#define NUM_ALIEN_ROWS 4
#define NUM_ALIEN_COLS 8
#define MAX_ALIENS (NUM_ALIEN_ROWS * NUM_ALIEN_COLS)
#define MAX_BULLETS 5

typedef struct {
    int x, y;
    bool active;
} bullet_t;

typedef struct {
    int x, y;
    bool active;
} alien_t;

typedef struct {
    int player_x;
    int score;
    bool game_over;
    alien_t aliens[MAX_ALIENS];
    bullet_t bullets[MAX_BULLETS];
    int alien_direction;
    int alien_move_cooldown;
    nocterm_pixelgrid_t* grid;
    nocterm_widget_t* score_label;
} game_state_t;

static game_state_t game;

void init_game();
void draw_player();
void draw_aliens();
void draw_bullets();
void update_bullets();
void update_aliens();
NOCTERM_TIMER_CALLBACK(game_loop);
NOCTERM_WIDGET_KEY_HANDLER(handle_input);
void update_score_text();

int main() {
    setlocale(LC_ALL, "");
    srand(time(NULL));

    game.grid = nocterm_pixelgrid_new(GRID_HEIGHT, GRID_WIDTH);
    nocterm_decorbox_t* box = nocterm_decorbox_new(NOCTERM_WIDGET(game.grid));
    nocterm_decorbox_set_border(box, nocterm_decorbox_border_shape(NOCTERM_DECORBOX_BORDER_SHAPE_UNICODE_ROUND), NOCTERM_ATTRIBUTE_EMPTY, NOCTERM_ATTRIBUTE_EMPTY);

    game.score_label = nocterm_widget_new(1, 80, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_REAL);
    
    nocterm_widget_t* container = nocterm_widget_new(GRID_HEIGHT/2 + 5, GRID_WIDTH + 4, NOCTERM_WIDGET_FOCUSABLE_YES, NOCTERM_WIDGET_TYPE_VIRTUAL);
    nocterm_widget_add_subwidget(container, NOCTERM_WIDGET(box));
    nocterm_widget_add_subwidget(container, game.score_label);
    
    nocterm_widget_set_position(NOCTERM_WIDGET(box), 1, 2);
    nocterm_widget_set_position(game.score_label, GRID_HEIGHT/2 + 3, 2);

    nocterm_widget_add_key_handler(container, handle_input);
    
    nocterm_page_t* page = nocterm_page_new("Space Invaders v2", 10, container);
    nocterm_page_stack_push(page);

    init_game();

    nocterm_timer_t* timer = nocterm_timer_create(container, 50, game_loop, NULL);
    nocterm_timer_start(timer);

    nocterm_init();
    nocterm_loop();
    nocterm_end();
    
    nocterm_timer_delete(timer);
    nocterm_page_delete(page);
    nocterm_decorbox_delete(box);
    nocterm_pixelgrid_delete(game.grid);
    nocterm_widget_delete(game.score_label);
    nocterm_widget_delete(container);

    return 0;
}

void init_game() {
    game.player_x = (GRID_WIDTH - PLAYER_WIDTH) / 2;
    game.score = 0;
    game.game_over = false;
    game.alien_direction = 1;
    game.alien_move_cooldown = 15;

    for (int i = 0; i < MAX_ALIENS; i++) {
        game.aliens[i].active = true;
        int row = i / NUM_ALIEN_COLS;
        int col = i % NUM_ALIEN_COLS;
        game.aliens[i].x = col * (ALIEN_WIDTH + 3) + 5;
        game.aliens[i].y = row * (ALIEN_HEIGHT + 2) + 3;
    }

    for (int i = 0; i < MAX_BULLETS; i++) {
        game.bullets[i].active = false;
    }
    update_score_text();
}

void update_score_text() {
    char buffer[128];
    if (game.game_over) {
        sprintf(buffer, "GAME OVER! Final Score: %d. Press 'R' to restart or 'Q' to quit.", game.score);
    } else {
        sprintf(buffer, "Score: %d | Controls: Left/Right Arrows, Space to Fire.", game.score);
    }
    nocterm_widget_clear(game.score_label);
    nocterm_char_t ch_str[128];
    uint64_t len = nocterm_char_string_from_stream(ch_str, 128, buffer, strlen(buffer));
    for (uint64_t i = 0; i < len; i++) {
        nocterm_widget_update(game.score_label, 0, i, ch_str[i], NOCTERM_ATTRIBUTE_EMPTY);
    }
}


void draw_player() {
    int y = GRID_HEIGHT - 3;
    nocterm_pixelgrid_set_pixel(game.grid, y, game.player_x + 2, 0, 255, 0);
    nocterm_pixelgrid_set_pixel(game.grid, y + 1, game.player_x + 1, 0, 255, 0);
    nocterm_pixelgrid_set_pixel(game.grid, y + 1, game.player_x + 2, 0, 255, 0);
    nocterm_pixelgrid_set_pixel(game.grid, y + 1, game.player_x + 3, 0, 255, 0);
    nocterm_pixelgrid_set_pixel(game.grid, y + 2, game.player_x, 0, 255, 0);
    nocterm_pixelgrid_set_pixel(game.grid, y + 2, game.player_x + 1, 0, 255, 0);
    nocterm_pixelgrid_set_pixel(game.grid, y + 2, game.player_x + 2, 0, 255, 0);
    nocterm_pixelgrid_set_pixel(game.grid, y + 2, game.player_x + 3, 0, 255, 0);
    nocterm_pixelgrid_set_pixel(game.grid, y + 2, game.player_x + 4, 0, 255, 0);
}

void draw_aliens() {
    for (int i = 0; i < MAX_ALIENS; i++) {
        if (game.aliens[i].active) {
            int x = game.aliens[i].x;
            int y = game.aliens[i].y;
            nocterm_pixelgrid_set_pixel(game.grid, y, x, 255, 0, 255);
            nocterm_pixelgrid_set_pixel(game.grid, y, x + 1, 255, 0, 255);
            nocterm_pixelgrid_set_pixel(game.grid, y, x + 2, 255, 0, 255);
            nocterm_pixelgrid_set_pixel(game.grid, y + 1, x + 1, 255, 0, 255);
        }
    }
}

void draw_bullets() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (game.bullets[i].active) {
            nocterm_pixelgrid_set_pixel(game.grid, game.bullets[i].y, game.bullets[i].x, 255, 255, 0);
            nocterm_pixelgrid_set_pixel(game.grid, game.bullets[i].y+1, game.bullets[i].x, 255, 255, 0);
        }
    }
}

void update_bullets() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (game.bullets[i].active) {
            game.bullets[i].y -= 1;
            if (game.bullets[i].y < 0) {
                game.bullets[i].active = false;
            }

            for (int j = 0; j < MAX_ALIENS; j++) {
                if (game.aliens[j].active && game.bullets[i].active) {
                    if (game.bullets[i].x >= game.aliens[j].x && game.bullets[i].x < game.aliens[j].x + ALIEN_WIDTH &&
                        game.bullets[i].y >= game.aliens[j].y && game.bullets[i].y < game.aliens[j].y + ALIEN_HEIGHT) {
                        game.aliens[j].active = false;
                        game.bullets[i].active = false;
                        game.score += 10;
                        update_score_text();
                    }
                }
            }
        }
    }
}

void update_aliens() {
    game.alien_move_cooldown--;
    if (game.alien_move_cooldown > 0) return;

    game.alien_move_cooldown = 15;
    bool wall_hit = false;
    for (int i = 0; i < MAX_ALIENS; i++) {
        if (game.aliens[i].active) {
            if ((game.aliens[i].x + ALIEN_WIDTH >= GRID_WIDTH && game.alien_direction > 0) ||
                (game.aliens[i].x <= 0 && game.alien_direction < 0)) {
                wall_hit = true;
                break;
            }
        }
    }

    if (wall_hit) {
        game.alien_direction *= -1;
        for (int i = 0; i < MAX_ALIENS; i++) {
            game.aliens[i].y++;
        }
    } else {
        for (int i = 0; i < MAX_ALIENS; i++) {
            game.aliens[i].x += game.alien_direction;
        }
    }

    for (int i = 0; i < MAX_ALIENS; i++) {
        if (game.aliens[i].active && game.aliens[i].y + ALIEN_HEIGHT >= GRID_HEIGHT - 3) {
            game.game_over = true;
            update_score_text();
            return;
        }
    }
}

NOCTERM_TIMER_CALLBACK(game_loop) {
    if (game.game_over) return;
    
    NOCTERM_WIDGET(game.grid)->hard_refresh = true;
    nocterm_pixelgrid_clear(game.grid);
    
    update_bullets();
    update_aliens();
    
    draw_player();
    draw_aliens();
    draw_bullets();
}

NOCTERM_WIDGET_KEY_HANDLER(handle_input) {
    nocterm_key_event_t event = nocterm_key_translate(key);

    if (key->buffer_length == 1) {
        char c = key->buffer[0];
        if (c == 'q' || c == 'Q') {
            nocterm_page_stack_pop();
            return;
        }
        if ((c == 'r' || c == 'R') && game.game_over) {
            init_game();
            return;
        }
        if (c == ' ' && !game.game_over) {
            for (int i = 0; i < MAX_BULLETS; i++) {
                if (!game.bullets[i].active) {
                    game.bullets[i].active = true;
                    game.bullets[i].x = game.player_x + PLAYER_WIDTH / 2;
                    game.bullets[i].y = GRID_HEIGHT - 4;
                    break;
                }
            }
        }
    }

    if (!game.game_over) {
        switch (event) {
            case NOCTERM_KEY_EVENT_LEFT:
                if (game.player_x > 0) game.player_x -= 2;
                break;
            case NOCTERM_KEY_EVENT_RIGHT:
                if (game.player_x < GRID_WIDTH - PLAYER_WIDTH) game.player_x += 2;
                break;
            default:
                break;
        }
    }
}
