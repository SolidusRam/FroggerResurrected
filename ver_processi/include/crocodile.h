
#include "../include/game.h" // Include the header file that defines struct position
#include "../include/projectiles.h"

void coccodrillo(int pipeout,int id);

static void handle_border_collision(struct position *p, int *original_width);

static void update_position(struct position *p, int direction);