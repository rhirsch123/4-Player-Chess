#include "engine.h"
#include <cmath>

Engine::Engine() {
    init();
}

float Engine::evaluation_base(Position& position) {
    // TODO: better eval
    int turn = position.turn;
    int our_material = position.material(turn) + position.material((turn + 2) % 4);
    int their_material = position.material((turn + 1) % 4) + position.material((turn + 3) % 4);
    return (our_material - their_material) * 1.0;
}

float Engine::evaluation_v1(Position& position) {
    // Mobility + material
    int turn = position.turn;
    int our_material = position.material(turn) + position.material((turn + 2) % 4);
    int their_material = position.material((turn + 1) % 4) + position.material((turn + 3) % 4);

    int legal_move_score = 0;
    for (int i = 0; i < 4; i++) {
        position.turn = (turn + i) % 4;
        MoveList legal_moves;
        get_legal_moves(position, &legal_moves);
        if (position.turn == turn || position.turn == ((turn + 2) % 4)) {
            legal_move_score += legal_moves.size;
        }
        else {
            legal_move_score -= legal_moves.size;
        }
    }
    position.turn = turn; 

    float w_material = 0.05; // TODO tune weights
    float w_legal_moves = 1.0;

    return w_material * (our_material - their_material) + w_legal_moves * legal_move_score;
}

float Engine::evaluation_v2(Position& position) {
    // v1 + king safety/castle + pawn structure + stronger mobility (open diagonals/files get higher weight)

    int turn = position.turn;
    int our_material = position.material(turn) + position.material((turn + 2) % 4);
    int their_material = position.material((turn + 1) % 4) + position.material((turn + 3) % 4);

    int strong_diag_threshold = 9; 
    int strong_line_threshold = 9;

    int legal_move_score = 0;
    for (int i = 0; i < 4; i++) {
        position.turn = (turn + i) % 4;
        MoveList legal_moves;
        get_legal_moves(position, &legal_moves);
        int w = (position.turn == turn || position.turn == ((turn + 2) % 4)) ? 1 : -1;
        for (int m = 0; m < legal_moves.size; m++) {
            int piece_type =  get_piece_type(position.state->board[legal_moves.moves[m].from()]);
            int row_from = legal_moves.moves[m].from() / 14, col_from = legal_moves.moves[m].from() % 14;
            int row_to = legal_moves.moves[m].to() / 14, col_to = legal_moves.moves[m].to() % 14; 

            if (((std::abs(row_from - row_to) > strong_line_threshold || std::abs(col_from - col_to) > strong_line_threshold) && (piece_type == ROOK || piece_type == QUEEN)) &&
                ((std::abs(row_from - row_to) > strong_diag_threshold || std::abs(col_from - col_to) > strong_diag_threshold) && (piece_type == BISHOP || piece_type == QUEEN))) {
                legal_move_score += (w * 2);
            } else {
                legal_move_score += w;
            }
        }
    }
    position.turn = turn; 

    // Castle
    int castle_score = 0;
    if (position.state->castle_rights & castle_by_color[turn][0]) castle_score++;
    if (position.state->castle_rights & castle_by_color[turn][1]) castle_score++;
    if (position.state->castle_rights & castle_by_color[(turn + 2) % 4][0]) castle_score++;
    if (position.state->castle_rights & castle_by_color[(turn + 2) % 4][1]) castle_score++;

    // Pawn Structure - Isolated pawns, double pawns
    int pawn_count[14] = {0};
    for (int r = 0; r < 14; r++) {
        for (int c = 0; c < 14; c++) {
            if (valid_square(r, c) && get_piece_type(position.state->board[r * 14 + c]) == PAWN) {
                if (turn == RED || turn == YELLOW) pawn_count[c]++; else pawn_count[r]++;
            }
        }
    }
    
    int pawn_penalty = 0;
    for (int r = 0; r < 14; r++) {
        for (int c = 0; c < 14; c++) {
            if (valid_square(r,c) && get_piece_type(position.state->board[r * 14 + c]) == PAWN && 
                (get_color(position.state->board[r * 14 + c]) == turn || get_color(position.state->board[r * 14 + c]) == ((turn + 2) % 4))) {
                if (turn == RED || turn == YELLOW) {
                    if (pawn_count[c - 1] == 0 && pawn_count[c + 1] == 0) pawn_penalty++;
                    if (pawn_count[c] > 1) pawn_penalty++;
                } else {
                    if (pawn_count[r - 1] == 0 && pawn_count[r + 1] == 0) pawn_penalty++;
                    if (pawn_count[r] > 1) pawn_penalty++;
                }
            }
        }
    }


    // King Safety - Virtual Mobility
    int king_safety = 0;
    position.state->board[position.king_square(turn)]--; // king -> queen
    position.state->board[position.king_square((turn + 2) % 4)]--;

    int k1 = position.king_square(RED), k2 = position.king_square(YELLOW);

    MoveList moves;
    position.turn = turn;
    get_legal_moves(position, &moves);
    for (int m = 0; m < moves.size; m++) {
        if (moves.moves[m].from() == k1) king_safety++;
    }

    position.turn = (turn + 2) % 4;
    get_legal_moves(position, &moves);
    for (int m = 0; m < moves.size; m++) {
        if (moves.moves[m].from() == k2) king_safety++;
    }

    position.state->board[position.king_square(turn)]++;
    position.state->board[position.king_square((turn + 2) % 4)]++;
    position.turn = turn;

    float w_material = 0.05; // TODO tune weights
    float  w_legal_moves = 1.0;
    float w_castle = 0.5;
    float w_pawn = 0.35;
    float w_king = 0.75; 

    int res = w_material * (our_material - their_material) + w_legal_moves * legal_move_score + 
              w_castle * castle_score - w_pawn * pawn_penalty - w_king * king_safety;

    return res;
}

