#pragma once
#include "defines.h"

typedef struct Win32State {
    struct HINSTANCE__* h_instance;
    struct HWND__* hwnd;
} Win32State;
