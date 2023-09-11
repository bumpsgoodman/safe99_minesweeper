#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "game.h"
#include "image_loader.h"
#include "mouse_event.h"
#include "safe99_common/assert.h"
#include "safe99_common/safe_delete.h"

static size_t depth = 0;

static image_t s_sprite_tiles;
static image_t s_sprite_numbers;
static image_t s_sprite_faces;

static bool load_sprites();
static void unload_sprites();

static void make_mine(bool* p_mines, const size_t rows, const size_t cols, const size_t num_mines);

static bool is_valid_position(const game_t* p_game, const size_t x, const size_t y);
static void open_tile_recursion(game_t* p_game, const size_t x, const size_t y);
static void open_tile(game_t* p_game, const size_t x, const size_t y);

bool init_game(HWND hwnd, game_t* p_game, const int rows, const int cols, const int num_mines)
{
    ASSERT(p_game != NULL, "p_game == NULL");
    ASSERT(rows >= 9, "width < 9");
    ASSERT(cols >= 9, "height < 9");
    ASSERT(num_mines > 0, "num_mines == 0");

    p_game->rows = rows;
    p_game->cols = cols;
    p_game->num_mines = num_mines;
    p_game->num_max_mines = num_mines;
    p_game->count = 0;
    p_game->b_gameover = false;
    p_game->b_left_mouse_pressed = false;
    p_game->num_tiles = rows * cols;

    srand((unsigned int)time(NULL));

    // 지뢰 초기화
    p_game->pa_mines = (bool*)malloc(sizeof(bool) * rows * cols);
    if (p_game->pa_mines == NULL)
    {
        ASSERT(false, "Failed to malloc mines");
        goto failed_malloc_mines;
    }
    memset(p_game->pa_mines, false, sizeof(bool) * rows * cols);

    // 타일 초기화
    p_game->pa_tiles = (tile_t*)malloc(sizeof(tile_t) * rows * cols);
    if (p_game->pa_tiles == NULL)
    {
        ASSERT(false, "Failed to malloc tiles");
        goto failed_malloc_tiles;
    }
    memset(p_game->pa_tiles, TILE_BLIND, sizeof(tile_t) * rows * cols);

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

    // 타이머 초기화
    if (!timer_init(&p_game->timer, 1000.0f))
    {
        ASSERT(false, "Failed to init timer");
        goto failed_init_timer;
    }

    const size_t window_width = renderer_ddraw_get_width(p_game->pa_renderer);
    const size_t window_height = renderer_ddraw_get_height(p_game->pa_renderer);

    p_game->face_x = window_width / 2 - SPRITE_FACE_WIDTH / 2;
    p_game->face_y = INFO_HEIGHT / 2 - SPRITE_FACE_HEIGHT / 2;

    make_mine(p_game->pa_mines, rows, cols, num_mines);

    return true;

failed_init_timer:
    unload_sprites();

failed_load_sprites:
    renderer_ddraw_release(p_game->pa_renderer);

failed_init_renderer:
    SAFE_FREE(p_game->pa_renderer);

failed_malloc_renderer:
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

    SAFE_FREE(p_game->pa_renderer);
    SAFE_FREE(p_game->pa_tiles);
    SAFE_FREE(p_game->pa_mines);

    memset(p_game, 0, sizeof(game_t));
}

