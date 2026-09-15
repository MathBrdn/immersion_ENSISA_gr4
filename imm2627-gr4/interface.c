#include <gtk/gtk.h>
#include <cairo.h>

#include "jeu.h"

/* Structure pour représenter une case */
typedef struct {
    int row;
    int col;
} Coord;

/* Coordonnées par défaut des barricades (10, 6) */
static const Coord coord_init_barricade = {6, 10}; /* 10e colonne, 6e ligne */

/* Coordonnées affichées sur le plateau (-1 = masqué tant qu'aucun clic n'a eu lieu) */
static int x1_col = -1, x1_row = -1; /* Croix BLEU */
static int x2_col = -1, x2_row = -1; /* Croix ROUGE */

/* Suivi du premier coup de chaque joueur */
static bool croix_posee_j1 = false;
static bool croix_posee_j2 = false;

/* Pièce actuellement sélectionnée (-1 si aucune) */
static int selected_piece = -1;

/* Tableaux de coordonnées autorisées pour la pose */
static const Coord CASES_AUTORISEES_BLEU[] = {
    {0, 4}, {0, 5}, {0, 6}, {0, 7}, {1, 4}, {1, 5}, {1, 6}, {2, 3}, {2, 4}, {2, 5},
    {3, 2}, {3, 3}, {3, 4}, {4, 0}, {4, 1}, {4, 2}, {4, 3}, {5, 0}, {5, 1}, {5, 2}
};
static const int NB_CASES_BLEU = sizeof(CASES_AUTORISEES_BLEU) / sizeof(CASES_AUTORISEES_BLEU[0]);

static const Coord CASES_AUTORISEES_ROUGE[] = {
    {1, 10}, {1, 9}, {1, 8}, {2, 10}, {2, 9}, {2, 8}, {2, 7}, {3, 8}, {3, 7}, {3, 6},
    {4, 7}, {4, 6}, {4, 5}, {5, 4}, {5, 5}, {5, 6}, {6, 6}, {6, 5}, {6, 4}, {6, 3}
};
static const int NB_CASES_ROUGE = sizeof(CASES_AUTORISEES_ROUGE) / sizeof(CASES_AUTORISEES_ROUGE[0]);

/* Vérifie si une coordonnée existe dans un tableau */
static bool est_dans_tableau(int row, int col, const Coord *tableau, int taille)
{
    for (int i = 0; i < taille; i++) {
        if (tableau[i].row == row && tableau[i].col == col) {
            return true;
        }
    }
    return false;
}

/* Dessine une pièce */
static void draw_piece(cairo_t *cr, const char *symbol, int color, double x, double y, double size)
{
    if (color == BLEU)
        cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);
    else
        cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);

    cairo_select_font_face(cr, "DejaVu Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, size * 0.72);

    cairo_text_extents_t extents;
    cairo_text_extents(cr, symbol, &extents);

    double tx = x + (size - extents.width) / 2.0 - extents.x_bearing;
    double ty = y + (size - extents.height) / 2.0 - extents.y_bearing;

    cairo_move_to(cr, tx, ty);
    cairo_show_text(cr, symbol);
}

/* Dessine une croix centrée */
static void draw_croix(cairo_t *cr, int col, int row, double cell_w, double cell_h)
{
    /* Ne dessine RIEN si la coordonnée vaut -1 (pas encore posée) */
    if (col == -1 || row == -1) return;

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);

    double font_size = MIN(cell_w, cell_h) * 0.7;
    cairo_set_font_size(cr, font_size);

    cairo_text_extents_t extents;
    cairo_text_extents(cr, "X", &extents);

    double center_x = col * cell_w + cell_w / 2.0;
    double center_y = row * cell_h + cell_h / 2.0;

    double tx = center_x - (extents.width / 2.0 + extents.x_bearing);
    double ty = center_y - (extents.height / 2.0 + extents.y_bearing);

    cairo_move_to(cr, tx, ty);
    cairo_show_text(cr, "X");
}

