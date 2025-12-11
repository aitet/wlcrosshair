#pragma once

#include <stdbool.h>

bool load_png(const char *path);

bool init_surface(
  int width,
  int height,
  const char *output_name
);

void free_png(void);

void draw_png(void);

void clear_surface(void);

void destroy_surface(void);