void update_game(game_t* p_game)
{
    ASSERT(p_game != NULL, "p_game == NULL");

    const size_t WINDOW_WIDTH = renderer_ddraw_get_width(p_game->pa_renderer);
    const size_t WINDOW_HEIGHT = renderer_ddraw_get_height(p_game->pa_renderer);

    const size_t ROWS = (size_t)p_game->rows;
    const size_t COLS = (size_t)p_game->cols;

    if (!p_game->b_left_mouse_pressed && get_left_mouse_state() == MOUSE_STATE_DOWN)
    {
        p_game->b_left_mouse_pressed = true;
    }

    if (p_game->b_right_mouse_pressed && get_right_mouse_state() == MOUSE_STATE_UP)
    {
        p_game->b_right_mouse_pressed = false;
    }

    if (p_game->b_left_mouse_pressed && get_left_mouse_state() == MOUSE_STATE_UP)
    {
        const size_t mouse_x = (size_t)get_mouse_x();
        const size_t mouse_y = (size_t)get_mouse_y();

        // 얼굴 클릭 시 게임 재시작
        if (mouse_x >= p_game->face_x && mouse_x <= p_game->face_x + SPRITE_FACE_WIDTH
            && mouse_y >= p_game->face_y && mouse_y <= p_game->face_y + SPRITE_FACE_HEIGHT)
        {
            p_game->num_mines = p_game->num_max_mines;
            p_game->num_tiles = (int)p_game->rows * (int)p_game->cols;
            p_game->count = 0;
            p_game->b_gameover = false;

            timer_reset(&p_game->timer);

            memset(p_game->pa_mines, false, sizeof(bool) * p_game->rows * p_game->cols);
            memset(p_game->pa_tiles, TILE_BLIND, sizeof(tile_t) * p_game->rows * p_game->cols);
            make_mine(p_game->pa_mines, p_game->rows, p_game->cols, p_game->num_max_mines);

            p_game->b_left_mouse_pressed = false;
            p_game->b_right_mouse_pressed = false;
        }

        if (p_game->b_gameover)
        {
            p_game->b_left_mouse_pressed = false;
        }
    }

    if (p_game->b_gameover)
    {
        return;
    }

    timer_update(&p_game->timer);
    if (timer_is_on_tick(&p_game->timer))
    {
        ++p_game->count;
    }

    if (p_game->b_left_mouse_pressed && get_left_mouse_state() == MOUSE_STATE_UP)
    {
        const size_t mouse_x = (size_t)get_mouse_x();
        const size_t mouse_y = (size_t)get_mouse_y();

        // 스크린 좌표 -> 타일 좌표 변환
        const size_t tile_x = mouse_x / SPRITE_TILE_WIDTH;
        const size_t tile_y = (mouse_y - INFO_HEIGHT) / SPRITE_TILE_HEIGHT;

        // 타일 클릭 시
        if (mouse_x >= 0 && mouse_x < WINDOW_WIDTH
            && mouse_y >= INFO_HEIGHT && mouse_y < WINDOW_HEIGHT)
        {
            // 지뢰일 경우
            const bool b_gameover_mine = p_game->pa_mines[tile_y * p_game->cols + tile_x];
            if (b_gameover_mine)
            {
                // 지뢰가 있는 타일 열기
                for (size_t i = 0; i < p_game->rows; ++i)
                {
                    for (size_t j = 0; j < p_game->cols; ++j)
                    {
                        const bool b_mine = p_game->pa_mines[i * p_game->cols + j];
                        if (b_mine)
                        {
                            p_game->pa_tiles[i * p_game->cols + j] = TILE_MINE;
                        }
                    }
                }

                p_game->pa_tiles[tile_y * p_game->cols + tile_x] = TILE_GAMEOVER_MINE;
                p_game->b_gameover = true;
            }
            else if (p_game->pa_tiles[tile_y * p_game->cols + tile_x] != TILE_FLAG)
            {
                //open_tile_recursion(p_game, tile_x, tile_y);
                //printf("depth: %llu", depth);
                //depth = 0;
                open_tile(p_game, tile_x, tile_y);

                // 남은 타일의 수와 지뢰 개수가 같으면 승리
                if (p_game->num_tiles == p_game->num_max_mines)
                {
                    p_game->num_mines = 0;
                    p_game->b_gameover = true;
                }
            }
        }

        p_game->b_left_mouse_pressed = false;
    }

    if (!p_game->b_right_mouse_pressed && get_right_mouse_state() == MOUSE_STATE_DOWN)
    {
        const size_t mouse_x = (size_t)get_mouse_x();
        const size_t mouse_y = (size_t)get_mouse_y();

        if (mouse_x >= 0 && mouse_x < WINDOW_WIDTH
            && mouse_y >= INFO_HEIGHT && mouse_y < WINDOW_HEIGHT)
        {
            // 스크린 좌표 -> 타일 좌표 변환
            const size_t tile_x = mouse_x / SPRITE_TILE_WIDTH;
            const size_t tile_y = (mouse_y - INFO_HEIGHT) / SPRITE_TILE_HEIGHT;

            const tile_t tile = p_game->pa_tiles[tile_y * p_game->cols + tile_x];
            switch (tile)
            {
            case TILE_BLIND:
                --p_game->num_mines;
                p_game->pa_tiles[tile_y * p_game->cols + tile_x] = TILE_FLAG;
                break;
            case TILE_FLAG:
                p_game->num_mines++;
                p_game->pa_tiles[tile_y * p_game->cols + tile_x] = TILE_UNKNOWN;
                break;
            case TILE_UNKNOWN:
                p_game->pa_tiles[tile_y * p_game->cols + tile_x] = TILE_BLIND;
                break;
            default:
                break;
            }
        }

        p_game->b_right_mouse_pressed = true;
    }
}

