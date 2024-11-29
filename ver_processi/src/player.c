#include "player.h"


/*char rana_sprite[4][3] = {
    {' ', 'O', ' '},
    {'O', 'O', 'O'},
    {' ', 'O', ' '},
    {'O', ' ', 'O'}
};*/

void rana(int pipeout)
{
    struct position p;
    p.c = '$';
    //centro dello schermo
    p.x = COLS/2;
    p.y = LINES/2;
    p.width = 3;
    p.height = 4;

    while (1)
    {
        int ch = getch();

        switch (ch)
        {
        case KEY_UP:
            if(p.y > 1) p.y--;
            break;
        case KEY_DOWN:
            if(p.y < LINES-2-p.height+1) p.y++;
            break;
        case KEY_LEFT:
            if(p.x > 1) p.x--;
            break;
        case KEY_RIGHT:
            if(p.x < COLS-2-p.width+1) p.x++;
            break;
        }

        //svuoto il buffer di input
        while ((ch = getch()) != ERR);
        
        //scrivo la posizione nella pipe
        write(pipeout, &p, sizeof(struct position));

        //controllo la velocità di movimento
        usleep(50000);
    }
}