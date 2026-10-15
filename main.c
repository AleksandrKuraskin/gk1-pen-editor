#include <cairo.h>
#include <gtk/gtk.h>

#include "config.h"
#include "pixel_buffer.h"
#include "renderer.h"

static void activate(GtkApplication *app, gpointer user_data) {
  PixelBuffer *pb = user_data;

  // create and configure main application window
  GtkWidget *window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), CONFIG_APP_NAME);
  gtk_window_set_default_size(GTK_WINDOW(window), CONFIG_WINDOW_WIDTH,
                              CONFIG_WINDOW_HEIGHT);
  gtk_window_set_resizable(GTK_WINDOW(window), FALSE);

  // window elements declaration
  GtkWidget *drawing_area = gtk_drawing_area_new();
  gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(drawing_area),
                                     CONFIG_CANVAS_WIDTH);
  gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(drawing_area),
                                      CONFIG_CANVAS_HEIGHT);
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drawing_area),
                                 renderer_draw_pixel_buffer, pb, NULL);

  gtk_window_set_child(GTK_WINDOW(window), drawing_area);
  gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
  PixelBuffer *pb =
      pixel_buffer_create(CONFIG_CANVAS_WIDTH, CONFIG_CANVAS_HEIGHT);
  if (pb == NULL) {
    return 1;
  }

  // application declaration
  GtkApplication *app =
      gtk_application_new(CONFIG_APP_ID, G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(activate), pb);

  // application lifecycle
  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
  pixel_buffer_destroy(pb);

  return status;
}