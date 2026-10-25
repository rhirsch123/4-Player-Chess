#ifndef engine_hh
#define engine_hh

#include "move.h"
#include "position.h"
#include "types.h"

class Engine {
public:
    Engine();
    void init();

    int evaluation(Position& position);
    void make_move(Position& position, Move move, int ply);
    void unmake_move(Position& position);

    int negamax(Position& position, int remaining_depth, int current_depth, int alpha, int beta);
    Move get_move(Position& position, SearchInfo info = SearchInfo());

    int total_nodes = 0;
};

#endif