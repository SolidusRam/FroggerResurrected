#ifndef CROCODILE_H
#define CROCODILE_H

#include "game.h"
#include "buffer.h"
#include <pthread.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// Struttura per passare argomenti al thread coccodrillo
typedef struct {
    int id;
} CrocodileArg;

// Funzione principale del thread coccodrillo
void *crocodile_thread(void *arg);

// Funzioni di supporto per la gestione dei bordi e del movimento
static void handle_border_collision(Position *pos, int *original_width);
static void update_position(Position *pos, int direction);

#endif // CROCODILE_H