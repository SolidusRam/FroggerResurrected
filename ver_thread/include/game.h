#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <ncurses.h>
#include <time.h>

// Game constants
#define GAME_WIDTH 80
#define GAME_HEIGHT 24
#define FLOOR_HEIGHT 20
#define LANES 8
#define LANE_HEIGHT 2
#define MAX_CROCODILES 16
#define MAX_BULLETS 100
#define NUM_TANE 5
#define TANA_WIDTH 7
#define TANA_HEIGHT 1

// Forward declarations
struct circular_buffer;
struct message;

// Position structure
typedef struct position {
    char c;
    int x;
    int y;
    int width;
    int height;
    int id;
    bool active;
    bool collision;
} position;

// Den structure
typedef struct tana {
    int x;
    int y;
    bool occupata;
} tana;

// Function declarations
void *game_thread(void *arg);
bool rana_coccodrillo(position *rana_pos, position crocodile_positions[], int num_coccodrilli, int *direction);
bool frog_on_the_water(position *rana_pos);

#endif