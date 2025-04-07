#include "../include/game.h"
#include "../include/utils.h"
#include <stdlib.h>
#include <unistd.h>

// Define the frog sprite
char rana_sprite[2][5] = {
    {' ', ' ', 'O', ' ', ' '},
    {'_', '`', 'O', '\'', '_'}
};

void reset_player_safely(game_state* state) {
    // STEP 1: Reset dei flag
    state->player_on_crocodile = false;
    state->player_crocodile_id = -1;
    
    // STEP 2: Creazione di una nuova posizione per il giocatore
    position new_pos;
    new_pos.c = '$';  
    new_pos.x = GAME_WIDTH/2;
    new_pos.y = GAME_HEIGHT-2;
    new_pos.width = 5;  
    new_pos.height = 2;  
    new_pos.id = 0;
    new_pos.active = true;
    new_pos.collision = false;

    
    // STEP 3: Blocco del mutex atomico
    pthread_mutex_lock(&state->player.mutex);
    state->player.x = GAME_WIDTH/2;
    state->player.y = GAME_HEIGHT-2;
    state->player.active = true;
    state->player.collision = false;
    new_pos = state->player;  // Nuovo messaggio con la posizione aggiornata
    //sblocco
    pthread_mutex_unlock(&state->player.mutex);


    
    // STEP 4: Creazione del messaggio
    game_message reset_msg;
    reset_msg.type = MSG_PLAYER;
    reset_msg.id = 0;
    reset_msg.pos = new_pos;


    buffer_put(&state->event_buffer, &reset_msg);

    // TIMER
    pthread_mutex_lock(&state->game_mutex);
    state->remaining_time = state->max_time;
    pthread_mutex_unlock(&state->game_mutex);



}

// Initialize the game state
void init_game_state(game_state* state) {
    // Initialize mutex and condition variables
    pthread_mutex_init(&state->game_mutex, NULL);
    pthread_mutex_init(&state->screen_mutex, NULL);
    pthread_cond_init(&state->game_update_cond, NULL);

    buffer_init(&state->event_buffer, BUFFER_SIZE);

    
    // Initialize player
    state->player.c = '$';
    state->player.x = GAME_WIDTH/2;
    state->player.y = GAME_HEIGHT-2;
    state->player.width = 5;
    state->player.height = 2;
    state->player.id = 0;
    state->player.active = true;
    state->player.collision = false;
    pthread_mutex_init(&state->player.mutex, NULL);
    
    // Initialize game stats
    state->vite = 3;
    state->score = 0;
    state->game_over = false;
    state->game_paused = false;  // Initialize pause flag
    state->max_time = 30;
    state->remaining_time = state->max_time;
    state->last_update = time(NULL);
    state->tane_occupate = 0;
    state->player_on_crocodile = false;
    state->player_crocodile_id = -1;
    
    // Initialize crocodiles
    for (int i = 0; i < MAX_CROCODILES; i++) {
        int lane = (i/2) % LANES;
        int direction = (lane % 2 == 0) ? 1 : -1;
        int width = (rand() % 2 + 2) * 5;
        int is_second = i % 2;
        int x;
        
        if (is_second) {
            x = (GAME_WIDTH/2) + (rand() % (GAME_WIDTH/4));
        } else {
            x = rand() % (GAME_WIDTH/4);
        }
        
        state->crocodiles[i].c = 'C';
        state->crocodiles[i].x = x;
        state->crocodiles[i].y = 4 + (lane * LANE_HEIGHT);
        state->crocodiles[i].width = width;
        state->crocodiles[i].height = 2;
        state->crocodiles[i].id = i;
        state->crocodiles[i].active = true;
        state->crocodiles[i].collision = false;
        pthread_mutex_init(&state->crocodiles[i].mutex, NULL);
    }
    
    // Initialize bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        state->bullets[i].pos.active = false;
        state->bullets[i].pos.collision = false;
        pthread_mutex_init(&state->bullets[i].pos.mutex, NULL);
    }
    
    // Initialize dens
    init_dens(state->tane);
}

