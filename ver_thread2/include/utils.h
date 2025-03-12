#ifndef UTILS_H
#define UTILS_H

#include "game.h"
#include <stdbool.h>

// Definizione della struttura tana
typedef struct {
    int x;
    int y;
    bool occupata;
} tana;

// Funzioni per disegnare la zona di gioco
void draw_time_bar(int remaining_time, int max_time);
void draw_score(int score);
void draw_river_borders(void);
void clear_frog_position(int x, int y, int width, int height);
void draw_game_borders(void);
void init_dens(tana tane[]);
void draw_dens(tana tane[]);
bool check_den_collision(int rana_x, int rana_y, int width, int height, tana tane[]);

#endif // UTILS_H