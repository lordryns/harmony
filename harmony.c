#include <assert.h>
#include <getopt.h>
#include <stdbool.h>
#include <unistd.h>
#include <wayland-server-core.h>
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
    struct wl_display *display;
    struct wlr_renderer *renderer; 
    struct wlr_allocator *allocator;
    struct wl_event_loop *event_loop;
    struct wl_list outputs;
};


int main(void) 
{
    struct harmony_server server; 

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

    server.event_loop = wl_display_get_event_loop(server.display);
    assert(server.event_loop);

    if (!wlr_backend_start(server.backend))
    {
        fprintf(stderr, "Failed to start backend!\n");
        wl_display_destroy(server.display);
        return 1;
    }


    // wlr_compositor_create(server.display, 5, server.renderer);
    // wlr_subcompositor_create(server.display);

    printf("Starting Harmony...");
    wl_display_run(server.display);
    wl_display_destroy(server.display);

    return 0;
}
