#include "../include/game.h"


void game(int pipein)
{
    struct position p;

    //la rana inizia dal centro dello schermo
    struct position rana = {'$', COLS/2, LINES/2};

    
    while (1)
    {
        ssize_t r = read(pipein, &p, sizeof(struct position));
        if (r <= 0) {
            mvprintw(LINES/2, COLS/2-10, "Pipe read error: %s", strerror(errno));
            refresh();
            sleep(1);
            continue;
        }
        mvaddch(rana.y, rana.x, ' ');

        if (p.c == '$') {
            rana = p;
        }

        /*stampo la ranachar rana[3][4]={
        {'#', '#', '#', '#'},
        {'#', ' ', ' ', '#'},
        {'#', '#', '#', '#'}
    };*/
        for (int i = 0; i < 3; i++)
        {
            for (int z = 0; i < 4; i++)
            {
                mvaddch(sprite_rana[i][z], rana.y + i, rana.x + z);
            }
            
        }
        

        refresh();

        //eventuale controllo di collisione

    }
    
}
