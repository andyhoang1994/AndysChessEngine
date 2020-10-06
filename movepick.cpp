#include <cassert>

#include "defines.h"
#include "position.h"
#include "movegen.h"
#include "movepick.h"
#include "thread.h"

namespace {

    enum Stages {
        MAIN_SEARCH, GOOD_CAPTURES, KILLERS, GOOD_QUIETS, BAD_QUIETS, BAD_CAPTURES,
        EVASION, ALL_EVASIONS,
        QSEARCH_WITH_CHECKS, QCAPTURES_1, CHECKS,
        QSEARCH_WITHOUT_CHECKS, QCAPTURES_2,
        PROBCUT, PROBCUT_CAPTURES,
        RECAPTURE, RECAPTURES,
        STOP
    };

    void insertion_sort(ExtMove* begin, ExtMove* end) {
        ExtMove tmp, * p, * q;

        for(p = begin + 1; p < end; ++p)
        {
            tmp = *p;
            for(q = p; q != begin && *(q - 1) < tmp; --q)
                *q = *(q - 1);
            *q = tmp;
        }
    }

    Move pick_best(ExtMove* begin, ExtMove* end) {
        std::swap(*begin, *std::max_element(begin, end));
        return *begin;
    }

}

MovePicker::MovePicker(const Position& p, Move ttm, Depth d, const HistoryStats& h,
    const CounterMovesStats& cmh, Move cm, Search::Stack* s)
    : position(p), history(h), counterMovesHistory(&cmh), ss(s), counterMove(cm), depth(d) {

    assert(d > DEPTH_ZERO);

    stage = position.checkersTo(position.getSide()) ? EVASION : MAIN_SEARCH;
    ttMove = ttm && position.checkLegality(ttm) ? ttm : NO_MOVE;
    endMoves += (ttMove != NO_MOVE);
}

MovePicker::MovePicker(const Position& p, Move ttm, Depth d,
    const HistoryStats& h, Square s)
    : position(p), history(h), counterMovesHistory(nullptr) {

    assert(d <= DEPTH_ZERO);

    if(position.checkersTo(position.getSide())) {
        stage = EVASION;
    }

    else if(d > DEPTH_QS_NO_CHECKS) {
        stage = QSEARCH_WITH_CHECKS;
    }

    else if(d > DEPTH_QS_RECAPTURES) {
        stage = QSEARCH_WITHOUT_CHECKS;
    }

    else {
        stage = RECAPTURE;
        recaptureSquare = s;
        ttm = NO_MOVE;
    }

    ttMove = ttm && position.checkLegality(ttm) ? ttm : NO_MOVE;
    endMoves += (ttMove != NO_MOVE);
}

MovePicker::MovePicker(const Position& p, Move ttm, const HistoryStats& h, Value th)
    : position(p), history(h), counterMovesHistory(nullptr), threshold(th) {

    assert(!position.checkersTo(position.getSide()));

    stage = PROBCUT;

    // In ProbCut we generate captures with SEE higher than the given threshold
    ttMove = ttm
        && position.checkLegality(ttm)
        && position.checkCapture(ttm)
        && position.see(ttm) > threshold ? ttm : NO_MOVE;

    endMoves += (ttMove != NO_MOVE);
}

template<>
void MovePicker::score<CAPTURES>() {
    for(auto& move : *this)
        move.value = (Value)pieceValue[position.getPieceOnSquare(getTo(move))].value()
        - Value(200 * relativeRank(position.getSide(), getRank(getTo(move))));
}

template<>
void MovePicker::score<QUIETS>() {

    for(auto& move : *this)
        move.value = history[position.getPieceOnSquare(getFrom(move))][getTo(move)]
        + (*counterMovesHistory)[position.getPieceOnSquare(getFrom(move))][getTo(move)];
}

