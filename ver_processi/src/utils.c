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
