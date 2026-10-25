#ifndef types_h
#define types_h

#include <cstdint>
#include <chrono>

#define INF 1000000000
#define MATE_SCORE 32000
#define SCORE_NONE 32001
#define MAX_DEPTH 64 // max search depth
#define MAX_MOVES 512 // max legal moves for one position

typedef std::chrono::time_point<std::chrono::high_resolution_clock> time_point;

struct SearchInfo {
    // in milliseconds
    int time_left = 0;
    int increment = 0;
    int movetime = 0;
    int depth = 0;
    int nodes = 0;

    // game options
    bool timed_game = false;
    bool fixed_time = false;
    bool fixed_depth = false;
    bool fixed_nodes = false;
};


constexpr int piece_values[6] = { 100, 300, 350, 500, 900, 0 };

enum Piece : uint8_t {
    RED_PAWN = 1,
    RED_KNIGHT = 2,
    RED_BISHOP = 3,
    RED_ROOK = 4,
    RED_QUEEN = 5,
    RED_KING = 6,

    BLUE_PAWN = 7,
    BLUE_KNIGHT = 8,
    BLUE_BISHOP = 9,
    BLUE_ROOK = 10,
    BLUE_QUEEN = 11,
    BLUE_KING = 12,

    YELLOW_PAWN = 13,
    YELLOW_KNIGHT = 14,
    YELLOW_BISHOP = 15,
    YELLOW_ROOK = 16,
    YELLOW_QUEEN = 17,
    YELLOW_KING = 18,

    GREEN_PAWN = 19,
    GREEN_KNIGHT = 20,
    GREEN_BISHOP = 21,
    GREEN_ROOK = 22,
    GREEN_QUEEN = 23,
    GREEN_KING = 24
};

enum PieceType {
    PAWN = 0,
    KNIGHT = 1,
    BISHOP = 2,
    ROOK = 3,
    QUEEN = 4,
    KING = 5,
    ALL_PIECES = 6
};

enum Color {
    RED = 0,
    BLUE = 1,
    YELLOW = 2,
    GREEN = 3
};

enum Team {
    WHITE = 0, // red and yellow
    BLACK = 1  // blue and green
};

static inline int get_color(int piece) {
    return (piece > 6) + (piece > 12) + (piece > 18);
}

static inline int get_piece_type(int piece) {
    return piece - 6 * get_color(piece) - 1;
}

static inline int next_color(int color) {
    return (color + 1) % 4;
}

static inline int last_color(int color) {
    return (color + 3) % 4;
}

static inline int get_team(int color) {
    return color % 2;
}


enum CastleRights {
    RED_KINGSIDE = 1,
    RED_QUEENSIDE = 1 << 1,
    BLUE_KINGSIDE = 1 << 2,
    BLUE_QUEENSIDE = 1 << 3,
    YELLOW_KINGSIDE = 1 << 4,
    YELLOW_QUEENSIDE = 1 << 5,
    GREEN_KINGSIDE = 1 << 6,
    GREEN_QUEENSIDE = 1 << 7
};

constexpr CastleRights castle_by_color[4][2] = {
    {RED_KINGSIDE,    RED_QUEENSIDE},
    {BLUE_KINGSIDE,   BLUE_QUEENSIDE},
    {YELLOW_KINGSIDE, YELLOW_QUEENSIDE},
    {GREEN_KINGSIDE,  GREEN_QUEENSIDE}
};

#endif