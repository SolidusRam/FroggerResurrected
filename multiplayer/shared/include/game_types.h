// game_types.h — Tipi base condivisi tra server e client
// Obiettivo didattico: definire uno "snapshot" di stato minimale che il server invia ai client.

#pragma once

#include <stdint.h>
#include <stdbool.h>

// Allineati alla versione singleplayer/ver_thread/include/game.h
enum { GAME_WIDTH = 80, GAME_HEIGHT = 24 };
enum { FLOOR_HEIGHT = 20, LANE_HEIGHT = 2 };
enum { MAX_PLAYERS = 2, MAX_CROCODILES = 16, MAX_BULLETS = 100 };
enum { NUM_TANE = 5, TANA_WIDTH = 7, TANA_HEIGHT = 1 };

typedef enum { DIR_NONE=0, DIR_LEFT=-1, DIR_RIGHT=1, DIR_UP=2, DIR_DOWN=3 } direction_t;

typedef struct {
    int32_t x, y;
    int32_t w, h;
} rect_t;

typedef struct {
    int32_t id;
    rect_t  box;       // posizione/ingombro
    bool    active;
} entity_t;

typedef struct {
    int32_t id;
    entity_t ent;
    int32_t lives;
    int32_t score;
    bool    connected;
} player_state_t;

typedef struct {
    int32_t id;
    entity_t ent;
    direction_t dir;
} crocodile_state_t;

typedef struct {
    int32_t id;
    entity_t ent;
    direction_t dir;
    bool enemy; // true = proiettile del coccodrillo
} bullet_state_t;

typedef struct {
    int32_t x;
    int32_t y;
    bool    occupied;
} den_state_t;

typedef struct {
    int32_t max_time;
    int32_t remaining_time;
} timer_state_t;

// Snapshot completo inviato dal server al client (per ora semplice e "grezzo")
typedef struct {
    uint32_t tick;
    uint8_t  num_players;
    player_state_t players[MAX_PLAYERS];

    uint8_t  num_crocodiles;
    crocodile_state_t crocodiles[MAX_CROCODILES];

    uint8_t  num_bullets;
    bullet_state_t bullets[MAX_BULLETS];

    den_state_t dens[NUM_TANE];
    timer_state_t timer;

    bool game_over;
    bool victory; // true se tutte le tane sono occupate (vittoria condivisa)
} game_snapshot_t;
