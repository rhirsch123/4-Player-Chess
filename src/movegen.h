#ifndef movegen_hh
#define movegen_hh

#include "position.h"
#include "move.h"

class Position;

struct MoveList {
    Move moves[MAX_MOVES];
    int size;

    MoveList() : size(0) {};

    inline void add(Move move) {
        moves[size++] = move;
    }

    inline Move* begin() {
        return moves;
    }

    inline Move* end() {
        return moves + size;
    }
};

void get_legal_moves(Position& position, MoveList* move_list);
uint64_t perft(Position& position, int depth);

#endif