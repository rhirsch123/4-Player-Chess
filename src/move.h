#ifndef move_hh
#define move_hh

#include <cstdint>
#include <string>

#include "types.h"

class Move {
    uint8_t from_square;
    uint8_t to_square;
    uint8_t promotion;

public:
    Move() : from_square(0), to_square(0), promotion(0) {}

    Move(uint8_t from, uint8_t to, uint8_t promotion = 0)
        : from_square(from), to_square(to), promotion(promotion) {}

    Move(const std::string& uci_move) {
        int i = 0;

        auto parse_square = [&]() {
            int col = uci_move[i++] - 'a';
            int rank = 0;

            while (i < uci_move.size() && uci_move[i] >= '0' && uci_move[i] <= '9') {
                rank = rank * 10 + (uci_move[i++] - '0');
            }

            return (14 - rank) * 14 + col;
        };

        from_square = parse_square();
        to_square = parse_square();

        promotion = 0;
        if (i < uci_move.size()) {
            char p = uci_move[i];

            if (p == 'n' || p == 'N') promotion = KNIGHT;
            else if (p == 'b' || p == 'B') promotion = BISHOP;
            else if (p == 'r' || p == 'R') promotion = ROOK;
            else if (p == 'q' || p == 'Q') promotion = QUEEN;
        }
    }

    inline int from() const {
        return from_square;
    }

    inline int to() const {
        return to_square;
    }

    inline int get_promotion() const {
        return promotion;
    }

    inline int from_row() const {
        return from() / 14;
    }

    inline int from_col() const {
        return from() % 14;
    }

    inline int to_row() const {
        return to() / 14;
    }

    inline int to_col() const {
        return to() % 14;
    }

    std::string to_uci() const {
        std::string uci_move =
            std::string(1, 'a' + from_col()) +
            std::to_string(14 - from_row()) +
            std::string(1, 'a' + to_col()) +
            std::to_string(14 - to_row());

        int promo = get_promotion();

        if (promo == KNIGHT) uci_move += 'n';
        else if (promo == BISHOP) uci_move += 'b';
        else if (promo == ROOK) uci_move += 'r';
        else if (promo == QUEEN) uci_move += 'q';

        return uci_move;
    }

    bool operator==(const Move& other) const {
        return (from_square == other.from_square)
            && (to_square == other.to_square)
            && (promotion == other.promotion);
    }

    bool operator!=(const Move& other) const {
        return !(*this == other);
    }

    explicit operator bool() const {
        return from_square != to_square;
    }
};

#endif