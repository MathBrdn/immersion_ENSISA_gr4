#include <gtk/gtk.h>
#include <cairo.h>

#include "jeu.h"

/* la piece actuellement selectionnee par le joueur, -1 si rien n'est selectionne */
static int selected_piece = -1;

/* dessine le symbole d'une piece (tour ou roi) au bon endroit */
static void draw_piece(cairo_t *cr, const char *symbol, int color, double x, double y, double size)
{
    if (color == BLEU)
        cairo_set_source_rgb(cr, 0.05, 0.15, 0.85);
    else
        cairo_set_source_rgb(cr, 0.90, 0.10, 0.10);

    cairo_select_font_face(cr, "DejaVu Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, size * 0.72);

    cairo_text_extents_t extents;
    cairo_text_extents(cr, symbol, &extents);

    double tx = x + (size - extents.width) / 2.0 - extents.x_bearing;
    double ty = y + (size - extents.height) / 2.0 - extents.y_bearing;

    cairo_move_to(cr, tx, ty);
    cairo_show_text(cr, symbol);
}

/* dessine tout le plateau : fond, cases jaunes, cases conquises, grille et pieces */
static void draw_board(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data)
{
    double cell_w = (double) width / COLS;
    double cell_h = (double) height / ROWS;

    /* fond blanc */
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_paint(cr);

    /* cases jaunes */
    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            if (jeu_case_jaune(row, col)) {
                cairo_set_source_rgb(cr, 1.0, 0.82, 0.28);
                cairo_rectangle(cr, col * cell_w, row * cell_h, cell_w, cell_h);
                cairo_fill(cr);
            }
        }
    }

    /* cases conquises, colorees en bleu ou rouge clair selon la derniere piece qui y est passee */
    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            int val = jeu_valeur_case(row, col);

            if (val == 1)
                cairo_set_source_rgba(cr, 0.05, 0.15, 0.85, 0.35);
            else if (val == -1)
                cairo_set_source_rgba(cr, 0.90, 0.10, 0.10, 0.35);
            else
                continue;

            cairo_rectangle(cr, col * cell_w, row * cell_h, cell_w, cell_h);
            cairo_fill(cr);
        }
    }

    /* case selectionnee en vert */
    if (selected_piece != -1) {
        Piece *p = jeu_piece(selected_piece);

        if (p != NULL && p->row >= 0) {
            cairo_set_source_rgb(cr, 0.30, 0.90, 0.30);
            cairo_rectangle(cr, p->col * cell_w, p->row * cell_h, cell_w, cell_h);
            cairo_fill(cr);
        }
    }

    /* grille */
    cairo_set_source_rgb(cr, 0.15, 0.15, 0.15);
    cairo_set_line_width(cr, 1.0);

    for (int col = 0; col <= COLS; col++) {
        double x = col * cell_w;
        cairo_move_to(cr, x, 0);
        cairo_line_to(cr, x, height);
    }

    for (int row = 0; row <= ROWS; row++) {
        double y = row * cell_h;
        cairo_move_to(cr, 0, y);
        cairo_line_to(cr, width, y);
    }

    cairo_stroke(cr);

    /* pieces */
    int nb_pieces = jeu_nombre_pieces();

    for (int i = 0; i < nb_pieces; i++) {
        Piece *p = jeu_piece(i);

        if (p == NULL || p->row < 0)
            continue;

        double x = p->col * cell_w;
        double y = p->row * cell_h;

        draw_piece(cr, p->symbol, p->color, x, y, MIN(cell_w, cell_h));
    }
}

/* gere le clic de souris sur le plateau */
static void board_clicked(GtkGestureClick *gesture, int n_press, double x, double y, gpointer user_data)
{
    GtkDrawingArea *drawing_area = GTK_DRAWING_AREA(user_data);

    int width = gtk_widget_get_width(GTK_WIDGET(drawing_area));
    int height = gtk_widget_get_height(GTK_WIDGET(drawing_area));

    double cell_w = (double) width / COLS;
    double cell_h = (double) height / ROWS;

    int col = (int)(x / cell_w);
    int row = (int)(y / cell_h);

    if (row < 0 || row >= ROWS || col < 0 || col >= COLS)
        return;

    /* aucune piece selectionnee pour l'instant */
    if (selected_piece == -1) {
        int p = jeu_piece_at(row, col);

        if (p != -1) {
            Piece *piece = jeu_piece(p);

            if (piece != NULL && piece->color == jeu_joueur_actuel()) {
                selected_piece = p;
                gtk_widget_queue_draw(GTK_WIDGET(drawing_area));
            }
        }

        return;
    }

    /* une piece est deja selectionnee */
    Piece *piece = jeu_piece(selected_piece);

    if (piece == NULL) {
        selected_piece = -1;
        return;
    }

    /* cliquer sur la meme case annule la selection */
    if (row == piece->row && col == piece->col) {
        selected_piece = -1;
        gtk_widget_queue_draw(GTK_WIDGET(drawing_area));
        return;
    }

    jeu_deplacer(selected_piece, row, col);

    selected_piece = -1;
    gtk_widget_queue_draw(GTK_WIDGET(drawing_area));
}

/* cree la fenetre et le plateau au demarrage */
static void activate(GtkApplication *app, gpointer data)
{
    GtkWidget *window;
    GtkWidget *drawing_area;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Échiquier 11 x 7");
    gtk_window_set_default_size(GTK_WINDOW(window), 770, 490);

    drawing_area = gtk_drawing_area_new();
    gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(drawing_area), 770);
    gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(drawing_area), 490);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(drawing_area), draw_board, NULL, NULL);

    GtkGesture *click = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click), GDK_BUTTON_PRIMARY);
    g_signal_connect(click, "pressed", G_CALLBACK(board_clicked), drawing_area);
    gtk_widget_add_controller(drawing_area, GTK_EVENT_CONTROLLER(click));

    gtk_window_set_child(GTK_WINDOW(window), drawing_area);
    gtk_window_present(GTK_WINDOW(window));
}

int main(int argc, char **argv)
{
    GtkApplication *app;
    int status;

    jeu_initialiser();

    app = gtk_application_new("com.example.echiquier", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    status = g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);

    return status;
}
