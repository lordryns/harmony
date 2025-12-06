#include <assert.h>
#include <bits/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/backend.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>

struct harmony_server {
    struct wl_display *wl_display;
    struct wl_event_loop *wl_event_loop;

    struct wlr_backend *backend;
    struct wl_listener new_output;
    struct wl_list outputs;
};

struct harmony_output {
    struct wlr_output *wlr_output; 
    struct harmony_server *server; 
    struct timespec last_frame;
    struct wl_list link;

    struct wl_listener destroy;
};

static void new_output_notify(struct wl_listener *listener, void *data)
{
    struct harmony_server *server = wl_container_of(listener, server, new_output);
    struct wlr_output *wlr_output = data;

    if (!wl_list_empty(&wlr_output->modes))
    {
        struct wlr_output_state state;
        wlr_output_state_init(&state);

        struct wlr_output_mode *mode = wl_container_of(wlr_output->modes.prev, mode, link);
        wlr_output_state_set_custom_mode(&state, mode->width, mode->height, mode->refresh);
    }

    struct harmony_output *output = calloc(1, sizeof(struct harmony_output));
    clock_gettime(CLOCK_MONOTONIC, &output->last_frame);
    output->server = server; 
    wl_list_insert(&server->outputs, &output->link);
}

int main(void) 
{
    struct harmony_server server; 
    server.wl_display = wl_display_create();
    assert(server.wl_display);

    server.wl_event_loop = wl_display_get_event_loop(server.wl_display);
    assert(server.wl_event_loop);

    server.backend = wlr_backend_autocreate(server.wl_event_loop, NULL);
    assert(server.backend);

    wl_list_init(&server.outputs);
    server.new_output.notify = new_output_notify;

    if (!wlr_backend_start(server.backend))
    {
        fprintf(stderr, "Failed to start backend!\n");
        wl_display_destroy(server.wl_display);
        return 1;
    }
    wl_display_run(server.wl_display);
    wl_display_destroy(server.wl_display);
    return 0;
}
