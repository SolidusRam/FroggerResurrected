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
            // Make sure we mark bullet as inactive before exiting
            pthread_mutex_lock(&state->bullets[bullet_id].pos.mutex);
            state->bullets[bullet_id].pos.active = false;
            pthread_mutex_unlock(&state->bullets[bullet_id].pos.mutex);
            break;
        }
        
        // Check pause condition - pause bullets when game is paused
        if (state->game_paused) {
            usleep(50000); // Check pause state every 50ms
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
            
            // Create deactivation message to ensure it gets removed from display
            game_message deactivate_msg;
            deactivate_msg.type = MSG_BULLET;
            deactivate_msg.id = bullet_id;
            deactivate_msg.pos = state->bullets[bullet_id].pos;
            
            pthread_mutex_unlock(&state->bullets[bullet_id].pos.mutex);
            
            // Send final message indicating bullet is inactive
            buffer_put(&state->event_buffer, &deactivate_msg);
            break;
        }
        
        // Create message for buffer
        game_message msg;
        msg.type = MSG_BULLET;
        msg.id = bullet_id;
        msg.pos = state->bullets[bullet_id].pos;
        
        // Unlock before putting message in buffer
        pthread_mutex_unlock(&state->bullets[bullet_id].pos.mutex);
        
        // Put message in buffer
        buffer_put(&state->event_buffer, &msg);
        
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

void create_bullet(game_state* state, int x, int y, int direction, bool is_enemy) {
    pthread_mutex_lock(&state->game_mutex);
    
    int bullet_idx = find_free_bullet_slot(state);
    if (bullet_idx >= 0) {
        pthread_mutex_lock(&state->bullets[bullet_idx].pos.mutex);
        
        // Set bullet properties
        state->bullets[bullet_idx].pos.c = is_enemy ? '@' : '*';
        state->bullets[bullet_idx].pos.x = x;
        state->bullets[bullet_idx].pos.y = y;
        state->bullets[bullet_idx].pos.width = 1;
        state->bullets[bullet_idx].pos.height = 1;
        state->bullets[bullet_idx].pos.active = true;
        state->bullets[bullet_idx].pos.collision = false;
        state->bullets[bullet_idx].direction = direction;
        state->bullets[bullet_idx].is_enemy = is_enemy;
        
        pthread_mutex_unlock(&state->bullets[bullet_idx].pos.mutex);
        
        // Create thread arguments
        bullet_args* args = malloc(sizeof(bullet_args));
        args->state = state;
        args->bullet_id = bullet_idx;
        
        // Create thread for bullet
        pthread_create(&state->bullets[bullet_idx].thread_id, NULL, bullet_thread, args);
    }
    
    pthread_mutex_unlock(&state->game_mutex);
}