#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stddef.h>

#include "safe99_core/util/timer.h"
#include "safe99_renderer_ddraw/renderer_ddraw.h"

typedef enum tile
{
    TILE_BLIND,
    TILE_OPEN,
    TILE_FLAG,
    TILE_UNKNOWN,
    TILE_OPEN_UNKNOWN,
    TILE_MINE,
    TILE_GAMEOVER_MINE,
    TILE_FLAG_MINE,
    TILE_1,
    TILE_2,
    TILE_3,
    TILE_4,
    TILE_5,
    TILE_6,
    TILE_7,
    TILE_8,
} tile_t;

typedef struct game
{
    int rows;
    int cols;
    int num_mines;
    bool* pa_mines;
    tile_t* pa_tiles;

    timer_t timer;
    size_t count;

    bool b_gameover;

    renderer_ddraw_t* pa_renderer;
} game_t;

bool init_game(HWND hwnd, game_t* p_game, const int rows, const int cols, const int num_mines);
void shutdown_game(game_t* p_game);

void update_game(game_t* p_game);
void draw_game(const game_t* p_game);

#endif // GAME_H