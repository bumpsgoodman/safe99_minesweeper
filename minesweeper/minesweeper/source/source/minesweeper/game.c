#include <stddef.h>
#include <stdlib.h>

#include "game.h"
#include "image_loader.h"
#include "mouse_event.h"
#include "safe99_common/assert.h"
#include "safe99_common/safe_delete.h"

static image_t s_sprite_tiles;
static image_t s_sprite_numbers;
static image_t s_sprite_faces;

static bool load_sprites();
static void unload_sprites();

bool init_game(HWND hwnd, game_t* p_game, const int rows, const int cols, const int num_mines)
{
    ASSERT(p_game != NULL, "p_game == NULL");
    ASSERT(rows >= 9, "width < 9");
    ASSERT(cols >= 9, "height < 9");
    ASSERT(num_mines > 0, "num_mines <= 0");

    p_game->rows = rows;
    p_game->cols = cols;
    p_game->num_mines = num_mines;
    p_game->count = 0;
    p_game->b_gameover = false;

    // 지뢰 초기화
    p_game->pa_mines = (bool*)malloc(sizeof(bool) * rows * cols);
    if (p_game->pa_mines == NULL)
    {
        ASSERT(false, "Failed to malloc mines");
        goto failed_malloc_mines;
    }
    memset(p_game->pa_mines, false, sizeof(bool) * rows * cols);

    p_game->pa_mines[0] = true;
    p_game->pa_mines[2] = true;
    p_game->pa_mines[3] = true;

    // 타일 초기화
    p_game->pa_tiles = (tile_t*)malloc(sizeof(tile_t) * rows * cols);
    if (p_game->pa_tiles == NULL)
    {
        ASSERT(false, "Failed to malloc tiles");
        goto failed_malloc_tiles;
    }
    memset(p_game->pa_tiles, TILE_BLIND, sizeof(tile_t) * rows * cols);

    // 타이머 초기화
    if (!timer_init(&p_game->timer, 1000.0f))
    {
        ASSERT(false, "Failed to init timer");
        goto failed_init_timer;
    }

    // 렌더러 생성
    p_game->pa_renderer = (renderer_ddraw_t*)malloc(sizeof(renderer_ddraw_t));
    if (p_game->pa_renderer == NULL)
    {
        ASSERT(false, "Failed to malloc renderer");
        goto failed_malloc_renderer;
    }

    // 렌더러 초기화
    if (!renderer_ddraw_init(p_game->pa_renderer, hwnd))
    {
        ASSERT(false, "Failed to init renderer");
        goto failed_init_renderer;
    }

    // 스프라이트 로드
    if (!load_sprites())
    {
        ASSERT(false, "Failed to load sprites");
        goto failed_load_sprites;
    }

    return true;

failed_load_sprites:
    renderer_ddraw_release(p_game->pa_renderer);

failed_init_renderer:
    SAFE_FREE(p_game->pa_renderer);

failed_malloc_renderer:
failed_init_timer:
    SAFE_FREE(p_game->pa_tiles);

failed_malloc_tiles:
    SAFE_FREE(p_game->pa_mines);

failed_malloc_mines:
    memset(p_game, 0, sizeof(game_t));
    return false;
}

void shutdown_game(game_t* p_game)
{
    ASSERT(p_game != NULL, "p_game == NULL");

    unload_sprites();

    SAFE_FREE(p_game->pa_tiles);

    memset(p_game, 0, sizeof(game_t));
}

