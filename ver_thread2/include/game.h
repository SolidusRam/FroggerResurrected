#ifndef GAME_H
#define GAME_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ncurses.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <semaphore.h>
#include <sys/time.h>
#include <time.h>

#include "buffer.h"


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

// Game functions
void *game_thread(void *arg);

#endif