#include <nocterm/nocterm.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>

#define GRID_WIDTH 60
#define GRID_HEIGHT 80
#define ROAD_WIDTH 30
#define CAR_WIDTH 5
#define CAR_HEIGHT 7
#define MAX_OBSTACLES 5

typedef struct {
    int x, y;
    bool active;
} obstacle_t;

typedef struct {
    int player_x;
    int player_y;
    int road_x;
    int score;
    int speed;
    bool game_over;
    obstacle_t obstacles[MAX_OBSTACLES];
    nocterm_pixelgrid_t* grid;
    nocterm_widget_t* score_widget;
} racing_game_t;

static racing_game_t game;

void draw_car(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    // A simple car shape
    //  .R.
    //  RRR
    //  .R.
    //  RRR
    //  R.R
    
    // Body
    for (int i = 0; i < CAR_HEIGHT; i++) {
        for (int j = 0; j < CAR_WIDTH; j++) {
            bool draw = false;
            if (i == 0 || i == 2 || i == 4) {
                if (j >= 1 && j <= 3) draw = true;
            } else if (i == 1 || i == 3 || i == 5) {
                draw = true;
            } else if (i == 6) {
                if (j == 0 || j == 4) draw = true;
            }
            
            if (draw) {
                nocterm_pixelgrid_set_pixel(game.grid, y + i, x + j, r, g, b);
            }
        }
    }
}

void erase_car(int x, int y) {
    for (int i = 0; i < CAR_HEIGHT; i++) {
        for (int j = 0; j < CAR_WIDTH; j++) {
            nocterm_pixelgrid_erase(game.grid, y + i, x + j);
        }
    }
}

void spawn_obstacle() {
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!game.obstacles[i].active) {
            game.obstacles[i].active = true;
            game.obstacles[i].x = game.road_x + 2 + rand() % (ROAD_WIDTH - CAR_WIDTH - 4);
            game.obstacles[i].y = -CAR_HEIGHT;
            break;
        }
    }
}

void update_score_display() {
    char score_str[128];
    if (game.game_over) {
        sprintf(score_str, "CRASH! Score: %d | Press 'R' to restart | Press 'Q' to quit", game.score);
    } else {
        sprintf(score_str, "Score: %d | Speed: %d | Controls: Left/Right | Press 'Q' to quit", game.score, game.speed);
    }
    
    nocterm_widget_clear(game.score_widget);
    nocterm_char_t ch_str[128];
    uint64_t len = nocterm_char_string_from_stream(ch_str, 128, score_str, strlen(score_str));
    for (uint64_t i = 0; i < len; i++) {
        nocterm_widget_update(game.score_widget, 0, i, ch_str[i], NOCTERM_ATTRIBUTE_EMPTY);
    }
}

void reset_game() {
    game.road_x = (GRID_WIDTH - ROAD_WIDTH) / 2;
    game.player_x = game.road_x + (ROAD_WIDTH - CAR_WIDTH) / 2;
    game.player_y = GRID_HEIGHT - CAR_HEIGHT - 5;
    game.score = 0;
    game.speed = 1;
    game.game_over = false;
    
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        game.obstacles[i].active = false;
    }
    
    nocterm_pixelgrid_clear(game.grid);
    update_score_display();
}