/* Dessin du plateau */
static void draw_board(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data)
{
    double cell_w = (double) width / COLS;
    double cell_h = (double) height / ROWS;

    /* Fond blanc */
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_paint(cr);

    /* Cases jaunes */
    cairo_set_source_rgb(cr, 1.0, 0.82, 0.28);
    cairo_rectangle(cr, 8 * cell_w, 0 * cell_h, cell_w, cell_h); cairo_fill(cr);
    cairo_rectangle(cr, 9 * cell_w, 0 * cell_h, cell_w, cell_h); cairo_fill(cr);
    cairo_rectangle(cr, 10 * cell_w, 0 * cell_h, cell_w, cell_h); cairo_fill(cr);
    cairo_rectangle(cr, 7 * cell_w, 1 * cell_h, cell_w, cell_h); cairo_fill(cr);
    cairo_rectangle(cr, 6 * cell_w, 2 * cell_h, cell_w, cell_h); cairo_fill(cr);
    cairo_rectangle(cr, 5 * cell_w, 3 * cell_h, cell_w, cell_h); cairo_fill(cr);
    cairo_rectangle(cr, 4 * cell_w, 4 * cell_h, cell_w, cell_h); cairo_fill(cr);
    cairo_rectangle(cr, 3 * cell_w, 5 * cell_h, cell_w, cell_h); cairo_fill(cr);
    cairo_rectangle(cr, 2 * cell_w, 6 * cell_h, cell_w, cell_h); cairo_fill(cr);
    cairo_rectangle(cr, 1 * cell_w, 6 * cell_h, cell_w, cell_h); cairo_fill(cr);
    cairo_rectangle(cr, 0 * cell_w, 6 * cell_h, cell_w, cell_h); cairo_fill(cr);

    /* Case bleue */
    cairo_set_source_rgb(cr, 0.0, 0.0, 1.0);
    cairo_rectangle(cr, 0 * cell_w, 0 * cell_h, cell_w, cell_h);
    cairo_fill(cr);

    for (int c = 2; c < 4; c++) {
        cairo_set_source_rgb(cr, 0.75, 0.84, 0.90);
        cairo_rectangle(cr, c * cell_w, 0 * cell_h, cell_w, cell_h); cairo_fill(cr);
    }
    for (int c = 1; c < 4; c++) {
        cairo_set_source_rgb(cr, 0.75, 0.84, 0.90);
        cairo_rectangle(cr, c * cell_w, 1 * cell_h, cell_w, cell_h); cairo_fill(cr);
    }
    for (int c = 0; c < 3; c++) {
        cairo_set_source_rgb(cr, 0.75, 0.84, 0.90);
        cairo_rectangle(cr, c * cell_w, 2 * cell_h, cell_w, cell_h); cairo_fill(cr);
    }
    for (int c = 0; c < 2; c++) {
        cairo_set_source_rgb(cr, 0.75, 0.84, 0.90);
        cairo_rectangle(cr, c * cell_w, 3 * cell_h, cell_w, cell_h); cairo_fill(cr);
    }

    /* Case rouge */
    cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
    cairo_rectangle(cr, 10 * cell_w, 6 * cell_h, cell_w, cell_h);
    cairo_fill(cr);

    for (int c = 9; c < 11; c++) {
        cairo_set_source_rgb(cr, 0.95, 0.70, 0.70);
        cairo_rectangle(cr, c * cell_w, 3 * cell_h, cell_w, cell_h); cairo_fill(cr);
    }
    for (int c = 8; c < 11; c++) {
        cairo_set_source_rgb(cr, 0.95, 0.70, 0.70);
        cairo_rectangle(cr, c * cell_w, 4 * cell_h, cell_w, cell_h); cairo_fill(cr);
    }
    for (int c = 7; c < 10; c++) {
        cairo_set_source_rgb(cr, 0.95, 0.70, 0.70);
        cairo_rectangle(cr, c * cell_w, 5 * cell_h, cell_w, cell_h); cairo_fill(cr);
    }
    for (int c = 7; c < 9; c++) {
        cairo_set_source_rgb(cr, 0.95, 0.70, 0.70);
        cairo_rectangle(cr, c * cell_w, 6 * cell_h, cell_w, cell_h); cairo_fill(cr);
    }

    /* Cases conquises */
    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            int val = jeu_valeur_case(row, col);

            if (val == 1 || val == 2 || val == 3)
                cairo_set_source_rgb(cr, 0.75, 0.84, 0.90);
            else if (val == -1 || val == -2 || val == -3)
                cairo_set_source_rgb(cr, 0.95, 0.70, 0.70);
            else
                continue;

            cairo_rectangle(cr, col * cell_w, row * cell_h, cell_w, cell_h);
            cairo_fill(cr);
        }
    }

    /* Case sélectionnée en vert */
    if (selected_piece != -1) {
        Piece *p = jeu_piece(selected_piece);

        if (p != NULL && p->row >= 0) {
            cairo_set_source_rgb(cr, 0.6, 1.0, 0.6);
            cairo_rectangle(cr, p->col * cell_w, p->row * cell_h, cell_w, cell_h);
            cairo_fill(cr);

            /* HAUT */
            for (int row = p->row - 1; row >= 0; row--) {
                if (jeu_piece_at(row, p->col) != -1 || jeu_est_barricade(row, p->col)) break;
                cairo_set_source_rgb(cr, 0.6, 1.0, 0.6);
                cairo_rectangle(cr, p->col * cell_w, row * cell_h, cell_w, cell_h);
                cairo_fill(cr);
            }
            /* BAS */
            for (int row = p->row + 1; row < ROWS; row++) {
                if (jeu_piece_at(row, p->col) != -1 || jeu_est_barricade(row, p->col)) break;
                cairo_set_source_rgb(cr, 0.6, 1.0, 0.6);
                cairo_rectangle(cr, p->col * cell_w, row * cell_h, cell_w, cell_h);
                cairo_fill(cr);
            }
            /* GAUCHE */
            for (int col = p->col - 1; col >= 0; col--) {
                if (jeu_piece_at(p->row, col) != -1 || jeu_est_barricade(p->row, col)) break;
                cairo_set_source_rgb(cr, 0.6, 1.0, 0.6);
                cairo_rectangle(cr, col * cell_w, p->row * cell_h, cell_w, cell_h);
                cairo_fill(cr);
            }
            /* DROITE */
            for (int col = p->col + 1; col < COLS; col++) {
                if (jeu_piece_at(p->row, col) != -1 || jeu_est_barricade(p->row, col)) break;
                cairo_set_source_rgb(cr, 0.6, 1.0, 0.6);
                cairo_rectangle(cr, col * cell_w, p->row * cell_h, cell_w, cell_h);
                cairo_fill(cr);
            }
        }
    }

    /* Grille */
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

    /* Affichage des deux croix (seulement si posées) */
    draw_croix(cr, x1_col, x1_row, cell_w, cell_h);
    draw_croix(cr, x2_col, x2_row, cell_w, cell_h);

    /* Pièces */
    int nb_pieces = jeu_nombre_pieces();
    for (int i = 0; i < nb_pieces; i++) {
        Piece *p = jeu_piece(i);
        if (p == NULL || p->row < 0) continue;

        double x = p->col * cell_w;
        double y = p->row * cell_h;

        draw_piece(cr, p->symbol, p->color, x, y, MIN(cell_w, cell_h));
    }

    /* Caractères chinois dans les coins */
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);  
    cairo_select_font_face(cr, "Noto Sans CJK SC", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 60);
    cairo_move_to(cr, 734, 485);   
    cairo_show_text(cr, "市");

    cairo_move_to(cr, 7, 60);   
    cairo_show_text(cr, "市");
}

