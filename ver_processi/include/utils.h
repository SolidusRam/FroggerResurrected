#include "game.h"
#include <stdio.h>
#include <ncurses.h>
#include <unistd.h>
#include <time.h>
#include "game.h"

//void timeBar(int tempo);

void draw_river_borders();
void clear_frog_position(struct position *pos);
void draw_game_borders();