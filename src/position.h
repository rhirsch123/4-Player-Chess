#ifndef position_hh
#define position_hh

#include <vector>
#include <string>
#include <sstream>
#include <cmath>

#include "types.h"
#include "movegen.h"
#include "move.h"

#define BOARD_SIZE 196
#define EMPTY_SQUARE 0
#define OOB_SQUARE 255
#define ROW_SIZE 14

struct PositionState {
    // 14x14 board, indexed top to bottom, left to right, from red's perspective. Corners are unused
    // TODO: set corners to OOB_SQUARE
    uint8_t board[BOARD_SIZE];

    // bitmask of castle rights, 2 bits per player, 1 bit each for queenside and kingside
    uint8_t castle_rights;
    // en passant squares of each color, SQUARE_NONE indicates no current en passant square
    Square en_passant_squares[4];
    uint8_t fifty_move_count;

    // this stuff isn't strictly state, investigate if worth it
    Square king_squares[4];
    int material[4];

    uint8_t piece_at_offset(Square orig_square, int offset) {
        if (-offset > orig_square || (orig_square + offset) >= BOARD_SIZE) {
            return OOB_SQUARE;
        }

        return this->board[orig_square + offset];
    }

    bool can_castle(Color color, bool queenside) {
        return (castle_rights & (1 << (2 * color + int(queenside)))) != 0;
    }
};


class Position {
public:
    std::vector<PositionState> stack;
    PositionState* state;
    // total number of plies, quarter move really
    int half_moves;
    Color player_to_move;

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
