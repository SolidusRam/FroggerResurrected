#ifndef GAME_H
#define GAME_H

#include <unistd.h>
#include <sys/types.h>
#include <ncurses.h>    
#include <errno.h>
#include <string.h>


struct position
{
    char c; // $ per la rana, 
    int x;
    int y;
    int width;
    int height;
};

void game(int);

#endif // GAME_H