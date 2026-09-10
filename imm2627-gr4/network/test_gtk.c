#include <gtk/gtk.h>
#include <cairo.h>
#include <stdlib.h>
#include <stdio.h>

#define ROWS 7
#define COLS 11
#define MAX_TURNS 64

#define BLUE 0
#define RED  1
#define NONE -1

#define EMPTY 0
#define BLUE_KING 1
#define BLUE_SOLDIER 2
#define RED_KING 3
#define RED_SOLDIER 4

typedef struct {
    int pieces[ROWS][COLS];
    int control[ROWS][COLS];
    gboolean barricade[ROWS][COLS];

    int player;
    int turn;

    int selected_r;
    int selected_c;

    int barricades_chosen;
    gboolean choosing_barricade;
    gboolean game_over;
} Game;

static Game game;
static GtkWidget *drawing_area;
static GtkWidget *turn_label;
static GtkWidget *score_label;
static GtkWidget *info_label;

/* ---------- OUTILS ---------- */

static gboolean inside(int r, int c) {
    return r >= 0 && r < ROWS && c >= 0 && c < COLS;
}

static int owner(int piece) {
    if (piece == BLUE_KING || piece == BLUE_SOLDIER) return BLUE;
    if (piece == RED_KING || piece == RED_SOLDIER) return RED;
    return NONE;
}

static gboolean is_city(int r, int c) {
    return (r == 0 && c == 0) || (r == 6 && c == 10);
}

/*
 * Les cases jaunes reprÃ©sentent la diagonale/zone spÃ©ciale
 * visible dans le sujet.
 */
static gboolean is_yellow(int r, int c) {
    return (r == 0 && c >= 8) ||
           (r == 1 && c == 7) ||
           (r == 2 && c == 6) ||
           (r == 3 && c == 5) ||
           (r == 4 && c == 4) ||
           (r == 5 && c == 3) ||
           (r == 6 && c <= 2);
}

static gboolean on_diagonal(int r, int c) {
    return c == r + 4;
}

static gboolean adjacent_city(int r, int c) {
    return (abs(r) + abs(c) == 1) ||
           (abs(r - 6) + abs(c - 10) == 1);
}

/* ---------- SCORE / ETAT ---------- */

static int score_player(int p) {
    int score = 0;

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (game.control[r][c] == p)
                score++;

            if (owner(game.pieces[r][c]) == p &&
                game.pieces[r][c] != (p == BLUE ? BLUE_KING : RED_KING))
                score++;
        }
    }

    return score;
}

static void update_labels(void) {
    char text[256];

    if (game.choosing_barricade) {
        snprintf(text, sizeof(text), "Choix barricade : Joueur %s",
                 game.player == BLUE ? "Bleu" : "Rouge");
    } else if (game.game_over) {
        snprintf(text, sizeof(text), "Partie terminÃ©e");
    } else {
        snprintf(text, sizeof(text), "Joueur %s",
                 game.player == BLUE ? "Bleu" : "Rouge");
    }

    gtk_label_set_text(GTK_LABEL(turn_label), text);

    snprintf(text, sizeof(text),
             "Bleu : %d     Rouge : %d",
             score_player(BLUE), score_player(RED));
    gtk_label_set_text(GTK_LABEL(score_label), text);

    if (game.choosing_barricade) {
        gtk_label_set_text(GTK_LABEL(info_label),
            "Choisissez une case pour votre barricade.");
    } else if (game.game_over) {
        gtk_label_set_text(GTK_LABEL(info_label),
            "Partie terminÃ©e â€” appuyez sur R pour recommencer.");
    } else {
        snprintf(text, sizeof(text),
                 "Tour %d / %d\n\nCliquez sur une piÃ¨ce puis sur une case verte.",
                 game.turn + 1, MAX_TURNS);
        gtk_label_set_text(GTK_LABEL(info_label), text);
    }
}

/* ---------- BARRICADE ---------- */

