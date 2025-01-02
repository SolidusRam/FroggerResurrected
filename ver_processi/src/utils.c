#include "../include/utils.h"

#define MAX_TIME 30 // Durata massima in secondi
#define BAR_WIDTH 50 // Larghezza massima della barra

//Funzione generica che stampa la barra del tempo in basso a sinistra delle schermo

/*void timeBar(int tempo) {

    int lunghezzaB = (tempo * BAR_WIDTH) / MAX_TIME; // Calcola la lunghezza della barra
    int x_start = 1; 
    int y_start = LINES - 2; 

// Disegna la barra vuota

 mvprintw(y_start - 1, x_start, "Tempo rimanente: "); // Etichetta della barra
 attron(COLOR_PAIR(2)); // Colore verde

 for (int i = 0; i < BAR_WIDTH; i++) {
      mvaddch(y_start, x_start + i, ' '); // Disegna la barra vuota
 }

  attroff(COLOR_PAIR(2)); // Disattiva il colore

// Disegna la parte piena della barra
attron(COLOR_PAIR(1)); // Attiva il colore pieno
 for (int i = 0; i < lunghezzaB; i++) {
     mvaddch(y_start, x_start + i, ACS_CKBOARD); // Disegna la parte piena
    }
    attroff(COLOR_PAIR(1)); // Disattiva il colore
}*/


void draw_river_borders() {
    attron(COLOR_PAIR(3)); // Nuovo colore per i bordi del fiume
    
    // Disegna bordo superiore del fiume (y=3)
    mvhline(3, 1, ACS_HLINE, GAME_WIDTH-2);
    mvaddch(3, 0, ACS_LTEE);
    mvaddch(3, GAME_WIDTH-1, ACS_RTEE);
    
    // Disegna bordo inferiore del fiume (y=FLOOR_HEIGHT)
    mvhline(FLOOR_HEIGHT, 1, ACS_HLINE, GAME_WIDTH-2);
    mvaddch(FLOOR_HEIGHT, 0, ACS_LTEE);
    mvaddch(FLOOR_HEIGHT, GAME_WIDTH-1, ACS_RTEE);
    
    attroff(COLOR_PAIR(3));
}

void clear_frog_position(struct position *pos) {
    for (int i = 0; i < pos->height; i++) {
        for (int j = 0; j < pos->width; j++) {
            // Se siamo su un bordo del fiume, ridisegna il bordo
            if (pos->y + i == 3 || pos->y + i == FLOOR_HEIGHT) {
                mvaddch(pos->y + i, pos->x + j, ACS_HLINE);
            } else {
                mvaddch(pos->y + i, pos->x + j, ' ');
            }
        }
    }
}

void draw_game_borders() {
    // Colore per la terra (marrone)
    init_pair(5, COLOR_YELLOW, COLOR_RED);
    
    // Disegna la terra di partenza (bottom)
    attron(COLOR_PAIR(5));
    for (int x = 1; x < GAME_WIDTH-1; x++) {
        for (int y = FLOOR_HEIGHT+1; y < GAME_HEIGHT-1; y++) {
            mvaddch(y, x, ACS_CKBOARD);
        }
    }
    
    // Disegna la tana di arrivo (top)
    for (int x = 1; x < GAME_WIDTH-1; x++) {
        for (int y = 1; y < 3; y++) {
            mvaddch(y, x, ACS_CKBOARD);
        }
    }
    attroff(COLOR_PAIR(5));
}