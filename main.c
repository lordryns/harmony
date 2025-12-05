#include <assert.h>
#include <stdio.h>
#include <wayland-server-core.h>
#include <wlr/backend.h>

struct harmony_server {
    struct wl_display *wl_display;
    struct wl_event_loop *wl_event_loop;

    struct wlr_backend *backend;
};


int main(void) 
{
    struct harmony_server server; 
    server.wl_display = wl_display_create();
    assert(server.wl_display);

    server.wl_event_loop = wl_display_get_event_loop(server.wl_display);
    assert(server.wl_event_loop);

    server.backend = wlr_backend_autocreate(server.wl_event_loop, NULL);
    assert(server.backend);

	if (server.backend == NULL) { 
		return 1;
	}

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
