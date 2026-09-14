#include "jeu.h"
#include "stddef.h"

/* Toutes les pièces du jeu */
static Piece pieces[] = {

    /* BLEU */
    {0, 2, "♜", BLEU, TOUR},
    {0, 3, "♜", BLEU, TOUR},
    {1, 1, "♛", BLEU, ROI},
    {1, 2, "♜", BLEU, TOUR},
    {1, 3, "♜", BLEU, TOUR},
    {2, 0, "♜", BLEU, TOUR},
    {2, 1, "♜", BLEU, TOUR},
    {2, 2, "♜", BLEU, TOUR},
    {3, 0, "♜", BLEU, TOUR},
    {3, 1, "♜", BLEU, TOUR},

    /* ROUGE */
    {6, 8, "♜", ROUGE, TOUR},
    {6, 7, "♜", ROUGE, TOUR},
    {5, 9, "♛", ROUGE, ROI},
    {5, 8, "♜", ROUGE, TOUR},
    {5, 7, "♜", ROUGE, TOUR},
    {4, 10, "♜", ROUGE, TOUR},
    {4, 9, "♜", ROUGE, TOUR},
    {4, 8, "♜", ROUGE, TOUR},
    {3, 10, "♜", ROUGE, TOUR},
    {3, 9, "♜", ROUGE, TOUR}
};

static int nb_pieces = sizeof(pieces) / sizeof(pieces[0]);
static int current_player = BLEU;

/* Garde en mémoire la couleur du dernier pion ou roi qui est passé sur chaque case */
static int conquises[ROWS][COLS];

/* Garde en mémoire les cases bloquées par une barricade (1 = bloquée) */
static int barricades[ROWS][COLS];

void jeu_initialiser(void)
{
    int r;
    int c;

    current_player = BLEU;

    for (r = 0; r < ROWS; r = r + 1) {
        for (c = 0; c < COLS; c = c + 1) {
            conquises[r][c] = -1;
            barricades[r][c] = 0;
        }
    }
}

int jeu_nombre_pieces(void)
{
    return nb_pieces;
}

Piece *jeu_piece(int index)
{
    if (index < 0 || index >= nb_pieces) {
        return NULL;
    }

    return &pieces[index];
}

int jeu_piece_at(int row, int col)
{
    int i;

    for (i = 0; i < nb_pieces; i = i + 1) {

        if (pieces[i].row < 0) {
            continue;
        }

        if (pieces[i].row == row && pieces[i].col == col) {
            return i;
        }
    }

    return -1;
}

int jeu_joueur_actuel(void)
{
    return current_player;
}

void jeu_changer_joueur(void)
{
    current_player = 1 - current_player;
}

int jeu_case_jaune(int row, int col)
{
    if (row == 6) {
        return col >= 0 && col <= 2;
    }

    if (row == 5) {
        return col == 3;
    }

    if (row == 4) {
        return col == 4;
    }

    if (row == 3) {
        return col == 5;
    }

    if (row == 2) {
        return col == 6;
    }

    if (row == 1) {
        return col == 7;
    }

    if (row == 0) {
        return col >= 8 && col <= 10;
    }

    return 0;
}

/* Vérifie qu'il n'y a pas de pièce ni de barricade entre le départ et l'arrivée */
static int chemin_libre(Piece *piece, int new_row, int new_col)
{
    int dr = new_row - piece->row;
    int dc = new_col - piece->col;

    int step_row = 0;
    int step_col = 0;

    int row;
    int col;

    if (dr > 0) {
        step_row = 1;
    }
    else if (dr < 0) {
        step_row = -1;
    }

    if (dc > 0) {
        step_col = 1;
    }
    else if (dc < 0) {
        step_col = -1;
    }

    row = piece->row + step_row;
    col = piece->col + step_col;

    while (row != new_row || col != new_col) {

        if (jeu_piece_at(row, col) != -1) {
            return 0;
        }

        if (barricades[row][col] == 1) {
            return 0;
        }

        row = row + step_row;
        col = col + step_col;
    }

    return 1;
}

int jeu_peut_deplacer(int piece_index, int new_row, int new_col)
{
    Piece *piece;
    int dr;
    int dc;
    int destination;

    if (piece_index < 0 || piece_index >= nb_pieces) {
        return 0;
    }

    if (new_row < 0 || new_row >= ROWS || new_col < 0 || new_col >= COLS) {
        return 0;
    }

    if (barricades[new_row][new_col] == 1) {
        return 0;
    }

    piece = &pieces[piece_index];

    if (piece->row < 0) {
        return 0;
    }

    if (piece->color != current_player) {
        return 0;
    }

    dr = new_row - piece->row;
    dc = new_col - piece->col;

    if (dr == 0 && dc == 0) {
        return 0;
    }

    /* TOUR et ROI : déplacements en ligne droite */
    if (piece->type == TOUR || piece->type == ROI) {
        if (dr != 0 && dc != 0) {
            return 0;
        }
    }

    if (chemin_libre(piece, new_row, new_col) == 0) {
        return 0;
    }

    destination = jeu_piece_at(new_row, new_col);

    if (destination == -1) {
        return 1;
    }

    /* Pièce alliée : impossible de se poser dessus */
    if (pieces[destination].color == piece->color) {
        return 0;
    }

    /* Pièce ennemie : capture autorisée */
    return 1;
}

int jeu_deplacer(int piece_index, int new_row, int new_col)
{
    Piece *piece;
    int destination;

    if (jeu_peut_deplacer(piece_index, new_row, new_col) == 0) {
        return 0;
    }

    piece = &pieces[piece_index];
    destination = jeu_piece_at(new_row, new_col);

    /* Capture */
    if (destination != -1) {
        pieces[destination].row = -1;
        pieces[destination].col = -1;
    }

    /* Déplacement */
    piece->row = new_row;
    piece->col = new_col;

    /* Marque la case d'arrivée comme conquise */
    conquises[new_row][new_col] = piece->color;

    /* Changement de joueur */
    current_player = 1 - current_player;

    return 1;
}

int jeu_valeur_case(int row, int col)
{
    int index = jeu_piece_at(row, col);
    Piece *p;
    int val;

    if (barricades[row][col] == 1) {
        return 4;
    }

    if (index != -1) {
        p = &pieces[index];

        if (p->type == ROI) {
            val = 3;
        }
        else {
            val = 2;
        }

        if (p->color == BLEU) {
            return val;
        }
        else {
            return -val;
        }
    }

    if (conquises[row][col] == BLEU) {
        return 1;
    }

    if (conquises[row][col] == ROUGE) {
        return -1;
    }

    return 0;
}

int jeu_couleur_conquete(int row, int col)
{
    if (conquises[row][col] == BLEU) {
        return 1;
    }

    if (conquises[row][col] == ROUGE) {
        return -1;
    }

    return 0;
}

void jeu_poser_barricade(int row, int col)
{
    if (row < 0 || row >= ROWS || col < 0 || col >= COLS) {
        return;
    }

    barricades[row][col] = 1;
}

int jeu_est_barricade(int row, int col)
{
    return barricades[row][col];
}
