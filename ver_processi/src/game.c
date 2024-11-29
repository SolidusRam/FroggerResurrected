#include "../include/game.h"

/*
  o
_`O'_
*/

char rana_sprite[2][5] = {
    {' ', ' ', 'o', ' ', ' '},
    {'_', '`', 'O', '\'', '_'}
    
};

void game(int pipein)
{
    struct position p;

    //la rana inizia dal centro dello schermo
    struct position rana_pos = {'$', GAME_WIDTH-3, GAME_HEIGHT-2, 2, 5};

    while (1)
    {
        ssize_t r = read(pipein, &p, sizeof(struct position));
        mvprintw(0, 0, "Read: char=%c, x=%d, y=%d", p.c, p.x, p.y);
        refresh();
        if (r <= 0) {
            mvprintw(LINES/2, COLS/2-10, "Pipe read error: %s", strerror(errno));
            refresh();
            sleep(1);
            continue;
        }


        //cancello la rana

        for(int i = 0; i < rana_pos.height; i++)
        {
            for(int z = 0; z < rana_pos.width; z++)
            {
                mvaddch(rana_pos.y + i, rana_pos.x + z, ' ');
            }
        }

        //aggiorno la posizione in base al carattere letto
        if (p.c == '$') {
            rana_pos = p;
        }

        //stampo la rana
        for (int i = 0; i < rana_pos.height; i++)
        {
            for (int j = 0; j < rana_pos.width; j++)
            {
                mvaddch(rana_pos.y + i, rana_pos.x + j, rana_sprite[i][j]);
            }
        }

        refresh();
    }
}