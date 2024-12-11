#include "../include/crocodile.h"

#include <time.h>

/*I coccodrilli si dividono in corsie in base al loro id */

void coccodrillo(int pipeout,int id){
    srand(time(NULL)^id);
    struct position p;
    int direction = 0; // 1 = right, -1 = left
    int original_width;
    
    //inizializzo i valori del coccodrillo
    p.c = 'C';
    p.id = id;
    p.width = (rand() % 2 +2) *5;
    original_width = p.width;
    p.height = 2;

    // direzione in base all'id
    direction = (id % 2 == 0) ? -1 : 1;  // Alternate direction based on ID

    // Posizionamento x casuale dentro i bordi
    p.x= rand() % (GAME_WIDTH - p.width - 2) + 1;

    int lane = id % LANES;  // Distribute across 8 lanes (0-7)
    p.y = 4 + (lane * LANE_HEIGHT);  // Starting from y=4 with 2 units between lanes


    while (1)
    {
        handle_border_collision(&p, &original_width);
        update_position(&p, direction);

        //scrivo la posizione nella pipe
        write(pipeout, &p, sizeof(struct position));

        usleep(200000);
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