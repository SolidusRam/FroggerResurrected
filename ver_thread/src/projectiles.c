#include "../include/game.h"
#include <unistd.h>
#include <stdlib.h>

void* bullet_thread(void* arg) {
    bullet_args* args = (bullet_args*)arg;
    game_state* state = args->state;
    int bullet_id = args->bullet_id;
    
    // Finche il proiettile è attivo
    while (1) {

        if (state->game_over) {
            pthread_mutex_lock(&state->bullets[bullet_id].pos.mutex);
            state->bullets[bullet_id].pos.active = false;
            pthread_mutex_unlock(&state->bullets[bullet_id].pos.mutex);
            break;
        }
        
        // Controllo pausa
        if (state->game_paused) {
            usleep(50000); 
            continue;
        }
        
        //controllo mutex
        pthread_mutex_lock(&state->bullets[bullet_id].pos.mutex);
        
        if (!state->bullets[bullet_id].pos.active || state->bullets[bullet_id].pos.collision) {
            pthread_mutex_unlock(&state->bullets[bullet_id].pos.mutex);
            break;
        }
        
        state->bullets[bullet_id].pos.x += state->bullets[bullet_id].direction;
        
        //controllo out of bounds come nei processi
        if (state->bullets[bullet_id].pos.x <= 0 || 
            state->bullets[bullet_id].pos.x >= GAME_WIDTH - 1) {
            state->bullets[bullet_id].pos.active = false;
            
            // Messaggio per disattivare il proiettile
            game_message deactivate_msg;
            deactivate_msg.type = MSG_BULLET;
            deactivate_msg.id = bullet_id;
            deactivate_msg.pos = state->bullets[bullet_id].pos;
            // Esplicitamente imposta attivo a falso nel messaggio
            deactivate_msg.pos.active = false;
            
            pthread_mutex_unlock(&state->bullets[bullet_id].pos.mutex);
            
            buffer_put(&state->event_buffer, &deactivate_msg);
            break;
        }
        
        // Crea il messaggio per il buffer
        game_message msg;
        msg.type = MSG_BULLET;
        msg.id = bullet_id;
        msg.pos = state->bullets[bullet_id].pos;
        
        pthread_mutex_unlock(&state->bullets[bullet_id].pos.mutex);
        
        // Messaggio buffer
        buffer_put(&state->event_buffer, &msg);
        
        usleep(50000); 
    }


    //Disattiva il proiettile
    pthread_mutex_lock(&state->bullets[bullet_id].pos.mutex);
    state->bullets[bullet_id].pos.active = false;
    pthread_mutex_unlock(&state->bullets[bullet_id].pos.mutex);
    
    // Invia un messaggio finale per assicurarsi che il proiettile venga rimosso dal display
    game_message final_msg;
    final_msg.type = MSG_BULLET;
    final_msg.id = bullet_id;
    pthread_mutex_lock(&state->bullets[bullet_id].pos.mutex);
    final_msg.pos = state->bullets[bullet_id].pos;
    final_msg.pos.active = false;
    pthread_mutex_unlock(&state->bullets[bullet_id].pos.mutex);
    
    // Invia il messaggio finale
    buffer_put(&state->event_buffer, &final_msg);
    
    free(arg);
    return NULL;
}

void create_bullet(game_state* state, int x, int y, int direction, bool is_enemy) {
    pthread_mutex_lock(&state->game_mutex);
    
    int bullet_idx = find_free_bullet_slot(state);
    if (bullet_idx >= 0) {
        pthread_mutex_lock(&state->bullets[bullet_idx].pos.mutex);
        
        // Imposta le proprietà dei proiettili
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
        
        // Creazione thread arguments
        bullet_args* args = malloc(sizeof(bullet_args));
        args->state = state;
        args->bullet_id = bullet_idx;
        
        pthread_create(&state->bullets[bullet_idx].thread_id, NULL, bullet_thread, args);
    }
    
    pthread_mutex_unlock(&state->game_mutex);
}
