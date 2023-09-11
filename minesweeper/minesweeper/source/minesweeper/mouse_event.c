#include "mouse_event.h"

static mouse_state_t s_left_state;
static mouse_state_t s_right_state;

static int32_t s_x;
static int32_t s_y;

void on_move_mouse(const int32_t x, const int32_t y)
{
    s_x = x;
    s_y = y;
}

void on_down_left_mouse(void)
{
    s_left_state = MOUSE_STATE_DOWN;
}

void on_up_left_mouse(void)
{
    s_left_state = MOUSE_STATE_UP;
}

void on_down_right_mouse(void)
{
    s_right_state = MOUSE_STATE_DOWN;
}

void on_up_right_mouse(void)
{
    s_right_state = MOUSE_STATE_UP;
}

void release_mouse(void)
{
    s_left_state = MOUSE_STATE_UP;
    s_right_state = MOUSE_STATE_UP;
}

int32_t get_mouse_x(void)
{
    return s_x;
}

int32_t get_mouse_y(void)
{
    return s_y;
}

mouse_state_t get_left_mouse_state(void)
{
    return s_left_state;
}

mouse_state_t get_right_mouse_state(void)
{
    return s_right_state;
}