#include "../include/game.h"
#include "../include/buffer.h"
#include "../include/utils.h"
#include <unistd.h>

static pthread_mutex_t screen_mutex = PTHREAD_MUTEX_INITIALIZER;

void safe_mvaddch(int y, int x, chtype ch) {
    pthread_mutex_lock(&screen_mutex);
    mvaddch(y, x, ch);
    pthread_mutex_unlock(&screen_mutex);
}

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

    // Initialize crocodile positions
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
    
    // Set up initial display
    clear();
    draw_time_bar(remaining_time, max_time);
    draw_score(score);
    draw_river_borders();
    draw_game_borders();
    
    while (!game_over && vite > 0) {
        time_t current_time = time(NULL);
        
        // Process all available messages in the buffer
        int processed = 0;
        int max_per_cycle = 5;  // Process up to 5 messages per cycle
        
        while (processed < max_per_cycle) {
            // Non-blocking get from buffer
            pthread_mutex_lock(&buffer->mutex);
            if (buffer->count == 0) {
                pthread_mutex_unlock(&buffer->mutex);
                break;
            }
            
            // Get item from buffer
            msg = buffer->array[buffer->head];
            buffer->head = (buffer->head + 1) % buffer->capacity;
            buffer->count--;
            
            pthread_cond_signal(&buffer->not_full);
            pthread_mutex_unlock(&buffer->mutex);
            
            processed++;
            
            // Update positions based on message type
            switch (msg.type) {
                case MSG_PLAYER:
                    // Clear old frog position
                    clear_frog_position(&rana_pos);
                    rana_pos = msg.pos;
                    break;
                    
                case MSG_CROCODILE:
                    // Update crocodile position
                    for (int i = 0; i < MAX_CROCODILES; i++) {
                        if (crocodile_positions[i].id == msg.id) {
                            // Clear old crocodile position
                            if (crocodile_positions[i].active) {
                                for (int h = 0; h < crocodile_positions[i].height; h++) {
                                    for (int w = 0; w < crocodile_positions[i].width; w++) {
                                        mvaddch(crocodile_positions[i].y + h, 
                                                crocodile_positions[i].x + w, ' ');
                                    }
                                }
                            }
                            crocodile_positions[i] = msg.pos;
                            crocodile_positions[i].active = true;
                            break;
                        }
                    }
                    break;
            }
        }
        
        // Now handle all screen updates in one block with mutex protection
        pthread_mutex_lock(&screen_mutex);
        
        // 1. Clear old positions
        clear_frog_position(&rana_pos);
        
        // Clear old crocodile positions
        for (int i = 0; i < MAX_CROCODILES; i++) {
            if (crocodile_positions[i].active) {
                for (int h = 0; h < crocodile_positions[i].height; h++) {
                    for (int w = 0; w < crocodile_positions[i].width; w++) {
                        mvaddch(crocodile_positions[i].y + h, 
                                crocodile_positions[i].x + w, ' ');
                    }
                }
            }
        }
        
        // 2. Draw background elements
        draw_river_borders();
        draw_game_borders();
        draw_score(score);
        mvprintw(LINES-1, GAME_WIDTH-20, "Vite: %d", vite);
        
        // 3. Draw crocodiles
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
        
        // 4. Draw frog (always last so it's on top)
        attron(COLOR_PAIR(1));
        for (int i = 0; i < rana_pos.height; i++) {
            for (int j = 0; j < rana_pos.width; j++) {
                mvaddch(rana_pos.y + i, rana_pos.x + j, rana_sprite[i][j]);
            }
        }
        attroff(COLOR_PAIR(1));
        
        // 5. Single refresh call at the end
        refresh();
        
        pthread_mutex_unlock(&screen_mutex);
        
        // Small delay to control frame rate
        usleep(50000);
    }
    
    return NULL;
}