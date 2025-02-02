#include "../include/player.h"



void *player_thread(void *args) {
    struct circular_buffer* buffer = (struct circular_buffer*)args;
    message msg;
    position old_pos;
    
    // Initialize player message
    msg.type = MSG_PLAYER;
    msg.pos.c = '$';
    msg.pos.x = GAME_WIDTH/2;
    msg.pos.y = GAME_HEIGHT-2;
    msg.pos.width = 5;
    msg.pos.height = 2;
    msg.pos.active = true;
    msg.id = 0;
    
    // Keep track of previous position
    old_pos = msg.pos;
    
    while (1) {
        int ch = getch();
        bool moved = false;
        
        // Save old position before updating
        old_pos = msg.pos;
        
        switch (ch) {
            case KEY_UP:
                if(msg.pos.y > 1) {
                    msg.pos.y -= msg.pos.height;
                    moved = true;
                }
                break;
            case KEY_DOWN:
                if(msg.pos.y < LINES-2-msg.pos.height+1) {
                    msg.pos.y += msg.pos.height;
                    moved = true;
                }
                break;
            case KEY_LEFT:
                if(msg.pos.x > 1) {
                    msg.pos.x -= msg.pos.width;
                    moved = true;
                }
                break;
            case KEY_RIGHT:
                if(msg.pos.x < COLS-2-msg.pos.width+1) {
                    msg.pos.x += msg.pos.width;
                    moved = true;
                }
                break;
            case 'q':
                msg.pos.x = GAME_WIDTH/2;
                msg.pos.y = 2;
                moved = true;
                break;
        }
        
        // Only send message if position actually changed
        if (moved) {
            //svuoto il buffer di input
            while ((ch = getch()) != ERR);

            buffer_put(buffer, &msg);
            // Add a small delay after movement
            usleep(50000);
            
        }
        
        // Shorter sleep when not moving
        usleep(50000);
    }
    
    return NULL;
}