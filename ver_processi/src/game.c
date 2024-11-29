#include "../include/game.h"

char sprite_rana[3][4] = {
    {'#', '#', '#', '#'},
    {'#', ' ', ' ', '#'},
    {'#', '#', '#', '#'} 
};

void game(int pipein)
{
    struct position p;

    //la rana inizia dal centro dello schermo
    struct position rana = {'$', COLS/2, LINES/2, 4, 3};

    while (1)
    {
        ssize_t r = read(pipein, &p, sizeof(struct position));
        if (r <= 0) {
            mvprintw(LINES/2, COLS/2-10, "Pipe read error: %s", strerror(errno));
            refresh();
            sleep(1);
            continue;
        }

        for(int i = 0; i < 3; i++)
        {
            for(int z = 0; z < 4; z++)
            {
                mvaddch(rana.y + i, rana.x + z, ' ');
            }
        }

        if (p.c == '$') {
            rana = p;
        }

        for (int i = 0; i < rana.height; i++)
        {
            for (int j = 0; j < rana.width; j++)
            {
                // Codice mancante
            }
        }
    }
}