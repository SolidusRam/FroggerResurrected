#include "../include/player.h"



void *player_thread(void *args) {
    struct circular_buffer* buffer = (struct circular_buffer*)args;
    message msg;
    
    // Initialize player message
    msg.type = MSG_PLAYER;
    msg.pos.c = '$';
    msg.pos.x = GAME_WIDTH/2;
    msg.pos.y = GAME_HEIGHT-2;
    msg.pos.width = 5;
    msg.pos.height = 2;
    msg.pos.active = true;
    msg.id = 0;
    msg.status = 1; // Active status
    
    while (1) {
        int ch = getch();
        bool moved = false;
        
        switch (ch) {
            case KEY_UP:
                if(msg.pos.y > 1) {
                    msg.pos.y -= 2; // Move by 2 units up
                    moved = true;
                }
                break;
            case KEY_DOWN:
                if(msg.pos.y < GAME_HEIGHT-msg.pos.height) {
                    msg.pos.y += 2; // Move by 2 units down
                    moved = true;
                }
                break;
            case KEY_LEFT:
                if(msg.pos.x > 1) {
                    msg.pos.x -= 2; // Move by 2 units left
                    moved = true;
                }
                break;
            case KEY_RIGHT:
                if(msg.pos.x < GAME_WIDTH-2-msg.pos.width) {
                    msg.pos.x += 2; // Move by 2 units right 
                    moved = true;
                }
                break;
            case ' ': // Spacebar for shooting
                // Send bullet message
                message bullet_msg;
                bullet_msg.type = MSG_PLAYER;
                bullet_msg.pos.c = '*';
                bullet_msg.pos.x = msg.pos.x;
                bullet_msg.pos.y = msg.pos.y;
                bullet_msg.pos.active = true;
                bullet_msg.status = 1;
                
                buffer_put(buffer, &bullet_msg);
                break;
        }
        //clear buffer input
        flushinp();


        // Only send position update if moved
        if (moved) {
            buffer_put(buffer, &msg);
            
            // Small delay after movement
            usleep(50000);
        }
        
        // Regular update interval
        usleep(20000);
    }
    
    return NULL;
}