NOCTERM_TIMER_CALLBACK(game_tick) {
    if (game.game_over) return;

    // Clear old drawings
    nocterm_pixelgrid_clear(game.grid);

    // Draw road
    for (int y = 0; y < GRID_HEIGHT; y++) {
        // Grass
        for (int x = 0; x < game.road_x; x++) {
            nocterm_pixelgrid_set_pixel(game.grid, y, x, 34, 139, 34); // Forest Green
        }
        for (int x = game.road_x + ROAD_WIDTH; x < GRID_WIDTH; x++) {
            nocterm_pixelgrid_set_pixel(game.grid, y, x, 34, 139, 34);
        }
        // Road
        for (int x = game.road_x; x < game.road_x + ROAD_WIDTH; x++) {
            nocterm_pixelgrid_set_pixel(game.grid, y, x, 50, 50, 50); // Dark Gray
        }
        // Side lines
        nocterm_pixelgrid_set_pixel(game.grid, y, game.road_x, 255, 255, 255);
        nocterm_pixelgrid_set_pixel(game.grid, y, game.road_x + ROAD_WIDTH - 1, 255, 255, 255);
        
        // Dashed center line
        if ((y + game.score / 2) % 10 < 5) {
            nocterm_pixelgrid_set_pixel(game.grid, y, game.road_x + ROAD_WIDTH / 2, 255, 255, 0);
        }
    }

    // Move and draw obstacles
    for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (game.obstacles[i].active) {
            game.obstacles[i].y += game.speed;
            if (game.obstacles[i].y >= GRID_HEIGHT) {
                game.obstacles[i].active = false;
                game.score += 10;
                if (game.score % 100 == 0) game.speed++;
                update_score_display();
            } else {
                draw_car(game.obstacles[i].x, game.obstacles[i].y, 0, 0, 255); // Blue obstacles
                
                // Collision check
                if (game.obstacles[i].y + CAR_HEIGHT > game.player_y && 
                    game.obstacles[i].y < game.player_y + CAR_HEIGHT &&
                    game.obstacles[i].x + CAR_WIDTH > game.player_x &&
                    game.obstacles[i].x < game.player_x + CAR_WIDTH) {
                    game.game_over = true;
                }
            }
        }
    }

    // Check road collision
    if (game.player_x < game.road_x || game.player_x + CAR_WIDTH > game.road_x + ROAD_WIDTH) {
        game.game_over = true;
    }

    if (game.game_over) {
        update_score_display();
    }

    // Draw player
    draw_car(game.player_x, game.player_y, 255, 0, 0); // Red player

    // Spawn new obstacles
    if (rand() % 20 == 0) {
        spawn_obstacle();
    }
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

    if (!game.game_over) {
        switch (event) {
            case NOCTERM_KEY_EVENT_LEFT:  game.player_x -= 2; break;
            case NOCTERM_KEY_EVENT_RIGHT: game.player_x += 2; break;
            default: break;
        }
    }
}

int main() {
    setlocale(LC_ALL, "");
    srand(time(NULL));

    game.grid = nocterm_pixelgrid_new(GRID_HEIGHT, GRID_WIDTH);
    nocterm_decorbox_t* box = nocterm_decorbox_new(NOCTERM_WIDGET(game.grid));
    nocterm_decorbox_set_border(box, nocterm_decorbox_border_shape(NOCTERM_DECORBOX_BORDER_SHAPE_UNICODE_ROUND), NOCTERM_ATTRIBUTE_EMPTY, NOCTERM_ATTRIBUTE_EMPTY);

    game.score_widget = nocterm_widget_new(1, 80, NOCTERM_WIDGET_FOCUSABLE_NO, NOCTERM_WIDGET_TYPE_REAL);

    nocterm_widget_t* container = nocterm_widget_new(GRID_HEIGHT/2 + 5, GRID_WIDTH + 4, NOCTERM_WIDGET_FOCUSABLE_YES, NOCTERM_WIDGET_TYPE_VIRTUAL);
    nocterm_widget_add_subwidget(container, NOCTERM_WIDGET(box));
    nocterm_widget_add_subwidget(container, NOCTERM_WIDGET(game.score_widget));
    
    nocterm_widget_set_position(NOCTERM_WIDGET(box), 1, 2);
    nocterm_widget_set_position(NOCTERM_WIDGET(game.score_widget), GRID_HEIGHT/2 + 3, 2);

    nocterm_widget_add_key_handler(container, handle_key);

    nocterm_page_t* page = nocterm_page_new("Racing Game", 10, container);
    nocterm_page_stack_push(page);

    reset_game();

    nocterm_timer_t* timer = nocterm_timer_create(container, 50, game_tick, NULL);
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
