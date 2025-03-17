#include "../include/game.h"
#include "../include/utils.h"
#include <stdlib.h>
#include <unistd.h>

// Define the frog sprite
char rana_sprite[2][5] = {
    {' ', ' ', 'O', ' ', ' '},
    {'_', '`', 'O', '\'', '_'}
};

// Initialize the game state
void init_game_state(game_state* state) {
    // Initialize mutex and condition variables
    pthread_mutex_init(&state->game_mutex, NULL);
    pthread_mutex_init(&state->screen_mutex, NULL);
    pthread_mutex_init(&state->pause_mutex, NULL);
    pthread_cond_init(&state->game_update_cond, NULL);
    pthread_cond_init(&state->pause_cond, NULL);

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
    state->game_paused = false;  // Game starts unpaused
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

// Clean up resources
void destroy_game_state(game_state* state) {
    pthread_mutex_destroy(&state->game_mutex);
    pthread_mutex_destroy(&state->screen_mutex);
    pthread_mutex_destroy(&state->pause_mutex);
    pthread_cond_destroy(&state->game_update_cond);
    pthread_cond_destroy(&state->pause_cond);
    
    pthread_mutex_destroy(&state->player.mutex);
    
    for (int i = 0; i < MAX_CROCODILES; i++) {
        pthread_mutex_destroy(&state->crocodiles[i].mutex);
    }
    
    for (int i = 0; i < MAX_BULLETS; i++) {
        pthread_mutex_destroy(&state->bullets[i].pos.mutex);
    }

    buffer_destroy(&state->event_buffer);
}

// Main game loop thread
void* game_thread(void* arg) {
    game_state* state = (game_state*)arg;
    int max_height_reached = GAME_HEIGHT-2; // Track highest position for scoring

    // Time between screen updates (milliseconds)
    const long FRAME_DELAY = 80000; // 80ms = 12.5 fps
    
    while (!state->game_over && state->vite > 0) {
        // Check if game is paused
        pthread_mutex_lock(&state->pause_mutex);
        while (state->game_paused && !state->game_over) {
            // Display pause message
            pthread_mutex_lock(&state->screen_mutex);
            mvprintw(LINES/2, COLS/2-10, "GIOCO IN PAUSA");
            mvprintw(LINES/2+1, COLS/2-15, "Premi 'p' per continuare");
            refresh();
            pthread_mutex_unlock(&state->screen_mutex);
            
            // Wait for pause condition signal
            pthread_cond_wait(&state->pause_cond, &state->pause_mutex);
        }
        pthread_mutex_unlock(&state->pause_mutex);
        
        // Skip game logic if game is over
        if (state->game_over) {
            break;
        }
        
        // Update timer
        time_t current_time = time(NULL);
        if (current_time - state->last_update >= 1) {
            pthread_mutex_lock(&state->game_mutex);
            state->last_update = current_time;
            state->remaining_time--;
            
            // Check if time is up
            if (state->remaining_time <= 0) {
                pthread_mutex_lock(&state->screen_mutex);
                mvprintw(LINES/2, COLS/2-10, "TEMPO SCADUTO!");
                refresh();
                pthread_mutex_unlock(&state->screen_mutex);
                napms(2000);
                state->game_over = true;
            }
            pthread_mutex_unlock(&state->game_mutex);
        }
        
        // Check if player is on any crocodile
        pthread_mutex_lock(&state->player.mutex);
        position player_copy = state->player; // Make a copy to avoid holding lock during checks
        pthread_mutex_unlock(&state->player.mutex);
        
        // Solo se il player non è già su un coccodrillo, controlliamo se è atterrato su uno
        if (!state->player_on_crocodile) {
            // Check for new collisions with crocodiles
            for (int i = 0; i < MAX_CROCODILES; i++) {
                pthread_mutex_lock(&state->crocodiles[i].mutex);
                position croc = state->crocodiles[i]; // Copy to avoid holding lock
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
        
        // Player fell in water check
        if (!state->player_on_crocodile && frog_on_the_water(&player_copy)) {
            // IMPORTANT: First unlock all mutexes before getting new ones to prevent deadlocks
            pthread_mutex_lock(&state->game_mutex);
            state->score = 0;
            state->vite--;
            max_height_reached = GAME_HEIGHT-2;
            
            if (state->vite > 0) {
                // Show message first without holding player mutex
                pthread_mutex_lock(&state->screen_mutex);
                mvprintw(LINES/2, COLS/2-10, "RANA IN ACQUA!");
                refresh();
                pthread_mutex_unlock(&state->screen_mutex);
                
                // CRITICAL FIX: Unlock game_mutex before locking player_mutex to avoid deadlock
                pthread_mutex_unlock(&state->game_mutex);
                
                // STEP 1: Reset flags
                state->player_on_crocodile = false;
                state->player_crocodile_id = -1;
                
                // STEP 2: Lock player position for updates
                pthread_mutex_lock(&state->player.mutex);
                
                // STEP 3: Reset position
                state->player.x = GAME_WIDTH/2;
                state->player.y = GAME_HEIGHT-2;
                state->player.active = true;
                state->player.collision = false;
                
                // Create reset message
                game_message reset_msg;
                reset_msg.type = MSG_PLAYER;
                reset_msg.id = 0;
                reset_msg.pos = state->player;
                
                // Unlock player mutex
                pthread_mutex_unlock(&state->player.mutex);
                
                // STEP 4: Now put the message in the buffer - critical to ensure player thread sees the change
                buffer_put(&state->event_buffer, &reset_msg);
                
                // STEP 5: Lock game mutex again to update timer
                pthread_mutex_lock(&state->game_mutex);
                state->remaining_time = state->max_time;
                pthread_mutex_unlock(&state->game_mutex);
                
                // STEP 6: Small delay to allow message processing
                napms(1000); // Reduced from 2000 to improve responsiveness
            } else {
                // Game over
                pthread_mutex_lock(&state->screen_mutex);
                mvprintw(LINES/2, COLS/2-10, "GAME OVER!");
                refresh();
                pthread_mutex_unlock(&state->screen_mutex);
                state->game_over = true;
                pthread_mutex_unlock(&state->game_mutex);
                napms(2000);
            }
        }
        
        // Check if player reached a den
        pthread_mutex_lock(&state->player.mutex);
        if (state->player.y <= 1) {
            bool den_reached = false;
            
            // Check for collision with a den
            for(int i = 0; i < NUM_TANE; i++) {
                if (!state->tane[i].occupata) {
                    // Get center of frog
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
                        
                        // If not won yet, reset player position
                        pthread_mutex_unlock(&state->game_mutex);
                        
                        // STEP 1: Reset flags
                        state->player_on_crocodile = false;
                        state->player_crocodile_id = -1;
                        
                        // STEP 2: Reset player position (already have player mutex from outer scope)
                        state->player.x = GAME_WIDTH/2;
                        state->player.y = GAME_HEIGHT-2;
                        state->player.active = true;
                        state->player.collision = false;
                        
                        // STEP 3: Create reset message
                        game_message reset_msg;
                        reset_msg.type = MSG_PLAYER;
                        reset_msg.id = 0;
                        reset_msg.pos = state->player;
                        
                        // STEP 4: Put message in buffer (player mutex still locked in outer scope)
                        buffer_put(&state->event_buffer, &reset_msg);
                        
                        break;
                    }
                }
            }
        }
        pthread_mutex_unlock(&state->player.mutex);
        
        // Check bullet collisions
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
            
            // Check for bullet collision with player
            if (is_enemy) {
                pthread_mutex_lock(&state->player.mutex);
                
                // Nuova logica di rilevamento della collisione più precisa
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
                    state->score = 0;
                    state->vite--;
                    max_height_reached = GAME_HEIGHT-2;
                    
                    pthread_mutex_lock(&state->bullets[i].pos.mutex);
                    state->bullets[i].pos.collision = true;
                    pthread_mutex_unlock(&state->bullets[i].pos.mutex);
                    
                    if (state->vite > 0) {
                        // Show message first
                        pthread_mutex_lock(&state->screen_mutex);
                        mvprintw(LINES/2, COLS/2-10, "RANA COLPITA! Vite: %d", state->vite);
                        refresh();
                        pthread_mutex_unlock(&state->screen_mutex);
                        
                        // CRITICAL FIX: Unlock game_mutex before locking player_mutex
                        pthread_mutex_unlock(&state->game_mutex);
                        
                        // STEP 1: Reset flags
                        state->player_on_crocodile = false;
                        state->player_crocodile_id = -1;
                        
                        // STEP 2: Lock player position for updates
                        pthread_mutex_lock(&state->player.mutex);
                        
                        // STEP 3: Reset position
                        state->player.x = GAME_WIDTH/2;
                        state->player.y = GAME_HEIGHT-2;
                        state->player.active = true;
                        state->player.collision = false;
                        
                        // Create reset message
                        game_message reset_msg;
                        reset_msg.type = MSG_PLAYER;
                        reset_msg.id = 0;
                        reset_msg.pos = state->player;
                        
                        // Unlock player mutex
                        pthread_mutex_unlock(&state->player.mutex);
                        
                        // STEP 4: Put message in buffer to notify player thread
                        buffer_put(&state->event_buffer, &reset_msg);
                        
                        // STEP 5: Update timer
                        pthread_mutex_lock(&state->game_mutex);
                        state->remaining_time = state->max_time;
                        pthread_mutex_unlock(&state->game_mutex);
                        
                        // STEP 6: Small delay for message processing
                        napms(1000);
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
                    pthread_mutex_unlock(&state->bullets[j].pos.mutex);
                    
                    if (!j_active || j_enemy || j_collided)
                        continue;
                    
                    if (bullet_pos.x == j_pos.x && bullet_pos.y == j_pos.y) {
                        // Bullets collided
                        pthread_mutex_lock(&state->game_mutex);
                        state->score += 50;
                        pthread_mutex_unlock(&state->game_mutex);
                        
                        pthread_mutex_lock(&state->bullets[i].pos.mutex);
                        state->bullets[i].pos.collision = true;
                        pthread_mutex_unlock(&state->bullets[i].pos.mutex);
                        
                        pthread_mutex_lock(&state->bullets[j].pos.mutex);
                        state->bullets[j].pos.collision = true;
                        pthread_mutex_unlock(&state->bullets[j].pos.mutex);
                        
                        break;
                    }
                }
            }
        }
        
        // Nel game_thread, aggiungere il consumo dei messaggi
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

        // Continua con il normale processing del gioco

        // Redraw game state - using our improved double buffering function
        // This will handle all drawing without screen flicker
        draw_game_state(state);
        
        // Sleep for stable frame rate
        usleep(FRAME_DELAY);
    }
    
    return NULL;
}

bool rana_coccodrillo(position* rana_pos, position crocodile_positions[], int num_coccodrilli, int* direction) {
    for (int i = 0; i < num_coccodrilli; i++) {
        // Check if frog is on crocodile
        if (rana_pos->y == crocodile_positions[i].y && 
            rana_pos->x >= crocodile_positions[i].x && 
            rana_pos->x + rana_pos->width <= crocodile_positions[i].x + crocodile_positions[i].width) {
            
            // Set the direction based on the crocodile's lane
            int lane = (crocodile_positions[i].id/2) % LANES;
            *direction = (lane % 2 == 0) ? 1 : -1;
            return true; // Frog is on crocodile
        }
    }
    return false; // Frog is not on any crocodile
}

bool frog_on_the_water(position* rana_pos) {
    if (rana_pos->y < FLOOR_HEIGHT && rana_pos->y > 3) {
        return true;
    }
    return false;
}

// Find an available bullet slot
int find_free_bullet_slot(game_state* state) {
    for (int i = 0; i < MAX_BULLETS; i++) {
        pthread_mutex_lock(&state->bullets[i].pos.mutex);
        bool is_active = state->bullets[i].pos.active;
        pthread_mutex_unlock(&state->bullets[i].pos.mutex);
        
        if (!is_active) {
            return i;
        }
    }
    return -1; // No free slots
}