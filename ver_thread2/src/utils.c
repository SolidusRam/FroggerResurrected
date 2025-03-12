#include "../include/utils.h"
#include "../include/game.h"
#include <ncurses.h>

void draw_score(int score) {
    int x_start = GAME_WIDTH - 12; 
    int y_start = GAME_HEIGHT - 1; 

    mvprintw(y_start, x_start, "SCORE: %d", score);
    refresh();
}

void draw_time_bar(int remaining_time, int max_time) {
    int bar_length = 20;  // Lunghezza della barra
    int filled_length = (remaining_time * bar_length) / max_time; // Calcola quanto riempire
    int x_start = 2;      
    int y_start = GAME_HEIGHT - 1; 
    
    // Stampa etichetta
    mvprintw(y_start, x_start, "TEMPO: ");

    // Disegna barra piena (verde)
    attron(COLOR_PAIR(1)); 
    for (int i = 0; i < filled_length; i++) {
        mvaddch(y_start, x_start + 8 + i, ACS_CKBOARD);
    }
    attroff(COLOR_PAIR(1));

    // Disegna barra vuota
    for (int i = filled_length; i < bar_length; i++) {
        mvaddch(y_start, x_start + 8 + i, ' ');
    }
}

void draw_river_borders() {
    attron(COLOR_PAIR(3)); // Colore per i bordi del fiume
    
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

void clear_frog_position(int x, int y, int width, int height) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            // Se siamo su un bordo del fiume, ridisegna il bordo
            if (y + i == 3 || y + i == FLOOR_HEIGHT) {
                mvaddch(y + i, x + j, ACS_HLINE);
            } else {
                mvaddch(y + i, x + j, ' ');
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

void init_dens(tana tane[]) {
    // Calcola lo spazio tra le tane
    int usable_width = GAME_WIDTH - 2; // Considera i bordi
    int den_space = (usable_width - (NUM_TANE * TANA_WIDTH)) / (NUM_TANE + 1);
    int current_x = 1; // Inizia dopo il bordo sinistro
    
    for(int i = 0; i < NUM_TANE; i++) {
        current_x += den_space; // Aggiungi spazio
        tane[i].x = current_x;
        tane[i].y = 1;  // Riga in alto
        tane[i].occupata = false;
        current_x += TANA_WIDTH; // Sposta oltre la larghezza della tana
    }
}

void draw_dens(tana tane[]) {
    for(int i = 0; i < NUM_TANE; i++) {
        // Disegna il bordo della tana
        attron(COLOR_PAIR(3)); // Usa il colore del bordo del fiume
        mvaddch(tane[i].y, tane[i].x - 1, ACS_LTEE);
        mvaddch(tane[i].y, tane[i].x + TANA_WIDTH, ACS_RTEE);
        attroff(COLOR_PAIR(3));

        if(tane[i].occupata) {
            // Disegna tana occupata
            attron(COLOR_PAIR(7)); // Colore per tana occupata
            for(int w = 0; w < TANA_WIDTH; w++) {
                mvaddch(tane[i].y, tane[i].x + w, ACS_CKBOARD);
            }
            attroff(COLOR_PAIR(7));
        } else {
            // Disegna tana vuota
            attron(COLOR_PAIR(6)); // Colore per tana vuota
            for(int w = 0; w < TANA_WIDTH; w++) {
                mvaddch(tane[i].y, tane[i].x + w, ' ');
            }
            attroff(COLOR_PAIR(6));
        }
    }
}

bool check_den_collision(int rana_x, int rana_y, int width, int height, tana tane[]) {
    // Punto centrale della rana
    int frog_center_x = rana_x + (width / 2);
    
    for(int i = 0; i < NUM_TANE; i++) {
        // Controlla se la tana è già occupata
        if(!tane[i].occupata) {
            // Controlla se il centro della rana è all'interno dei limiti della tana
            if(frog_center_x >= tane[i].x && 
               frog_center_x <= tane[i].x + TANA_WIDTH &&
               rana_y <= tane[i].y + TANA_HEIGHT) {
                tane[i].occupata = true;
                return true;
            }
        }
    }
    return false;
}