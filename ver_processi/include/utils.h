#ifndef UTILS_H
#define UTILS_H

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
void init_dens(struct tana tane[]);
void draw_dens(struct tana tane[]);
bool check_tana_collision(struct position *rana_pos, struct tana *tane);

#endif // UTILS_H