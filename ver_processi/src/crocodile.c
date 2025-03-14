#include "../include/crocodile.h"

#include <time.h>

/*I coccodrilli si dividono in corsie in base al loro id */

void coccodrillo(int pipeout,int id){
    srand(time(NULL)^id);
    struct position p;
    int direction = 0; // 1 = right, -1 = left
    int original_width;
    int speed;
    
    //inizializzo i valori del coccodrillo
    p.c = 'C';
    p.id = id;
    p.width = (rand() % 2 +2) *5;
    original_width = p.width;
    p.height = 2;

    //divido i coccodrilli in corsie
    int lane = (id/2) % LANES;

    direction = (lane % 2 == 0) ? 1 : -1; //alternare la direzione di partenza

    // Assegno differenti velocità in base alla corsia
    // Più basso è il valore, più veloce sarà il coccodrillo
    switch(lane % 4) {
        case 0:
            speed = 250000; // lane 0, 4, 8 - velocità media
            break;
        case 1:
            speed = 300000; // lane 1, 5, 9 - velocità lenta
            break;
        case 2:
            speed = 180000; // lane 2, 6, 10 - velocità veloce
            break;
        case 3:
            speed = 220000; // lane 3, 7, 11 - velocità medio-veloce
            break;
    }

    // Aggiungo un po' di variazione alla velocità del 10%
    speed = speed * (90 + rand() % 21) / 100;


    int is_second= id % 2;

    if (is_second ){
        p.x = (GAME_WIDTH/2) + (rand() % (GAME_WIDTH/4));
    }else{
        p.x =rand() % (GAME_WIDTH/4);
    }

    p.y = 4+(lane*LANE_HEIGHT);

    while (1)
    {
        handle_border_collision(&p, &original_width);
        update_position(&p, direction);

        //il coccodrillo spara un proiettile il 20% delle volte
        if(rand() % 100 < 5){
            struct position bomb = {
                .c = '@',
                //dalla testa del coccodrillo
                .x = (direction > 0) ? p.x + p.width - 1 : p.x,
                .y = p.y,
                .width = 1,
                .height = 1,
                .active = 1,
                .pid = getpid()
            };
            if(fork() == 0) {

                bullet(pipeout, &bomb, direction);
                _exit(0);
            }
        }

        //scrivo la posizione nella pipe
        write(pipeout, &p, sizeof(struct position));

        usleep(speed);
    }
    

}

 void handle_border_collision(struct position *p, int *original_width) {
    if (p->x <= 1) {
        p->x = 2;
        p->width = p->width - 1;
        if(p->width <= 0) {
            p->width = *original_width;
            p->x = GAME_WIDTH-2;
        }
    } else if (p->x >= GAME_WIDTH-3) {
        p->x = GAME_WIDTH-3;
        p->width = p->width - 1;
        if (p->width <= 0) {
            p->width = *original_width;
            p->x = 1;
        }
    }
}

 void update_position(struct position *p, int direction) {
    p->x += direction;
}