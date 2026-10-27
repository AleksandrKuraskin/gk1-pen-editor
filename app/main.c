#include <cairo.h>
#include <gtk/gtk.h>

#include "config/config.h"
#include "controller/drawing_area_controller.h"
#include "glib-object.h"
#include "model/app_state.h"
#include "view/renderer.h"

static void on_algo_toggled(GtkCheckButton *radio, gpointer user_data) {
  AppState *state = user_data;
  if (gtk_check_button_get_active(radio)) {
    state->use_bresenham = TRUE;
  } else {
    state->use_bresenham = FALSE;
  }
}

static void show_documentation_dialog(GtkButton *btn, gpointer user_data) {
  (void)btn;
  GtkWindow *parent = user_data;
  GtkWidget *dialog = gtk_window_new();
  gtk_window_set_title(GTK_WINDOW(dialog), "Dokumentacja programu");
  gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_window_set_default_size(GTK_WINDOW(dialog), 720, 520);

  GtkWidget *scrolled = gtk_scrolled_window_new();
  GtkWidget *label = gtk_label_new(
      "DOKUMENTACJA SYSTEMU POLYPEN\n"
      "Edytor wielokątów i wielokątów krzywoliniowych\n\n"
      "1. OBSŁUGA INTERFEJSU I EDYCJA:\n"
      " - Przeciąganie wierzchołka: LPM na wierzchołek pozwala na zmianę jego "
      "położenia.\n"
      " - Przeciąganie krawędzi: kliknięcie i przeciągnięcie krawędzi przesuwa "
      "cały wielokąt.\n"
      " - Przeciąganie punktu kontrolnego: zmiana kształtu segmentu Béziera.\n"
      " - Menu wierzchołka (PPM): usuwanie wierzchołka oraz wybór klasy "
      "ciągłości (G0, G1, C1).\n"
      " - Menu krawędzi (PPM): zmiana typu (odcinek, Bézier, łuk), dodanie "
      "wierzchołka oraz nakładanie i usuwanie ograniczeń.\n\n"
      "2. OGRANICZENIA GEOMETRYCZNE:\n"
      " - Zadana długość krawędzi: utrzymywanie stałej odległości między "
      "końcami segmentu.\n"
      " - Równoległość dwóch krawędzi: wymuszenie współliniowości wektorów "
      "kierunkowych wybranej pary krawędzi.\n"
      " - Solver ograniczeń: iteracyjny algorytm relaksacyjny korygujący "
      "położenie wierzchołków z uwzględnieniem punktu aktualnie "
      "przeciąganego.\n"
      " - Modyfikacja topologii: wstawienie lub usunięcie wierzchołka usuwa "
      "powiązane relacje geometryczne.\n\n"
      "3. KLASY CIĄGŁOŚCI W WĘZŁACH:\n"
      " - G0: ciągłość położenia (wspólny wierzchołek, niezależne kierunki "
      "stycznych).\n"
      " - G1: ciągłość kierunku (jednostkowego wektora stycznego). Punkty "
      "kontrolne leżą na wspólnej prostej przechodzącej przez wierzchołek.\n"
      " - C1: ciągłość wektora stycznego (równość wektorów pochodnych, "
      "symetria uchwytów względem węzła).\n"
      " - Łuki okręgów: co najwyżej jeden koniec danego łuku może posiadać "
      "ciągłość G1.\n\n"
      "4. ALGORYTMY RENDEROWANIA:\n"
      " - Przełącznik rasteryzacji: wybór pomiędzy własną implementacją "
      "algorytmu Bresenhama a algorytmem bibliotecznym Cairo.\n"
      " - Segmenty Béziera: wyznaczanie punktów metodą przyrostową w bazie "
      "potęgowej z wykorzystaniem różnic skończonych trzeciego rzędu.\n");
  gtk_label_set_wrap(GTK_LABEL(label), TRUE);
  gtk_widget_set_margin_start(label, 20);
  gtk_widget_set_margin_end(label, 20);
  gtk_widget_set_margin_top(label, 20);
  gtk_widget_set_margin_bottom(label, 20);

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled), label);
  gtk_window_set_child(GTK_WINDOW(dialog), scrolled);
  gtk_window_present(GTK_WINDOW(dialog));
}

static void activate(GtkApplication *app, gpointer user_data) {
  AppState *state = user_data;
  if (state == NULL)
    return;

  GtkWidget *window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), CONFIG_APP_NAME);
  gtk_window_set_default_size(GTK_WINDOW(window), CONFIG_WINDOW_WIDTH,
                              CONFIG_WINDOW_HEIGHT);

  GtkWidget *header = gtk_header_bar_new();
  gtk_window_set_titlebar(GTK_WINDOW(window), header);

  GtkWidget *radio_bresenham = gtk_check_button_new_with_label("Bresenham");
  GtkWidget *radio_cairo = gtk_check_button_new_with_label("Cairo");
  gtk_check_button_set_group(GTK_CHECK_BUTTON(radio_cairo),
                             GTK_CHECK_BUTTON(radio_bresenham));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(radio_bresenham), TRUE);
  g_signal_connect(radio_bresenham, "toggled", G_CALLBACK(on_algo_toggled),
                   state);

  GtkWidget *algo_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_append(GTK_BOX(algo_box), radio_bresenham);
  gtk_box_append(GTK_BOX(algo_box), radio_cairo);
  gtk_header_bar_pack_start(GTK_HEADER_BAR(header), algo_box);

  GtkWidget *help_btn = gtk_button_new_with_label("Documentation");
  g_signal_connect(help_btn, "clicked", G_CALLBACK(show_documentation_dialog),
                   window);
  gtk_header_bar_pack_end(GTK_HEADER_BAR(header), help_btn);

  // Obszar rysowania
  GtkWidget *drawing_area = gtk_drawing_area_new();
  gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(drawing_area),
                                     CONFIG_WINDOW_WIDTH);
  gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(drawing_area),
                                      CONFIG_WINDOW_HEIGHT);
  gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drawing_area),
                                 renderer_draw_pixel_buffer, state, NULL);

  drawing_area_connect_drag(drawing_area, state);
  gtk_window_set_child(GTK_WINDOW(window), drawing_area);
  gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv) {
  AppState *state = app_state_create();
  if (state == NULL) {
    return 1;
  }

  // application declaration
  GtkApplication *app =
      gtk_application_new(CONFIG_APP_ID, G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK(activate), state);

  // application lifecycle
  int status = g_application_run(G_APPLICATION(app), argc, argv);
  g_object_unref(app);
  app_state_destroy(state);

  return status;
}