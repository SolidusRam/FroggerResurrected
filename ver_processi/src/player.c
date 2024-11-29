#include "player.h"

void rana(int pipeout)
{
    struct position p;
    p.c = '$';
    //centro dello schermo
    p.x = LINES/2;
    p.y = COLS/2;

    while (1)
    {
        int ch = getch();

        switch (ch)
        {
        case KEY_UP:
            if(p.y > 1) p.y--;
            break;
        case KEY_DOWN:
            if(p.y < LINES-2) p.y++;
            break;
        case KEY_LEFT:
            if(p.x > 1) p.x--;
            break;
        case KEY_RIGHT:
            if(p.x < COLS-2) p.x++;
            break;
        }

        //scrivo la posizione nella pipe
        write(pipeout, &p, sizeof(struct position));

        //controllo la velocità di movimento
        usleep(100000);
    }
}