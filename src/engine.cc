#include "engine.h"

Engine::Engine() {
    init();
}

int Engine::evaluation(Position& position) {
    // TODO: better eval
    int turn = position.turn;
    int our_material = position.material(turn) + position.material((turn + 2) % 4);
    int their_material = position.material((turn + 1) % 4) + position.material((turn + 3) % 4);
    return our_material - their_material;
}

void Engine::make_move(Position& position, Move move, int ply) {
    total_nodes++;
    position.make_move(move);
}

void Engine::unmake_move(Position& position) {
    position.pop();
}


int Engine::negamax(Position& position, int remaining_depth, int current_depth, int alpha, int beta) {
    // TODO: negamax search with alpha beta pruning     
}


Move Engine::get_move(Position& position, SearchInfo info) {
    // TODO: iterative deepening
}


void Engine::init() {
    // TODO
}