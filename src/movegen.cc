#include "movegen.h"


inline void add_legal_move(Position& position, MoveList* move_list, int from, int to, int promotion = 0) {
    Move move(from, to, promotion);

    int color = position.turn;
    position.make_move(move);
    bool legal = !position.in_check(color);
    position.pop();

    if (legal) {
        move_list->add(move);
    }
}

inline void add_pawn_move(Position& position, MoveList* move_list, int from, int to) {
    const int row = to / 14;
    const int col = to % 14;
    const int turn = position.turn;
    bool promo = (turn == RED && row == 3) ||
                 (turn == BLUE && col == 10) ||
                 (turn == YELLOW && row == 10) ||
                 (turn == GREEN && col == 3);
    if (promo) {
        for (int p = KNIGHT; p <= QUEEN; p++) {
            add_legal_move(position, move_list, from, to, p);
        }
    } else {
        add_legal_move(position, move_list, from, to);
    }
}


void get_legal_moves(Position& position, MoveList* move_list) {
    move_list->size = 0;

    const int turn = position.turn;
    const int team = get_team(turn);

    for (int from = 0; from < 196; from++) {
        int piece = position.piece_on(from);
        if (!piece || get_color(piece) != turn) {
            continue;
        }

        int piece_type = get_piece_type(piece);
        int row = from / 14;
        int col = from % 14;

        if (piece_type == PAWN) {
            // single push
            int push_row = row + row_dirs[turn];
            int push_col = col + col_dirs[turn];
            bool can_push = valid_square(push_row, push_col) && !position.piece_on(get_square(push_row, push_col));
            if (can_push) {
                add_pawn_move(position, move_list, from, get_square(push_row, push_col));
            }

            // double push
            bool starting = (turn == RED && row == 12) ||
                            (turn == BLUE && col == 1) ||
                            (turn == YELLOW && row == 1) ||
                            (turn == GREEN && col == 12);
            int double_row = push_row + row_dirs[turn];
            int double_col = push_col + col_dirs[turn];
            if (starting && can_push && valid_square(double_row, double_col) &&
                !position.piece_on(get_square(double_row, double_col))) {
                add_pawn_move(position, move_list, from, get_square(double_row, double_col));
            }

            // captures
            for (int side = -1; side <= 1; side += 2) {
                int target_row = push_row + (row_dirs[turn] == 0 ? side : 0);
                int target_col = push_col + (col_dirs[turn] == 0 ? side : 0);
                if (!valid_square(target_row, target_col)) {
                    continue;
                }

                int to = get_square(target_row, target_col);
                int target_piece = position.piece_on(to);
                if (target_piece && get_team(get_color(target_piece)) != team) {
                    add_pawn_move(position, move_list, from, to);
                }
            }

        } else {
            // normal piece moves
            for (int offset : move_offsets[piece_type]) {
                if (!offset) {
                    break;
                }

                int prev = from;
                int to = from + offset;
                while (valid_step(prev, to)) {
                    int target = position.piece_on(to);
                    if (target && get_team(get_color(target)) == team) {
                        break;
                    }

                    add_legal_move(position, move_list, from, to);

                    if (target || piece_type == KNIGHT || piece_type == KING) {
                        break;
                    }

                    prev = to;
                    to += offset;
                }
            }

        }
        
        // castle
        if (piece_type == KING && !position.in_check(turn)) {
            for (int side = 0; side < 2; side++) {
                if (!(position.state->castle_rights & castle_by_color[turn][side])) {
                    continue;
                }

                int direction = side == 0 ? castle_step[turn] : -castle_step[turn];

                bool clear = true;
                for (int i = 1; i < (side == 0 ? 3 : 4); i++) {
                    if (position.piece_on(from + i * direction)) {
                        clear = false;
                        break;
                    }
                }
                if (!clear) continue;

                // don't castle through check
                position.make_move(Move(from, from + direction));
                bool safe = !position.in_check(turn);
                position.pop();

                if (safe) {
                    add_legal_move(position, move_list, from, from + 2 * direction);
                }
            }
        }
    }
}


uint64_t perft(Position& position, int depth) {
    if (depth == 0) {
        return 1;
    }

    MoveList moves;
    get_legal_moves(position, &moves);

    if (depth == 1) {
        return moves.size;
    }

    uint64_t nodes = 0;
    for (Move move : moves) {
        position.make_move(move);
        nodes += perft(position, depth - 1);
        position.pop();
    }

    return nodes;
}