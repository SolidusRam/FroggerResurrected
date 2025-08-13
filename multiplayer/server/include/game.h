// server/include/game.h — Logica di gioco lato server (POC)
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "../../shared/include/game_types.h"
#include "../../shared/include/protocol.h"

typedef struct {
    uint32_t tick;
    // Giocatori
    uint8_t num_players;
    player_state_t players[MAX_PLAYERS];
    // Stato di attacco al coccodrillo per ciascun giocatore
    bool player_on_croc[MAX_PLAYERS];
    int  player_croc_id[MAX_PLAYERS]; // indice in crocs[], -1 se nessuno
    // Coccodrilli
    crocodile_state_t crocs[MAX_CROCODILES];
    int num_crocs;
    // Tane e timer
    den_state_t dens[NUM_TANE];
    timer_state_t timer;
} server_game_t;

// Inizializzazione stato e posizioni (coccodrilli, tane, player)
void game_init(server_game_t* g);

// Aggiungi/rimuovi giocatore
// Ritorna 0 in caso di successo, -1 se pieno
int game_add_player(server_game_t* g, uint32_t player_id);
void game_remove_player(server_game_t* g, uint32_t player_id);

// Applica input del client al player con player_id (clamp sui bordi)
void game_apply_input(server_game_t* g, uint32_t player_id, const msg_input_t* in);

// Avanza la simulazione (movimento coccodrilli, timer, score)
void game_update(server_game_t* g);

// Costruisce uno snapshot da inviare al client
void game_build_snapshot(const server_game_t* g, game_snapshot_t* out);
