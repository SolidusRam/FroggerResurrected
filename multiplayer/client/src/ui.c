#include "../include/ui.h"
#include <string.h>

// Sprite identici alla singleplayer
static const char RANA[2][5] = {
    {' ', ' ', 'O', ' ', ' '},
    {'_', '`', 'O', '\'', '_'}
};

static const char CROCO_DX[2][15] = {
    {'_', '_', '_', '_', '_', '_', '_', '/', '^', '\\', '_', '_', '_', '_', ' '},
    {'=', '=', '=', '=', '=', '=', '=', '=', '=', '=', '=', '=', '=', '=', '\\'}
};
static const char CROCO_SX[2][15] = {
    {' ', '_', '_', '_', '/', '^', '\\', '_', '_', '_', '_', '_', '_', '_', '_'},
    {'/', '=', '=', '=', '=', '=', '=', '=', '=', '=', '=', '=', '=', '=', '='}
};

void ui_init(void) {
    initscr(); cbreak(); noecho(); keypad(stdscr, TRUE); nodelay(stdscr, TRUE); curs_set(0);
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_YELLOW, COLOR_BLACK); // rana/testi
        init_pair(2, COLOR_GREEN, COLOR_BLACK);  // coccodrilli
        init_pair(3, COLOR_CYAN, COLOR_BLACK);   // bordi fiume
        init_pair(5, COLOR_YELLOW, COLOR_RED);   // terra
        init_pair(6, COLOR_WHITE, COLOR_BLACK);  // tana vuota
        init_pair(7, COLOR_MAGENTA, COLOR_BLACK);// tana piena
    }
}

void ui_teardown(void) {
    endwin();
}

static void draw_background(void) {
    // bordo
    box(stdscr, ACS_VLINE, ACS_HLINE);
    // bordi fiume
    attron(COLOR_PAIR(3));
    mvhline(3, 1, ACS_HLINE, GAME_WIDTH-2);
    mvaddch(3, 0, ACS_LTEE);
    mvaddch(3, GAME_WIDTH-1, ACS_RTEE);
    mvhline(FLOOR_HEIGHT, 1, ACS_HLINE, GAME_WIDTH-2);
    mvaddch(FLOOR_HEIGHT, 0, ACS_LTEE);
    mvaddch(FLOOR_HEIGHT, GAME_WIDTH-1, ACS_RTEE);
    attroff(COLOR_PAIR(3));
    // terra
    attron(COLOR_PAIR(5));
    for (int x = 1; x < GAME_WIDTH-1; x++)
        for (int y = FLOOR_HEIGHT+1; y < GAME_HEIGHT-1; y++)
            mvaddch(y, x, ACS_CKBOARD);
    for (int x = 1; x < GAME_WIDTH-1; x++)
        for (int y = 1; y < 3; y++)
            mvaddch(y, x, ACS_CKBOARD);
    attroff(COLOR_PAIR(5));
}

static void draw_dens(const game_snapshot_t* s) {
    for (int i = 0; i < NUM_TANE; ++i) {
        int x = s->dens[i].x, y = s->dens[i].y;
        attron(COLOR_PAIR(3));
        mvaddch(y, x - 1, ACS_LTEE);
        mvaddch(y, x + TANA_WIDTH, ACS_RTEE);
        attroff(COLOR_PAIR(3));
        if (s->dens[i].occupied) attron(COLOR_PAIR(7)); else attron(COLOR_PAIR(6));
        for (int w = 0; w < TANA_WIDTH; ++w) mvaddch(y, x + w, s->dens[i].occupied ? ACS_CKBOARD : ' ');
        if (s->dens[i].occupied) attroff(COLOR_PAIR(7)); else attroff(COLOR_PAIR(6));
    }
}