void draw_game(const game_t* p_game)
{
    ASSERT(p_game != NULL, "p_game == NULL");

    const size_t WINDOW_WIDTH = renderer_ddraw_get_width(p_game->pa_renderer);
    const size_t WINDOW_HEIGHT = renderer_ddraw_get_height(p_game->pa_renderer);

    const int32_t NUM_MINES_DIGIT0_X = SPRITE_NUMBER_WIDTH * 2;
    const int32_t NUM_MINES_DIGIT0_Y = INFO_HEIGHT / 2 - SPRITE_NUMBER_HEIGHT / 2;
    const int32_t NUM_MINES_DIGIT1_X = SPRITE_NUMBER_WIDTH * 1;
    const int32_t NUM_MINES_DIGIT1_Y = INFO_HEIGHT / 2 - SPRITE_NUMBER_HEIGHT / 2;
    const int32_t NUM_MINES_DIGIT2_X = SPRITE_NUMBER_WIDTH * 0;
    const int32_t NUM_MINES_DIGIT2_Y = INFO_HEIGHT / 2 - SPRITE_NUMBER_HEIGHT / 2;

    const int32_t TIMER_DIGIT0_X = (int32_t)WINDOW_WIDTH - SPRITE_NUMBER_WIDTH * 1;
    const int32_t TIMER_DIGIT0_Y = INFO_HEIGHT / 2 - SPRITE_NUMBER_HEIGHT / 2;
    const int32_t TIMER_DIGIT1_X = (int32_t)WINDOW_WIDTH - SPRITE_NUMBER_WIDTH * 2;
    const int32_t TIMER_DIGIT1_Y = INFO_HEIGHT / 2 - SPRITE_NUMBER_HEIGHT / 2;
    const int32_t TIMER_DIGIT2_X = (int32_t)WINDOW_WIDTH - SPRITE_NUMBER_WIDTH * 3;
    const int32_t TIMER_DIGIT2_Y = INFO_HEIGHT / 2 - SPRITE_NUMBER_HEIGHT / 2;

    const int32_t START_TILE_X = 0;
    const int32_t START_TILE_Y = INFO_HEIGHT;

    const uint32_t mouse_x = get_mouse_x();
    const uint32_t mouse_y = get_mouse_y();

    uint32_t face_index = 0;

    renderer_ddraw_begin_draw(p_game->pa_renderer);
    {
        renderer_ddraw_clear(p_game->pa_renderer, 0xffc6c6c6);

        // 지뢰 개수 그리기
        {
            const int32_t digit0_index = (p_game->num_mines <= 0) ? 0 : p_game->num_mines % 10;
            const int32_t digit1_index = (p_game->num_mines <= 0) ? 0 : p_game->num_mines / 10 % 10;
            const int32_t digit2_index = (p_game->num_mines <= 0) ? 0 : p_game->num_mines / 100 % 10;
            renderer_ddraw_draw_bitmap(p_game->pa_renderer, NUM_MINES_DIGIT0_X, NUM_MINES_DIGIT0_Y, digit0_index * SPRITE_NUMBER_WIDTH, 0, SPRITE_NUMBER_WIDTH, SPRITE_NUMBER_HEIGHT, s_sprite_numbers.width, s_sprite_numbers.height, s_sprite_numbers.pa_bitmap);
            renderer_ddraw_draw_bitmap(p_game->pa_renderer, NUM_MINES_DIGIT1_X, NUM_MINES_DIGIT1_Y, digit1_index * SPRITE_NUMBER_WIDTH, 0, SPRITE_NUMBER_WIDTH, SPRITE_NUMBER_HEIGHT, s_sprite_numbers.width, s_sprite_numbers.height, s_sprite_numbers.pa_bitmap);
            renderer_ddraw_draw_bitmap(p_game->pa_renderer, NUM_MINES_DIGIT2_X, NUM_MINES_DIGIT2_Y, digit2_index * SPRITE_NUMBER_WIDTH, 0, SPRITE_NUMBER_WIDTH, SPRITE_NUMBER_HEIGHT, s_sprite_numbers.width, s_sprite_numbers.height, s_sprite_numbers.pa_bitmap);
        }

        // 타이머 그리기
        {
            const int32_t digit0_index = p_game->count % 10;
            const int32_t digit1_index = p_game->count / 10 % 10;
            const int32_t digit2_index = p_game->count / 100 % 10;
            renderer_ddraw_draw_bitmap(p_game->pa_renderer, TIMER_DIGIT0_X, TIMER_DIGIT0_Y, digit0_index * SPRITE_NUMBER_WIDTH, 0, SPRITE_NUMBER_WIDTH, SPRITE_NUMBER_HEIGHT, s_sprite_numbers.width, s_sprite_numbers.height, s_sprite_numbers.pa_bitmap);
            renderer_ddraw_draw_bitmap(p_game->pa_renderer, TIMER_DIGIT1_X, TIMER_DIGIT1_Y, digit1_index * SPRITE_NUMBER_WIDTH, 0, SPRITE_NUMBER_WIDTH, SPRITE_NUMBER_HEIGHT, s_sprite_numbers.width, s_sprite_numbers.height, s_sprite_numbers.pa_bitmap);
            renderer_ddraw_draw_bitmap(p_game->pa_renderer, TIMER_DIGIT2_X, TIMER_DIGIT2_Y, digit2_index * SPRITE_NUMBER_WIDTH, 0, SPRITE_NUMBER_WIDTH, SPRITE_NUMBER_HEIGHT, s_sprite_numbers.width, s_sprite_numbers.height, s_sprite_numbers.pa_bitmap);
        }

        // 타일 그리기
        // 윈도우 좌표 -> 타일 좌표 변환
        uint32_t tile_x = mouse_x / SPRITE_TILE_WIDTH;
        uint32_t tile_y = (mouse_y - INFO_HEIGHT) / SPRITE_TILE_HEIGHT;

        for (size_t y = 0; y < p_game->rows; ++y)
        {
            for (size_t x = 0; x < p_game->cols; ++x)
            {
                tile_t tile = p_game->pa_tiles[y * p_game->cols + x];
                if (get_left_mouse_state() == MOUSE_STATE_DOWN && tile_x == x && tile_y == y)
                {
                    face_index = 2;

                    if (!p_game->b_gameover && tile == TILE_BLIND)
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
                    if (p_game->b_gameover && p_game->num_tiles == p_game->num_max_mines
                        && tile == TILE_BLIND)
                    {
                        tile = TILE_FLAG;
                    }

                    renderer_ddraw_draw_bitmap(p_game->pa_renderer, START_TILE_X + (int32_t)x * SPRITE_TILE_WIDTH, START_TILE_Y + (int32_t)y * SPRITE_TILE_HEIGHT, tile * SPRITE_TILE_WIDTH, 0, SPRITE_TILE_WIDTH, SPRITE_TILE_HEIGHT, s_sprite_tiles.width, s_sprite_tiles.height, s_sprite_tiles.pa_bitmap);
                    break;
                case TILE_1:
                case TILE_2:
                case TILE_3:
                case TILE_4:
                case TILE_5:
                case TILE_6:
                case TILE_7:
                case TILE_8:
                    renderer_ddraw_draw_bitmap(p_game->pa_renderer, START_TILE_X + (int32_t)x * SPRITE_TILE_WIDTH, START_TILE_Y + (int32_t)y * SPRITE_TILE_HEIGHT, (tile - 8) * SPRITE_TILE_WIDTH, SPRITE_TILE_HEIGHT, SPRITE_TILE_WIDTH, SPRITE_TILE_HEIGHT, s_sprite_tiles.width, s_sprite_tiles.height, s_sprite_tiles.pa_bitmap);
                    break;
                default:
                    ASSERT(false, "Invalid tile");
                    break;
                }
            }
        }

        // 얼굴 그리기
        if (get_left_mouse_state() == MOUSE_STATE_DOWN
            && mouse_x >= p_game->face_x && mouse_x <= p_game->face_x + SPRITE_FACE_WIDTH
            && mouse_y >= p_game->face_y && mouse_y <= p_game->face_y + SPRITE_FACE_HEIGHT)
        {
            face_index = 1;
        }

        if (p_game->b_gameover)
        {
            face_index = (p_game->num_tiles == p_game->num_max_mines) ? 3 : 4;
        }

        renderer_ddraw_draw_bitmap(p_game->pa_renderer, (int32_t)p_game->face_x, (int32_t)p_game->face_y, face_index * SPRITE_FACE_WIDTH, 0, SPRITE_FACE_WIDTH, SPRITE_FACE_HEIGHT, s_sprite_faces.width, s_sprite_faces.height, s_sprite_faces.pa_bitmap);
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

static void make_mine(bool* p_mines, const size_t rows, const size_t cols, const size_t num_mines)
{
    ASSERT(p_mines != NULL, "p_mines == NULL");

    size_t count = 0;
    while (count != num_mines)
    {
        const size_t index = rand() % (rows * cols);
        //const size_t x = rand() % cols;
        //const size_t y = rand() % rows;
        
        if (p_mines[index])
        {
            continue;
        }

        p_mines[index] = true;

        ++count;
    }

    for (size_t y = 0; y < rows; ++y)
    {
        for (size_t x = 0; x < cols; ++x)
        {
            printf("%c ", p_mines[y * cols + x] == true ? 'o' : '.');
        }
        printf("\n");
    }
    printf("\n");
}

static bool is_valid_position(const game_t* p_game, const size_t x, const size_t y)
{
    ASSERT(p_game != NULL, "p_game == NULL");
    return (x < p_game->cols && y < p_game->rows);
}

static void open_tile_recursion(game_t* p_game, const size_t x, const size_t y)
{
    ASSERT(p_game != NULL, "p_game == NULL");
    ++depth;

    if (!is_valid_position(p_game, x, y))
    {
        return;
    }

    const tile_t tile = p_game->pa_tiles[y * p_game->cols + x];
    if (tile != TILE_BLIND && tile != TILE_FLAG && tile != TILE_UNKNOWN)
    {
        return;
    }

    size_t count = 0;
    count += (is_valid_position(p_game, x, y - 1) && p_game->pa_mines[(y - 1) * p_game->cols + x]);
    count += (is_valid_position(p_game, x, y + 1) && p_game->pa_mines[(y + 1) * p_game->cols + x]);
    count += (is_valid_position(p_game, x - 1, y) && p_game->pa_mines[y * p_game->cols + (x - 1)]);
    count += (is_valid_position(p_game, x + 1, y) && p_game->pa_mines[y * p_game->cols + (x + 1)]);
    count += (is_valid_position(p_game, x - 1, y - 1) && p_game->pa_mines[(y - 1) * p_game->cols + (x - 1)]);
    count += (is_valid_position(p_game, x + 1, y - 1) && p_game->pa_mines[(y - 1) * p_game->cols + (x + 1)]);
    count += (is_valid_position(p_game, x - 1, y + 1) && p_game->pa_mines[(y + 1) * p_game->cols + (x - 1)]);
    count += (is_valid_position(p_game, x + 1, y + 1) && p_game->pa_mines[(y + 1) * p_game->cols + (x + 1)]);

    if (p_game->pa_tiles[y * p_game->cols + x] == TILE_FLAG)
    {
        ++p_game->num_mines;
    }

    if (count == 0)
    {
        p_game->pa_tiles[y * p_game->cols + x] = TILE_OPEN;

        open_tile_recursion(p_game, x, y - 1);
        open_tile_recursion(p_game, x, y + 1);
        open_tile_recursion(p_game, x - 1, y);
        open_tile_recursion(p_game, x + 1, y);
        open_tile_recursion(p_game, x - 1, y - 1);
        open_tile_recursion(p_game, x + 1, y - 1);
        open_tile_recursion(p_game, x - 1, y + 1);
        open_tile_recursion(p_game, x + 1, y + 1);
    }
    else
    {
        p_game->pa_tiles[y * p_game->cols + x] = TILE_1 + count - 1;
    }

    --p_game->num_tiles;
}

static void open_tile(game_t* p_game, const size_t x, const size_t y)
{
    ASSERT(p_game != NULL, "p_game == NULL");

    // 재귀 횟수 근사값
    // (rows * cols - num_max_mines) * 8

    size_t stack_index = 0;
    size_t* stack_x = (size_t*)malloc(sizeof(size_t) * (p_game->rows * p_game->cols - p_game->num_max_mines) * 8);
    size_t* stack_y = (size_t*)malloc(sizeof(size_t) * (p_game->rows * p_game->cols - p_game->num_max_mines) * 8);
    ASSERT(stack_x != NULL, "Failed to malloc stack_x");
    ASSERT(stack_y != NULL, "Failed to malloc stack_y");

    stack_x[stack_index] = x;
    stack_y[stack_index] = y;
    ++stack_index;

    while (stack_index > 0)
    {
        --stack_index;
        const size_t tile_x = stack_x[stack_index];
        const size_t tile_y = stack_y[stack_index];

        if (!is_valid_position(p_game, tile_x, tile_y))
        {
            continue;
        }

        const tile_t tile = p_game->pa_tiles[tile_y * p_game->cols + tile_x];
        if (tile != TILE_BLIND && tile != TILE_FLAG && tile != TILE_UNKNOWN)
        {
            continue;
        }

        size_t count = 0;
        count += (is_valid_position(p_game, tile_x, tile_y - 1) && p_game->pa_mines[(tile_y - 1) * p_game->cols + tile_x]);
        count += (is_valid_position(p_game, tile_x, tile_y + 1) && p_game->pa_mines[(tile_y + 1) * p_game->cols + tile_x]);
        count += (is_valid_position(p_game, tile_x - 1, tile_y) && p_game->pa_mines[tile_y * p_game->cols + (tile_x - 1)]);
        count += (is_valid_position(p_game, tile_x + 1, tile_y) && p_game->pa_mines[tile_y * p_game->cols + (tile_x + 1)]);
        count += (is_valid_position(p_game, tile_x - 1, tile_y - 1) && p_game->pa_mines[(tile_y - 1) * p_game->cols + (tile_x - 1)]);
        count += (is_valid_position(p_game, tile_x + 1, tile_y - 1) && p_game->pa_mines[(tile_y - 1) * p_game->cols + (tile_x + 1)]);
        count += (is_valid_position(p_game, tile_x - 1, tile_y + 1) && p_game->pa_mines[(tile_y + 1) * p_game->cols + (tile_x - 1)]);
        count += (is_valid_position(p_game, tile_x + 1, tile_y + 1) && p_game->pa_mines[(tile_y + 1) * p_game->cols + (tile_x + 1)]);

        if (p_game->pa_tiles[tile_y * p_game->cols + tile_x] == TILE_FLAG)
        {
            ++p_game->num_mines;
        }

        if (count == 0)
        {
            p_game->pa_tiles[tile_y * p_game->cols + tile_x] = TILE_OPEN;

            stack_x[stack_index] = tile_x;
            stack_y[stack_index] = tile_y - 1;
            ++stack_index;

            stack_x[stack_index] = tile_x;
            stack_y[stack_index] = tile_y + 1;
            ++stack_index;

            stack_x[stack_index] = tile_x - 1;
            stack_y[stack_index] = tile_y;
            ++stack_index;

            stack_x[stack_index] = tile_x + 1;
            stack_y[stack_index] = tile_y;
            ++stack_index;

            stack_x[stack_index] = tile_x - 1;
            stack_y[stack_index] = tile_y - 1;
            ++stack_index;

            stack_x[stack_index] = tile_x + 1;
            stack_y[stack_index] = tile_y - 1;
            ++stack_index;

            stack_x[stack_index] = tile_x - 1;
            stack_y[stack_index] = tile_y + 1;
            ++stack_index;

            stack_x[stack_index] = tile_x + 1;
            stack_y[stack_index] = tile_y + 1;
            ++stack_index;
        }
        else
        {
            p_game->pa_tiles[tile_y * p_game->cols + tile_x] = TILE_1 + count - 1;
        }

        --p_game->num_tiles;
    }
}