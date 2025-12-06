#include <assert.h>
#include <bits/time.h>
#include <getopt.h>
#include <stdbool.h>
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


struct harmony_server {
    struct wlr_backend *backend; 
    struct wlr_renderer *renderer;
    struct wlr_allocator *allocator;
    struct wl_display *display;
    struct wlr_scene *scene;
    struct wl_list outputs; 
    struct wl_listener new_output;
    struct wlr_scene_output_layout *scene_layout;
    struct wlr_output_layout *output_layout;
    struct wl_listener frame;
    struct wlr_output *output;
};

static void notify_frame(struct wl_listener *listener, void *data)
{
    struct harmony_server *server = wl_container_of(listener, server, frame);

    struct wlr_scene_output *scene_output = wlr_scene_get_scene_output(server->scene, server->output);

    wlr_scene_output_commit(scene_output, NULL);

    struct timespec now; 
    clock_gettime(CLOCK_MONOTONIC, &now);

    wlr_scene_output_send_frame_done(scene_output, &now);
}

static void new_output(struct wl_listener *listener, void *data)
{
    struct harmony_server *server = wl_container_of(listener, server, new_output);

    struct wlr_output *output = data; 
    wlr_log(WLR_INFO, "NEW OUTPUT: %s", output->name);


    wlr_output_init_render(output, server->allocator, server->renderer);
    struct wlr_output_state state;
    wlr_output_state_init(&state);
    wlr_output_state_set_enabled(&state, true);


    struct wlr_output_mode *mode = wlr_output_preferred_mode(output);
    if (mode)
    {
        wlr_log(WLR_INFO, "Setting mode: %dx%d", mode->width, mode->height);
        wlr_output_state_set_mode(&state, mode);
    }

    if (!wlr_output_commit_state(output, &state)) 
    {
        wlr_log(WLR_ERROR, "Failed to commit output state");
    }
    wlr_output_state_finish(&state);

    server->output = output; 
    wlr_scene_output_create(server->scene, output);

    server->frame.notify = notify_frame;
    wl_signal_add(&output->events.frame, &server->frame);
    wlr_log(WLR_INFO, "Output configured successfully");
}

int main (void) 
{
    struct harmony_server server = {0};
    wlr_log_init(WLR_DEBUG, NULL);


    server.display = wl_display_create();
    assert(server.display);

    server.backend = wlr_backend_autocreate(wl_display_get_event_loop(server.display), NULL);

    if (server.backend == NULL)
    {
        wlr_log(WLR_ERROR, "Failed to create wlr_backend!");
        return 1; 
    }

    server.renderer = wlr_renderer_autocreate(server.backend);
     if (server.renderer == NULL)
    {
        wlr_log(WLR_ERROR, "Failed to create wlr_renderer!");
        return 1; 
    }

     wlr_renderer_init_wl_display(server.renderer, server.display);

     server.allocator = wlr_allocator_autocreate(server.backend, server.renderer);
     if (server.allocator == NULL)
    {
        wlr_log(WLR_ERROR, "Failed to create wlr_allocator!");
        return 1; 
    }

    wlr_compositor_create(server.display, 5, server.renderer);
    server.scene = wlr_scene_create();

    server.new_output.notify = new_output;
    wl_signal_add(&server.backend->events.new_output, &server.new_output);

   wlr_log(WLR_INFO, "Starting backend..."); 
   if (!wlr_backend_start(server.backend))
   {
       wlr_log(WLR_ERROR, "Failed to start backend :(");
       return 1;
   }

   wl_display_run(server.display);

   wl_display_destroy(server.display);
    return 0;
}
