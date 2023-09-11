#ifndef MOUSE_EVENT_H
#define MOUSE_EVENT_H

#include <stdbool.h>
#include <stdint.h>

void on_move_mouse(const int32_t x, const int32_t y);
void on_left_down_mouse(const int32_t x, const int32_t y);
void on_left_up_mouse(const int32_t x, const int32_t y);

void release_mouse(void);

bool is_left_down_mouse(void);
bool is_left_up_mouse(void);

int32_t get_mouse_pos_x(void);
int32_t get_mouse_pos_y(void);
int32_t get_mouse_left_down_x(void);
int32_t get_mouse_left_down_y(void);
int32_t get_mouse_left_up_x(void);
int32_t get_mouse_left_up_y(void);

#endif // MOUSE_EVENT_H