static void draw_crocs(const game_snapshot_t* s) {
    attron(COLOR_PAIR(2));
    for (int i = 0; i < s->num_crocodiles; ++i) {
        int x = s->crocodiles[i].ent.box.x;
        int y = s->crocodiles[i].ent.box.y;
        int w = s->crocodiles[i].ent.box.w;
        const char (*sprite)[15] = (s->crocodiles[i].dir == DIR_RIGHT) ? CROCO_DX : CROCO_SX;
        int draw_w = w < 15 ? w : 15;
        for (int h = 0; h < 2; ++h) {
            for (int j = 0; j < draw_w; ++j) {
                if (y + h >= 0 && y + h < GAME_HEIGHT && x + j >= 0 && x + j < GAME_WIDTH)
                    mvaddch(y + h, x + j, sprite[h][j]);
            }
        }
    }
    attroff(COLOR_PAIR(2));
}

static void draw_player(const game_snapshot_t* s) {
    attron(COLOR_PAIR(1));
    // Disegna tutti i giocatori: il primo normale, gli altri in "ghost"
    for (int p = 0; p < s->num_players; ++p) {
        int x = s->players[p].ent.box.x;
        int y = s->players[p].ent.box.y;
        bool ghost = (p != 0);
        for (int h = 0; h < 2; ++h) {
            for (int w = 0; w < 5; ++w) {
                int yy = y + h, xx = x + w;
                if (yy >= 0 && yy < GAME_HEIGHT && xx >= 0 && xx < GAME_WIDTH) {
                    chtype ch = RANA[h][w] | (ghost ? A_DIM : A_BOLD);
                    mvaddch(yy, xx, ch);
                }
            }
        }
    }
    attroff(COLOR_PAIR(1));
}

static void draw_hud(const game_snapshot_t* s) {
    // Scores and lives (bottom line, compact for multiple players)
    int x = GAME_WIDTH - 40;
    for (int i = 0; i < s->num_players; ++i) {
        mvprintw(GAME_HEIGHT-1, x, "P%d L:%d S:%d", i+1, s->players[i].lives, s->players[i].score);
        x += 13;
    }

    // Time bar (uses timer if provided)
    int max_t = s->timer.max_time > 0 ? s->timer.max_time : 1;
    int rem_t = s->timer.remaining_time;
    if (rem_t < 0) rem_t = 0;
    if (rem_t > max_t) rem_t = max_t;
    int bar_len = 20;
    int filled = (rem_t * bar_len) / max_t;
    int x_start = 2;
    int y_start = GAME_HEIGHT - 1;
    mvprintw(y_start, x_start, "TEMPO: ");
    attron(COLOR_PAIR(1));
    for (int i = 0; i < filled; ++i) mvaddch(y_start, x_start + 8 + i, ACS_CKBOARD);
    attroff(COLOR_PAIR(1));
    for (int i = filled; i < bar_len; ++i) mvaddch(y_start, x_start + 8 + i, ' ');
    // Overlay for victory/game over
    if (s->victory) {
        attron(A_BOLD);
        mvprintw(GAME_HEIGHT/2, (GAME_WIDTH/2)-6, "HAI VINTO!");
        attroff(A_BOLD);
    } else if (s->game_over) {
        attron(A_BOLD);
        mvprintw(GAME_HEIGHT/2, (GAME_WIDTH/2)-6, "GAME OVER");
        attroff(A_BOLD);
    }
}

void ui_draw_snapshot(const game_snapshot_t* snap, uint32_t local_player_id) {
    erase();
    mvprintw(0, 0, "Frogger Multiplayer — q per uscire");
    // Riga informativa opzionale
    mvprintw(1, 0, "tick=%u", snap->tick);
    draw_background();
    draw_dens(snap);
    draw_crocs(snap);
    // Draw local solid, others ghost
    attron(COLOR_PAIR(1));
    for (int i = 0; i < snap->num_players; ++i) {
        bool ghost = ((uint32_t)snap->players[i].id != local_player_id);
        int x = snap->players[i].ent.box.x;
        int y = snap->players[i].ent.box.y;
        for (int h = 0; h < 2; ++h) {
            for (int w = 0; w < 5; ++w) {
                int yy = y + h, xx = x + w;
                if (yy >= 0 && yy < GAME_HEIGHT && xx >= 0 && xx < GAME_WIDTH) {
                    chtype ch = RANA[h][w] | (ghost ? A_DIM : A_BOLD);
                    mvaddch(yy, xx, ch);
                }
            }
        }
    }
    attroff(COLOR_PAIR(1));
    draw_hud(snap);
    refresh();
}