// Clean up 
void destroy_game_state(game_state* state) {
    pthread_mutex_destroy(&state->game_mutex);
    pthread_mutex_destroy(&state->screen_mutex);
    pthread_cond_destroy(&state->game_update_cond);
    
    pthread_mutex_destroy(&state->player.mutex);
    
    for (int i = 0; i < MAX_CROCODILES; i++) {
        pthread_mutex_destroy(&state->crocodiles[i].mutex);
    }
    
    for (int i = 0; i < MAX_BULLETS; i++) {
        pthread_mutex_destroy(&state->bullets[i].pos.mutex);
    }

    buffer_destroy(&state->event_buffer);
}

// Game loop
void* game_thread(void* arg) {
    game_state* state = (game_state*)arg;
    int max_height_reached = GAME_HEIGHT-2; // Altezza piu alta per il punteggio

    // Temporizzatzione dei frame
    const long FRAME_DELAY = 80000; // 80ms = 12.5 fps
    
    while (!state->game_over && state->vite > 0) {

        // Funzione di pausa
        if (state->game_paused) {
            usleep(FRAME_DELAY);
            continue;
        }
        
        // Timer e variabile del tempo  
        time_t current_time = time(NULL);
        if (current_time - state->last_update >= 1) {
            pthread_mutex_lock(&state->game_mutex);
            state->last_update = current_time;
            state->remaining_time--;
            
            // Controlla se il tempo è scaduto
            if (state->remaining_time <= 0) {
                // Imposta game_over prima di tutto il resto
                state->game_over = true;
                
                // Mostra il messaggio mentre si tiene il lock
                pthread_mutex_lock(&state->screen_mutex);
                mvprintw(LINES/2, COLS/2-10, "TEMPO SCADUTO!");
                mvprintw((LINES/2) + 1, COLS/2-10, "SCORE FINALE: %d", state->score);
                refresh();
                pthread_mutex_unlock(&state->screen_mutex);
                
                // Rilascia il mutex prima di bloccare il thread
                pthread_mutex_unlock(&state->game_mutex);
                
                // Pausa breve per mostrare il messaggio
                napms(2000);
                
                // Esci immediatamente dal ciclo
                break;
            }
            pthread_mutex_unlock(&state->game_mutex);
        }
        
        // Rana sopra coccodrillo
        pthread_mutex_lock(&state->player.mutex);
        position player_copy = state->player; 
        pthread_mutex_unlock(&state->player.mutex);
        
        // Solo se il player non è già su un coccodrillo, controlliamo se è atterrato su uno
        if (!state->player_on_crocodile) {
            // Check for new collisions with crocodiles
            for (int i = 0; i < MAX_CROCODILES; i++) {
                pthread_mutex_lock(&state->crocodiles[i].mutex);
                position croc = state->crocodiles[i]; 
                pthread_mutex_unlock(&state->crocodiles[i].mutex);
                
                if (croc.active && 
                    player_copy.y == croc.y && 
                    player_copy.x >= croc.x && 
                    player_copy.x + player_copy.width <= croc.x + croc.width) {
                    
                    state->player_on_crocodile = true;
                    state->player_crocodile_id = i;
                    
                    pthread_mutex_lock(&state->game_mutex);
                    if (player_copy.y < max_height_reached) {
                        state->score += 5;
                        max_height_reached = player_copy.y;
                    }
                    pthread_mutex_unlock(&state->game_mutex);
                    break;
                }
            }
        }
        
        // Caduta in acqua
        if (!state->player_on_crocodile && frog_on_the_water(&player_copy)) {
            pthread_mutex_lock(&state->game_mutex);
            state->vite--;
            max_height_reached = GAME_HEIGHT-2;
            
            if (state->vite > 0) {
                pthread_mutex_unlock(&state->game_mutex);
                
                reset_player_safely(state);

            } else {
                // Game over
                pthread_mutex_lock(&state->screen_mutex);
                mvprintw(LINES/2, COLS/2-10, "GAME OVER!");
                mvprintw((LINES/2) + 1, COLS/2-10, "SCORE FINALE: %d", state->score);
                refresh();
                pthread_mutex_unlock(&state->screen_mutex);
                
                state->game_over = true;
                pthread_mutex_unlock(&state->game_mutex);
                napms(2000);
            }
        }
             
        // Collisione con le tane
        pthread_mutex_lock(&state->player.mutex);
        if (state->player.y <= 1) {
            bool den_reached = false;
       
            for(int i = 0; i < NUM_TANE; i++) {
                if (!state->tane[i].occupata) {
                    // Trovo il centro della rana
                    int frog_center_x = state->player.x + (state->player.width / 2);
                    
                    // Check if frog center is within den bounds
                    if (frog_center_x >= state->tane[i].x && 
                        frog_center_x <= state->tane[i].x + TANA_WIDTH) {
                        
                        pthread_mutex_lock(&state->game_mutex);
                        state->tane[i].occupata = true;
                        state->score += 100;
                        state->tane_occupate++;
                        state->remaining_time = state->max_time;
                        max_height_reached = GAME_HEIGHT-2;
                        den_reached = true;
                        
                        // Check win condition first
                        if (state->tane_occupate == NUM_TANE) {
                            pthread_mutex_lock(&state->screen_mutex);
                            clear();
                            mvprintw(LINES/2, COLS/2-10, "HAI VINTO!");
                            refresh();
                            pthread_mutex_unlock(&state->screen_mutex);
                            state->game_over = true;
                            pthread_mutex_unlock(&state->game_mutex);
                            napms(2000);
                            break;
                        }
                        
                        //Condizione nel quale non si vince
                        pthread_mutex_unlock(&state->game_mutex);
                        
                        // Rilascia il mutex del player prima di chiamare la funzione di reset sicura
                        pthread_mutex_unlock(&state->player.mutex);
                        
                        reset_player_safely(state);

                        // Riacquisiamo il mutex per continuare il check delle tane
                        pthread_mutex_lock(&state->player.mutex);
                        
                        
                        break;
                    }
                }
            }
            
            // Tana non valida
            if (!den_reached) {
                // Modifichiamo statistiche di gioco prima di rilasciare il mutex
                max_height_reached = GAME_HEIGHT-2;
                
                pthread_mutex_unlock(&state->player.mutex);
                
                //tolgo le vite
                pthread_mutex_lock(&state->game_mutex);
                state->vite--;
                bool still_alive = state->vite > 0;
                pthread_mutex_unlock(&state->game_mutex);
                
                if (still_alive) {                   
                    // Usa la funzione sicura per il reset 
                    reset_player_safely(state);  
                } else {
                    // Game over 
                    pthread_mutex_lock(&state->game_mutex);
                    pthread_mutex_lock(&state->screen_mutex);
                    mvprintw(LINES/2, COLS/2-10, "GAME OVER!");
                    mvprintw((LINES/2) + 1, COLS/2-10, "SCORE FINALE: %d", state->score);
                    refresh();
                    pthread_mutex_unlock(&state->screen_mutex);
                    
                    state->game_over = true;
                    pthread_mutex_unlock(&state->game_mutex);
                    napms(2000);
                }
            }
        }
        pthread_mutex_unlock(&state->player.mutex);
        
        // Check bullet rana 
        for (int i = 0; i < MAX_BULLETS; i++) {
            pthread_mutex_lock(&state->bullets[i].pos.mutex);
            bool is_active = state->bullets[i].pos.active;
            bool is_collided = state->bullets[i].pos.collision;
            pthread_mutex_unlock(&state->bullets[i].pos.mutex);
            
            if (!is_active || is_collided) 
                continue;
            
            pthread_mutex_lock(&state->bullets[i].pos.mutex);
            position bullet_pos = state->bullets[i].pos;
            bool is_enemy = state->bullets[i].is_enemy;
            pthread_mutex_unlock(&state->bullets[i].pos.mutex);
            
            if (is_enemy) {
                pthread_mutex_lock(&state->player.mutex);
                
                
                bool hit = false;
                
                // Verifichiamo se il proiettile colpisce un carattere effettivo della rana e non uno spazio
                if (bullet_pos.x >= state->player.x && 
                    bullet_pos.x < state->player.x + state->player.width &&
                    bullet_pos.y >= state->player.y && 
                    bullet_pos.y < state->player.y + state->player.height) {
                    
                    // Calcoliamo la posizione relativa all'interno del pattern della rana
                    int rel_x = bullet_pos.x - state->player.x;
                    int rel_y = bullet_pos.y - state->player.y;
                    
                    // Verifichiamo se il carattere nella posizione del proiettile è effettivamente parte della rana
                    char rana_char = rana_sprite[rel_y][rel_x];
                    hit = (rana_char != ' ');
                }
                
                pthread_mutex_unlock(&state->player.mutex);
                
                if (hit) {
                    pthread_mutex_lock(&state->game_mutex);
                    state->vite--;
                    max_height_reached = GAME_HEIGHT-2;
                    
                    pthread_mutex_lock(&state->bullets[i].pos.mutex);
                    state->bullets[i].pos.collision = true;
                    pthread_mutex_unlock(&state->bullets[i].pos.mutex);
                    
                    if (state->vite > 0) {
                        pthread_mutex_unlock(&state->game_mutex);
                        
                        // Safe reset
                        reset_player_safely(state);
                        
                    } else {
                        // Game over 
                        pthread_mutex_lock(&state->screen_mutex);
                        mvprintw(LINES/2, COLS/2-10, "GAME OVER!");
                        mvprintw((LINES/2) + 1, COLS/2-10, "SCORE FINALE: %d", state->score);
                        refresh();
                        pthread_mutex_unlock(&state->screen_mutex);
                        
                        state->game_over = true;
                        pthread_mutex_unlock(&state->game_mutex);
                        napms(2000);
                    }
                }
            }
            
            // Check for bullet-bullet collisions
            if (is_enemy) {
                for (int j = 0; j < MAX_BULLETS; j++) {
                    if (j == i) continue;
                    
                    pthread_mutex_lock(&state->bullets[j].pos.mutex);
                    bool j_active = state->bullets[j].pos.active;
                    bool j_enemy = state->bullets[j].is_enemy;
                    bool j_collided = state->bullets[j].pos.collision;
                    position j_pos = state->bullets[j].pos;
                    int j_direction = state->bullets[j].direction;
                    pthread_mutex_unlock(&state->bullets[j].pos.mutex);
                    
                    if (!j_active || j_enemy || j_collided)
                        continue;
                    
                    // Memorizza la posizione precedente per rilevare "attraversamenti"
                    int prev_i_x = bullet_pos.x - state->bullets[i].direction;
                    int prev_j_x = j_pos.x - j_direction;
                    int next_i_x = bullet_pos.x + state->bullets[i].direction;
                    int next_j_x = j_pos.x + j_direction;
                    
                    // Verifica diversi tipi di collisione
                    bool direct_collision = (bullet_pos.x == j_pos.x && bullet_pos.y == j_pos.y);
                    bool crossed_paths = (bullet_pos.x == prev_j_x && j_pos.x == prev_i_x && 
                                       bullet_pos.y == j_pos.y &&
                                       state->bullets[i].direction != j_direction);
                    
                    // Verifica se i proiettili saranno molto vicini nel prossimo frame
                    bool will_cross_next = (next_i_x == j_pos.x || bullet_pos.x == next_j_x) && 
                                          bullet_pos.y == j_pos.y &&
                                          state->bullets[i].direction != j_direction;
                                          
                    // Verifica se i proiettili sono adiacenti con direzioni opposte
                    bool adjacent_opposing = (bullet_pos.x == j_pos.x + j_direction &&
                                           j_pos.x == bullet_pos.x + state->bullets[i].direction &&
                                           bullet_pos.y == j_pos.y &&
                                           state->bullets[i].direction != j_direction);
                    
                    if (direct_collision || crossed_paths || will_cross_next || adjacent_opposing) {
                        // Bullets collided
                        pthread_mutex_lock(&state->game_mutex);
                        state->score += 50;
                        
                        // Visualizzazione temporanea della collisione
                        pthread_mutex_lock(&state->screen_mutex);
                        // Determina dove visualizzare la collisione
                        int collision_x;
                        if (direct_collision) {
                            collision_x = bullet_pos.x;
                        } else if (crossed_paths || adjacent_opposing) {
                            collision_x = (bullet_pos.x + j_pos.x) / 2;
                        } else { // will_cross_next
                            collision_x = (next_i_x + next_j_x) / 2;
                        }
                        mvprintw(bullet_pos.y, collision_x, "X");
                        refresh();
                        pthread_mutex_unlock(&state->screen_mutex);
                        
                        pthread_mutex_unlock(&state->game_mutex);
                        
                        // Disattiva entrambi i proiettili
                        pthread_mutex_lock(&state->bullets[i].pos.mutex);
                        state->bullets[i].pos.collision = true;
                        state->bullets[i].pos.active = false;  // Imposta come inattivo
                        pthread_mutex_unlock(&state->bullets[i].pos.mutex);
                        
                        pthread_mutex_lock(&state->bullets[j].pos.mutex);
                        state->bullets[j].pos.collision = true;
                        state->bullets[j].pos.active = false;  // Imposta come inattivo
                        pthread_mutex_unlock(&state->bullets[j].pos.mutex);
                        
                        // Invia messaggi finali per aggiornare lo stato dei proiettili
                        game_message msg_i, msg_j;
                        
                        msg_i.type = MSG_BULLET;
                        msg_i.id = i;
                        msg_i.pos = state->bullets[i].pos;
                        msg_i.pos.active = false;
                        msg_i.pos.collision = true;
                        
                        msg_j.type = MSG_BULLET;
                        msg_j.id = j;
                        msg_j.pos = state->bullets[j].pos;
                        msg_j.pos.active = false;
                        msg_j.pos.collision = true;
                        
                        // Invia i messaggi
                        buffer_put(&state->event_buffer, &msg_i);
                        buffer_put(&state->event_buffer, &msg_j);
                        
                        break;
                    }
                }
            }
        }
        
        // Nel game_thread, il consumo dei messaggi
        game_message msg;

        // Processa tutti i messaggi disponibili
        while (buffer_try_get(&state->event_buffer, &msg)) {
            switch (msg.type) {
                case MSG_PLAYER:
                    // Aggiorna posizione player
                    pthread_mutex_lock(&state->player.mutex);
                    state->player = msg.pos;
                    pthread_mutex_unlock(&state->player.mutex);
                    break;
                    
                case MSG_CROCODILE:
                    // Aggiorna posizione coccodrillo
                    pthread_mutex_lock(&state->crocodiles[msg.id].mutex);
                    state->crocodiles[msg.id] = msg.pos;
                    pthread_mutex_unlock(&state->crocodiles[msg.id].mutex);
                    
                    // Se il player è sul coccodrillo, aggiorna posizione player
                    if (state->player_on_crocodile && state->player_crocodile_id == msg.id) {
                        pthread_mutex_lock(&state->player.mutex);
                        state->player.x += msg.direction;
                        pthread_mutex_unlock(&state->player.mutex);
                    }
                    break;
                    
                case MSG_BULLET:
                    // Aggiorna posizione proiettile
                    pthread_mutex_lock(&state->bullets[msg.id].pos.mutex);
                    state->bullets[msg.id].pos = msg.pos;
                    pthread_mutex_unlock(&state->bullets[msg.id].pos.mutex);
                    break;
            }
        }

        draw_game_state(state);
        
        // Sleep for stable frame rate
        usleep(FRAME_DELAY);
    }
    
    return NULL;
}

