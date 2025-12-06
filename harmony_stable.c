#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon.h>
#include <stdlib.h>

struct harmony_output {
    struct wl_list link;
    struct harmony_server *server;
    struct wlr_output *wlr_output;
    struct wl_listener frame;
};

struct harmony_server {
    struct wlr_backend *backend; 
    struct wl_display *display;
    struct wlr_renderer *renderer; 
    struct wlr_allocator *allocator;
    struct wl_event_loop *event_loop;
    struct wl_list outputs;
    struct wl_listener new_output; 
    struct wlr_scene *scene;
    struct wlr_scene_output_layout *scene_layout;
    struct wlr_output_layout *output_layout;
};

static void output_frame(struct wl_listener *listener, void *data)
{
    struct harmony_output *output = wl_container_of(listener, output, frame);
    struct wlr_scene *scene = output->server->scene;
    
    struct wlr_scene_output *scene_output = wlr_scene_get_scene_output(
        scene, output->wlr_output);
    
    /* Render the scene and commit the output */
    wlr_scene_output_commit(scene_output, NULL);
    
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    wlr_scene_output_send_frame_done(scene_output, &now);
}

static void new_output_notify(struct wl_listener *listener, void *data)
{
    struct harmony_server *server = wl_container_of(listener, server, new_output); 
    struct wlr_output *wlr_output = data;
    
    wlr_log(WLR_DEBUG, "new output: %s", wlr_output->name);
    
    /* Initialize the output for rendering */
    wlr_output_init_render(wlr_output, server->allocator, server->renderer);
    
    /* Configure the output */
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, true);
    
    struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
    if (mode) {
        wlr_output_state_set_mode(&state, mode);
    }
    
    wlr_output_commit_state(wlr_output, &state);
    wlr_output_state_finish(&state);
    
    /* Allocate our output structure */
    struct harmony_output *output = calloc(1, sizeof(*output));
    output->wlr_output = wlr_output;
    output->server = server;
    
    /* Set up frame listener */
    output->frame.notify = output_frame;
    wl_signal_add(&wlr_output->events.frame, &output->frame);
    
    /* Add to output layout */
    struct wlr_output_layout_output *l_output = 
        wlr_output_layout_add_auto(server->output_layout, wlr_output);
    
    /* Create scene output and add to scene layout */
    struct wlr_scene_output *scene_output = 
        wlr_scene_output_create(server->scene, wlr_output);
    wlr_scene_output_layout_add_output(server->scene_layout, l_output, scene_output);
    
    wl_list_insert(&server->outputs, &output->link);
}

int main(void) 
{
    struct harmony_server server;
    memset(&server, 0, sizeof(server)); 
    wlr_log_init(WLR_DEBUG, NULL);
    
    server.display = wl_display_create();
    assert(server.display); 
    
    server.backend = wlr_backend_autocreate(wl_display_get_event_loop(server.display), NULL);
    if (server.backend == NULL) {
        wlr_log(WLR_ERROR, "failed to create wlr_backend");
        return 1;
    }
    
    server.renderer = wlr_renderer_autocreate(server.backend);
    if (server.renderer == NULL) {
        wlr_log(WLR_ERROR, "failed to create wlr_renderer");
        return 1;
    }
    
    wlr_renderer_init_wl_display(server.renderer, server.display);
    
    server.allocator = wlr_allocator_autocreate(server.backend, server.renderer);
    if (server.allocator == NULL) {
        wlr_log(WLR_ERROR, "failed to create wlr_allocator");
        return 1;
    }
    
    /* Create compositor protocols */
    wlr_compositor_create(server.display, 5, server.renderer);
    wlr_subcompositor_create(server.display);
    wlr_data_device_manager_create(server.display);
    
    /* Create output layout and scene */
    server.output_layout = wlr_output_layout_create(server.display);
    server.scene = wlr_scene_create();
    server.scene_layout = wlr_scene_attach_output_layout(server.scene, server.output_layout);
    
    /* Initialize output list */
    wl_list_init(&server.outputs);
    
    /* Set up output listener */
    server.new_output.notify = new_output_notify;
    wl_signal_add(&server.backend->events.new_output, &server.new_output);
    
    /* Add a Unix socket to the Wayland display */
    const char *socket = wl_display_add_socket_auto(server.display);
    if (!socket) {
        wlr_log(WLR_ERROR, "Unable to open wayland socket");
        wlr_backend_destroy(server.backend);
        return 1;
    }
    
    /* Start the backend */
    if (!wlr_backend_start(server.backend)) {
        wlr_log(WLR_ERROR, "Failed to start backend");
        wl_display_destroy(server.display);
        return 1;
    }
    
    setenv("WAYLAND_DISPLAY", socket, true);
    wlr_log(WLR_INFO, "Running Harmony compositor on WAYLAND_DISPLAY=%s", socket);
    
    wl_display_run(server.display);
    wl_display_destroy(server.display);
    return 0;
}
