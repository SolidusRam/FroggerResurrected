#include "../include/game.h"

/*
  o
_`O'_
*/

char rana_sprite[2][5] = {
    {' ', ' ', 'O', ' ', ' '},
    {'_', '`', 'O', '\'', '_'}
    
};


void game(int pipein,int num_coccodrilli)
{
    struct position p;
    

    //la rana inizia dal centro dello schermo
    struct position rana_pos = {'$', GAME_WIDTH-3, GAME_HEIGHT-2, 2, 5};
    struct position crocodile_positions [num_coccodrilli];

    // Initialize all crocodile positions
    // i coccodrilli si dividono in corsie
    for (int i = 0; i < num_coccodrilli; i++) {
        int lane = i % LANES;  // Distribute across 8 lanes (0-7)
        int x = (i % 2 == 0) ? 1 : GAME_WIDTH - 6;  // Alternate starting from left/right
        int y = 4 + (lane * LANE_HEIGHT);  // Starting from y=4 with 2 units between lanes
        
        crocodile_positions[i] = (struct position) {
            .c = 'C',
            .x = x,
            .y = y,
            .width = 1,
            .height = 1,
            .id = i
        };
    }
    
    while (1)
    {
        ssize_t r = read(pipein, &p, sizeof(struct position));
        if (r <= 0) {
            mvprintw(LINES/2, COLS/2-10, "Pipe read error: %s", strerror(errno));
            refresh();
            sleep(1);
            continue;
        }


        // cancello la rana

        for (int i = 0; i < rana_pos.height; i++)
        {
            for (int j = 0; j < rana_pos.width; j++)
            {
                mvaddch(rana_pos.y + i, rana_pos.x + j, ' ');
            }
        }

        //cancello i coccodrilli
        for(int i=0; i<num_coccodrilli; i++){
            mvaddch(crocodile_positions[i].y, crocodile_positions[i].x, ' ');
        }

            
        //aggiorno la posizione in base al carattere letto
        if(p.c == '$'){
            rana_pos = p;
        }else if (p.c == 'C'){
            for(int i = 0; i<num_coccodrilli; i++){
                if (crocodile_positions[i].id == p.id){
                    crocodile_positions[i] = p;
                    break;
                }
            }
        }

        //disegno la rana
        for (int i = 0; i < rana_pos.height; i++)
        {
            for (int j = 0; j < rana_pos.width; j++)
            {
                mvaddch(rana_pos.y + i, rana_pos.x + j, rana_sprite[i][j]);
            }
        }

        //disegno i coccodrilli

        for (int i=0; i<num_coccodrilli; i++){
            mvaddch(crocodile_positions[i].y, crocodile_positions[i].x, 'C');
        }

        refresh();
    }
}