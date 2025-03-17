#include "../include/game.h"
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

// Velocità di movimento del coccodrillo in microsecondi (300000 = 0.3 secondi)
#define CROCODILE_SPEED 300000

void* crocodile_thread(void* arg) {
    crocodile_args* args = (crocodile_args*)arg;
    game_state* state = args->state;
    int id = args->id;
    
    // Copy required data under lock to avoid holding lock during sleep
    pthread_mutex_lock(&state->crocodiles[id].mutex);
    int lane = (state->crocodiles[id].id / 2) % LANES;
    int direction = (lane % 2 == 0) ? 1 : -1;
    int original_width = state->crocodiles[id].width;
    pthread_mutex_unlock(&state->crocodiles[id].mutex);
    
    // Seed random number generator uniquely for this thread
    srand(time(NULL) ^ id);
    
    while (!state->game_over) {
        // Lock position for update
        pthread_mutex_lock(&state->crocodiles[id].mutex);
        
        // Controlla se il player è su questo coccodrillo per garantire
        // movimento sincronizzato
        bool has_player = (state->player_on_crocodile && state->player_crocodile_id == id);
        
        // Update position
        state->crocodiles[id].x += direction;
        
        // Handle boundary wrapping
        if (direction > 0) {  // Moving right
            if (state->crocodiles[id].x + state->crocodiles[id].width > GAME_WIDTH - 1) {
                // Partially or fully off screen to the right
                int overflow = (state->crocodiles[id].x + state->crocodiles[id].width) - (GAME_WIDTH - 1);
                state->crocodiles[id].width = state->crocodiles[id].width - overflow;
                
                if (state->crocodiles[id].width <= 0) {
                    // Wrap around to left side
                    state->crocodiles[id].x = 1;
                    state->crocodiles[id].width = 1; // Start growing from left side
                    
                    // Se il player è sul coccodrillo che sta sparendo, cade in acqua
                    if (has_player) {
                        pthread_mutex_lock(&state->game_mutex);
                        state->player_on_crocodile = false;
                        state->player_crocodile_id = -1;
                        pthread_mutex_unlock(&state->game_mutex);
                    }
                }
            } else if (state->crocodiles[id].width < original_width) {
                // Keep growing when entering from left
                state->crocodiles[id].width++;
            }
        } else {  // Moving left
            if (state->crocodiles[id].x <= 0) {
                // Partially or fully off screen to the left
                state->crocodiles[id].width = state->crocodiles[id].width - 1;
                state->crocodiles[id].x = 1;
                
                if (state->crocodiles[id].width <= 0) {
                    // Wrap around to right side
                    state->crocodiles[id].x = GAME_WIDTH - 2;
                    state->crocodiles[id].width = 1; // Start growing from right side
                    
                    // Se il player è sul coccodrillo che sta sparendo, cade in acqua
                    if (has_player) {
                        pthread_mutex_lock(&state->game_mutex);
                        state->player_on_crocodile = false;
                        state->player_crocodile_id = -1;
                        pthread_mutex_unlock(&state->game_mutex);
                    }
                }
            } else if (state->crocodiles[id].width < original_width) {
                // Keep growing when entering from right
                state->crocodiles[id].width++;
                state->crocodiles[id].x--;
                
                // Se il player è sul coccodrillo, dobbiamo mantenerlo nella posizione relativa
                if (has_player) {
                    pthread_mutex_lock(&state->player.mutex);
                    state->player.x--;
                    pthread_mutex_unlock(&state->player.mutex);
                }
            }
        }
        
        // Randomly shoot bullet (5% chance)
        if (rand() % 100 < 5) {
            pthread_mutex_lock(&state->game_mutex);
            int bullet_idx = find_free_bullet_slot(state);
            if (bullet_idx >= 0) {
                pthread_mutex_lock(&state->bullets[bullet_idx].pos.mutex);
                
                state->bullets[bullet_idx].pos.c = '@';
                state->bullets[bullet_idx].pos.x = (direction > 0) ? 
                    state->crocodiles[id].x + state->crocodiles[id].width - 1 : state->crocodiles[id].x;
                state->bullets[bullet_idx].pos.y = state->crocodiles[id].y;
                state->bullets[bullet_idx].pos.width = 1;
                state->bullets[bullet_idx].pos.height = 1;
                state->bullets[bullet_idx].pos.active = true;
                state->bullets[bullet_idx].pos.collision = false;
                state->bullets[bullet_idx].direction = direction;
                state->bullets[bullet_idx].is_enemy = true;
                
                pthread_mutex_unlock(&state->bullets[bullet_idx].pos.mutex);
                
                // Create thread arguments
                bullet_args* b_args = malloc(sizeof(bullet_args));
                b_args->state = state;
                b_args->bullet_id = bullet_idx;
                
                // Create thread for bullet
                pthread_create(&state->bullets[bullet_idx].thread_id, NULL, bullet_thread, b_args);
            }
            pthread_mutex_unlock(&state->game_mutex);
        }
        
        // Modificare la parte in crocodile_thread che aggiorna lo stato
        // Al posto di aggiornare direttamente state->crocodiles[id]
        
        // Dopo aver calcolato la nuova posizione
        game_message msg;
        msg.type = MSG_CROCODILE;
        msg.id = id;
        msg.pos = state->crocodiles[id]; // Copiare i valori aggiornati
        msg.direction = direction;
        
        // Invia il messaggio invece di aggiornare direttamente
        buffer_put(&state->event_buffer, &msg);
        
        // Rimuovere la manipolazione del player qui, sarà gestita dal game_thread
        
        pthread_mutex_unlock(&state->crocodiles[id].mutex);
        
        // Sleep to control movement speed (slower than player)
        usleep(CROCODILE_SPEED);
    }
    
    // Free allocated memory for arguments
    free(arg);
    return NULL;
}