#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

#include "../include/game.h"
#include "../include/player.h"
#include "../include/crocodile.h"
#include "../include/buffer.h"

int main(){
    /*
    // Buffer operations
    void buffer_init(circular_buffer* buf, int capacity);
    void buffer_destroy(circular_buffer* buf);
    void buffer_put(circular_buffer* buf, struct position* item);
    void buffer_get(circular_buffer* buf, struct position* item);

    // Thread functions
    void* player_thread(void* arg);
    void* crocodile_thread(void* arg);
    void* projectile_thread(void* arg);
    void* game_thread(void* arg);
    */

    //inizializzo schermo
    initscr();

    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);//colore rana
    init_pair(2, COLOR_RED, COLOR_BLACK);//colore coccodrillo
    init_pair(3, COLOR_BLUE, COLOR_CYAN);    // River borders
    init_pair(6, COLOR_GREEN, COLOR_BLACK);  // Empty den
    init_pair(7, COLOR_YELLOW, COLOR_GREEN); // Occupied den


    //controllo dimensioni schermo
    if (LINES < GAME_HEIGHT || COLS < GAME_WIDTH) {
        endwin();
        fprintf(stderr, "Terminal too small. Needs at least %dx%d\n", GAME_WIDTH, GAME_HEIGHT);
        exit(1);
    }
    

    
    resize_term(GAME_HEIGHT, GAME_WIDTH);
    
    //inizializzo ncurses
    noecho();cbreak();nodelay(stdscr, TRUE);keypad(stdscr, TRUE);
    curs_set(0); // Hide cursor


    // Initialize buffer
    circular_buffer game_buffer;
    buffer_init(&game_buffer, BUFFER_SIZE);

    pthread_t player_tid, game_tid;
    pthread_t crocodile_tids[MAX_CROCODILES];
    
    // Create threads with minimal arguments
    pthread_create(&player_tid, NULL, player_thread, &game_buffer);
    pthread_create(&game_tid, NULL, game_thread, &game_buffer);

    // Create crocodile threads
    // Create crocodile threads
    /*for (int i = 0; i < MAX_CROCODILES; i++) {
        message* init_msg = malloc(sizeof(message));
        init_msg->type = MSG_CROCODILE;
        init_msg->id = i;
        init_msg->pos.active = true;
        
        pthread_create(&crocodile_tids[i], NULL, crocodile_thread, init_msg);
    }*/

    // Wait for game thread
    pthread_join(game_tid, NULL);

    // Cleanup
    buffer_destroy(&game_buffer);
    for (int i = 0; i < LANES * 2; i++) {
        pthread_cancel(crocodile_tids[i]);
    }
    pthread_cancel(player_tid);

    endwin();
    return 0;


}