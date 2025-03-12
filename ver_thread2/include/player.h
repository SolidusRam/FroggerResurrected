#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include "game.h"
#include "buffer.h"

void *player_thread(void *arg);

#endif // PLAYER_H