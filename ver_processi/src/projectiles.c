#include "../include/projectiles.h"

void bullet(int pipeout, struct position *p, int direction)
{
    struct position bullet = {
        .c = p->c,
        .x = p->x,
        .y = p->y,
        .width = 1,
        .height = 1,
        .active = 1,
        .pid = getpid(),
        .collision = 0
    };

    while(bullet.active) {
        bullet.x += direction;
        
        // Use GAME_WIDTH instead of COLS for consistent boundary checking
        if(bullet.x >= GAME_WIDTH-2 || bullet.x <= 1) {
            // Make sure to clearly mark this bullet as inactive
            bullet.active = 0;
            
            // Send final inactive state before exiting
            write(pipeout, &bullet, sizeof(struct position));

            // Clean up and exit
            close(pipeout);
            _exit(0);
        }
        
        // Send updated position
        write(pipeout, &bullet, sizeof(struct position));
        usleep(50000);
    }
    
    close(pipeout);
    _exit(0);
}