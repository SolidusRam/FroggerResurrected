#include "../include/crocodile.h"
#include <time.h>


void coccodrillo(int pipeout,int id){
    struct position p;
    int direction = 0; // 1 = right, -1 = left
    
    p.c = 'C';
    p.id = id;
    p.width = 1;
    p.height = 1;

    //bordo destro o sinistro
    //stabilisco anche la direzione (opposta al bordo)
    if(id%2 == 0){
        p.x = GAME_WIDTH-2;
        direction = -1;
    }else{
        p.x = 1;
        direction = 1;
    }

    //altezza in base alla corsia 
    int lane = id % LANES;  // Distribute across 8 lanes (0-7)
    p.y = 4 + (lane * LANE_HEIGHT);  // Starting from y=4 with 2 units between lanes


    while (1)
    {
        //controllo se bordo
        if (p.x <= 1) {
            p.x = 2;
            direction = 1;
        } else if (p.x >= GAME_WIDTH-3) {
            p.x = GAME_WIDTH-3;
            direction = -1;
        }
        //movimento
        p.x += direction;

        //scrivo la posizione nella pipe
        write(pipeout, &p, sizeof(struct position));

        usleep(100000);
    }
    

}