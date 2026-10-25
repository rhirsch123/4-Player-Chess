#include "movegen.h"


void get_legal_moves(Position& position, MoveList* move_list) {
    // TODO
}


uint64_t perft(Position& position, int depth) {
    uint64_t nodes = 0;
    
    MoveList moves;
    get_legal_moves(position, &moves);

    if (position.half_moves == depth - 1) {
        return moves.size;
    }

    for (Move move : moves) {
        position.make_move(move);
        nodes += perft(position, depth);
        position.pop();
    }

    return nodes;
}