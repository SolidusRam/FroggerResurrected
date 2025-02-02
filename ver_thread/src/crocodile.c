#include "../include/crocodile.h"
#include "../include/buffer.h"
#include <unistd.h>
#include <time.h>

void *crocodile_thread(void *args) {
    struct circular_buffer* buffer = (struct circular_buffer*)args;
    message msg;
    int direction = 0;
    int original_width;
    
    // Initialize crocodile message
    msg.type = MSG_CROCODILE;
    msg.pos.c = 'C';
    msg.pos.width = (rand() % 2 + 2) * 5;
    msg.pos.height = 2;
    original_width = msg.pos.width;
    
    // Set lane and direction based on thread ID
    int lane = (msg.id/2) % LANES;
    direction = (lane % 2 == 0) ? 1 : -1;
    
    // Set initial position
    int is_second = msg.id % 2;
    if (is_second) {
        msg.pos.x = (GAME_WIDTH/2) + (rand() % (GAME_WIDTH/4));
    } else {
        msg.pos.x = rand() % (GAME_WIDTH/4);
    }
    msg.pos.y = 4 + (lane * LANE_HEIGHT);
    
    while (1) {
        // Handle border collision
        if (msg.pos.x <= 1 || msg.pos.x >= GAME_WIDTH - msg.pos.width - 1) {
            // Restore original width if compressed
            msg.pos.width = original_width;
            
            if (msg.pos.x <= 1) {
                msg.pos.x = 1;
                direction = 1;
            } else {
                msg.pos.x = GAME_WIDTH - msg.pos.width - 1;
                direction = -1;
            }
        }
        
        // Update position
        msg.pos.x += direction;
        
        /* Random projectile creation (5% chance)
        if (rand() % 100 < 5) {
            message bullet_msg;
            bullet_msg.type = MSG_CROCODILE;
            bullet_msg.pos.c = '@';
            bullet_msg.pos.x = (direction > 0) ? msg.pos.x + msg.pos.width - 1 : msg.pos.x;
            bullet_msg.pos.y = msg.pos.y;
            bullet_msg.pos.width = 1;
            bullet_msg.pos.height = 1;
            bullet_msg.pos.active = true;
            
            pthread_t bullet_tid;
            pthread_create(&bullet_tid, NULL, projectile_thread, &bullet_msg);
        }*/
        
        // Send position update through buffer
        buffer_put(buffer, &msg);
        
        // Control movement speed
        usleep(200000);
    }
    
    return NULL;
}

static void handle_border_collision(position *p, int *original_width) {
    if (p->x <= 1 || p->x >= GAME_WIDTH - p->width - 1) {
        p->width = *original_width;
    }
}

static void update_position(position *p, int direction) {
    p->x += direction;
}