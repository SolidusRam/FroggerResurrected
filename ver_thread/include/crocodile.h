#include "../include/game.h" // Include the header file that defines struct position
#include "../include/projectiles.h"

void *crocodile_thread(void *args);

static void handle_border_collision(struct position *p, int *original_width);

static void update_position(struct position *p, int direction);