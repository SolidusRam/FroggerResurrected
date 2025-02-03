#include "../include/game.h"
#include "../include/buffer.h"
#include "../include/utils.h"
#include <unistd.h>


char rana_sprite[2][5] = {
    {' ', ' ', 'O', ' ', ' '},
    {'_', '`', 'O', '\'', '_'}
};

void* game_thread(void* arg) {
    circular_buffer* buffer = (circular_buffer*)arg;
    message msg;
    position p;
    int vite = 3;
    int score = 0;
    bool game_over = false;

    position crocodile_positions[MAX_CROCODILES];
    
    // Initialize ranapos
    position rana_pos = {
        .c = '$',
        .x = GAME_WIDTH/2,
        .y = GAME_HEIGHT-2,
        .width = 5,
        .height = 2
    };

    //initialize crocodile positions
    for (int i = 0; i < MAX_CROCODILES; i++) {
        int lane = i % LANES;
        crocodile_positions[i] = (position) {
            .c = 'C',
            .x = (i % 2 == 0) ? 1 : GAME_WIDTH - 6,
            .y = 4 + (lane * LANE_HEIGHT),
            .width = (rand() % 2 + 3) * 5,
            .height = 2,
            .id = i,
            .active = false
        };
    }

    // Initialize time variables
    int max_time = 30;
    int remaining_time = max_time;
    time_t last_update = time(NULL);
    
    // Initialize display
    draw_time_bar(remaining_time, max_time);
    draw_score(score);
    draw_river_borders();
    
    while (!game_over && vite > 0) {
        time_t current_time = time(NULL);
        
        // Get frog position from buffer
        buffer_get(buffer, &msg);
        
        
        // Clear old position
        clear_frog_position(&rana_pos);
        
        // Update position
        switch (msg.type)
        {
        case MSG_PLAYER:
            rana_pos = msg.pos;
            break;
        case MSG_CROCODILE:
        for (int i = 0; i < MAX_CROCODILES; i++) {
            if (crocodile_positions[i].id == msg.id) {
                crocodile_positions[i] = msg.pos;
                crocodile_positions[i].active = true;  // Important!
                break;
            }
        }
        break;
        }

        
        
        // Draw game elements
        draw_river_borders();
        draw_game_borders();
        draw_score(score);
        mvprintw(LINES-1, GAME_WIDTH-20, "Vite: %d", vite);
        
        // Draw frog
        attron(COLOR_PAIR(1));
        for (int i = 0; i < rana_pos.height; i++) {
            for (int j = 0; j < rana_pos.width; j++) {
                mvaddch(rana_pos.y + i, rana_pos.x + j, rana_sprite[i][j]);
            }
        }
        attroff(COLOR_PAIR(1));

        // Draw crocodiles
        attron(COLOR_PAIR(2));
        for (int i = 0; i < MAX_CROCODILES; i++) {
            if (crocodile_positions[i].active) {
                for (int h = 0; h < crocodile_positions[i].height; h++) {
                    for (int w = 0; w < crocodile_positions[i].width; w++) {
                        mvaddch(crocodile_positions[i].y + h, 
                            crocodile_positions[i].x + w, 'C');
                    }
                }
            }
        }
        attroff(COLOR_PAIR(2));
        refresh();
        
        refresh();
        usleep(50000);
    }
    
    return NULL;
}