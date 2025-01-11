#include "player.h"



void rana(int pipeout,int pipein)
{
    struct position p;
    p.c = '$';

    p.x = GAME_WIDTH/2;
    p.y = GAME_HEIGHT-2;
    p.width = 5;
    p.height = 2;

    fcntl(pipein, F_SETFL, O_NONBLOCK);

    while (1)
    {

        // Controlla se ci sono dati da leggere dalla pipe
        struct position update;
        if (read(pipein, &update, sizeof(struct position)) > 0) {
            // Aggiorna la posizione con quella ricevuta dal game
            p.x = update.x;
            p.y = update.y;
        }

        int ch = getch();

        switch (ch)
        {
        case KEY_UP:
            if(p.y > 1) p.y-=p.height;
            break;
        case KEY_DOWN:
            if(p.y < LINES-2-p.height+1) p.y+=p.height;
            break;
        case KEY_LEFT:
            if(p.x > 1) p.x-=p.width;
            break;
        case KEY_RIGHT:
            if(p.x < COLS-2-p.width+1) p.x+=p.width;
            break;

        case ' ':
            struct position right_bullet = p;
            right_bullet.c = '*';
            right_bullet.x = p.x + p.width; // Start from right edge of frog
            
            // Create left bullet
            struct position left_bullet = p;
            left_bullet.c = '*';
            left_bullet.x = p.x - 1; // Start from left edge of frog

            // Launch right bullet
            if(fork() == 0) {
                bullet(pipeout, &right_bullet, 1);
                _exit(0);
            }
            
            // Launch left bullet  
            if(fork() == 0) {
                bullet(pipeout, &left_bullet, -1);
                _exit(0);
            }
            break;

            case 'q':
                //teletrasporto il player poco prima della tana
                p.x = GAME_WIDTH/2;
                p.y = 2;
                break;

        }

        //svuoto il buffer di input
        while ((ch = getch()) != ERR);
        
        //scrivo la posizione nella pipe
        write(pipeout, &p, sizeof(struct position));

        //controllo la velocità di movimento
        usleep(200000);
    }
}