template<>
void MovePicker::score<EVASIONS>() {
    Value see;

    for(auto& move : *this)
        if((see = position.seeSign(move)) < VALUE_ZERO)
            move.value = see - HistoryStats::Max; // At the bottom

        else if(position.checkCapture(move))
            move.value = (Value)pieceValue[position.getPieceOnSquare(getTo(move))].value()
            - Value(getPieceType(position.getPieceOnSquare(getFrom(move)))) + HistoryStats::Max;
        else
            move.value = history[position.getPieceOnSquare(getFrom(move))][getTo(move)];
}

void MovePicker::generate_next_stage() {
    assert(stage != STOP);

    curr = moves;

    switch(++stage) {

    case GOOD_CAPTURES: case QCAPTURES_1: case QCAPTURES_2:
    case PROBCUT_CAPTURES: case RECAPTURES:
        endMoves = generateCaptureMoves(position, moves);
        score<CAPTURES>();
        break;

    case KILLERS:
        killers[0] = ss->killers[0];
        killers[1] = ss->killers[1];
        killers[2] = counterMove;
        curr = killers;
        endMoves = curr + 2 + (counterMove != killers[0] && counterMove != killers[1]);
        break;

    case GOOD_QUIETS:
        endQuiets = endMoves = generateQuietMoves(position, moves);
        score<QUIETS>();
        endMoves = std::partition(curr, endMoves, [](const ExtMove& move) {
            return move.value > VALUE_ZERO;
        });
        insertion_sort(curr, endMoves);
        break;

    case BAD_QUIETS:
        curr = endMoves;
        endMoves = endQuiets;
        if(depth >= 3 * ONE_PLY) {
            insertion_sort(curr, endMoves);
        }
        break;

    case BAD_CAPTURES:
        curr = moves + MAX_MOVES - 1;
        endMoves = endBadCaptures;
        break;

    case ALL_EVASIONS:
        endMoves = generateEvasions(position, moves);
        if(endMoves - moves > 1) {
            score<EVASIONS>();
        }
        break;

    case CHECKS:
        endMoves = generateTacticalMoves(position, moves);
        break;

    case EVASION: case QSEARCH_WITH_CHECKS: case QSEARCH_WITHOUT_CHECKS:
    case PROBCUT: case RECAPTURE: case STOP:
        stage = STOP;
        break;

    default:
        assert(false);
    }
}

Move MovePicker::next_move() {
    Move move;

    while(true) {
        while(curr == endMoves && stage != STOP) {
            generate_next_stage();
        }

        switch(stage) {

        case MAIN_SEARCH: case EVASION: case QSEARCH_WITH_CHECKS:
        case QSEARCH_WITHOUT_CHECKS: case PROBCUT:
            ++curr;
            return ttMove;

        case GOOD_CAPTURES:
            move = pick_best(curr++, endMoves);
            if(move != ttMove)
            {
                if(position.seeSign(move) >= VALUE_ZERO)
                    return move;

                // Losing capture, move it to the tail of the array
                *endBadCaptures-- = move;
            }
            break;

        case KILLERS:
            move = *curr++;
            if(move != NO_MOVE
                && move != ttMove
                && position.checkLegality(move)
                && !position.checkCapture(move))
                return move;
            break;

        case GOOD_QUIETS: case BAD_QUIETS:
            move = *curr++;
            if(move != ttMove
                && move != killers[0]
                && move != killers[1]
                && move != killers[2])
                return move;
            break;

        case BAD_CAPTURES:
            return *curr--;

        case ALL_EVASIONS: case QCAPTURES_1: case QCAPTURES_2:
            move = pick_best(curr++, endMoves);
            if(move != ttMove)
                return move;
            break;

        case PROBCUT_CAPTURES:
            move = pick_best(curr++, endMoves);
            if(move != ttMove && position.see(move) > threshold)
                return move;
            break;

        case RECAPTURES:
            move = pick_best(curr++, endMoves);
            if(getTo(move) == recaptureSquare)
                return move;
            break;

        case CHECKS:
            move = *curr++;
            if(move != ttMove)
                return move;
            break;

        case STOP:
            return NO_MOVE;

        default:
            assert(false);
        }
    }
}