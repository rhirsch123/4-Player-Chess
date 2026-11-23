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

int Engine::negamax(Position& position, int remaining_depth, int current_depth, int alpha, int beta, int root_team) {
    if (remaining_depth == 0) {
        int score = evaluation(position);
        return get_team(position.turn) == root_team ? score : -score;
    }

    MoveList moves;
    get_legal_moves(position, &moves);

    if (moves.size == 0) {
        int score = evaluation(position);
        return get_team(position.turn) == root_team ? score : -score;
    }

    bool maximizing = get_team(position.turn) == root_team;

    if (maximizing) {
        int best_score = -INF;

        for (Move move : moves) {
            make_move(position, move, current_depth);
            int score = negamax(position, remaining_depth - 1, current_depth + 1, alpha, beta, root_team);
            unmake_move(position);

            if (score > best_score) {
                best_score = score;
            }

            if (best_score > alpha) {
                alpha = best_score;
            }

            if (alpha >= beta) {
                break;
            }
        }

        return best_score;
    }

    int best_score = INF;

    for (Move move : moves) {
        make_move(position, move, current_depth);
        int score = negamax(position, remaining_depth - 1, current_depth + 1, alpha, beta, root_team);
        unmake_move(position);

        if (score < best_score) {
            best_score = score;
        }

        if (best_score < beta) {
            beta = best_score;
        }

        if (alpha >= beta) {
            break;
        }
    }

    return best_score;
}

Move Engine::get_move(Position& position, SearchInfo info) {
    MoveList moves;
    get_legal_moves(position, &moves);

    if (moves.size == 0) {
        return Move();
    }

    int depth = info.fixed_depth ? info.depth : 3;
    if (depth < 1) {
        depth = 1;
    }

    if (depth > MAX_DEPTH) {
        depth = MAX_DEPTH;
    }

    total_nodes = 0;
    int root_team = get_team(position.turn);
    Move best_move = moves.moves[0];
    int best_score = -INF;

    for (Move move : moves) {
        make_move(position, move, 0);
        int score = negamax(position, depth - 1, 1, -INF, INF, root_team);
        unmake_move(position);

        if (score > best_score) {
            best_score = score;
            best_move = move;
        }
    }

    return best_move;
}

void Engine::init() {
    total_nodes = 0;
}