bool rana_coccodrillo(position* rana_pos, position crocodile_positions[], int num_coccodrilli, int* direction) {
    for (int i = 0; i < num_coccodrilli; i++) {
        // Controlla se la rana e' sopra il coccodrillo
        if (rana_pos->y == crocodile_positions[i].y && 
            rana_pos->x >= crocodile_positions[i].x && 
            rana_pos->x + rana_pos->width <= crocodile_positions[i].x + crocodile_positions[i].width) {
            
            int lane = (crocodile_positions[i].id/2) % LANES;
            *direction = (lane % 2 == 0) ? 1 : -1;
            return true; // Frog is on crocodile
        }
    }
    return false; // Frog is not on crocodile
}

bool frog_on_the_water(position* rana_pos) {
    if (rana_pos->y < FLOOR_HEIGHT && rana_pos->y > 3) {
        return true;
    }
    return false;
}

//controllo sul'array dei proiettili
// per trovare un proiettile libero
int find_free_bullet_slot(game_state* state) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        pthread_mutex_lock(&state->bullets[i].pos.mutex);
        bool is_active = state->bullets[i].pos.active;
        pthread_mutex_unlock(&state->bullets[i].pos.mutex);
        
        if (!is_active) {
            return i; //ritorna l'indice del proiettile libero
        }
    }
    return -1; // No free slots
}