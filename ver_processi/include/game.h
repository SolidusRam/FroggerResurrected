#ifndef GAME_H
#define GAME_H

#include <unistd.h>
#include <sys/types.h>
#include <ncurses.h>    
#include <errno.h>
#include <string.h>

#define GAME_WIDTH 80    // Standard terminal width
#define GAME_HEIGHT 24   // Standard terminal height
#define FLOOR_HEIGHT 20  // Actual playable area height
#define LANES 8         // Number of lanes for obstacles
#define LANE_HEIGHT 2   // Height of each lane


struct position
{
    char c; // $ per la rana, C per il coccodrillo
    int x;
    int y;
    int width;
    int height;
};

void game(int);

#endif // GAME_H