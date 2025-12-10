#pragma once

int wl_init(
  int width,
  int height,
  const char *png_path,
  const char *output_name
);
void wl_show(void);
void wl_hide(void);
void wl_destroy(void);
int cross_visible(void);
