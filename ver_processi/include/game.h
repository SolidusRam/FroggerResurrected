#include <unistd.h>
#include <sys/types.h>
#include <ncurses.h>    
#include <errno.h>


char sprite_rana[3][4]={
    {'#', '#', '#', '#'},
    {'#', ' ', ' ', '#'},
    {'#', '#', '#', '#'}
};

struct position
{
    char c; // $ per la rana, 
    int x;
    int y;
};


void game(int);