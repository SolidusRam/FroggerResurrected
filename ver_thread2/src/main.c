#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <ncurses.h>

#include "../include/game.h"
#include "../include/utils.h"

int main() {
    // Initialize random number generator
    srand(time(NULL));
    
    // Initialize ncurses
    initscr();
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);  // Player color
    init_pair(2, COLOR_RED, COLOR_BLACK);    // Crocodile color
    init_pair(3, COLOR_BLUE, COLOR_CYAN);    // River borders
    init_pair(5, COLOR_YELLOW, COLOR_RED);   // Land color
    init_pair(6, COLOR_GREEN, COLOR_BLACK);  // Empty den
    init_pair(7, COLOR_YELLOW, COLOR_GREEN); // Occupied den
    
    // Check terminal size
    if (LINES < GAME_HEIGHT || COLS < GAME_WIDTH) {
        endwin();
        fprintf(stderr, "Terminal too small. Needs at least %dx%d but got %dx%d\n", 
                GAME_WIDTH, GAME_HEIGHT, COLS, LINES);
        exit(1);
    }
    
    // Set up terminal
    resize_term(GAME_HEIGHT, GAME_WIDTH);
    noecho();
    cbreak();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    curs_set(0);  // Hide cursor
    
    // Create and initialize game state
    game_state state;
    init_game_state(&state);
    
    // Create threads
    pthread_t player_tid, game_tid;
    pthread_t crocodile_tids[MAX_CROCODILES];
    crocodile_args* croc_args[MAX_CROCODILES];
    
    // Start game thread
    pthread_create(&game_tid, NULL, game_thread, &state);
    
    // Start player thread
    pthread_create(&player_tid, NULL, player_thread, &state);
    
    // Start crocodile threads
    for (int i = 0; i < MAX_CROCODILES; i++) {
        croc_args[i] = malloc(sizeof(crocodile_args));
        croc_args[i]->state = &state;
        croc_args[i]->id = i;
        pthread_create(&crocodile_tids[i], NULL, crocodile_thread, croc_args[i]);
    }
    
    // Wait for game thread to finish
    pthread_join(game_tid, NULL);
    
    // Game over, clean up
    for (int i = 0; i < MAX_CROCODILES; i++) {
        pthread_cancel(crocodile_tids[i]);
        free(croc_args[i]); // Free the argument structures
    }
    pthread_cancel(player_tid);
    
    // Clean up bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (state.bullets[i].pos.active) {
            pthread_cancel(state.bullets[i].thread_id);
        }
    }
    
    // Clean up resources
    destroy_game_state(&state);
    
    // Display final message
    clear();
    mvprintw(LINES/2, COLS/2 - 10, "Game Over - Score: %d", state.score);
    refresh();
    sleep(2);
    
    // End ncurses
    endwin();
    
    return 0;
}