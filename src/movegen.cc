#include "movegen.h"
#include "position.h"
#include "types.h"

bool is_occupied(uint8_t square) {
    return square != EMPTY_SQUARE && square != OOB_SQUARE;
}

bool has_pawn_moved(Square pawn_square, Color color) {
    switch (color) {
        case RED:
            return pawn_square > 12 * ROW_SIZE;
        case YELLOW:
            return pawn_square < 2 * ROW_SIZE;
        // TODO: module by 14 not great
        case BLUE:
            return pawn_square % ROW_SIZE == 1;
        case GREEN:
            return pawn_square % ROW_SIZE == 12;
    }
}

bool is_pawn_promotion_square(Square square, Color color) {
    switch (color) {
        case RED:
            return square < ROW_SIZE;
        case YELLOW:
            return square > 13 * ROW_SIZE;
        // TODO: module by 14 not great
        case BLUE:
            return square % ROW_SIZE == 13;
        case GREEN:
            return square % ROW_SIZE == 0;
    }
}

int DIAGANOL_DIRECTIONS[] = {-ROW_SIZE - 1, -ROW_SIZE + 1, ROW_SIZE - 1, ROW_SIZE + 1};
int ROW_DIRECTIONS[] = {ROW_SIZE, -ROW_SIZE, 1, -1};

// order of index is Red, Blue, Yellow, Green
// this indexes based on color to get pawn forward directions
int PAWN_FORWARD_DIRECTIONS[] = {-ROW_SIZE, 1, ROW_SIZE, -1};
// each 2 indexes are direction for a color
int PAWN_ATTACK_DIRECTIONS[] = {
    // red
    -ROW_SIZE - 1, -ROW_SIZE + 1,
    // blue
    -ROW_SIZE + 1, ROW_SIZE + 1,
    // yellow
    ROW_SIZE - 1, ROW_SIZE + 1,
    // green
    -ROW_SIZE - 1, ROW_SIZE - 1,
};

// 8 L-shaped jumps: 2 rows + 1 col, or 1 row + 2 cols
int KNIGHT_DIRECTIONS[] = {
    -2 * ROW_SIZE - 1, -2 * ROW_SIZE + 1,
    -ROW_SIZE - 2,     -ROW_SIZE + 2,
     ROW_SIZE - 2,      ROW_SIZE + 2,
     2 * ROW_SIZE - 1,  2 * ROW_SIZE + 1,
};

// adds a pawn move, with promotion variants if needed
void add_pawn_moves(Square start, Square end, Color color, MoveList *move_list) {
    if (is_pawn_promotion_square(end, color)) {
        move_list->add(Move(start, end, KNIGHT));
        move_list->add(Move(start, end, BISHOP));
        move_list->add(Move(start, end, ROOK));
        move_list->add(Move(start, end, QUEEN));
    } else {
        move_list->add(Move(start, end));
    }
}

void add_moves_in_direction(Square start, int direction, Color color, PositionState *state, MoveList *list) {
    int move_amount = 1;
    for (;;) {
        uint8_t target = state->piece_at_offset(start, move_amount * direction);
        uint8_t target_square = start + move_amount * direction;
        if (target == OOB_SQUARE) {
            break;
        }

        if (target == EMPTY_SQUARE) {
            list->add(Move(start, target_square));
        } else {
            // break on hitting occupied square, can take it if it is enemy piece
            if (get_team(get_color(target)) != color) {
                list->add(Move(start, target_square));
            }
            break;
        }
    }
}

// checks if a square is under attack by a team
bool is_square_under_attack(PositionState *state, int team, Square square) {
    // TODO
    return false;
}

void add_king_move_direction(Square king_start_square, int direction, Color color, PositionState *state, MoveList *list) {
    if (state->piece_at_offset(king_start_square, direction) != OOB_SQUARE && !is_square_under_attack(state, other_team(get_team(color)), king_start_square + direction)) {
        list->add(Move(king_start_square, king_start_square + direction));
    }
}

void get_legal_moves(Position& position, MoveList* move_list) {
    // stupid board repr and algo to make moves to start, using existing position definition
    // TODO: fancy simd bitboards
    //
    // TODO: handle 50 move rule and repetition rule (idk if these are even things in 4 player chess)

    PositionState *state = position.state;

    for (size_t current_square = 0; current_square < BOARD_SIZE; current_square++) {
        uint8_t piece = state->board[current_square];
        Color piece_color = (Color) get_color(piece);
        int piece_team = get_team(piece_color);
        PieceType piece_type = (PieceType) get_piece_type(piece);

        if (!is_occupied(piece) || piece_color != position.player_to_move) {
            continue;
        }

        if (piece_type == PAWN) {
            int direction = PAWN_FORWARD_DIRECTIONS[piece_color];

            // check 1 forward move to empty square
            if (state->piece_at_offset(current_square, direction) == EMPTY_SQUARE) {
                add_pawn_moves(current_square, current_square + direction, piece_color, move_list);

                // check 2 forward moves if pawn hasn't moved, and 1 forward move was empty square
                if (!has_pawn_moved(current_square, piece_color) && state->piece_at_offset(current_square, 2 * direction) == EMPTY_SQUARE) {
                    add_pawn_moves(current_square, current_square + 2 * direction, piece_color, move_list);
                }
            }

            // check diagonal attacks
            for (size_t i = 2 * piece_color; i < (2 * piece_color + 2); i++) {
                uint8_t target_piece = state->piece_at_offset(current_square, PAWN_ATTACK_DIRECTIONS[i]);
                Square target_square = current_square + PAWN_ATTACK_DIRECTIONS[i];
                if (is_occupied(target_piece) && get_team(get_color(target_piece)) != piece_team) {
                    add_pawn_moves(current_square, target_square, piece_color, move_list);
                }

                // check en passant squares for opposite team
                for (int j = 0; j < 4; j++) {
                    if (get_team(j) != piece_team && state->en_passant_squares[j] == target_square) {
                        add_pawn_moves(current_square, target_square, piece_color, move_list);
                    }
                }
            }
        }

        // handle horizantal/vertical moving pieces
        if (piece_type == ROOK || piece_type == QUEEN) {
            for (size_t i = 0; i < 4; i++) {
                add_moves_in_direction(current_square, ROW_DIRECTIONS[i], piece_color, state, move_list);
            }
        }

        // handle diagonally moving pieces
        if (piece_type == BISHOP || piece_type == QUEEN) {
            for (size_t i = 0; i < 4; i++) {
                add_moves_in_direction(current_square, DIAGANOL_DIRECTIONS[i], piece_color, state, move_list);
            }
        }

        if (piece_type == KNIGHT) {
            for (size_t i = 0; i < 8; i++) {
                if (state->piece_at_offset(current_square, KNIGHT_DIRECTIONS[i]) != OOB_SQUARE) {
                    move_list->add(Move(current_square, current_square + KNIGHT_DIRECTIONS[i]));
                }
            }
        }

        if (piece_type == KING) {
            for (size_t i = 0; i < 4; i++) {
                add_king_move_direction(current_square, ROW_DIRECTIONS[i], piece_color, state, move_list);
                add_king_move_direction(current_square, DIAGANOL_DIRECTIONS[i], piece_color, state, move_list);
            }
        }
    }

    for (size_t castle_type = 0; castle_type < 2; castle_type++) {
        if (state->can_castle(position.player_to_move, castle_type)) {
            // TODO: castling
        }
    }
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
