#pragma once
#include <atomic>
#include <memory>
#include <stack>
#include <vector>

#include "utils.h"
#include "position.h"
#include "defines.h"

namespace Search {

    struct Stack {
        Move* pv;
        int ply;
        Move currentMove;
        Move excludedMove;
        Move killers[2];
        Value staticEval;
        bool skipEarlyPruning;
        int moveCount;
    };

    struct RootMove {
        explicit RootMove(Move m) : pv(1, m) {}

        bool operator<(const RootMove& m) const { return m.score < score; } // Descending sort
        bool operator==(const Move& m) const { return pv[0] == m; }
        void insert_pv_in_tt(Position& position);
        bool extract_ponder_from_tt(Position& position);

        Value score = -VALUE_INFINITE;
        Value previousScore = -VALUE_INFINITE;
        std::vector<Move> pv;
    };

    typedef std::vector<RootMove> RootMoveVector;

    struct LimitsType {
        LimitsType() {
            nodes = time[WHITE] = time[BLACK] = inc[WHITE] = inc[BLACK] = npmsec = movestogo =
                depth = movetime = mate = infinite = ponder = 0;
        }

        bool use_time_management() const {
            return !(mate | movetime | depth | nodes | infinite);
        }

        std::vector<Move> searchmoves;
        int time[COLOUR_COUNT], inc[COLOUR_COUNT], npmsec, movestogo, depth, movetime, mate, infinite, ponder;
        int64_t nodes;
        TimePoint startTime;
    };

    struct SignalsType {
        std::atomic_bool stop, stopOnPonderhit;
    };

    typedef std::unique_ptr<std::stack<Undo>> UndoStackPtr;

    extern SignalsType Signals;
    extern LimitsType Limits;
    extern UndoStackPtr SetupUndo;

    void init();
    void clear();
    template<bool Root = true> uint64_t perft(Position position, Depth depth);
}