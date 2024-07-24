#include "defines.h"

#ifdef Win32

//TODO: windows

#include <windows.h>
#include <windowsx.h>
#include <stdlib.h>

#include "defines.h"

static HINSTANCE h_instance;
static HWND hwnd;

typedef void (*PFN_on_window_close)();

PFN_on_window_close window_close;

LRESULT CALLBACK win32_process_message(HWND hwnd, u32 msg, WPARAM w_param, LPARAM l_param);

b8 window_create(const char* name, i32 x, i32 y, i32 width, i32 height) {

    h_instance = GetModuleHandleA(0);

    // Setup and register window class.
    WNDCLASSA wc;
    memset(&wc, 0, sizeof(wc));
    wc.style = CS_DBLCLKS;  // Get double-clicks
    wc.lpfnWndProc = win32_process_message;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = h_instance;
    wc.hIcon = LoadIcon(h_instance, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);  // NULL; // Manage the cursor manually
    wc.hbrBackground = NULL;                   // Transparent
    wc.lpszClassName = "triangle_window_class";

    if (!RegisterClassA(&wc)) {
        MessageBoxA(0, "Window registration failed", "Error", MB_ICONEXCLAMATION | MB_OK);
        return false;
    }

    // Create window
    u32 client_x = x;
    u32 client_y = y;
    u32 client_width = width;
    u32 client_height = height;

    u32 window_x = client_x;
    u32 window_y = client_y;
    u32 window_width = client_width;
    u32 window_height = client_height;

    u32 window_style = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_THICKFRAME;
    u32 window_ex_style = WS_EX_APPWINDOW;

    // Obtain the size of the border.
    RECT border_rect = {0, 0, 0, 0};
    AdjustWindowRectEx(&border_rect, window_style, 0, window_ex_style);

    // In this case, the border rectangle is negative.
    window_x += border_rect.left;
    window_y += border_rect.top;

    // Grow by the size of the OS border.
    window_width += border_rect.right - border_rect.left;
    window_height += border_rect.bottom - border_rect.top;

    HWND handle = CreateWindowExA(
        window_ex_style, "triangle_window_class", name,
        window_style, window_x, window_y, window_width, window_height,
        0, 0, h_instance, 0);

    if (handle == 0) {
        MessageBoxA(NULL, "Window creation failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
        return false;
    } else {
        hwnd = handle;
    }
    return true;
}

void window_destroy() {
    DestroyWindow(hwnd);
    UnregisterClassA("triangle_window_class", h_instance);
}

b8 window_show() {
    ShowWindow(hwnd, SW_NORMAL);
    return true;
}

b8 window_hide() {
    ShowWindow(hwnd, SW_HIDE);
    return true;
}

b8 window_get_size(u32 *width, u32 *height) {
    RECT rect;
    if (GetClientRect(hwnd, &rect)) {
        *width = rect.right - rect.left;
        *height = rect.bottom - rect.top;
        return true;
    }

    return false;
}

b8 window_pump_messages() {
    MSG message;
    while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
    return true;
}

LRESULT CALLBACK win32_process_message(HWND hwnd, u32 msg, WPARAM w_param, LPARAM l_param) {
    switch (msg) {
        case WM_ERASEBKGND:
            // Notify the OS that erasing will be handled by the application to prevent flicker.
            return 1;
        case WM_CLOSE:
            window_close();
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_SIZE: {
            // Get the updated size.
            // RECT r;
            // GetClientRect(hwnd, &r);
            // u32 width = r.right - r.left;
            // u32 height = r.bottom - r.top;

            // TODO: Fire an event for window resize.
        } break;
    }

    return DefWindowProcA(hwnd, msg, w_param, l_param);
}

#endif