float Engine::evaluation_v3(Position& position) {
    // v2 + game phase dependent scoring (gating by material)

    float w_material = 0.5;
    float  w_legal_moves = 1.0;
    float w_castle = 0.5;
    float w_pawn = 0.35;
    float w_king = 0.75; 

    int material_threshold = 6000; 
    int curr_material = 0;
    for (int i = 0; i < 4; i++) {
        curr_material += position.material(i);
    }
    if (curr_material < material_threshold) {
        // update weights for the endgame
        w_material = 1.0;
        w_legal_moves = 0.25;
        w_castle = 0.0;
        w_pawn = 0.15;
        w_king = 1.0;
    }

    int turn = position.turn;
    int our_material = position.material(turn) + position.material((turn + 2) % 4);
    int their_material = position.material((turn + 1) % 4) + position.material((turn + 3) % 4);

    int strong_diag_threshold = 9; 
    int strong_line_threshold = 9;

    int legal_move_score = 0;
    for (int i = 0; i < 4; i++) {
        position.turn = (turn + i) % 4;
        MoveList legal_moves;
        get_legal_moves(position, &legal_moves);
        int w = (position.turn == turn || position.turn == ((turn + 2) % 4)) ? 1 : -1;
        for (int m = 0; m < legal_moves.size; m++) {
            int piece_type =  get_piece_type(position.state->board[legal_moves.moves[m].from()]);
            int row_from = legal_moves.moves[m].from() / 14, col_from = legal_moves.moves[m].from() % 14;
            int row_to = legal_moves.moves[m].to() / 14, col_to = legal_moves.moves[m].to() % 14; 

            if (((std::abs(row_from - row_to) > strong_line_threshold || std::abs(col_from - col_to) > strong_line_threshold) && (piece_type == ROOK || piece_type == QUEEN)) &&
                ((std::abs(row_from - row_to) > strong_diag_threshold || std::abs(col_from - col_to) > strong_diag_threshold) && (piece_type == BISHOP || piece_type == QUEEN))) {
                legal_move_score += (w * 2);
            } else {
                legal_move_score += w;
            }
        }
    }
    position.turn = turn; 

    // Castle
    int castle_score = 0;
    if (position.state->castle_rights & castle_by_color[turn][0]) castle_score++;
    if (position.state->castle_rights & castle_by_color[turn][1]) castle_score++;
    if (position.state->castle_rights & castle_by_color[(turn + 2) % 4][0]) castle_score++;
    if (position.state->castle_rights & castle_by_color[(turn + 2) % 4][1]) castle_score++;

    // Pawn Structure - Isolated pawns, double pawns
    int pawn_count[14] = {0};
    for (int r = 0; r < 14; r++) {
        for (int c = 0; c < 14; c++) {
            if (valid_square(r, c) && get_piece_type(position.state->board[r * 14 + c]) == PAWN) {
                if (turn == RED || turn == YELLOW) pawn_count[c]++; else pawn_count[r]++;
            }
        }
    }
    
    int pawn_penalty = 0;
    for (int r = 0; r < 14; r++) {
        for (int c = 0; c < 14; c++) {
            if (valid_square(r,c) && get_piece_type(position.state->board[r * 14 + c]) == PAWN && 
                (get_color(position.state->board[r * 14 + c]) == turn || get_color(position.state->board[r * 14 + c]) == ((turn + 2) % 4))) {
                if (turn == RED || turn == YELLOW) {
                    if (pawn_count[c - 1] == 0 && pawn_count[c + 1] == 0) pawn_penalty++;
                    if (pawn_count[c] > 1) pawn_penalty++;
                } else {
                    if (pawn_count[r - 1] == 0 && pawn_count[r + 1] == 0) pawn_penalty++;
                    if (pawn_count[r] > 1) pawn_penalty++;
                }
            }
        }
    }


    // King Safety - Virtual Mobility
    int king_safety = 0;
    position.state->board[position.king_square(turn)]--; // king -> queen
    position.state->board[position.king_square((turn + 2) % 4)]--;

    int k1 = position.king_square(RED), k2 = position.king_square(YELLOW);

    MoveList moves;
    position.turn = turn;
    get_legal_moves(position, &moves);
    for (int m = 0; m < moves.size; m++) {
        if (moves.moves[m].from() == k1) king_safety++;
    }

    position.turn = (turn + 2) % 4;
    get_legal_moves(position, &moves);
    for (int m = 0; m < moves.size; m++) {
        if (moves.moves[m].from() == k2) king_safety++;
    }

    position.state->board[position.king_square(turn)]++;
    position.state->board[position.king_square((turn + 2) % 4)]++;
    position.turn = turn;

    int res = w_material * (our_material - their_material) + w_legal_moves * legal_move_score + 
              w_castle * castle_score - w_pawn * pawn_penalty - w_king * king_safety;

    return res;
    
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