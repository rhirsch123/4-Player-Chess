#include "position.h"

void Position::set_fen(std::string fen) {
    if (fen.empty() || fen == "startpos") {
        fen =
            "R-0,0,0,0-1,1,1,1-1,1,1,1-0,0,0,0-0-"
            "3,yR,yN,yB,yK,yQ,yB,yN,yR,3/"
            "3,yP,yP,yP,yP,yP,yP,yP,yP,3/"
            "14/"
            "bR,bP,10,gP,gR/"
            "bN,bP,10,gP,gN/"
            "bB,bP,10,gP,gB/"
            "bK,bP,10,gP,gQ/"
            "bQ,bP,10,gP,gK/"
            "bB,bP,10,gP,gB/"
            "bN,bP,10,gP,gN/"
            "bR,bP,10,gP,gR/"
            "14/"
            "3,rP,rP,rP,rP,rP,rP,rP,rP,3/"
            "3,rR,rN,rB,rQ,rK,rB,rN,rR,3";
    }

    stack.resize(512);
    half_moves = 0;
    state = &stack[half_moves];

    // FEN4 format used by chess.com that encodes a position:
    // turn-eliminated-kingside_castle-queenside_castle-points-50mr-board

    std::string fields[7];
    std::istringstream fen_stream(fen);

    for (int i = 0; i < 7; i++) {
        if (!std::getline(fen_stream, fields[i], '-')) {
            throw std::invalid_argument("Invalid FEN4");
        }
    }

    auto parse_ints = [](const std::string& str, int* values) {
        std::istringstream ss(str);
        std::string token;

        for (int i = 0; i < 4; i++) {
            if (!std::getline(ss, token, ',')) {
                throw std::invalid_argument("Invalid FEN4");
            }
            values[i] = std::stoi(token);
        }
    };

    int eliminated[4]; // not used
    int kingside[4];
    int queenside[4];
    int points[4]; // not used

    parse_ints(fields[1], eliminated);
    parse_ints(fields[2], kingside);
    parse_ints(fields[3], queenside);
    parse_ints(fields[4], points);

    state->fifty_move_count = std::stoi(fields[5]);

    std::memset(state->board, 0, sizeof(state->board));
    std::memset(state->material, 0, sizeof(state->material));
    state->castle_rights = 0;

    switch (fields[0][0]) {
        case 'R': turn = RED;    break;
        case 'B': turn = BLUE;   break;
        case 'Y': turn = YELLOW; break;
        case 'G': turn = GREEN;  break;
        default:
            throw std::invalid_argument("Invalid FEN4");
    }

    // parse board
    std::istringstream board_stream(fields[6]);
    std::string rank_str;
    for (int row = 0; row < 14; row++) {
        if (!std::getline(board_stream, rank_str, '/')) {
            throw std::invalid_argument("Invalid FEN4");
        }

        std::istringstream rank_stream(rank_str);
        std::string token;
        int col = 0;

        while (std::getline(rank_stream, token, ',')) {
            // empty squares
            if (std::isdigit(static_cast<unsigned char>(token[0]))) {
                col += std::stoi(token);
                continue;
            }

            // corner square
            if (token == "x") {
                col++;
                continue;
            }

            int color;
            switch (token[0]) {
                case 'r': color = RED;    break;
                case 'b': color = BLUE;   break;
                case 'y': color = YELLOW; break;
                case 'g': color = GREEN;  break;
                default:
                    throw std::invalid_argument("Invalid FEN4");
            }

            int type;
            switch (token[1]) {
                case 'P': type = PAWN;   break;
                case 'N': type = KNIGHT; break;
                case 'B': type = BISHOP; break;
                case 'R': type = ROOK;   break;
                case 'Q': type = QUEEN;  break;
                case 'K': type = KING;   break;
                default:
                    throw std::invalid_argument("Invalid FEN4");
            }

            int piece = color * 6 + type + 1;
            int square = get_square(row, col);

            if (type == KING) {
                state->king_squares[color] = square;
            }

            state->board[square] = piece;
            state->material[color] += piece_values[type];

            col++;
        }
    }

    for (int color = RED; color <= GREEN; color++) {
        if (eliminated[color]) {
            continue;
        }

        if (kingside[color]) {
            state->castle_rights |= castle_by_color[color][0];
        }

        if (queenside[color]) {
            state->castle_rights |= castle_by_color[color][1];
        }
    }
}

