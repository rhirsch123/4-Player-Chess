#ifndef position_hh
#define position_hh

#include <vector>
#include <string>
#include <sstream>
#include <cmath>

#include "types.h"
#include "movegen.h"
#include "move.h"


struct PositionState {
    // 14x14 board, indexed top to bottom, left to right, from red's perspective. Corners are unused
    uint8_t board[196];

    uint32_t castle_rights;
    int king_squares[4];
    int material[4];
    int ep_square;
    int fifty_move_count;
};


class Position {
public:
    std::vector<PositionState> stack;
    PositionState* state;
    int half_moves;
    int turn;

    void set_fen(std::string fen);

    Position(std::string fen = "");

    int piece_on(int square) {
        return state->board[square];
    }

    int king_square(int color) {
        return state->king_squares[color];
    }

    int material(int color) {
        return state->material[color];
    }

    void make_move(Move move);
    void pop();
};

#endif