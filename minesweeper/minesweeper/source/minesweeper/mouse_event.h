#ifndef MOUSE_EVENT_H
#define MOUSE_EVENT_H

#include <stdbool.h>
#include <stdint.h>

typedef enum mouse_state
{
    MOUSE_STATE_UP,
    MOUSE_STATE_DOWN
} mouse_state_t;

void on_move_mouse(const int32_t x, const int32_t y);
void on_down_left_mouse(void);
void on_up_left_mouse(void);
void on_down_right_mouse(void);
void on_up_right_mouse(void);

void release_mouse(void);

int32_t get_mouse_x(void);
int32_t get_mouse_y(void);

mouse_state_t get_left_mouse_state(void);
mouse_state_t get_right_mouse_state(void);

#endif // MOUSE_EVENT_H