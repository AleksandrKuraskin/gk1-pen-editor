#include "cairo.h"
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

typedef struct {
  int width;
  int height;
  uint32_t *pixels;
} FrameBuffer;

static FrameBuffer *fb = NULL;

FrameBuffer *fb_create(int w, int h) {
  FrameBuffer *buf = malloc(sizeof(FrameBuffer));
  buf->width = w;
  buf->height = h;
  buf->pixels = calloc(w * h, sizeof(uint32_t));
  return buf;
}

void fb_clear(FrameBuffer *buf) {
  memset(buf->pixels, 50, buf->width * buf->height * sizeof(uint32_t));
}

void fb_put_pixel(FrameBuffer *buf, int x, int y, uint32_t color) {
  if (x >= 0 && x < buf->width && y >= 0 && y < buf->height) {
    buf->pixels[y * buf->width + x] = color;
  }
}

void draw_line_bresenham(FrameBuffer *buf, int x0, int y0, int x1, int y1,
                         uint32_t color) {
  int dx = abs(x1 - x0);
  int sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0);
  int sy = y0 < y1 ? 1 : -1;
  int err = dx + dy;

  while (1) {
    fb_put_pixel(buf, x0, y0, color);
    if (x0 == x1 && y0 == y1)
      break;
    int e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

// Funkcja rysująca specyficzna dla GTK 4
static void drawing_area_draw(GtkDrawingArea *area, cairo_t *cr, int width,
                              int height, gpointer user_data) {
  (void)area;
  (void)width;
  (void)height;
  (void)user_data;

  // Owijamy nasz bufor RAM w powierzchnię Cairo
  cairo_surface_t *surface = cairo_image_surface_create_for_data(
      (unsigned char *)fb->pixels, CAIRO_FORMAT_ARGB32, fb->width, fb->height,
      fb->width * 4);

  // Kopiujemy tablicę pikseli na kontekst ekranowy
  cairo_set_source_surface(cr, surface, 0, 0);
  cairo_paint(cr);

  cairo_surface_destroy(surface);
}

static void activate(GtkApplication *app, gpointer user_data) {
  (void)user_data;

  GtkWidget *window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), CONFIG_APP_NAME);
  gtk_window_set_default_size(GTK_WINDOW(window), CONFIG_WINDOW_WIDTH,
                              CONFIG_WINDOW_HEIGHT);
  gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

  GtkWidget *drawing_area = gtk_drawing_area_new();
  gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(drawing_area),
                                     CONFIG_CANVAS_WIDTH);
  gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(drawing_area),
                                      CONFIG_CANVAS_HEIGHT);

  // W GTK 4 rejestrujemy funkcję rysującą za pomocą draw_func
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drawing_area),
                                 drawing_area_draw, NULL, NULL);

  gtk_window_set_child(GTK_WINDOW(window), drawing_area);
  gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
  fb = fb_create(CONFIG_CANVAS_WIDTH, CONFIG_CANVAS_HEIGHT);
  fb_clear(fb);

  // Rysowanie testowe Bresenhamem
  draw_line_bresenham(fb, 50, 50, 750, 550, CONFIG_DEMO_RED);
  draw_line_bresenham(fb, 50, 550, 750, 50, CONFIG_DEMO_GREEN);

  GtkApplication *app = gtk_application_new(CONFIG_APP_ID,
                                            G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);

  free(fb->pixels);
  free(fb);

  return status;
}