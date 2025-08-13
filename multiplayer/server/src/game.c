#include "../include/game.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>

static int lane_for_index(int idx) {
    return (idx / 2); // due coccodrilli per corsia
}

static direction_t dir_for_lane(int lane) {
    return (lane % 2 == 0) ? DIR_RIGHT : DIR_LEFT;
}

// Restituisce ogni quanti tick muovere (1 = ogni tick, 2 = ogni 2 tick, ...)
static int speed_div_for_lane(int lane) {
    switch (lane % 4) {
        case 0: return 2; // media
        case 1: return 3; // lenta
        case 2: return 1; // veloce
        case 3: return 2; // medio-veloce
        default: return 2;
    }
}

static int find_player_index(const server_game_t* g, uint32_t pid) {
    for (int i = 0; i < g->num_players; ++i) {
        if ((uint32_t)g->players[i].id == pid) return i;
    }
    return -1;
}

void game_init(server_game_t* g) {
    srand((unsigned)time(NULL));
    g->tick = 0;
    g->num_players = 0;
    for (int i = 0; i < MAX_PLAYERS; ++i) { g->player_on_croc[i] = false; g->player_croc_id[i] = -1; }

    // Tane in alto come singleplayer
    int usable_width = GAME_WIDTH - 2;
    int den_space = (usable_width - (NUM_TANE * TANA_WIDTH)) / (NUM_TANE + 1);
    int current_x = 1;
    for (int i = 0; i < NUM_TANE; ++i) {
        current_x += den_space;
        g->dens[i].x = current_x;
        g->dens[i].y = 1;
        g->dens[i].occupied = false;
        current_x += TANA_WIDTH;
    }

    // Coccodrilli
    g->num_crocs =  16; // POC
    for (int i = 0; i < g->num_crocs; ++i) {
        int lane = lane_for_index(i);
        int width = ( (rand() % 2) + 2 ) * 5; // 10 o 15
        int x = (i % 2 == 0) ? (rand() % (GAME_WIDTH/4))
                             : (GAME_WIDTH/2 + rand() % (GAME_WIDTH/4));
        g->crocs[i].id = i+1;
        g->crocs[i].ent.active = true;
        g->crocs[i].ent.box.w = width;
        g->crocs[i].ent.box.h = 2;
        g->crocs[i].ent.box.x = x;
        g->crocs[i].ent.box.y = 4 + lane * LANE_HEIGHT;
        g->crocs[i].dir = dir_for_lane(lane);
    }

    // Timer
    g->timer.max_time = 30;
    g->timer.remaining_time = 30;
}

int game_add_player(server_game_t* g, uint32_t player_id) {
    if (g->num_players >= MAX_PLAYERS) return -1;
    int i = g->num_players++;
    player_state_t* p = &g->players[i];
    p->id = (int32_t)player_id;
    p->connected = true;
    p->lives = 3;
    p->score = 0;
    p->ent.id = (int32_t)player_id;
    p->ent.active = true;
    p->ent.box.w = 5;
    p->ent.box.h = 2;
    // Alternate spawn sides
    p->ent.box.x = (i == 0) ? GAME_WIDTH/2 : (GAME_WIDTH/2 - 6);
    p->ent.box.y = GAME_HEIGHT - 3;
    g->player_on_croc[i] = false; g->player_croc_id[i] = -1;
    return 0;
}

void game_remove_player(server_game_t* g, uint32_t player_id) {
    int idx = find_player_index(g, player_id);
    if (idx < 0) return;
    for (int j = idx; j < g->num_players - 1; ++j) g->players[j] = g->players[j+1];
    g->num_players--;
    // collapse attachment arrays similarly
    for (int j = idx; j < MAX_PLAYERS - 1; ++j) {
        g->player_on_croc[j] = g->player_on_croc[j+1];
        g->player_croc_id[j] = g->player_croc_id[j+1];
    }
    g->player_on_croc[g->num_players] = false;
    g->player_croc_id[g->num_players] = -1;
}

void game_apply_input(server_game_t* g, uint32_t player_id, const msg_input_t* in) {
    int idx = find_player_index(g, player_id);
    if (idx < 0) return;
    player_state_t* pl = &g->players[idx];
    int dx = 0, dy = 0;
    if (in->buttons & IN_LEFT)  dx -= 1;
    if (in->buttons & IN_RIGHT) dx += 1;
    if (in->buttons & IN_UP)    dy -= 1;
    if (in->buttons & IN_DOWN)  dy += 1;

    pl->ent.box.x += dx;
    pl->ent.box.y += dy;
    if (dx != 0 || dy != 0) { g->player_on_croc[idx] = false; g->player_croc_id[idx] = -1; }

    // Clamp dentro i bordi di gioco (lasciare 1 di bordo)
    int min_x = 1;
    int max_x = GAME_WIDTH - 1 - pl->ent.box.w;
    int min_y = 1;
    int max_y = GAME_HEIGHT - 1 - pl->ent.box.h;
    if (pl->ent.box.x < min_x) pl->ent.box.x = min_x;
    if (pl->ent.box.x > max_x) pl->ent.box.x = max_x;
    if (pl->ent.box.y < min_y) pl->ent.box.y = min_y;
    if (pl->ent.box.y > max_y) pl->ent.box.y = max_y;
}

