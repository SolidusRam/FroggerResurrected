#include "../include/utils.h"

void draw_score(int score) {
    
    int x_start = COLS - 12; 
    int y_start = LINES - 1; 

    mvprintw(y_start, x_start, "SCORE: %d", score);
    refresh();
}

void draw_time_bar(int remaining_time, int max_time) {

    int bar_length = 20;  // Lunghezza della barra
    int filled_length = (remaining_time * bar_length) / max_time; // Calcola quanto riempire
    int x_start = 2;      
    int y_start = LINES - 1; 
    
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


void init_dens(struct tana *tane) {
    // Calculate spacing between dens
    int usable_width = GAME_WIDTH - 2; // Account for borders
    int den_space = (usable_width - (NUM_TANE * TANA_WIDTH)) / (NUM_TANE + 1);
    int current_x = 1; // Start after left border
    
    for(int i = 0; i < NUM_TANE; i++) {
        current_x += den_space; // Add spacing
        tane[i].x = current_x;
        tane[i].y = 1;  // Top row
        tane[i].occupata = false;
        current_x += TANA_WIDTH; // Move past den width
    }
}

void draw_dens(struct tana tane[]) {
    for(int i = 0; i < NUM_TANE; i++) {
        // Draw den border
        attron(COLOR_PAIR(3)); // Use river border color
        mvaddch(tane[i].y, tane[i].x - 1, ACS_LTEE);
        mvaddch(tane[i].y, tane[i].x + TANA_WIDTH, ACS_RTEE);
        attroff(COLOR_PAIR(3));

        if(tane[i].occupata) {
            // Draw occupied den
            attron(COLOR_PAIR(7)); // Color for occupied den
            for(int w = 0; w < TANA_WIDTH; w++) {
                mvaddch(tane[i].y, tane[i].x + w, ACS_CKBOARD);
            }
            attroff(COLOR_PAIR(7));
        } else {
            // Draw empty den
            attron(COLOR_PAIR(6)); // Color for empty den
            for(int w = 0; w < TANA_WIDTH; w++) {
                mvaddch(tane[i].y, tane[i].x + w, ' ');
            }
            attroff(COLOR_PAIR(6));
        }
    }
}
bool check_den_collision(struct position *rana_pos, struct tana tane[]) {
    // Center point of the frog
    int frog_center_x = rana_pos->x + (rana_pos->width / 2);
    
    for(int i = 0; i < NUM_TANE; i++) {
        // Check if den is already occupied
        if(!tane[i].occupata) {
            // Check if frog's center is within den bounds
            if(frog_center_x >= tane[i].x && 
               frog_center_x <= tane[i].x + TANA_WIDTH &&
               rana_pos->y <= tane[i].y + TANA_HEIGHT) {
                tane[i].occupata = true;
                return true;
            }
        }
    }
    return false;
}
