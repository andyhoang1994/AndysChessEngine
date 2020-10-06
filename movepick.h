#pragma once

#include <algorithm>
#include <cstring>

#include "movegen.h"
#include "position.h"
#include "search.h"
#include "defines.h"

template<typename T, bool CM = false>
struct Stats {

    static const Value Max = Value(1 << 28);

    const T* operator[](Piece pc) const { return table[pc]; }
    T* operator[](Piece pc) { return table[pc]; }
    void clear() { std::memset(table, 0, sizeof(table)); }

    void update(Piece pc, Square to, Move m) {

        if(m != table[pc][to])
            table[pc][to] = m;
    }

    void update(Piece pc, Square to, Value v) {

        if(abs(int(v)) >= 324) {
            return;
        }

        table[pc][to] -= table[pc][to] * abs(int(v)) / (CM ? 512 : 324);
        table[pc][to] += int(v) * (CM ? 64 : 32);
    }

private:
    T table[PIECE_COUNT][SQUARE_COUNT];
};

typedef Stats<Move> MovesStats;
typedef Stats<Value, false> HistoryStats;
typedef Stats<Value, true> CounterMovesStats;
typedef Stats<CounterMovesStats> CounterMovesHistoryStats;

class MovePicker {
public:
    MovePicker(const MovePicker&) = delete;
    MovePicker& operator=(const MovePicker&) = delete;

    MovePicker(const Position&, Move, Depth, const HistoryStats&, Square);
    MovePicker(const Position&, Move, const HistoryStats&, Value);
    MovePicker(const Position&, Move, Depth, const HistoryStats&, const CounterMovesStats&, Move, Search::Stack*);

    Move next_move();

private:
    template<GenType> void score();
    void generate_next_stage();
    ExtMove* begin() { return moves; }
    ExtMove* end() { return endMoves; }

    const Position& position;
    const HistoryStats& history;
    const CounterMovesStats* counterMovesHistory;
    Search::Stack* ss;
    Move counterMove;
    Depth depth;
    Move ttMove;
    ExtMove killers[3];
    Square recaptureSquare;
    Value threshold;
    int stage;
    ExtMove* endQuiets, * endBadCaptures = moves + MAX_MOVES - 1;
    ExtMove moves[MAX_MOVES], * curr = moves, * endMoves = moves;
};