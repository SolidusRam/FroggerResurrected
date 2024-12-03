#include "../include/crocodile.h"

#include <time.h>


void coccodrillo(int pipeout,int id){
    srand(time(NULL)^id);
    struct position p;
    int direction = 0; // 1 = right, -1 = left
    int original_width;
    
    p.c = 'C';
    p.id = id;
    p.width = (rand() % 2 +2) *5;
    original_width = p.width;
    p.height = 2;

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
            p.width = p.width -1;
            if(p.width <= 0){
                p.width = original_width;
                p.x = GAME_WIDTH-2;
            }
            
        } else if (p.x >= GAME_WIDTH-3) {
            p.x = GAME_WIDTH-3;
            p.width = p.width -1;
            if (p.width <= 0)
            {
                p.width = original_width;
                p.x = 1;
            }
            
        }
        //movimento
        p.x += direction;

        //scrivo la posizione nella pipe
        write(pipeout, &p, sizeof(struct position));

        usleep(100000);
    }
    

}