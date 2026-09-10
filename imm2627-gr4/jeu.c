#include "jeu.h"
#include "stddef.h"

/* toutes les pieces du jeu */
static Piece pieces[] = {

    /* BLEU */
    {0, 2, "♜", BLEU, PION},
    {0, 3, "♜", BLEU, PION},
    {1, 1, "♛", BLEU, ROI},
    {1, 2, "♜", BLEU, PION},
    {1, 3, "♜", BLEU, PION},
    {2, 0, "♜", BLEU, PION},
    {2, 1, "♜", BLEU, PION},
    {2, 2, "♜", BLEU, PION},
    {3, 0, "♜", BLEU, PION},
    {3, 1, "♜", BLEU, PION},

    /* ROUGE */
    {6, 8, "♜", ROUGE, PION},
    {6, 7, "♜", ROUGE, PION},
    {5, 9, "♛", ROUGE, ROI},
    {5, 8, "♜", ROUGE, PION},
    {5, 7, "♜", ROUGE, PION},
    {4, 10, "♜", ROUGE, PION},
    {4, 9, "♜", ROUGE, PION},
    {4, 8, "♜", ROUGE, PION},
    {3, 10, "♜", ROUGE, PION},
    {3, 9, "♜", ROUGE, PION}
};

static int nb_pieces = sizeof(pieces) / sizeof(pieces[0]);
static int current_player = BLEU;

/* garde en memoire la couleur du dernier pion ou roi qui est passe sur chaque case */
static int conquises[ROWS][COLS];

void jeu_initialiser(void)
{
    current_player = BLEU;

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            conquises[r][c] = -1;
        }
    }
}

int jeu_nombre_pieces(void)
{
    return nb_pieces;
}

Piece *jeu_piece(int index)
{
    if (index < 0 || index >= nb_pieces)
        return NULL;

    return &pieces[index];
}

int jeu_piece_at(int row, int col)
{
    for (int i = 0; i < nb_pieces; i++) {
        if (pieces[i].row < 0)
            continue;

        if (pieces[i].row == row && pieces[i].col == col)
            return i;
    }

    return -1;
}

int jeu_joueur_actuel(void)
{
    return current_player;
}

int jeu_case_jaune(int row, int col)
{
    if (row == 6)
        return col >= 0 && col <= 2;

    if (row == 5)
        return col == 3;

    if (row == 4)
        return col == 4;

    if (row == 3)
        return col == 5;

    if (row == 2)
        return col == 6;

    if (row == 1)
        return col == 7;

    if (row == 0)
        return col >= 8 && col <= 10;

    return 0;
}

/* verifie qu'il n'y a pas de piece entre le depart et l'arrivee */
static int chemin_libre(Piece *piece, int new_row, int new_col)
{
    int dr = new_row - piece->row;
    int dc = new_col - piece->col;

    int step_row = 0;
    int step_col = 0;

    if (dr > 0)
        step_row = 1;
    else if (dr < 0)
        step_row = -1;

    if (dc > 0)
        step_col = 1;
    else if (dc < 0)
        step_col = -1;

    int row = piece->row + step_row;
    int col = piece->col + step_col;

    while (row != new_row || col != new_col) {
        if (jeu_piece_at(row, col) != -1)
            return 0;

        row += step_row;
        col += step_col;
    }

    return 1;
}

int jeu_peut_deplacer(int piece_index, int new_row, int new_col)
{
    if (piece_index < 0 || piece_index >= nb_pieces)
        return 0;

    if (new_row < 0 || new_row >= ROWS || new_col < 0 || new_col >= COLS)
        return 0;

    Piece *piece = &pieces[piece_index];

    if (piece->row < 0)
        return 0;

    if (piece->color != current_player)
        return 0;

    int dr = new_row - piece->row;
    int dc = new_col - piece->col;

    if (dr == 0 && dc == 0)
        return 0;

    /* PION */
    if (piece->type == PION) {
        if (dr != 0 && dc != 0)
            return 0;
    }
    /* ROI : meme regle que le programme original */
    else if (piece->type == ROI) {
        if (dr != 0 && dc != 0)
            return 0;
    }

    if (!chemin_libre(piece, new_row, new_col))
        return 0;

    int destination = jeu_piece_at(new_row, new_col);

    if (destination == -1)
        return 1;

    /* piece alliee, on ne peut pas se poser dessus */
    if (pieces[destination].color == piece->color)
        return 0;

    /* piece ennemie, capture autorisee */
    return 1;
}

int jeu_deplacer(int piece_index, int new_row, int new_col)
{
    if (!jeu_peut_deplacer(piece_index, new_row, new_col))
        return 0;

    Piece *piece = &pieces[piece_index];

    int destination = jeu_piece_at(new_row, new_col);

    /* capture */
    if (destination != -1) {
        pieces[destination].row = -1;
        pieces[destination].col = -1;
    }

    /* deplacement */
    piece->row = new_row;
    piece->col = new_col;
    conquises[new_row][new_col] = piece->color;
    current_player = 1 - current_player;

    return 1;
}

/* donne un code pour savoir ce qu'il y a sur une case :
   0 = vide et jamais visitee
   1 = vide mais visitee par bleu
  -1 = vide mais visitee par rouge
   2 = pion bleu
  -2 = pion rouge
   3 = roi bleu
  -3 = roi rouge */
int jeu_valeur_case(int row, int col)
{
    int index = jeu_piece_at(row, col);

    if (index != -1) {
        Piece *p = &pieces[index];
        int val = (p->type == ROI) ? 3 : 2;

        return (p->color == BLEU) ? val : -val;
    }

    if (conquises[row][col] == BLEU)
        return 1;

    if (conquises[row][col] == ROUGE)
        return -1;

    return 0;
}
