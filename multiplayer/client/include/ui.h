#pragma once
#include <ncurses.h>
#include <stdint.h>
#include "../../shared/include/game_types.h"

void ui_init(void);
void ui_teardown(void);
// local_player_id: id del giocatore locale per rendere gli altri "ghost"
void ui_draw_snapshot(const game_snapshot_t* snap, uint32_t local_player_id);
