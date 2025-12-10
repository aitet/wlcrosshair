#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "layer.h"

#include <wayland-client.h>
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-output-unstable-v1-client-protocol.h"

#include <png.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <linux/memfd.h>
#include <stdio.h>

static struct wl_display *display;
static struct wl_compositor *compositor;
static struct wl_shm *shm;
static struct zwlr_layer_shell_v1 *layer_shell;
static struct zxdg_output_manager_v1 *xdg_output_manager;

static struct wl_output *target_output = NULL;
static char requested_output[64];

static struct wl_surface *surface;
static struct zwlr_layer_surface_v1 *layer_surface;
static struct wl_buffer *buffer;

static int png_w, png_h;
static unsigned char *img_rgba;

static int surf_w, surf_h;

static int shm_fd = -1;
static void *shm_data = NULL;
static size_t shm_size;

static int visible = 0;
static int configured = 0;

static void xdg_noop_xy(
	void *data,
	struct zxdg_output_v1 *xdg,
	int32_t x,
	int32_t y
) {
}

static void xdg_noop_wh(
	void *data,
	struct zxdg_output_v1 *xdg,
	int32_t w,
	int32_t h
) {
}

static void xdg_done(
	void *data,
	struct zxdg_output_v1 *xdg
) {
}

static void xdg_desc(
	void *data,
	struct zxdg_output_v1 *xdg,
	const char *desc
) {
}


static int create_shm_file(size_t size) {
	int fd = syscall(SYS_memfd_create, "wlcrosshair", MFD_CLOEXEC);
	if (fd < 0)
		return -1;
	ftruncate(fd, size);
	return fd;
}


static void fatal_png(const char *path) {
	fprintf(stderr, "failed to open PNG: %s\n", path);
	_exit(1);
}


static void premultiply_alpha(void) {
	uint8_t *p = img_rgba;

	for (int i = 0; i < png_w * png_h; i++) {
		uint8_t a = p[3];
		p[0] = (p[0] * a) / 255;
		p[1] = (p[1] * a) / 255;
		p[2] = (p[2] * a) / 255;
		p += 4;
	}
}

static void load_png(const char *path) {
	FILE *fp = fopen(path, "rb");
	if (!fp)
		fatal_png(path);

	png_structp png =
		png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	png_infop info = png_create_info_struct(png);

	png_init_io(png, fp);
	png_read_info(png, info);

	png_w = png_get_image_width(png, info);
	png_h = png_get_image_height(png, info);

	png_set_expand(png);
	png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
	png_read_update_info(png, info);

	free(img_rgba);
	img_rgba = malloc(png_w * png_h * 4);

	png_bytep rows[png_h];
	for (int y = 0; y < png_h; y++)
		rows[y] = img_rgba + y * png_w * 4;

	png_read_image(png, rows);
	png_destroy_read_struct(&png, &info, NULL);
	fclose(fp);

  premultiply_alpha();
}


static void alloc_shm(void) {
	if (shm_fd >= 0) {
		munmap(shm_data, shm_size);
		close(shm_fd);
	}

	shm_size = surf_w * surf_h * 4;
	shm_fd = create_shm_file(shm_size);
  if (shm_fd < 0) {
    fprintf(stderr, "fatal error\n");
    _exit(1);
  }
	shm_data = mmap(NULL, shm_size, PROT_READ | PROT_WRITE,
	                MAP_SHARED, shm_fd, 0);
  if (shm_data == MAP_FAILED) {
    fprintf(stderr, "fatal error\n");
    _exit(1);
  }
	memset(shm_data, 0, shm_size);
}


static void create_buffer(void) {
	if (buffer)
		wl_buffer_destroy(buffer);

	struct wl_shm_pool *pool =
		wl_shm_create_pool(shm, shm_fd, shm_size);

	buffer = wl_shm_pool_create_buffer(
		pool, 0, surf_w, surf_h, surf_w * 4,
		WL_SHM_FORMAT_ARGB8888
	);

	wl_shm_pool_destroy(pool);
}


static void blit_scaled(void) {
	uint32_t *dst = shm_data;
	uint32_t *src = (uint32_t *)img_rgba;

	for (int y = 0; y < surf_h; y++) {
		int sy = y * png_h / surf_h;
		for (int x = 0; x < surf_w; x++) {
			int sx = x * png_w / surf_w;
			dst[y * surf_w + x] =
				src[sy * png_w + sx];
		}
	}
}

static void clear_buffer(void) {
	memset(shm_data, 0, shm_size);
}

static void commit(void) {
	wl_surface_damage_buffer(surface, 0, 0, surf_w, surf_h);
	wl_surface_attach(surface, buffer, 0, 0);
	wl_surface_commit(surface);
	wl_display_flush(display);
}


struct output_ctx {
	struct wl_output *wl;
};

static void xdg_name(
	void *data,
	struct zxdg_output_v1 *xdg,
	const char *name
) {
	struct output_ctx *ctx = data;
	if (name && strcmp(name, requested_output) == 0)
		target_output = ctx->wl;
}

