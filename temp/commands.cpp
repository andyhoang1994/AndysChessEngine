
#include <atomic>
#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <vector>

#include "commands.h"
#include "position.h"
#include "search.h"

#define START_POSITION ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")
#define ENGINE_NAME "ANDY'S CHESS ENGINE"
#define AUTHOR "ANDY HOANG"

std::atomic_bool searching = false;

void init() {
    std::cout << "id name " << ENGINE_NAME << '\n'
        << "id author " << AUTHOR << std::endl;
    /*for(auto& [name, option] : options::checks) {
        std::string val = option.default_value ? "true" : "false";
        std::cout << "option name " << name << " type check"
            << " default " << val
            << std::endl;
    }
    for(auto& [name, option] : options::spins) {
        std::cout << "option name " << name << " type spin"
            << " default " << option.value
            << " min " << option.min
            << " max " << option.max
            << std::endl;
    }*/
    std::cout << "uciok" << std::endl;
}

Move getParsedMove(Position& position, std::string& candidateMove) {
    Move moveList[MAX_MOVES];
    int size = generateMoves(position, moveList);

    for(int i = 0; i < size; ++i) {
        if(getMoveString(moveList[i]) == candidateMove) {
            return moveList[i];
        }
    }

    return 0;
}

void makeMove(Position& position, std::string& candidateMove) {
    Move moveList[MAX_MOVES];
    int size = generateLegalMoves(position, moveList);

    for(int i = 0; i < size; ++i) {
        if(getMoveString(moveList[i]) == candidateMove) {
            position.makeMove(moveList[i]);
            return;
        }
    }
}

void getLegalMoves(Position& position) {
    Move moveList[MAX_MOVES];
    int size = generateLegalMoves(position, moveList);

    for(int i = 0; i < size; ++i) {
        std::cout << getMoveString(moveList[i]) << std::endl;
    }
    return;
}

/*void go(Position& position, std::istringstream& iss) {
    std::string token;
    Time.stopSearch = false;
    Time.timeDependent = false;
    Time.limitedSearch = false;
    Time.analyzing = false;
    Time.maxPly = MAX_PLY;
    Time.startTime = utils::curr_time();
    Time.endTime = Time.startTime;
    time_ms timeToGo = 1000;
    int movesToGo = 35;
    int increment = 0;
    while(iss >> token) {
        if(token == "infinite" || token == "ponder") {
            Time.timeDependent = false;
            Time.analyzing = true;
        }
        else if(token == "movetime") {
            Time.timeDependent = true;
            time_ms movetime;
            iss >> movetime;
            timeToGo = movetime;
            movesToGo = 1;
        }
        else if(token == "wtime") {
            Time.timeDependent = true;
            time_ms wtime;
            iss >> wtime;
            if(position.getSide() == WHITE) {
                timeToGo = wtime;
            }
        }
        else if(token == "btime") {
            Time.timeDependent = true;
            time_ms btime;
            iss >> btime;
            if(position.getSide() == BLACK) {
                timeToGo = btime;
            }
        }
        else if(token == "winc") {
            Time.timeDependent = true;
            time_ms winc;
            iss >> winc;
            if(position.getSide() == WHITE) {
                increment = winc;
            }
        }
        else if(token == "binc") {
            Time.timeDependent = true;
            time_ms binc;
            iss >> binc;
            if(position.getSide() == BLACK) {
                increment = binc;
            }
        }
        else if(token == "searchmoves") {
            Time.limitedSearch = true;
            Time.searchMoves.clear();
            std::string candidateMove;
            while(iss >> candidateMove) {
                Move move = getParsedMove(position, candidateMove);
                Time.searchMoves.push_back(move);
            }
        }
        else if(token == "movestogo") {
            iss >> movesToGo;
        }
        else if(token == "depth") {
            Time.timeDependent = false;
            int limitedDepth;
            if(iss >> limitedDepth) {
                Time.maxPly = limitedDepth;
            }
        }
    }

    if(Time.timeDependent) {
        Time.endTime += (timeToGo + (movesToGo - 1) * increment) / movesToGo;

        if(movesToGo == 1) {
            Time.endTime -= 50;
        }
    }
    if(!searching) {
        searching = true;

        std::thread searchThread([&position]() {
            auto bestMove = position.bestMove();

            std::cout << "bestmove " << getMoveString(bestMove.first);
            if(allow_ponder && bestMove.second) {
                std::cout << " ponder " << getMoveString(bestMove.second);
            }
            std::cout << std::endl;
            searching = false;
            });

        searchThread.detach();
    }
}*/

void commandLoop(int argc, char* argv[]) {
    Position position;
    position.init(START_POSITION);

    int command = 0;
    std::string token = "";
    std::string cmd = "";


    for(int i = 1; i < argc; ++i) cmd += std::string(argv[i]) + " ";

    do {
        if(argc == 1 && !getline(std::cin, cmd)) cmd = "quit";
        std::istringstream iss(cmd);
        token.clear();
        iss >> token;
        if(token == "move" || token == "m") {
            iss >> token;
            makeMove(position, token);
        }
        else if(token == "position") {
            iss >> token;
            if(token == "startpos") {
                position.init(START_POSITION);
            }
            else if(token == "fen") {
                std::getline(iss, token);
                position.init(token.substr(1));
            }

            if(iss >> token && token == "moves") {
                while(iss >> token) {
                    position.makeMove(getParsedMove(position, token));
                }
            }
        }
        else if(token == "display" || token == "d") {
            position.display();
        }
        else if(token == "movelist") {
            getLegalMoves(position);
        }
        else if(token == "perft") {
            if(iss.rdbuf()->in_avail() != 0) {
                iss >> token;
            }
            else {
                token = "2";
            }
            uint64_t t1 = utils::curr_time();
            uint64_t nodes = position.perft(std::stoi(token), true);
            uint64_t t2 = utils::curr_time();
            std::cout << "nodes:" << nodes << " time:" << (t2 - t1) << std::endl;
        }
        else if(token == "isready") {
            std::cout << "readyok" << std::endl;
        }
        else if(token == "quit") break;
    } while(token != "quit" && argc == 1);
}