void update_game(game_t* p_game)
{
    ASSERT(p_game != NULL, "p_game == NULL");

    const size_t window_width = renderer_ddraw_get_width(p_game->pa_renderer);
    const size_t window_height = renderer_ddraw_get_height(p_game->pa_renderer);

    if (!p_game->b_gameover)
    {
        timer_update(&p_game->timer);
        if (timer_is_on_tick(&p_game->timer))
        {
            ++p_game->count;
        }
    }

    if (is_left_up_mouse())
    {
        const uint32_t mouse_x = get_mouse_pos_x();
        const uint32_t mouse_y = get_mouse_pos_y();

        // 타일 클릭 시
        if (mouse_x >= 0 && mouse_x < window_width
            && mouse_y >= 48 && mouse_y < window_height)
        {
            // 윈도우 좌표 -> 타일 좌표 변환
            const uint32_t x = mouse_x / 16;
            const uint32_t y = (mouse_y - 48) / 16;

            // 지뢰일 경우
            const bool b_gameover_mine = *(p_game->pa_mines + y * p_game->cols + x);
            if (b_gameover_mine)
            {
                for (size_t i = 0; i < p_game->rows; ++i)
                {
                    for (size_t j = 0; j < p_game->cols; ++j)
                    {
                        const bool b_mine = *(p_game->pa_mines + i * p_game->cols + j);
                        if (b_mine)
                        {
                            *(p_game->pa_tiles + i * p_game->cols + j) = TILE_MINE;
                        }
                    }
                }

                p_game->b_gameover = true;
                *(p_game->pa_tiles + y * p_game->cols + x) = TILE_GAMEOVER_MINE;
            }
        }

        release_mouse();
    }
}

