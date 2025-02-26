#include "../include/crocodile.h"
#include "../include/buffer.h"
#include <unistd.h>
#include <time.h>


static int next_croc_id = 0;
static pthread_mutex_t id_mutex = PTHREAD_MUTEX_INITIALIZER;


void *crocodile_thread(void *args) {
    struct circular_buffer* buffer = (struct circular_buffer*)args;
    message msg;
   
    // Get a unique ID using the global counter
    pthread_mutex_lock(&id_mutex);
    int id = next_croc_id++;
    pthread_mutex_unlock(&id_mutex);
    
    // Seed random number generator with unique value per thread
    srand(time(NULL) ^ id);
    
    int direction = 0;
    int original_width;
    
    // Initialize crocodile message
    msg.type = MSG_CROCODILE;
    msg.id = id;  
    msg.pos.id = id;
    msg.pos.active = true;
    msg.pos.c = 'C';
    msg.pos.width = (rand() % 2 + 2) * 5;
    msg.pos.height = 2;
    original_width = msg.pos.width;
    
    // Set lane and direction based on thread ID
    int lane = (id / 2) % LANES;
    direction = (lane % 2 == 0) ? 1 : -1;
    
    // Set initial position
    int is_second = id % 2;
    if (is_second) {
        msg.pos.x = (GAME_WIDTH/2) + (rand() % (GAME_WIDTH/4));
    } else {
        msg.pos.x = rand() % (GAME_WIDTH/4);
    }
    msg.pos.y = 4 + (lane * LANE_HEIGHT);
    
    while (1) {
        // First handle position update
        msg.pos.x += direction;
        
        // Handle border wrapping with smooth transitions
        if (direction > 0) {  // Moving right
            if (msg.pos.x > GAME_WIDTH) {
                msg.pos.x = 1 - original_width + 1;
                msg.pos.width = 1;
            } else if (msg.pos.x + msg.pos.width > GAME_WIDTH) {
                // Partially off screen
                msg.pos.width = GAME_WIDTH - msg.pos.x;
            } else if (msg.pos.width < original_width) {
                // Coming in from left
                msg.pos.width++;
            }
        } else {  // Moving left
            if (msg.pos.x + msg.pos.width < 1) {
                msg.pos.x = GAME_WIDTH - 1;
                msg.pos.width = 1; 
            } else if (msg.pos.x < 1) {
                // Partially off screen
                int overlap = 1 - msg.pos.x;
                msg.pos.x = 1;
                msg.pos.width = msg.pos.width - overlap;
            } else if (msg.pos.width < original_width) {
                // Coming in from right
                msg.pos.width++;
            }
        }
        
        // Send position update through buffer
        buffer_put(buffer, &msg);
        
        // Control movement speed
        usleep(300000);
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