Position::Position(std::string fen) {
    set_fen(fen);
}


bool Position::attacked_by(int square, int team) {
    int row = square / 14;
    int col = square % 14;

    for (int color = RED; color <= GREEN; color++) {
        if (get_team(color) != team) {
            continue;
        }

        for (int side = -1; side <= 1; side += 2) {
            int from_row = row - row_dirs[color] + (row_dirs[color] == 0 ? side : 0);
            int from_col = col - col_dirs[color] + (col_dirs[color] == 0 ? side : 0);
            if (valid_square(from_row, from_col) && state->board[get_square(from_row, from_col)] == color * 6 + PAWN + 1) {
                return true;
            }
        }
    }

    for (int type = KNIGHT; type <= ROOK; type++) {
        for (int offset : move_offsets[type]) {
            if (!offset) {
                break;
            }

            int prev = square;
            int cur = square + offset;
            while (valid_step(prev, cur)) {
                int piece = state->board[cur];
                if (piece) {
                    if (get_team(get_color(piece)) == team) {
                        int attacker_type = get_piece_type(piece);
                        if (attacker_type == type ||
                            (attacker_type == QUEEN && type != KNIGHT) ||
                            (attacker_type == KING && cur == square + offset && type != KNIGHT)) {
                            return true;
                        }
                    }
                    break;
                }

                if (type == KNIGHT) {
                    break;
                }

                prev = cur;
                cur += offset;
            }
        }
    }

    return false;
}

bool Position::in_check(int color) {
    return attacked_by(state->king_squares[color], !get_team(color));
}

bool Position::in_check() {
    return in_check(turn);
}


void Position::make_move(Move move) {
    half_moves++;
    if (half_moves >= stack.size()) {
        stack.resize(stack.size() * 2);
    }
    std::memcpy(&stack[half_moves], &stack[half_moves - 1], sizeof(PositionState));
    state = &stack[half_moves];

    const int from = move.from();
    const int to = move.to();
    const int piece = state->board[from];
    const int color = get_color(piece);
    const int piece_type = get_piece_type(piece);

    static constexpr int king_home[4] = { 189, 84, 6, 111 };

    int captured = state->board[to];
    int capture_square = to;

    if (captured) {
        int enemy = get_color(captured);
        state->material[enemy] -= piece_values[get_piece_type(captured)];
    }

    state->board[from] = 0;
    state->board[to] = piece;

    if (piece_type == KING) {
        state->king_squares[color] = to;
        state->castle_rights &= ~(castle_by_color[color][0] | castle_by_color[color][1]);

        if (from == king_home[color] && (to == from + 2 * castle_step[color] ||
                                         to == from - 2 * castle_step[color])) {
            int direction = to == from + 2 * castle_step[color] ? castle_step[color] : -castle_step[color];
            int rook_from = from + (direction == castle_step[color] ? 3 : 4) * direction;
            int rook_to = from + direction;
            state->board[rook_to] = state->board[rook_from];
            state->board[rook_from] = 0;
        }
    }

    // update castle rights
    for (int c = RED; c <= GREEN; c++) {
        int kingside_rook = king_home[c] + 3 * castle_step[c];
        int queenside_rook = king_home[c] - 4 * castle_step[c];

        if (from == kingside_rook || (capture_square == kingside_rook && captured)) {
            state->castle_rights &= ~castle_by_color[c][0];
        }
        if (from == queenside_rook || (capture_square == queenside_rook && captured)) {
            state->castle_rights &= ~castle_by_color[c][1];
        }
    }

    if (piece_type == PAWN) {
        int promoted = move.get_promotion();
        if (promoted) {
            state->board[to] = color * 6 + promoted + 1;
            state->material[color] += piece_values[promoted] - piece_values[PAWN];
        }
    }

    if (piece_type == PAWN || captured) {
        state->fifty_move_count = 0;
    } else {
        state->fifty_move_count++;
    }

    turn = next_color(turn);
}


void Position::pop() {
    state = &stack[--half_moves];
    turn = last_color(turn);
}