
#include <atomic>
#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <vector>

#include "uci.h"
#include "movegen.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "time.h"
#include "tt.h"
#include "uci.h"

#define START_POSITION ("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")
#define ENGINE_NAME "ANDY'S CHESS ENGINE"
#define AUTHOR "ANDY HOANG"

using namespace std;

namespace {
    const char* StartFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    Search::UndoStackPtr SetupUndo;

    void setUpPosition(Position& position, istringstream& is) {
        Move move;
        string token, fen;

        is >> token;

        if(token == "startpos") {
            fen = START_POSITION;
            is >> token;
        }
        else if(token == "fen") {
            while(is >> token && token != "moves") {
                fen += token + " ";
            }
        }
        else {
            return;
        }

        position.init(fen, Threads.main());
        SetupUndo = Search::UndoStackPtr(new std::stack<Undo>);

        while(is >> token && (move = UCI::to_move(position, token)) != NO_MOVE) {
            SetupUndo->push(Undo());
            position.makeMove(&SetupUndo->top(), move);
        }
    }

    void setoption(istringstream& is) {
        string token, name, value;

        is >> token;

        while(is >> token && token != "value") {
            name += string(" ", name.empty() ? 0 : 1) + token;
        }

        while(is >> token) {
            value += string(" ", value.empty() ? 0 : 1) + token;
        }

        if(Options.count(name)) {
            Options[name] = value;
        }
        else {
            sync_cout << "No such option: " << name << sync_endl;
        }
    }

    void go(const Position& position, istringstream& is) {
        Search::LimitsType limits;
        string token;

        limits.startTime = now();

        while(is >> token) {
            if(token == "searchmoves") {
                while(is >> token) {
                    limits.searchmoves.push_back(UCI::to_move(position, token));
                }
            }
            else if(token == "wtime")     is >> limits.time[WHITE];
            else if(token == "btime")     is >> limits.time[BLACK];
            else if(token == "winc")      is >> limits.inc[WHITE];
            else if(token == "binc")      is >> limits.inc[BLACK];
            else if(token == "movestogo") is >> limits.movestogo;
            else if(token == "depth")     is >> limits.depth;
            else if(token == "nodes")     is >> limits.nodes;
            else if(token == "movetime")  is >> limits.movetime;
            else if(token == "mate")      is >> limits.mate;
            else if(token == "infinite")  limits.infinite = 1;
            else if(token == "ponder")    limits.ponder = 1;
        }

        Threads.start_thinking(position, limits, SetupUndo);
    }

} // namespace

void UCI::loop(int argc, char* argv[]) {
    Position position(START_POSITION, Threads.main());
    string token, cmd;

    for(int i = 1; i < argc; ++i) {
        cmd += std::string(argv[i]) + " ";
    }

    do {
        if(argc == 1 && !getline(cin, cmd)) {
            cmd = "quit";
        }

        istringstream is(cmd);

        token.clear();
        is >> skipws >> token;

        if(token == "quit" || token == "stop" || (token == "ponderhit" && Search::Signals.stopOnPonderhit)) {
            Search::Signals.stop = true;
            Threads.main()->start_searching(true);
        }
        else if(token == "ponderhit") {
            Search::Limits.ponder = 0;
        }
        else if(token == "uci") {
            sync_cout << "\n" << Options
            << "\nuciok" << sync_endl;
        }
        else if(token == "ucinewgame") {
            Search::clear();
            Time.availableNodes = 0;
        }
        else if(token == "isready")    sync_cout << "readyok" << sync_endl;
        else if(token == "go")         go(position, is);
        else if(token == "position")   setUpPosition(position, is);
        else if(token == "setoption")  setoption(is);
        else if(token == "d")          position.display();
        else if(token == "perft") {
            int depth;
            stringstream ss;

            is >> depth;
            ss << Options["Hash"] << " "
                << Options["Threads"] << " " << depth << " current perft";
            std::cout << perft(position, Depth(depth), true) << std::endl;

        }
        else {
            sync_cout << "Unknown command: " << cmd << sync_endl;
        }

    } while(token != "quit" && argc == 1);

    Threads.main()->wait_for_search_finished();
}

string UCI::value(Value v) {
    stringstream ss;

    if(abs(v) < VALUE_MATE - MAX_PLY) {
        ss << "cp " << v * 100 / valuePawnEg;
    }
    else {
        ss << "mate " << (v > 0 ? VALUE_MATE - v + 1 : -VALUE_MATE - v) / 2;
    }

    return ss.str();
}

std::string UCI::square(Square s) {
    return std::string { char('a' + getFile(s)), char('1' + getRank(s)) };
}

string UCI::move(Move move) {
    Square from = getFrom(move);
    Square to = getTo(move);

    if(move == NO_MOVE) {
        return "(none)";
    }
    if(move == NULL_MOVE) {
        return "0000";
    }

    if(getMoveType(move) == CASTLING) {
        to = makeSquare(to > from ? FILE_G : FILE_C, getRank(from));
    }

    string moveString = UCI::square(from) + UCI::square(to);

    if(getMoveType(move) == PROMOTION) {
        moveString += " pnbrqk"[(getPromotionType(move) >> 12) + KNIGHT];
    }

    return moveString;
}

Move UCI::to_move(const Position& position, string& str) {
    ExtMove moveList[MAX_MOVES];

    if(str.length() == 5) {
        str[4] = char(tolower(str[4]));
    }

    int moveCount = generateLegalMoves(position, moveList);

    for(int i = 0; i < moveCount; ++i) {
        if(str == UCI::move(moveList[i])) {
            return moveList[i];
        }
    }

    return NO_MOVE;
}