/* Gestionnaire de clic */
static void board_clicked(GtkGestureClick *gesture, int n_press,
                          double x, double y, gpointer user_data)
{
    GtkDrawingArea *drawing_area = GTK_DRAWING_AREA(user_data);

    int width = gtk_widget_get_width(GTK_WIDGET(drawing_area));
    int height = gtk_widget_get_height(GTK_WIDGET(drawing_area));

    if (width <= 0 || height <= 0)
        return;

    double cell_w = (double) width / COLS;
    double cell_h = (double) height / ROWS;

    int col = (int)(x / cell_w);
    int row = (int)(y / cell_h);

    if (row < 0 || row >= ROWS || col < 0 || col >= COLS)
        return;

    int joueur = jeu_joueur_actuel();

    /* --- PHASE 1 : POSE DES CROIX --- */

    /* 1.1 Pose de la Croix BLEUE */
    if (joueur == BLEU && !croix_posee_j1) {
        if (!est_dans_tableau(row, col, CASES_AUTORISEES_BLEU, NB_CASES_BLEU)) {
            return;
        }

        /* Active l'affichage de la croix sur la case cliquée */
        x1_col = col;
        x1_row = row;
        croix_posee_j1 = true;

        jeu_poser_barricade(row, col); /* Bloque la case dans le moteur */
        jeu_changer_joueur();          /* Tour au Rouge pour poser son X */
        gtk_widget_queue_draw(GTK_WIDGET(drawing_area));
        return;
    }

    /* 1.2 Pose de la Croix ROUGE */
    if (joueur == ROUGE && !croix_posee_j2) {
        if (!est_dans_tableau(row, col, CASES_AUTORISEES_ROUGE, NB_CASES_ROUGE)) {
            return;
        }

        /* Active l'affichage de la croix sur la case cliquée */
        x2_col = col;
        x2_row = row;
        croix_posee_j2 = true;

        jeu_poser_barricade(row, col); /* Bloque la case dans le moteur */
        jeu_changer_joueur();          /* Tour au Bleu pour démarrer la partie */
        gtk_widget_queue_draw(GTK_WIDGET(drawing_area));
        return;
    }

    /* --- PHASE 2 : DÉPLACEMENT DES PIÈCES --- */

    if (selected_piece == -1) {
        int p = jeu_piece_at(row, col);

        if (p != -1) {
            Piece *piece = jeu_piece(p);
            if (piece != NULL && piece->color == joueur) {
                selected_piece = p;
                gtk_widget_queue_draw(GTK_WIDGET(drawing_area));
            }
        }
        return;
    }

    Piece *piece = jeu_piece(selected_piece);
    if (piece == NULL) {
        selected_piece = -1;
        gtk_widget_queue_draw(GTK_WIDGET(drawing_area));
        return;
    }

    if (row == piece->row && col == piece->col) {
        selected_piece = -1;
        gtk_widget_queue_draw(GTK_WIDGET(drawing_area));
        return;
    }

    if (jeu_deplacer(selected_piece, row, col)) {
        selected_piece = -1;
        gtk_widget_queue_draw(GTK_WIDGET(drawing_area));
    }
}

static void activate(GtkApplication *app, gpointer data)
{
    GtkWidget *window;
    GtkWidget *drawing_area;

    window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Klosnowo");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 500);

    drawing_area = gtk_drawing_area_new();
    gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(drawing_area), 800);
    gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(drawing_area), 500);
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