static gboolean valid_barricade(int r, int c) {
    if (!inside(r,c) || is_city(r,c) || game.barricade[r][c])
        return FALSE;

    if (on_diagonal(r,c) || adjacent_city(r,c))
        return FALSE;

    /* Bleu choisit Ã  gauche de la diagonale, rouge Ã  droite. */
    if (game.player == BLUE)
        return c < r + 4;

    return c > r + 4;
}

/* ---------- DEPLACEMENT ---------- */

static gboolean legal_move(int sr, int sc, int dr, int dc) {
    int piece, vr, vc, r, c;

    if (!inside(sr,sc) || !inside(dr,dc))
        return FALSE;

    piece = game.pieces[sr][sc];

    if (piece == EMPTY || owner(piece) != game.player)
        return FALSE;

    if (sr == dr && sc == dc)
        return FALSE;

    if (game.pieces[dr][dc] != EMPTY || game.barricade[dr][dc])
        return FALSE;

    /* Mouvement horizontal ou vertical uniquement. */
    if (sr != dr && sc != dc)
        return FALSE;

    vr = (dr > sr) - (dr < sr);
    vc = (dc > sc) - (dc < sc);

    r = sr + vr;
    c = sc + vc;

    /* Impossible de traverser une autre piÃ¨ce ou une barricade. */
    while (r != dr || c != dc) {
        if (game.pieces[r][c] != EMPTY || game.barricade[r][c])
            return FALSE;

        r += vr;
        c += vc;
    }

    /* Seul le roi peut entrer dans la citÃ© adverse. */
    if (is_city(dr,dc)) {
        if (game.player == BLUE && dr == 6 && dc == 10)
            return piece == BLUE_KING;

        if (game.player == RED && dr == 0 && dc == 0)
            return piece == RED_KING;

        return FALSE;
    }

    return TRUE;
}

/* ---------- CAPTURES ---------- */

