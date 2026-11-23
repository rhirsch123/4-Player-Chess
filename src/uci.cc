#include <string>
#include <sstream>
#include <iostream>

#include "position.h"
#include "engine.h"
#include "move.h"
#include "movegen.h"

static std::string board_to_fen4(Position& position) {
    std::string fen;

    switch (position.turn) {
        case RED: fen += "R-"; break;
        case BLUE: fen += "B-"; break;
        case YELLOW: fen += "Y-"; break;
        case GREEN: fen += "G-"; break;
    }

    const char colors[4] = {'r', 'b', 'y', 'g'};
    const char types[6] = {'P', 'N', 'B', 'R', 'Q', 'K'};
    fen += "0,0,0,0-";
    fen += "0,0,0,0-";
    fen += "0,0,0,0-";
    fen += "0,0,0,0-";
    fen += std::to_string(position.state->fifty_move_count);
    fen += "-";

    for (int row = 0; row < 14; row++) {
        if (row != 0) {
            fen += "/";
        }

        std::string rank;
        int empty = 0;
        for (int col = 0; col < 14; col++) {
            int square = row * 14 + col;
            int piece = position.piece_on(square);
            if (!piece) {
                if (valid_square(square)) {
                    empty++;
                } else {
                    if (empty) {
                        rank += std::to_string(empty) + ",";
                        empty = 0;
                    }
                    rank += "x,";
                }
                continue;
            }

            if (empty) {
                rank += std::to_string(empty) + ",";
                empty = 0;
            }

            int color = get_color(piece);
            int type = get_piece_type(piece);
            rank += std::string(1, colors[color]) + types[type] + ",";
        }

        if (empty) {
            rank += std::to_string(empty);
        } else if (!rank.empty() && rank.back() == ',') {
            rank.pop_back();
        }
        fen += rank;
    }

    return fen;
}

int main(int argc, char* argv[]) {
    Position position;
    Engine engine;

    std::string line;
    for (int i = 1; i < argc; i++) {
        if (i > 1) {
            line += " ";
        }
        line += argv[i];
    }
    bool invoked_with_arguments = !line.empty();

    do {
        if (line == "uci") {
            printf("uciok\n");
            fflush(stdout);
        } else if (line == "isready") {
            printf("readyok\n");
            fflush(stdout);
        } else if (line == "ucinewgame") {
            engine.init();
        } else if (line == "getfen") {
            printf("%s\n", board_to_fen4(position).c_str());
            fflush(stdout);
        } else if (line == "legalmoves") {
            MoveList moves;
            get_legal_moves(position, &moves);
            for (int i = 0; i < moves.size; i++) {
                if (i != 0) {
                    printf(" ");
                }
                printf("%s", moves.moves[i].to_uci().c_str());
            }
            printf("\n");
            fflush(stdout);
        } else if (line == "board") {
            printf("%s\n", board_to_fen4(position).c_str());
            fflush(stdout);
        } else if (line == "turn") {
            switch (position.turn) {
                case RED: printf("R\n"); break;
                case BLUE: printf("B\n"); break;
                case YELLOW: printf("Y\n"); break;
                case GREEN: printf("G\n"); break;
            }
            fflush(stdout);
        } else if (line.rfind("position", 0) == 0) {
            std::string fen = "";

            std::istringstream iss(line);
            std::string token;
            iss >> token;
            iss >> token;
            if (token == "startpos") {
                iss >> token;
            } else if (token == "fen") {
                fen.clear();
                while (iss >> token && token != "moves") {
                    fen += token + " ";
                }
            }

            position.set_fen(fen);

            if (token == "moves") {
                while (iss >> token) {
                    Move move = Move(token);
                    MoveList moves;
                    get_legal_moves(position, &moves);
                    bool is_legal = false;
                    for (int i = 0; i < moves.size; i++) {
                        if (moves.moves[i] == move) {
                            is_legal = true;
                            break;
                        }
                    }

                    if (!is_legal) {
                        break;
                    }
                    position.make_move(move);
                }
            }
        } else if (line.rfind("perft", 0) == 0) {
            std::istringstream iss(line);
            std::string arg;
            iss >> arg;
            iss >> arg;
            int depth = std::stoi(arg);
            printf("%llu\n", perft(position, depth));
        } else if (line.rfind("go", 0) == 0) {
            int depth = 0, nodes = 0, movetime = 0,
                rtime = 0, btime = 0, ytime = 0, gtime = 0,
                rinc = 0, binc = 0, yinc = 0, ginc = 0;

            std::istringstream iss(line);
            std::string token;
            iss >> token;
            while (iss >> token) {
                if (token == "infinite") depth = MAX_DEPTH;
                else if (token == "depth") iss >> depth;
                else if (token == "nodes") iss >> nodes;
                else if (token == "movetime") iss >> movetime;
                else if (token == "rtime") iss >> rtime;
                else if (token == "btime") iss >> btime;
                else if (token == "ytime") iss >> ytime;
                else if (token == "gtime") iss >> gtime;
                else if (token == "rinc") iss >> rinc;
                else if (token == "binc") iss >> binc;
                else if (token == "yinc") iss >> yinc;
                else if (token == "ginc") iss >> ginc;
            }

            SearchInfo info;
            if (depth) {
                info.fixed_depth = true;
                info.depth = depth;
            } else if (nodes) {
                info.fixed_nodes = true;
                info.nodes = nodes;
            } else if (movetime) {
                info.fixed_time = true;
                info.movetime = std::max(movetime - 30, 10);
            } else {
                info.timed_game = true;
                if (position.turn == RED) {
                    info.time_left = rtime;
                    info.increment = rinc;
                } else if (position.turn == BLUE) {
                    info.time_left = btime;
                    info.increment = binc;
                } else if (position.turn == YELLOW) {
                    info.time_left = ytime;
                    info.increment = yinc;
                } else if (position.turn == GREEN) {
                    info.time_left = gtime;
                    info.increment = ginc;
                }
                info.time_left = std::max(info.time_left - 30, 10);
            }

            Move best_move = engine.get_move(position, info);
            printf("bestmove %s\n", best_move.to_uci().c_str());
            fflush(stdout);
        } else if (line == "quit") {
            break;
        }
        if (invoked_with_arguments) {
            break;
        }
    } while (std::getline(std::cin, line));

    return 0;
}