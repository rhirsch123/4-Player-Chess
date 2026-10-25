#include <string>
#include <sstream>
#include <iostream>

#include "position.h"
#include "engine.h"
#include "move.h"
#include "movegen.h"

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

    do {
        if (line == "uci") {
            printf("uciok\n");
        } else if (line == "isready") {
            printf("readyok\n");
        } else if (line == "ucinewgame") {
            engine.init();
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
    } while (std::getline(std::cin, line));

    return 0;
}