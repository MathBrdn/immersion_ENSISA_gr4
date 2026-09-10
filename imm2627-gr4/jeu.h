#ifndef JEU_H
#define JEU_H

#define ROWS 7
#define COLS 11

#define PION 0
#define ROI  1

#define BLEU  0
#define ROUGE 1

typedef struct {
    int row;
    int col;
    const char *symbol;
    int color;
    int type;
} Piece;

void jeu_initialiser(void);
int jeu_nombre_pieces(void);
Piece *jeu_piece(int index);
int jeu_piece_at(int row, int col);
int jeu_joueur_actuel(void);
int jeu_peut_deplacer(int piece_index, int new_row, int new_col);
int jeu_deplacer(int piece_index, int new_row, int new_col);
int jeu_case_jaune(int row, int col);
int jeu_valeur_case(int row, int col);

#endif