void game_update(server_game_t* g) {
    uint32_t t = g->tick++;
    // Aggiorna coccodrilli in base alla corsia
    for (int i = 0; i < g->num_crocs; ++i) {
        if (!g->crocs[i].ent.active) continue;
        int lane = lane_for_index(i);
        int div = speed_div_for_lane(lane);
        if (div <= 1 || (t % (uint32_t)div) == 0) {
            int step = (g->crocs[i].dir == DIR_RIGHT ? 1 : -1);
            int nx = g->crocs[i].ent.box.x + step;
            // Wrap con riapparizione dall'altro lato
            if (g->crocs[i].dir == DIR_RIGHT) {
                if (nx + g->crocs[i].ent.box.w >= GAME_WIDTH - 1) {
                    nx = 1 - g->crocs[i].ent.box.w; // rientra da sinistra gradualmente
                }
            } else {
                if (nx <= 0) {
                    nx = GAME_WIDTH - 2; // rientra da destra
                }
            }
            int oldx = g->crocs[i].ent.box.x;
            g->crocs[i].ent.box.x = nx;
            // Se qualche player è attaccato a questo croc, muovilo insieme
            for (int p = 0; p < g->num_players; ++p) {
                if (g->player_on_croc[p] && g->player_croc_id[p] == i) {
                    g->players[p].ent.box.x += (nx - oldx);
                }
            }
        }
    }

    // Timer ~1s: tick server ~5Hz => decrementa ogni 5 tick
    if (t % 5 == 0 && g->timer.remaining_time > 0) {
        g->timer.remaining_time--;
    }

    // Punteggio di prova
    for (int i = 0; i < g->num_players; ++i) g->players[i].score += 1;

    // Collisioni base: acqua/coccodrilli, dens
    int dens_occupied = 0;
    for (int i = 0; i < NUM_TANE; ++i) if (g->dens[i].occupied) dens_occupied++;

    for (int p = 0; p < g->num_players; ++p) {
        player_state_t* pl = &g->players[p];
        int x = pl->ent.box.x;
        int y = pl->ent.box.y;
        int w = pl->ent.box.w;

        // Zona acqua: righe 4..FLOOR_HEIGHT-1
        bool in_water_band = (y >= 4 && y < FLOOR_HEIGHT);
        bool on_croc = false;
        if (in_water_band) {
            int found_id = -1;
            for (int i = 0; i < g->num_crocs; ++i) {
                rect_t c = g->crocs[i].ent.box;
                bool vertical_overlap = (y < c.y + c.h) && (y + pl->ent.box.h > c.y);
                bool horizontal_overlap = (x < c.x + c.w) && (x + w > c.x);
                if (vertical_overlap && horizontal_overlap) { on_croc = true; found_id = i; break; }
            }
            if (on_croc) { g->player_on_croc[p] = true; g->player_croc_id[p] = found_id; }
        } else {
            g->player_on_croc[p] = false; g->player_croc_id[p] = -1;
        }
        if (in_water_band && !on_croc) {
            // Death: lose a life and respawn
            pl->lives -= 1;
            if (pl->lives < 0) pl->lives = 0;
            // Respawn at start
            pl->ent.box.x = (p == 0) ? GAME_WIDTH/2 : (GAME_WIDTH/2 - 6);
            pl->ent.box.y = GAME_HEIGHT - 3;
            // Reset timer for now (simple rule)
            g->timer.remaining_time = g->timer.max_time;
        }

        // Dens reach logic: se la rana arriva alla riga delle tane (y<=1), occupa una tana libera se il centro è dentro
        if (pl->ent.box.y <= 1) {
            int frog_center_x = pl->ent.box.x + (pl->ent.box.w / 2);
            for (int i = 0; i < NUM_TANE; ++i) {
                if (!g->dens[i].occupied) {
                    int tana_x = g->dens[i].x;
                    if (frog_center_x >= tana_x && frog_center_x < tana_x + TANA_WIDTH) {
                        g->dens[i].occupied = true;
                        pl->score += 100;
                        // Respawn after successful den
                        pl->ent.box.x = (p == 0) ? GAME_WIDTH/2 : (GAME_WIDTH/2 - 6);
                        pl->ent.box.y = GAME_HEIGHT - 3;
                        g->timer.remaining_time = g->timer.max_time;
                        dens_occupied++;
                        break;
                    }
                }
            }
        }
    }

    // Vittoria condivisa se tutte le tane sono occupate
    if (dens_occupied >= NUM_TANE) {
        // Flag victory via snapshot builder (we’ll set a field there)
        // Here we can freeze movement by not updating further if desired.
    }
}

void game_build_snapshot(const server_game_t* g, game_snapshot_t* out) {
    memset(out, 0, sizeof(*out));
    out->tick = g->tick;
    out->timer = g->timer;
    out->num_players = g->num_players;
    for (int i = 0; i < g->num_players; ++i) out->players[i] = g->players[i];
    out->num_crocodiles = (uint8_t)g->num_crocs;
    for (int i = 0; i < g->num_crocs; ++i) out->crocodiles[i] = g->crocs[i];
    for (int i = 0; i < NUM_TANE; ++i) out->dens[i] = g->dens[i];
    // game_over: true if any player has 0 lives; victory: all dens occupied
    bool any_dead = false; int dens_occ = 0;
    for (int i = 0; i < g->num_players; ++i) if (g->players[i].lives <= 0) any_dead = true;
    for (int i = 0; i < NUM_TANE; ++i) if (g->dens[i].occupied) dens_occ++;
    out->game_over = any_dead;
    out->victory = (dens_occ >= NUM_TANE);
}
