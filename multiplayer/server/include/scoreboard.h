// scoreboard.h — Classifica persistente su file, sicura per accessi concorrenti (flock)
#pragma once

#include <stdint.h>

#define SCOREBOARD_PATH "server/scoreboard.dat"

typedef struct {
    uint32_t player_id;
    int32_t  score;
    long     timestamp;
} scoreboard_entry_t;

// Appende un record punteggio-giocatore al file di classifica.
// Ritorna 0 in caso di successo, -1 in caso di errore I/O.
int scoreboard_record(uint32_t player_id, int32_t score);

// Legge il file di classifica e restituisce (in out) i primi max_n punteggi
// più alti, in ordine decrescente. Ritorna il numero di voci scritte in out.
int scoreboard_get_top(scoreboard_entry_t* out, int max_n);
