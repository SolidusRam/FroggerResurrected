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
        
        // Handle pause toggle
        if (ch == 'p' || ch == 'P') {
            pthread_mutex_lock(&state->game_mutex);
            state->game_paused = !state->game_paused;
            
            // Show pause message when paused
            if (state->game_paused) {
                pthread_mutex_lock(&state->screen_mutex);
                mvprintw(LINES/2, COLS/2-10, "GIOCO IN PAUSA");
                mvprintw(LINES/2+1, COLS/2-15, "Premi 'p' per continuare");
                refresh();
                pthread_mutex_unlock(&state->screen_mutex);
            } else {
                // When unpausing, update the last_update time to avoid time jump
                state->last_update = time(NULL);
                
                // Redraw game state to clear pause message
                draw_game_state(state);
            }
            
            pthread_mutex_unlock(&state->game_mutex);
            continue; // Skip other input processing when toggling pause
        }
        
        // If game is paused, ignore all other inputs except 'p'
        if (state->game_paused) {
            usleep(50000);
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
                // Create left bullet
                create_bullet(
                    state, 
                    state->player.x - 1,       // x position
                    state->player.y,           // y position
                    -1,                        // direction (left)
                    false                      // not an enemy bullet
                );
                
                // Create right bullet
                create_bullet(
                    state, 
                    state->player.x + state->player.width, // x position
                    state->player.y,                       // y position
                    1,                                     // direction (right)
                    false                                  // not an enemy bullet
                );
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