static const struct zxdg_output_v1_listener xdg_listener = {
	.logical_position = xdg_noop_xy,
	.logical_size = xdg_noop_wh,
	.done = xdg_done,
	.name = xdg_name,
	.description = xdg_desc,
};


static void layer_configure(
	void *data,
	struct zwlr_layer_surface_v1 *surf,
	uint32_t serial,
	uint32_t w,
  uint32_t h
) {
  zwlr_layer_surface_v1_ack_configure(surf, serial);

  if ((int)w != surf_w || (int)h != surf_h) {
    surf_w = w;
    surf_h = h;
    alloc_shm();
    create_buffer();
  }
  configured = 1;
}


static const struct zwlr_layer_surface_v1_listener layer_listener = {
	.configure = layer_configure,
	.closed = NULL,
};

static void create_surface(void) {
	surface = wl_compositor_create_surface(compositor);

	layer_surface = zwlr_layer_shell_v1_get_layer_surface(
		layer_shell,
		surface,
		target_output,
		ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
		"wlcrosshair"
	);

	zwlr_layer_surface_v1_add_listener(layer_surface, &layer_listener, NULL);

  zwlr_layer_surface_v1_set_exclusive_zone(layer_surface, -1);
	zwlr_layer_surface_v1_set_keyboard_interactivity(layer_surface, 0);
  zwlr_layer_surface_v1_set_size(layer_surface, surf_w, surf_h);
  zwlr_layer_surface_v1_set_anchor(
    layer_surface,
    ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP |
    ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM |
    ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
    ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT
  );

	wl_surface_commit(surface);
}


static void registry_add(
	void *data,
	struct wl_registry *reg,
	uint32_t name,
	const char *iface,
	uint32_t ver
) {
	if (!strcmp(iface, wl_compositor_interface.name))
		compositor = wl_registry_bind(reg, name, &wl_compositor_interface, 4);
	else if (!strcmp(iface, wl_shm_interface.name))
		shm = wl_registry_bind(reg, name, &wl_shm_interface, 1);
	else if (!strcmp(iface, zwlr_layer_shell_v1_interface.name))
		layer_shell = wl_registry_bind(reg, name, &zwlr_layer_shell_v1_interface, 1);
	else if (!strcmp(iface, zxdg_output_manager_v1_interface.name))
		xdg_output_manager =
			wl_registry_bind(reg, name,
				&zxdg_output_manager_v1_interface, 3);
	else if (!strcmp(iface, wl_output_interface.name)) {
		struct wl_output *out =
			wl_registry_bind(reg, name, &wl_output_interface, 2);

		if (xdg_output_manager) {
			struct output_ctx *ctx = calloc(1, sizeof *ctx);
			ctx->wl = out;

			struct zxdg_output_v1 *xdg =
				zxdg_output_manager_v1_get_xdg_output(
					xdg_output_manager, out);

			zxdg_output_v1_add_listener(xdg, &xdg_listener, ctx);
		}
	}
}

static const struct wl_registry_listener registry_listener = {
	.global = registry_add,
};


int wl_init(
	int width,
	int height,
	const char *png_path,
	const char *output_name
) {
	surf_w = width;
	surf_h = height;

	strncpy(requested_output, output_name,
	        sizeof(requested_output) - 1);

	display = wl_display_connect(NULL);
	if (!display)
		return -1;

	struct wl_registry *reg = wl_display_get_registry(display);
	wl_registry_add_listener(reg, &registry_listener, NULL);

	wl_display_roundtrip(display);
	wl_display_roundtrip(display);

	if (!target_output) {
		fprintf(stderr, "output not found: %s\n", requested_output);
		_exit(1);
	}

	load_png(png_path);
	alloc_shm();
	create_surface();

	while (!configured)
		wl_display_roundtrip(display);

	create_buffer();
	clear_buffer();
	commit();
	visible = 0;

	return 0;
}


void wl_show(void) {
	if (visible)
		return;

	blit_scaled();
	commit();
	visible = 1;
}

void wl_hide(void) {
	if (!visible)
		return;

	clear_buffer();
	commit();
	visible = 0;
}

void wl_destroy(void) {
  if (buffer)
    wl_buffer_destroy(buffer);
  if (layer_surface)
    zwlr_layer_surface_v1_destroy(layer_surface);
  if (surface)
    wl_surface_destroy(surface);
  if (shm_data)
    munmap(shm_data, shm_size);
  if (shm_fd >= 0)
    close(shm_fd);
  if (layer_shell)
    zwlr_layer_shell_v1_destroy(layer_shell);
  if (shm)
    wl_shm_destroy(shm);
  if (compositor)
    wl_compositor_destroy(compositor);
  if (display)
    wl_display_disconnect(display);

  free(img_rgba);
  img_rgba = NULL;
}

int cross_visible(void) {
	return visible;
}
