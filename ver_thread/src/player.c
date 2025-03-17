#include "../include/game.h"
#include <unistd.h>
#include <stdlib.h>

void* player_thread(void* arg) {
    game_state* state = (game_state*)arg;
    
    while (!state->game_over) {
        int ch = getch();
        if (ch == ERR) {
            // No input, just continue waiting
            usleep(20000);
            continue;
        }
        
        // Handle pause key separately from player movement
        if (ch == 'p' || ch == 'P') {
            pthread_mutex_lock(&state->pause_mutex);
            state->game_paused = !state->game_paused;  // Toggle pause state
            
            if (!state->game_paused) {
                // Game is being unpaused, signal the condition
                pthread_cond_signal(&state->pause_cond);
            }
            
            pthread_mutex_unlock(&state->pause_mutex);
            
            // After toggling pause, skip the rest of the input handling
            continue;
        }
        
        // Skip movement handling if game is paused
        pthread_mutex_lock(&state->pause_mutex);
        bool is_paused = state->game_paused;
        pthread_mutex_unlock(&state->pause_mutex);
        
        if (is_paused) {
            // Don't process movement if paused
            continue;
        }
        
        // Lock player position for safe updates
        pthread_mutex_lock(&state->player.mutex);
        
        // Make a copy of current position for collision checks
        int prev_x = state->player.x;
        int prev_y = state->player.y;
        bool was_on_crocodile = state->player_on_crocodile;
        
        switch (ch) {
            case KEY_UP:
                if (state->player.y > 1) {
                    state->player.y -= state->player.height;
                }
                break;
            case KEY_DOWN:
                if (state->player.y < LINES - 2 - state->player.height + 1) {
                    state->player.y += state->player.height;
                }
                break;
            case KEY_LEFT:
                if (state->player.x > 1) {
                    state->player.x -= 2;
                }
                break;
            case KEY_RIGHT:
                if (state->player.x < COLS - 2 - state->player.width + 1) {
                    state->player.x += 2;
                }
                break;
            case ' ': // Spacebar for shooting
                // Create bullets (left and right)
                pthread_mutex_lock(&state->game_mutex);
                
                // Left bullet
                int left_idx = find_free_bullet_slot(state);
                if (left_idx >= 0) {
                    pthread_mutex_lock(&state->bullets[left_idx].pos.mutex);
                    state->bullets[left_idx].pos.c = '*';
                    state->bullets[left_idx].pos.x = state->player.x - 1;
                    state->bullets[left_idx].pos.y = state->player.y;
                    state->bullets[left_idx].pos.width = 1;
                    state->bullets[left_idx].pos.height = 1;
                    state->bullets[left_idx].pos.active = true;
                    state->bullets[left_idx].pos.collision = false;
                    state->bullets[left_idx].direction = -1;
                    state->bullets[left_idx].is_enemy = false;
                    pthread_mutex_unlock(&state->bullets[left_idx].pos.mutex);
                    
                    // Create thread arguments
                    bullet_args* left_args = malloc(sizeof(bullet_args));
                    left_args->state = state;
                    left_args->bullet_id = left_idx;
                    
                    // Create thread for bullet
                    pthread_create(&state->bullets[left_idx].thread_id, NULL, 
                                  bullet_thread, left_args);
                }
                
                // Right bullet
                int right_idx = find_free_bullet_slot(state);
                if (right_idx >= 0) {
                    pthread_mutex_lock(&state->bullets[right_idx].pos.mutex);
                    state->bullets[right_idx].pos.c = '*';
                    state->bullets[right_idx].pos.x = state->player.x + state->player.width;
                    state->bullets[right_idx].pos.y = state->player.y;
                    state->bullets[right_idx].pos.width = 1;
                    state->bullets[right_idx].pos.height = 1;
                    state->bullets[right_idx].pos.active = true;
                    state->bullets[right_idx].pos.collision = false;
                    state->bullets[right_idx].direction = 1;
                    state->bullets[right_idx].is_enemy = false;
                    pthread_mutex_unlock(&state->bullets[right_idx].pos.mutex);
                    
                    // Create thread arguments
                    bullet_args* right_args = malloc(sizeof(bullet_args));
                    right_args->state = state;
                    right_args->bullet_id = right_idx;
                    
                    // Create thread for bullet
                    pthread_create(&state->bullets[right_idx].thread_id, NULL, 
                                  bullet_thread, right_args);
                }
                
                pthread_mutex_unlock(&state->game_mutex);
                break;
                
            case 'q':  // Cheat code (for testing)
                state->player.x = GAME_WIDTH/2;
                state->player.y = 2;
                break;
                
            case 'Q':  // Quit game
                state->game_over = true;
                break;
        }
        
        // Ensure player stays within bounds
        if (state->player.x < 1) 
            state->player.x = 1;
        if (state->player.x > GAME_WIDTH - state->player.width - 1) 
            state->player.x = GAME_WIDTH - state->player.width - 1;
        
        // If player moved horizontally or vertically, reset crocodile association
        if ((prev_x != state->player.x || prev_y != state->player.y) && 
            was_on_crocodile) {
            // When player moves by themselves, they are no longer riding a crocodile
            // The game thread will re-detect if they landed on another crocodile
            state->player_on_crocodile = false;
            state->player_crocodile_id = -1;
        }
        
        game_message msg;
        msg.type = MSG_PLAYER;
        msg.id = 0; // Player ID è sempre 0
        msg.pos = state->player; // Copiare i valori aggiornati
        buffer_put(&state->event_buffer, &msg);

        pthread_mutex_unlock(&state->player.mutex);
        
        // Small delay to prevent CPU hogging and reduce input repetition
        usleep(50000);
    }
    
    return NULL;
}