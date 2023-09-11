#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stddef.h>

#include "safe99_core/util/timer.h"
#include "safe99_renderer_ddraw/renderer_ddraw.h"

#define SPRITE_TILE_WIDTH 16
#define SPRITE_TILE_HEIGHT 16
#define SPRITE_NUMBER_WIDTH 13
#define SPRITE_NUMBER_HEIGHT 23
#define SPRITE_FACE_WIDTH 24
#define SPRITE_FACE_HEIGHT 24

#define INFO_HEIGHT 48

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
    size_t rows;
    size_t cols;
    int num_mines;
    int num_max_mines;
    int num_tiles;
    bool* pa_mines;
    tile_t* pa_tiles;

    renderer_ddraw_t* pa_renderer;

    size_t face_x;
    size_t face_y;

    timer_t timer;
    size_t count;

    bool b_gameover;

    bool b_left_mouse_pressed;
    bool b_right_mouse_pressed;
} game_t;

bool init_game(HWND hwnd, game_t* p_game, const int rows, const int cols, const int num_mines);
void shutdown_game(game_t* p_game);

void update_game(game_t* p_game);
void draw_game(const game_t* p_game);

#endif // GAME_H