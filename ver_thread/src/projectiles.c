#include "../include/game.h"
#include <unistd.h>
#include <stdlib.h>

void* bullet_thread(void* arg) {
    bullet_args* args = (bullet_args*)arg;
    game_state* state = args->state;
    int bullet_id = args->bullet_id;
    
    // Run until bullet goes off screen or collides
    while (1) {
        // Check game over condition first
        if (state->game_over) {
            break;
        }
        
        // Check if game is paused
        pthread_mutex_lock(&state->pause_mutex);
        bool is_paused = state->game_paused;
        pthread_mutex_unlock(&state->pause_mutex);
        
        if (is_paused) {
            // If paused, just wait a bit and check again
            usleep(100000);  // 100ms
            continue;
        }
        
        // Lock position for update
        pthread_mutex_lock(&state->bullets[bullet_id].pos.mutex);
        
        // Check if bullet is still active
        if (!state->bullets[bullet_id].pos.active || state->bullets[bullet_id].pos.collision) {
            pthread_mutex_unlock(&state->bullets[bullet_id].pos.mutex);
            break;
        }
        
        // Update position based on direction
        state->bullets[bullet_id].pos.x += state->bullets[bullet_id].direction;
        
        // Check if bullet is out of bounds
        if (state->bullets[bullet_id].pos.x <= 0 || 
            state->bullets[bullet_id].pos.x >= GAME_WIDTH - 1) {
            state->bullets[bullet_id].pos.active = false;
            pthread_mutex_unlock(&state->bullets[bullet_id].pos.mutex);
            break;
        }
        
        // Create message to update bullet position
        game_message msg;
        msg.type = MSG_BULLET;
        msg.id = bullet_id;
        msg.pos = state->bullets[bullet_id].pos;
        msg.is_enemy = state->bullets[bullet_id].is_enemy;
        
        // Send the message to the game thread
        buffer_put(&state->event_buffer, &msg);
        
        pthread_mutex_unlock(&state->bullets[bullet_id].pos.mutex);
        
        // Control bullet speed (faster than player/crocodile)
        usleep(50000); 
    }
    
    // Thread is ending, mark bullet as inactive if not already
    pthread_mutex_lock(&state->bullets[bullet_id].pos.mutex);
    state->bullets[bullet_id].pos.active = false;
    pthread_mutex_unlock(&state->bullets[bullet_id].pos.mutex);
    
    // Free allocated memory for arguments
    free(arg);
    return NULL;
}