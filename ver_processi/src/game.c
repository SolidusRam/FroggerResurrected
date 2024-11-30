#include "../include/game.h"

/*
  o
_`O'_
*/

char rana_sprite[2][5] = {
    {' ', ' ', 'O', ' ', ' '},
    {'_', '`', 'O', '\'', '_'}
    
};


void game(int pipein)
{
    struct position p;
    int NUM_CROCODILES = 8;

    //la rana inizia dal centro dello schermo
    struct position rana_pos = {'$', GAME_WIDTH-3, GAME_HEIGHT-2, 2, 5};
    struct position crocodile_positions [NUM_CROCODILES];

    // Initialize all crocodile positions
    for (int i = 0; i < NUM_CROCODILES; i++) {
        crocodile_positions[i].c = 'C';
        crocodile_positions[i].width = 5;
        crocodile_positions[i].height = 1;
        crocodile_positions[i].y = 4 + (i * LANE_HEIGHT); // Add initial y positions
        crocodile_positions[i].x = (i % 2 == 0) ? 1 : GAME_WIDTH - 6; // Add initial x positions
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


        //cancello la rana
        if (p.c == '$') {
            for(int i = 0; i < rana_pos.height; i++)
            {
                for(int z = 0; z < rana_pos.width; z++)
                {
                    mvaddch(rana_pos.y + i, rana_pos.x + z, ' ');
                }
            }
            rana_pos = p;}
            else if (p.c == 'C') {
            // Clear and update specific crocodile
                for (int i = 0; i < NUM_CROCODILES; i++) {
                    if (crocodile_positions[i].y == p.y) {  // Match exact y position
                        // Clear old position
                        for (int h = 0; h < crocodile_positions[i].height; h++) {
                            for (int w = 0; w < crocodile_positions[i].width; w++) {
                                mvaddch(crocodile_positions[i].y + h, crocodile_positions[i].x + w, ' ');
                            }
                        }
                crocodile_positions[i] = p;
                break;
                    }
                }

            }


        //stampo la rana
        for (int i = 0; i < rana_pos.height; i++)
        {
            for (int j = 0; j < rana_pos.width; j++)
            {
                mvaddch(rana_pos.y + i, rana_pos.x + j, rana_sprite[i][j]);
            }
        }

        // Disegno i coccodrilli
        for (int i = 0; i < NUM_CROCODILES; i++) {
            if (crocodile_positions[i].y != 0) {  // Only draw initialized crocodiles
                for (int h = 0; h < crocodile_positions[i].height; h++) {
                    for (int w = 0; w < crocodile_positions[i].width; w++) {
                        mvaddch(crocodile_positions[i].y + h, crocodile_positions[i].x + w, 'C');
                    }
                }
            }
        }

        refresh();
    }
}