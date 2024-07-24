#pragma once

#include "defines.h"

typedef struct Window {
    void* internal_state;
}Window;

b8 platform_create_window(const char* window_name, u32 pos_x, u32 pos_y, u32 width, u32 height, Window* window);
void platform_destroy_window(Window* window);
b8 platform_show_window(Window* window);
b8 platform_hide_window(Window* window);
b8 platform_process_window_messages(Window* window);

void platform_console_write(const char* message, u8 colour);
void platform_console_write_error(const char* message, u8 colour);