void draw_game(const game_t* p_game)
{
    ASSERT(p_game != NULL, "p_game == NULL");

    const size_t window_width = renderer_ddraw_get_width(p_game->pa_renderer);
    const size_t window_height = renderer_ddraw_get_height(p_game->pa_renderer);

    const size_t width = p_game->cols * 16;
    const size_t height = p_game->rows * 16;

    const size_t start_x = window_width / 2 - width / 2;
    const size_t start_y = window_height - height;

    const size_t start_info_y = 12;

    const size_t start_face_x = window_width / 2 - 12;
    const size_t start_face_y = 12;

    const uint32_t mouse_x = get_mouse_pos_x();
    const uint32_t mouse_y = get_mouse_pos_y();

    size_t face_index = 0;

    renderer_ddraw_begin_draw(p_game->pa_renderer);
    {
        renderer_ddraw_clear(p_game->pa_renderer, 0xffc6c6c6);

        // 지뢰 개수 그리기
        {
            const size_t digit0_index = p_game->num_mines % 10;
            const size_t digit1_index = p_game->num_mines / 10 % 10;
            const size_t digit2_index = p_game->num_mines / 100 % 10;
            renderer_ddraw_draw_bitmap(p_game->pa_renderer, 0, start_info_y, digit0_index * 13, 0, 13, 23, s_sprite_numbers.width, s_sprite_numbers.height, s_sprite_numbers.pa_bitmap);
            renderer_ddraw_draw_bitmap(p_game->pa_renderer, 13, start_info_y, digit1_index * 13, 0, 13, 23, s_sprite_numbers.width, s_sprite_numbers.height, s_sprite_numbers.pa_bitmap);
            renderer_ddraw_draw_bitmap(p_game->pa_renderer, 26, start_info_y, digit2_index * 13, 0, 13, 23, s_sprite_numbers.width, s_sprite_numbers.height, s_sprite_numbers.pa_bitmap);
        }

        // 타이머 그리기
        {
            const size_t digit0_index = p_game->count % 10;
            const size_t digit1_index = p_game->count / 10 % 10;
            const size_t digit2_index = p_game->count / 100 % 10;
            renderer_ddraw_draw_bitmap(p_game->pa_renderer, window_width - 13, start_info_y, digit0_index * 13, 0, 13, 23, s_sprite_numbers.width, s_sprite_numbers.height, s_sprite_numbers.pa_bitmap);
            renderer_ddraw_draw_bitmap(p_game->pa_renderer, window_width - 26, start_info_y, digit1_index * 13, 0, 13, 23, s_sprite_numbers.width, s_sprite_numbers.height, s_sprite_numbers.pa_bitmap);
            renderer_ddraw_draw_bitmap(p_game->pa_renderer, window_width - 39, start_info_y, digit2_index * 13, 0, 13, 23, s_sprite_numbers.width, s_sprite_numbers.height, s_sprite_numbers.pa_bitmap);
        }

        // 타일 그리기
        // 윈도우 좌표 -> 타일 좌표 변환
        uint32_t tile_x = mouse_x / 16;
        uint32_t tile_y = (mouse_y - 48) / 16;

        for (size_t y = 0; y < p_game->rows; ++y)
        {
            for (size_t x = 0; x < p_game->cols; ++x)
            {
                tile_t tile = p_game->pa_tiles[y * p_game->cols + x];
                if (is_left_down_mouse() && tile_x == x && tile_y == y)
                {
                    face_index = 2;

                    if (tile == TILE_BLIND)
                    {
                        tile = TILE_OPEN;
                    }
                }

                switch (tile)
                {
                case TILE_BLIND:
                case TILE_OPEN:
                case TILE_FLAG:
                case TILE_UNKNOWN:
                case TILE_OPEN_UNKNOWN:
                case TILE_MINE:
                case TILE_GAMEOVER_MINE:
                case TILE_FLAG_MINE:
                    renderer_ddraw_draw_bitmap(p_game->pa_renderer, start_x + x * 16, start_y + y * 16, tile * 16, 0, 16, 16, s_sprite_tiles.width, s_sprite_tiles.height, s_sprite_tiles.pa_bitmap);
                    break;
                case TILE_1:
                case TILE_2:
                case TILE_3:
                case TILE_4:
                case TILE_5:
                case TILE_6:
                case TILE_7:
                case TILE_8:
                    renderer_ddraw_draw_bitmap(p_game->pa_renderer, start_x + x * 16, start_y + y * 16, tile / 8 * 16, 16, 16, 16, s_sprite_tiles.width, s_sprite_tiles.height, s_sprite_tiles.pa_bitmap);
                    break;
                default:
                    ASSERT(false, "Invalid tile");
                    break;
                }
            }
        }

        // 얼굴 그리기
        if (is_left_down_mouse() && mouse_x >= start_face_x && mouse_x <= start_face_x + 24
            && mouse_y >= start_face_y && mouse_y <= start_face_y + 24)
        {
            face_index = 1;
        }
        
        if (p_game->b_gameover)
        {
            face_index = 4;
        }

        renderer_ddraw_draw_bitmap(p_game->pa_renderer, start_face_x, start_face_y, face_index * 24, 0, 24, 24, s_sprite_faces.width, s_sprite_faces.height, s_sprite_faces.pa_bitmap);
    }
    renderer_ddraw_end_draw(p_game->pa_renderer);

    renderer_ddraw_on_draw(p_game->pa_renderer);
}

static bool load_sprites()
{
    unload_sprites();

    if (!load_a8r8g8b8_dds("sprite/tiles.dds", &s_sprite_tiles))
    {
        ASSERT(false, "Failed to load tile sprites");
        goto failed_load_sprites;
    }

    if (!load_a8r8g8b8_dds("sprite/numbers.dds", &s_sprite_numbers))
    {
        ASSERT(false, "Failed to load number sprites");
        goto failed_load_sprites;
    }

    if (!load_a8r8g8b8_dds("sprite/faces.dds", &s_sprite_faces))
    {
        ASSERT(false, "Failed to load face sprites");
        goto failed_load_sprites;
    }

    return true;

failed_load_sprites:
    unload_sprites();
    return false;
}

static void unload_sprites()
{
    if (s_sprite_tiles.pa_bitmap != NULL)
    {
        SAFE_FREE(s_sprite_tiles.pa_bitmap);
    }

    if (s_sprite_numbers.pa_bitmap != NULL)
    {
        SAFE_FREE(s_sprite_numbers.pa_bitmap);
    }

    if (s_sprite_faces.pa_bitmap != NULL)
    {
        SAFE_FREE(s_sprite_faces.pa_bitmap);
    }
}