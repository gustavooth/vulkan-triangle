#include "defines.h"

#ifdef PLATFORM_WAYLAND
#include "platform_wayland.h"
#include "platform/platform.h"
#include "core/logger.h"
#include "core/events.h"

#include <wayland-client.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "xdg-shell-client-protocol.h"

#define DRM_FORMAT_ARGB8888 0x34325241

static void global_registry_handler(void* data, struct wl_registry *registry, u32 id,
	const char *interface, u32 version);
static void global_registry_remover(void* data, struct wl_registry *registry, u32 id);
static const struct wl_registry_listener registry_listener = {
    global_registry_handler,
    global_registry_remover
};

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *shell, u32 serial);
static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping,
};

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, u32 serial);
static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure,
};

static void xdg_toplevel_configure (void *data, struct xdg_toplevel *xdg_toplevel, i32 width, 
    i32 height, struct wl_array *states);
void xgd_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel);
void configure_bounds(void *data, struct xdg_toplevel *xdg_toplevel, i32 width, i32 height);
void wm_capabilities(void *data, struct xdg_toplevel *xdg_toplevel, struct wl_array *capabilities);
static const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure,
    .close = xgd_toplevel_close,
    .configure_bounds = configure_bounds,
    .wm_capabilities = wm_capabilities,
};

void frame_new(void *data, struct wl_callback *wl_callback, u32 callback_data);
struct wl_callback_listener callback_listener = {
	.done = frame_new
};

b8 platform_create_window(const char* window_name, u32 pos_x, u32 pos_y, u32 width, u32 height, Window* window) {
    WaylandState* state = malloc(sizeof(WaylandState));
    memset(state, 0, sizeof(WaylandState));
    window->internal_state = state;

    state->display = wl_display_connect(NULL);
    if (!state->display) {
        fprintf(stderr, "Failed to connect to Wayland display\n");
        return false;
    }

    state->registry = wl_display_get_registry(state->display);
    wl_registry_add_listener(state->registry, &registry_listener, state);

    // warn server to send menssage to client
    wl_display_roundtrip(state->display);

    state->surface = wl_compositor_create_surface(state->compositor);
    
    struct wl_callback* callback = wl_surface_frame(state->surface);
	wl_callback_add_listener(callback, &callback_listener, state);

    state->xdg_surface = xdg_wm_base_get_xdg_surface(state->xdg_wm_base, state->surface);
    state->xdg_toplevel = xdg_surface_get_toplevel(state->xdg_surface);

    xdg_toplevel_set_title(state->xdg_toplevel, window_name);
    xdg_toplevel_set_app_id(state->xdg_toplevel, window_name);

    xdg_surface_add_listener(state->xdg_surface, &xdg_surface_listener, state);
    xdg_toplevel_add_listener(state->xdg_toplevel, &xdg_toplevel_listener, state);

    state->width = width;
    state->height = height;

    wl_surface_commit(state->surface);
    wl_display_roundtrip(state->display);

    REXINFO("Wayland state initialized!");

    return true;
}

void platform_destroy_window(Window* window) {
    WaylandState* state = (WaylandState*)window->internal_state;

    wl_surface_destroy(state->surface);
    wl_display_disconnect(state->display);
}

b8 platform_show_window(Window* window) {
    WaylandState* state = (WaylandState*)window->internal_state;
    xdg_toplevel_set_maximized(state->xdg_toplevel);
    return true;
}
b8 platform_hide_window(Window* window) {
    WaylandState* state = (WaylandState*)window->internal_state;
    xdg_toplevel_set_minimized(state->xdg_toplevel);
    return true;
}
b8 platform_process_window_messages(Window* window) {
    WaylandState* state = (WaylandState*)window->internal_state;
    wl_display_dispatch(state->display);
    return true;
}

static void global_registry_handler(void* data, struct wl_registry *registry, u32 id,
const char *interface, u32 version) {
    
    WaylandState* state = (WaylandState*)data;
    if (!strcmp(interface, wl_compositor_interface.name)) {
        state->compositor = wl_registry_bind(registry, id, &wl_compositor_interface, version);
    }else if (!strcmp(interface, xdg_wm_base_interface.name)) {
        state->xdg_wm_base = wl_registry_bind(registry, id, &xdg_wm_base_interface, version);
        xdg_wm_base_add_listener(state->xdg_wm_base, &xdg_wm_base_listener, data);
    }
}

static void global_registry_remover(void* data, struct wl_registry *registry, u32 id) {}

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *shell, u32 serial) {
    xdg_wm_base_pong(shell, serial);
}

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, u32 serial) {}

static void xdg_toplevel_configure (void *data, struct xdg_toplevel *xdg_toplevel, i32 width, 
    i32 height, struct wl_array *states) {

    WaylandState* state = (WaylandState*)data;

    if (!width || !height) return;
	if (width == state->width && height == state->height) return;

    EventContext ctx = {0};
    ctx.data.u16[0] = width;
    ctx.data.u16[1] = height;
    event_fire(EVENT_CODE_RESIZED, state, ctx);
}

void xgd_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel) {
    EventContext ctx = {0};
    event_fire(EVENT_CODE_APPLICATION_QUIT, data, ctx);
}

void configure_bounds(void *data, struct xdg_toplevel *xdg_toplevel, i32 width, i32 height) {}
void wm_capabilities(void *data, struct xdg_toplevel *xdg_toplevel, struct wl_array *capabilities) {}

void frame_new(void *data, struct wl_callback *wl_callback, u32 callback_data) {}

void platform_console_write(const char* message, u8 colour) {
    // FATAL,ERROR,WARN,INFO,DEBUG,TRACE
    const char* colour_strings[] = {"0;41", "1;31", "1;33", "1;32", "1;34", "1;30"};
    printf("\033[%sm%s\033[0m", colour_strings[colour], message);
}

void platform_console_write_error(const char* message, u8 colour) {
    //FATAL,ERROR,WARN,INFO,DEBUG,TRACE
    const char* colour_strings[] = {"0;41", "1;31", "1;33", "1;32", "1;34", "1;30"};
    printf("\033[%sm%s\033[0m", colour_strings[colour], message);
}

#endif