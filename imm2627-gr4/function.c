#include <stdio.h>

typedef struct
{
    int row;
    int col;
} Position;

typedef struct
{
    int row;
    int col;
} Move;

typedef struct
{
    Move moves[16];
    int count;
} MoveSet;


int generate_pawn_moves(
    int board[7][11],
    Position pawn,
    Move moves[16])
    //MoveSet avail_moves
{
    int move_count = 0;

    for (int i = 1; ; i++) {
        if (pawn.row + i < 7, -1 <= board[pawn.row + i][pawn.col] <= 1) {
            moves[move_count].col = pawn.col;
            moves[move_count].row = pawn.row + i;
            move_count++;
        } 
        else break
    }
    for (int i = 1; ; i++) {
        if (pawn.col + i <  11, -1 <= board[pawn.row][pawn.col + i] <= 1) {
            moves[move_count].col = pawn.col + i;
            moves[move_count].row = pawn.row;
            move_count++;
        }
        else {
            break
        }
    }
    for (int i = 1; ; i++) {
        if (pawn.row - i > 0, -1 <= board[pawn.row - i][pawn.col] <= 1) {
            moves[move_count].col = pawn.col;
            moves[move_count].row = pawn.row - i;
            move_count++;
        }
        else {
            break
        }
    }
    for (int i = 1; ; i++) {
        if (pawn.col - i > 0, -1 <= board[pawn.row][pawn.col - i] <= 1) {
            moves[move_count].col = pawn.col - i;
            moves[move_count].row = pawn.row;
            move_count++;
        }
        else {
            break
        }
    }

}






int legal_moves(int table[][], int row, int col) {

    int moves_available[16][2] = {};

    for (int i = lin + 1; i < 7; i++) {

        if (i > 7) break;

        else {

            
        }
    }
    return 0;
}