static void capture_piece(int r, int c) {
    int piece = game.pieces[r][c];

    if (piece == EMPTY)
        return;

    game.pieces[r][c] = EMPTY;
    game.control[r][c] = NONE;

    if (piece == BLUE_KING || piece == RED_KING) {
        game.game_over = TRUE;

        char msg[256];
        snprintf(msg, sizeof(msg),
                 "%s gagne !\n\nLe roi adverse a Ã©tÃ© capturÃ©.",
                 game.player == BLUE ? "Bleu" : "Rouge");

        GtkWidget *dialog = gtk_message_dialog_new(
            NULL, GTK_DIALOG_MODAL,
            GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s", msg);

        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
}

/*
 * Linca = prise sandwich.
 * La piÃ¨ce qui vient de bouger encadre une piÃ¨ce ennemie
 * avec une piÃ¨ce alliÃ©e.
 */
static void linca(int r, int c) {
    const int directions[4][2] = {
        {1,0}, {-1,0}, {0,1}, {0,-1}
    };

    int me = owner(game.pieces[r][c]);

    for (int i = 0; i < 4; i++) {
        int r1 = r + directions[i][0];
        int c1 = c + directions[i][1];
        int r2 = r + 2*directions[i][0];
        int c2 = c + 2*directions[i][1];

        if (inside(r2,c2) &&
            game.pieces[r1][c1] != EMPTY &&
            owner(game.pieces[r1][c1]) != me &&
            game.pieces[r2][c2] != EMPTY &&
            owner(game.pieces[r2][c2]) == me) {
            capture_piece(r1,c1);
        }
    }
}

/*
 * Seultou : prise en poussant selon la rÃ¨gle donnÃ©e
 * dans les exemples du sujet.
 */
static void seultou(int r, int c) {
    const int directions[4][2] = {
        {1,0}, {-1,0}, {0,1}, {0,-1}
    };

    int me = owner(game.pieces[r][c]);

    for (int i = 0; i < 4; i++) {
        int ar = r + directions[i][0];
        int ac = c + directions[i][1];

        if (!inside(ar,ac) || game.pieces[ar][ac] == EMPTY)
            continue;

        if (owner(game.pieces[ar][ac]) == me)
            continue;

        int br = ar + directions[i][0];
        int bc = ac + directions[i][1];

        gboolean enemy_support =
            inside(br,bc) &&
            game.pieces[br][bc] != EMPTY &&
            owner(game.pieces[br][bc]) == owner(game.pieces[ar][ac]);

        if (!enemy_support && !is_city(ar,ac) && !game.barricade[ar][ac])
            capture_piece(ar,ac);
    }
}

/* ---------- FIN DE PARTIE ---------- */

static void finish_game(int winner, const char *reason) {
    char msg[512];

    if (winner == BLUE)
        snprintf(msg,sizeof(msg),"Victoire BLEU !\n\n%s",reason);
    else if (winner == RED)
        snprintf(msg,sizeof(msg),"Victoire ROUGE !\n\n%s",reason);
    else
        snprintf(msg,sizeof(msg),"Ã‰GALITÃ‰ !\n\n%s",reason);

    char scores[128];
    snprintf(scores,sizeof(scores),
             "\n\nScore final : Bleu %d â€” Rouge %d",
             score_player(BLUE), score_player(RED));
    strncat(msg, scores, sizeof(msg)-strlen(msg)-1);

    GtkWidget *dialog = gtk_message_dialog_new(
        NULL, GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s", msg);

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void check_victory(void) {
    if (game.game_over)
        return;

    /* Le dernier joueur est celui qui vient de jouer. */
    int last = 1 - game.player;
    int king = last == BLUE ? BLUE_KING : RED_KING;

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            if (game.pieces[r][c] != king)
                continue;

            if (last == BLUE && r == 6 && c == 10) {
                game.game_over = TRUE;
                finish_game(BLUE, "ConquÃªte de la citÃ© adverse.");
                return;
            }

            if (last == RED && r == 0 && c == 0) {
                game.game_over = TRUE;
                finish_game(RED, "ConquÃªte de la citÃ© adverse.");
                return;
            }
        }
    }

    if (game.turn >= MAX_TURNS) {
        int blue = score_player(BLUE);
        int red = score_player(RED);

        game.game_over = TRUE;

        if (blue > red)
            finish_game(BLUE, "64 tours atteints : meilleur score.");
        else if (red > blue)
            finish_game(RED, "64 tours atteints : meilleur score.");
        else
            finish_game(NONE, "64 tours atteints : Ã©galitÃ©.");
    }
}

/* ---------- JOUER UN COUP ---------- */

static void play_move(int sr, int sc, int dr, int dc) {
    int piece = game.pieces[sr][sc];

    game.pieces[sr][sc] = EMPTY;
    game.pieces[dr][dc] = piece;

    /* Une case visitÃ©e devient contrÃ´lÃ©e. */
    game.control[dr][dc] = game.player;

    linca(dr,dc);

    if (!game.game_over)
        seultou(dr,dc);

    game.turn++;
    game.player = 1 - game.player;

    game.selected_r = -1;
    game.selected_c = -1;

    check_victory();
    update_labels();
    gtk_widget_queue_draw(drawing_area);
}

/* ---------- INITIALISATION ---------- */

static void reset_game(void) {
    int r,c;

    for (r=0;r<ROWS;r++)
        for (c=0;c<COLS;c++) {
            game.pieces[r][c] = EMPTY;
            game.control[r][c] = NONE;
            game.barricade[r][c] = FALSE;
        }

    /* Bleu : roi + 9 soldats. */
    game.pieces[1][1] = BLUE_KING;

    int blue[9][2] = {
        {0,2},{0,3},{1,2},{1,3},
        {2,0},{2,1},{2,2},{3,0},{3,1}
    };

    for (int i=0;i<9;i++)
        game.pieces[blue[i][0]][blue[i][1]] = BLUE_SOLDIER;

    /* Rouge : roi + 9 soldats. */
    game.pieces[5][9] = RED_KING;

    int red[9][2] = {
        {3,9},{3,10},{4,8},{4,9},{4,10},
        {5,7},{5,8},{6,7},{6,8}
    };

    for (int i=0;i<9;i++)
        game.pieces[red[i][0]][red[i][1]] = RED_SOLDIER;

    /* ContrÃ´le initial : les 10 cases occupÃ©es. */
    for (r=0;r<ROWS;r++)
        for (c=0;c<COLS;c++)
            if (game.pieces[r][c] != EMPTY)
                game.control[r][c] = owner(game.pieces[r][c]);

    game.player = BLUE;
    game.turn = 0;
    game.selected_r = -1;
    game.selected_c = -1;
    game.barricades_chosen = 0;
    game.choosing_barricade = TRUE;
    game.game_over = FALSE;

    update_labels();
    gtk_widget_queue_draw(drawing_area);
}

/* ---------- DESSIN ---------- */

static void rounded_rect(cairo_t *cr, double x, double y,
                         double w, double h, double radius) {
    cairo_new_sub_path(cr);
    cairo_arc(cr,x+w-radius,y+radius,radius,-G_PI,-G_PI/2);
    cairo_arc(cr,x+w-radius,y+h-radius,radius,-G_PI/2,0);
    cairo_arc(cr,x+radius,y+h-radius,radius,0,G_PI/2);
    cairo_arc(cr,x+radius,y+radius,radius,G_PI/2,G_PI);
    cairo_close_path(cr);
}

static void draw_piece(cairo_t *cr, int x, int y, int size, int piece) {
    int p = owner(piece);
    gboolean king = piece == BLUE_KING || piece == RED_KING;

    if (p == BLUE)
        cairo_set_source_rgb(cr,0.08,0.27,0.88);
    else
        cairo_set_source_rgb(cr,0.88,0.08,0.13);

    double cx = x + size/2.0;
    double cy = y + size/2.0;

    if (king) {
        /* Couronne stylisÃ©e. */
        cairo_move_to(cr,cx-25,cy+20);
        cairo_line_to(cr,cx-22,cy-16);
        cairo_line_to(cr,cx-10,cy-4);
        cairo_line_to(cr,cx,cy-22);
        cairo_line_to(cr,cx+10,cy-4);
        cairo_line_to(cr,cx+22,cy-16);
        cairo_line_to(cr,cx+25,cy+20);
        cairo_close_path(cr);
        cairo_fill(cr);

        cairo_rectangle(cr,cx-28,cy+17,56,9);
        cairo_fill(cr);
    } else {
        /* Soldat : forme simple type tour. */
        rounded_rect(cr,cx-21,cy-16,42,38,5);
        cairo_fill(cr);

        cairo_rectangle(cr,cx-27,cy+19,54,8);
        cairo_fill(cr);

        cairo_rectangle(cr,cx-18,cy+27,36,7);
        cairo_fill(cr);
    }
}

static void draw_city(cairo_t *cr, int x, int y, int size) {
    cairo_set_source_rgb(cr,1,1,1);
    cairo_select_font_face(cr,"Sans",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr,42);

    cairo_move_to(cr,x+size/2-20,y+size/2+15);
    cairo_show_text(cr,"å¸‚");
}

static gboolean draw_board(GtkWidget *widget, cairo_t *cr, gpointer data) {
    GtkAllocation a;
    gtk_widget_get_allocation(widget,&a);

    int cell = 76;
    int board_w = COLS * cell;
    int board_h = ROWS * cell;
    int ox = 30;
    int oy = 22;

    /* Fond. */
    cairo_set_source_rgb(cr,0.03,0.08,0.14);
    cairo_paint(cr);

    /* Cadre du plateau. */
    cairo_set_source_rgb(cr,0.08,0.17,0.26);
    rounded_rect(cr,ox-15,oy-15,board_w+30,board_h+30,16);
    cairo_fill(cr);

    for (int r=0;r<ROWS;r++) {
        for (int c=0;c<COLS;c++) {
            int x=ox+c*cell;
            int y=oy+r*cell;

            if (is_city(r,c)) {
                if (r==0)
                    cairo_set_source_rgb(cr,0.08,0.34,0.90);
                else
                    cairo_set_source_rgb(cr,0.92,0.08,0.10);
            } else if (game.barricade[r][c]) {
                cairo_set_source_rgb(cr,0.58,0.39,0.13);
            } else if (game.selected_r >= 0 &&
                       legal_move(game.selected_r,game.selected_c,r,c)) {
                cairo_set_source_rgb(cr,0.45,0.82,0.55);
            } else if (is_yellow(r,c)) {
                cairo_set_source_rgb(cr,1.0,0.80,0.22);
            } else if (game.control[r][c] == BLUE) {
                cairo_set_source_rgb(cr,0.78,0.87,0.94);
            } else if (game.control[r][c] == RED) {
                cairo_set_source_rgb(cr,0.96,0.79,0.81);
            } else {
                cairo_set_source_rgb(cr,0.82,0.91,0.78);
            }

            cairo_rectangle(cr,x,y,cell,cell);
            cairo_fill(cr);

            cairo_set_source_rgb(cr,0.10,0.15,0.18);
            cairo_set_line_width(cr,1.2);
            cairo_rectangle(cr,x,y,cell,cell);
            cairo_stroke(cr);

            if (game.barricade[r][c]) {
                cairo_set_source_rgb(cr,0.25,0.16,0.07);
                cairo_set_line_width(cr,5);
                cairo_move_to(cr,x+15,y+cell-15);
                cairo_line_to(cr,x+cell-15,y+15);
                cairo_move_to(cr,x+25,y+cell-15);
                cairo_line_to(cr,x+cell-5,y+25);
                cairo_stroke(cr);
            }

            if (is_city(r,c))
                draw_city(cr,x,y,cell);

            if (game.pieces[r][c] != EMPTY)
                draw_piece(cr,x,y,cell,game.pieces[r][c]);

            if (game.selected_r == r && game.selected_c == c) {
                cairo_set_source_rgb(cr,1,1,1);
                cairo_set_line_width(cr,4);
                cairo_rectangle(cr,x+4,y+4,cell-8,cell-8);
                cairo_stroke(cr);
            }
        }
    }

    return FALSE;
}

/* ---------- CLIC ---------- */

static gboolean button_press(GtkWidget *widget, GdkEventButton *event,
                             gpointer data) {
    int cell=76, ox=30, oy=22;
    int c=(int)((event->x-ox)/cell);
    int r=(int)((event->y-oy)/cell);

    if (!inside(r,c) || game.game_over)
        return TRUE;

    /* Phase des barricades. */
    if (game.choosing_barricade) {
        if (valid_barricade(r,c)) {
            game.barricade[r][c]=TRUE;
            game.barricades_chosen++;

            if (game.barricades_chosen == 2) {
                game.choosing_barricade=FALSE;
                game.player=BLUE;
            } else {
                game.player=RED;
            }

            update_labels();
            gtk_widget_queue_draw(drawing_area);
        }
        return TRUE;
    }

    /* SÃ©lection. */
    if (game.selected_r < 0) {
        if (game.pieces[r][c] != EMPTY &&
            owner(game.pieces[r][c]) == game.player) {
            game.selected_r=r;
            game.selected_c=c;
            gtk_widget_queue_draw(drawing_area);
        }
        return TRUE;
    }

    /* DÃ©placement. */
    if (legal_move(game.selected_r,game.selected_c,r,c)) {
        play_move(game.selected_r,game.selected_c,r,c);
    } else if (game.pieces[r][c] != EMPTY &&
               owner(game.pieces[r][c]) == game.player) {
        game.selected_r=r;
        game.selected_c=c;
        gtk_widget_queue_draw(drawing_area);
    } else {
        game.selected_r=-1;
        game.selected_c=-1;
        gtk_widget_queue_draw(drawing_area);
    }

    return TRUE;
}

/* ---------- REGLES ---------- */

static void show_rules(GtkWidget *widget, gpointer data) {
    const char *rules =
        "KLOSNOWO â€” RÃˆGLES\n\n"
        "â€¢ Plateau : 11 Ã— 7, Bleu commence.\n"
        "â€¢ Chaque joueur : 1 roi + 9 soldats.\n"
        "â€¢ DÃ©placement horizontal ou vertical.\n"
        "â€¢ Une piÃ¨ce ne peut pas sauter une autre piÃ¨ce.\n"
        "â€¢ Seul le roi peut entrer dans la citÃ© adverse.\n"
        "â€¢ Victoire par conquÃªte ou capture du roi.\n"
        "â€¢ Linca : prise sandwich.\n"
        "â€¢ Seultou : prise en poussant.\n"
        "â€¢ Une case visitÃ©e reste contrÃ´lÃ©e.\n"
        "â€¢ Une capture rend la case neutre.\n"
        "â€¢ AprÃ¨s 64 tours : cases contrÃ´lÃ©es + soldats vivants.\n"
        "â€¢ La barricade est infranchissable et non capturable.";

    GtkWidget *dialog=gtk_message_dialog_new(
        NULL,GTK_DIALOG_MODAL,GTK_MESSAGE_INFO,GTK_BUTTONS_OK,
        "%s",rules);

    gtk_window_set_title(GTK_WINDOW(dialog),"RÃ¨gles de Klosnowo");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

/* ---------- CLAVIER ---------- */

static gboolean key_press(GtkWidget *widget, GdkEventKey *event,
                          gpointer data) {
    if (event->keyval == GDK_KEY_r ||
        event->keyval == GDK_KEY_R) {
        reset_game();
        return TRUE;
    }

    if (event->keyval == GDK_KEY_Escape) {
        gtk_main_quit();
        return TRUE;
    }

    return FALSE;
}

/* ---------- INTERFACE ---------- */

static GtkWidget *make_label(const char *text, int size, GdkRGBA color) {
    GtkWidget *label=gtk_label_new(text);

    PangoAttrList *attrs=pango_attr_list_new();
    PangoAttribute *font=pango_attr_size_new(size*PANGO_SCALE);
    PangoAttribute *weight=pango_attr_weight_new(PANGO_WEIGHT_BOLD);
    PangoAttribute *fg=pango_attr_foreground_new(
        color.red*65535,color.green*65535,color.blue*65535);

    pango_attr_list_insert(attrs,font);
    pango_attr_list_insert(attrs,weight);
    pango_attr_list_insert(attrs,fg);

    gtk_label_set_attributes(GTK_LABEL(label),attrs);
    pango_attr_list_unref(attrs);

    gtk_label_set_xalign(GTK_LABEL(label),0);
    return label;
}

static GtkWidget *panel_box(void) {
    GtkWidget *box=gtk_box_new(GTK_ORIENTATION_VERTICAL,8);
    gtk_widget_set_margin_start(box,14);
    gtk_widget_set_margin_end(box,14);
    gtk_widget_set_margin_top(box,14);
    gtk_widget_set_margin_bottom(box,14);
    return box;
}

static void activate(GtkApplication *app, gpointer data) {
    GtkWidget *window=gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window),"Klosnowo");
    gtk_window_set_default_size(GTK_WINDOW(window),1220,760);
    gtk_window_set_resizable(window,TRUE);

    GtkWidget *main=gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_container_add(GTK_CONTAINER(window),main);

    /* HEADER */
    GtkWidget *header=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,20);
    gtk_widget_set_margin_start(header,22);
    gtk_widget_set_margin_end(header,22);
    gtk_widget_set_margin_top(header,12);
    gtk_widget_set_margin_bottom(header,12);

    GdkRGBA white={1,1,1,1};
    GdkRGBA grey={0.70,0.76,0.84,1};

    gtk_box_pack_start(GTK_BOX(header),
        make_label("Klosnowo",28,white),FALSE,FALSE,0);

    gtk_box_pack_start(GTK_BOX(header),
        make_label("Jeu de stratÃ©gie â€” 11 Ã— 7",14,grey),FALSE,FALSE,0);

    GtkWidget *spacer=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    gtk_box_pack_start(GTK_BOX(header),spacer,TRUE,TRUE,0);

    GtkWidget *rules=gtk_button_new_with_label("âš™  RÃ¨gles");
    g_signal_connect(rules,"clicked",G_CALLBACK(show_rules),NULL);
    gtk_box_pack_start(GTK_BOX(header),rules,FALSE,FALSE,0);

    GtkWidget *restart=gtk_button_new_with_label("â†»  Recommencer");
    g_signal_connect_swapped(restart,"clicked",G_CALLBACK(reset_game),NULL);
    gtk_box_pack_start(GTK_BOX(header),restart,FALSE,FALSE,0);

    gtk_box_pack_start(GTK_BOX(main),header,FALSE,FALSE,0);

    /* CONTENU */
    GtkWidget *content=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    gtk_box_pack_start(GTK_BOX(main),content,TRUE,TRUE,0);

    drawing_area=gtk_drawing_area_new();
    gtk_widget_set_size_request(drawing_area,900,590);
    gtk_widget_add_events(drawing_area,GDK_BUTTON_PRESS_MASK);
    g_signal_connect(drawing_area,"draw",G_CALLBACK(draw_board),NULL);
    g_signal_connect(drawing_area,"button-press-event",
                     G_CALLBACK(button_press),NULL);

    gtk_box_pack_start(GTK_BOX(content),drawing_area,TRUE,TRUE,0);

    /* SIDEBAR */
    GtkWidget *side=gtk_box_new(GTK_ORIENTATION_VERTICAL,12);
    gtk_widget_set_size_request(side,260,-1);
    gtk_widget_set_margin_start(side,10);
    gtk_widget_set_margin_end(side,15);
    gtk_widget_set_margin_top(side,10);
    gtk_widget_set_margin_bottom(side,10);

    turn_label=make_label("",16,white);
    score_label=make_label("",15,white);
    info_label=make_label("",13,grey);

    gtk_box_pack_start(GTK_BOX(side),
        make_label("TOUR ACTUEL",12,grey),FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(side),turn_label,FALSE,FALSE,0);

    gtk_box_pack_start(GTK_BOX(side),
        make_label("SCORE DU PLATEAU",12,grey),FALSE,FALSE,8);
    gtk_box_pack_start(GTK_BOX(side),score_label,FALSE,FALSE,0);

    gtk_box_pack_start(GTK_BOX(side),
        make_label("LÃ‰GENDE",12,grey),FALSE,FALSE,12);

    gtk_box_pack_start(GTK_BOX(side),
        make_label("â™”  Roi bleu",14,white),FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(side),
        make_label("â™œ  Soldat bleu",14,white),FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(side),
        make_label("â™”  Roi rouge",14,white),FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(side),
        make_label("â™œ  Soldat rouge",14,white),FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(side),
        make_label("â–   CitÃ© / zone jaune",14,white),FALSE,FALSE,0);
    gtk_box_pack_start(GTK_BOX(side),
        make_label("â–   Case contrÃ´lÃ©e",14,white),FALSE,FALSE,0);

    GtkWidget *grow=gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_box_pack_start(GTK_BOX(side),grow,TRUE,TRUE,0);

    gtk_box_pack_start(GTK_BOX(side),info_label,FALSE,FALSE,0);

    gtk_box_pack_start(GTK_BOX(content),side,FALSE,FALSE,0);

    /* FOOTER */
    GtkWidget *footer=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);
    gtk_widget_set_margin_start(footer,22);
    gtk_widget_set_margin_end(footer,22);
    gtk_widget_set_margin_top(10);
    gtk_widget_set_margin_bottom(14);

    gtk_box_pack_start(GTK_BOX(footer),
        make_label("â—  SÃ©lectionnez une piÃ¨ce pour voir ses dÃ©placements possibles.",
                   13,grey),TRUE,TRUE,0);

    gtk_box_pack_end(GTK_BOX(footer),
        make_label("R : Recommencer     Ã‰chap : Quitter",
                   12,grey),FALSE,FALSE,0);

    gtk_box_pack_start(GTK_BOX(main),footer,FALSE,FALSE,0);

    g_signal_connect(window,"key-press-event",G_CALLBACK(key_press),NULL);

    reset_game();
    gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app=gtk_application_new("fr.ensisa.klosnowo",
                            G_APPLICATION_DEFAULT_FLAGS);

    g_signal_connect(app,"activate",G_CALLBACK(activate),NULL);

    status=g_application_run(G_APPLICATION(app),argc,argv);

    g_object_unref(app);
    return status;
}
