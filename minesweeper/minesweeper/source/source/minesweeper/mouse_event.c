#include "mouse_event.h"

static bool sb_left_down = false;
static bool sb_left_up = false;
static int32_t s_left_down_x;
static int32_t s_left_down_y;
static int32_t s_left_up_x;
static int32_t s_left_up_y;
static int32_t s_x;
static int32_t s_y;

void on_move_mouse(const int32_t x, const int32_t y)
{
    s_x = x;
    s_y = y;
}

void on_left_down_mouse(const int32_t x, const int32_t y)
{
    sb_left_down = true;
    sb_left_up = false;
    s_left_down_x = x;
    s_left_down_y = y;
}

void on_left_up_mouse(const int32_t x, const int32_t y)
{
    sb_left_down = false;
    sb_left_up = true;
    s_left_up_x = x;
    s_left_up_y = y;
}

void release_mouse(void)
{
    sb_left_down = false;
    sb_left_up = false;
    s_left_down_x = -1;
    s_left_down_y = -1;
    s_left_up_x = -1;
    s_left_up_y = -1;
    s_x = -1;
    s_y = -1;
}

bool is_left_down_mouse(void)
{
    return sb_left_down;
}

bool is_left_up_mouse(void)
{
    return sb_left_up;
}

int32_t get_mouse_pos_x(void)
{
    return s_x;
}

int32_t get_mouse_pos_y(void)
{
    return s_y;
}

int32_t get_mouse_left_down_x(void)
{
    return s_left_down_x;
}

int32_t get_mouse_left_down_y(void)
{
    return s_left_down_y;
}

int32_t get_mouse_left_up_x(void)
{
    return s_left_up_x;
}

int32_t get_mouse_left_up_y(void)
{
    return s_left_up_y;
}