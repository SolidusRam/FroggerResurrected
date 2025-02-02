#ifndef UTILS_H
#define UTILS_H


#include "game.h"


void draw_time_bar(int remaining_time, int max_time);
void draw_score(int score);
void draw_river_borders(void);
void clear_frog_position(position *pos);
void draw_game_borders(void);
void init_dens(tana tane[]);
void draw_dens(tana tane[]);
bool check_den_collision(position *rana_pos, tana *tane);

#endif // UTILS_H
