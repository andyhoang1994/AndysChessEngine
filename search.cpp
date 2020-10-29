
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <sstream>

#include "defines.h"
#include "evaluate.h"
#include "utils.h"
#include "movegen.h"
#include "movepick.h"
#include "search.h"
#include "time.h"
#include "thread.h"
#include "tt.h"
#include "uci.h"

namespace Search {
    SignalsType Signals;
    LimitsType Limits;
    UndoStackPtr SetupUndo;
}

namespace Tablebases {

    int Cardinality;
    uint64_t Hits;
    bool RootInTB;
    bool UseRule50;
    Depth ProbeDepth;
    Value Score;
}

namespace TB = Tablebases;

using std::string;
using Evaluator::evaluate;
using namespace Search;

namespace {
    enum NodeType { Root, PV, NonPV };

    constexpr int razor_margin[4] = { 483, 570, 603, 554 };
    Value futility_margin(Depth d) { return Value(200 * d); }

    int FutilityMoveCounts[2][16];
    Depth Reductions[2][2][64][64];

    template <bool PvNode> Depth reduction(bool i, Depth d, int mn) {
        return Reductions[PvNode][i][std::min(d, 63 * ONE_PLY)][std::min(mn, 63)];
    }

    struct EasyMoveManager {

        void clear() {
            stableCount = 0;
            expectedPosKey = 0;
            pv[0] = pv[1] = pv[2] = NO_MOVE;
        }

        Move get(Key key) const {
            return expectedPosKey == key ? pv[2] : NO_MOVE;
        }

        void update(Position& position, const std::vector<Move>& newPv) {

            assert(newPv.size() >= 3);

            stableCount = (newPv[2] == pv[2]) ? stableCount + 1 : 0;

            if(!std::equal(newPv.begin(), newPv.begin() + 3, pv))
            {
                std::copy(newPv.begin(), newPv.begin() + 3, pv);

                Undo undo[2];
                position.makeMove(undo, newPv[0]);
                position.makeMove(undo + 1, newPv[1]);
                expectedPosKey = position.getPositionKey();
                position.undoMove(undo + 1, newPv[1]);
                position.undoMove(undo, newPv[0]);
            }
        }

        int stableCount;
        Key expectedPosKey;
        Move pv[3];
    };

    EasyMoveManager EasyMove;
    Value DrawValue[COLOUR_COUNT];
    CounterMovesHistoryStats CounterMovesHistory;

    template <NodeType NT>
    Value search(Position& position, Stack* ss, Value alpha, Value beta, Depth depth, bool cutNode);

    template <NodeType NT, bool InCheck>
    Value qsearch(Position& position, Stack* ss, Value alpha, Value beta, Depth depth);

    Value value_to_tt(Value v, int ply);
    Value value_from_tt(Value v, int ply);
    void update_pv(Move* pv, Move move, Move* childPv);
    void update_stats(const Position& position, Stack* ss, Move move, Depth depth, Move* quiets, int quietsCount);
    void check_time();

}

void Search::init() {
    const double K[][2] = { { 0.799, 2.281 }, { 0.484, 3.023 } };

    for(int pv = 0; pv <= 1; ++pv) {
        for(int imp = 0; imp <= 1; ++imp) {
            for(int d = 1; d < 64; ++d) {
                for(int mc = 1; mc < 64; ++mc) {
                    double r = K[pv][0] + log(d) * log(mc) / K[pv][1];

                    if(r >= 1.5) {
                        Reductions[pv][imp][d][mc] = int(r) * ONE_PLY;
                    }
                    if(!pv && !imp && Reductions[pv][imp][d][mc] >= 2 * ONE_PLY) {
                        Reductions[pv][imp][d][mc] += ONE_PLY;
                    }
                }
            }
        }
    }

    for(int d = 0; d < 16; ++d) {
        FutilityMoveCounts[0][d] = int(2.4 + 0.773 * pow(d + 0.00, 1.8));
        FutilityMoveCounts[1][d] = int(2.9 + 1.045 * pow(d + 0.49, 1.8));
    }
}

void Search::clear() {

    TT.clear();
    CounterMovesHistory.clear();

    for(Thread* th : Threads)
    {
        th->history.clear();
        th->counterMoves.clear();
    }
}

template<bool Root>
uint64_t Search::perft(Position position, Depth depth) {
    ExtMove moveList[MAX_MOVES];
    Undo undo[1];
    uint64_t leaves = uint64_t(0);

    int size = generateLegalMoves(position, moveList);
    if(depth == 1) {
        return size;
    }

    for(size -= 1; size >= 0; size--) {
        Position child = position;
        child.makeMove(undo, moveList[size]);

        if(child.checkersTo(~child.getSide())) {
            continue;
        }
        uint64_t count = perft(child, depth - 1, false);
        leaves += count;

        if(Root == true) {
            std::cout << getMoveString(moveList[size]) << ": " << count << std::endl;
        }
    }

    return leaves;
}

template uint64_t Search::perft<true>(Position, Depth);

void MainThread::search() {
    Colour us = rootPos.getSide();
    Time.init(Limits, us, rootPos.getPly());

    int contempt = Options["Contempt"] * valuePawnEg / 100;
    DrawValue[us] = VALUE_DRAW - Value(contempt);
    DrawValue[~us] = VALUE_DRAW + Value(contempt);

    if(rootMoves.empty()) {
        rootMoves.push_back(RootMove(NO_MOVE));
        sync_cout << "info depth 0 score "
            << UCI::value(rootPos.checkersTo(us) ? -VALUE_MATE : VALUE_DRAW)
            << sync_endl;
    }
    else {
        for(Thread* thread : Threads) {
            thread->maxPly = 0;
            thread->rootDepth = DEPTH_ZERO;
            if(thread != this) {
                thread->rootPos = Position(rootPos, thread);
                thread->rootMoves = rootMoves;
                thread->start_searching();
            }
        }

        Thread::search();
    }

    if(Limits.npmsec) {
        Time.availableNodes += Limits.inc[us] - Threads.nodes_searched();
    }

    if(!Signals.stop && (Limits.ponder || Limits.infinite)) {
        Signals.stopOnPonderhit = true;
        wait(Signals.stop);
    }

    Signals.stop = true;

    for(Thread* thread : Threads) {
        if(thread != this) {
            thread->wait_for_search_finished();
        }
    }

    Thread* bestThread = this;
    if(!this->easyMovePlayed && Options["MultiPV"] == 1) {
        for(Thread* thread : Threads) {
            if(thread->completedDepth > bestThread->completedDepth
            && thread->rootMoves[0].score > bestThread->rootMoves[0].score) {
                bestThread = thread;
            }
        }
    }

    if(bestThread != this) {
        sync_cout << UCI::pv(bestThread->rootPos, bestThread->completedDepth, -VALUE_INFINITE, VALUE_INFINITE) << sync_endl;
    }

    sync_cout << "bestmove " << UCI::move(bestThread->rootMoves[0].pv[0]);

    if(bestThread->rootMoves[0].pv.size() > 1 || bestThread->rootMoves[0].extract_ponder_from_tt(rootPos)) {
        std::cout << " ponder " << UCI::move(bestThread->rootMoves[0].pv[1]);
    }

    std::cout << sync_endl;
}

void Thread::search() {
    Stack stack[MAX_PLY + 4], * ss = stack + 2;
    Value bestValue, alpha, beta, delta;
    Move easyMove = NO_MOVE;
    MainThread* mainThread = (this == Threads.main() ? Threads.main() : nullptr);

    std::memset(ss - 2, 0, 5 * sizeof(Stack));

    bestValue = delta = alpha = -VALUE_INFINITE;
    beta = VALUE_INFINITE;
    completedDepth = DEPTH_ZERO;

    if(mainThread) {
        easyMove = EasyMove.get(rootPos.getPositionKey());
        EasyMove.clear();
        mainThread->easyMovePlayed = mainThread->failedLow = false;
        mainThread->bestMoveChanges = 0;
        TT.new_search();
        check_time();
    }

    size_t multiPV = Options["MultiPV"];

    multiPV = std::min(multiPV, rootMoves.size());

    while(++rootDepth < DEPTH_MAX && !Signals.stop && (!Limits.depth || rootDepth <= Limits.depth)) {
        if(!mainThread) {
            int d = rootDepth + rootPos.getPly();

            if(idx <= 6 || idx > 24) {
                if(((d + idx) >> (rbitscan(idx + 1) - 1)) % 2)
                    continue;
            }
            else {
                static const int HalfDensityMap[] = {
                  0x07, 0x0b, 0x0d, 0x0e, 0x13, 0x16, 0x19, 0x1a, 0x1c,
                  0x23, 0x25, 0x26, 0x29, 0x2c, 0x31, 0x32, 0x34, 0x38
                };

                if((HalfDensityMap[idx - 7] >> (d % 6)) & 1)
                    continue;
            }
        }

        if(mainThread) {
            mainThread->bestMoveChanges *= 0.505, mainThread->failedLow = false;
        }

        for(RootMove& rm : rootMoves) {
            rm.previousScore = rm.score;
        }

        for(PVIdx = 0; PVIdx < multiPV && !Signals.stop; ++PVIdx) {
            if(rootDepth >= 5 * ONE_PLY) {
                delta = Value(18);
                alpha = std::max(rootMoves[PVIdx].previousScore - delta, -VALUE_INFINITE);
                beta = std::min(rootMoves[PVIdx].previousScore + delta, VALUE_INFINITE);
            }

            while(true) {
                bestValue = ::search<Root>(rootPos, ss, alpha, beta, rootDepth, false);

                std::stable_sort(rootMoves.begin() + PVIdx, rootMoves.end());

                for(size_t i = 0; i <= PVIdx; ++i) {
                    rootMoves[i].insert_pv_in_tt(rootPos);
                }

                if(Signals.stop) {
                    break;
                }

                if(mainThread
                    && multiPV == 1
                    && (bestValue <= alpha || bestValue >= beta)
                && Time.elapsed() > 3000) {
                    sync_cout << UCI::pv(rootPos, rootDepth, alpha, beta) << sync_endl;
                }

                if(bestValue <= alpha) {
                    beta = (alpha + beta) / 2;
                    alpha = std::max(bestValue - delta, -VALUE_INFINITE);

                    if(mainThread)
                    {
                        mainThread->failedLow = true;
                        Signals.stopOnPonderhit = false;
                    }
                }
                else if(bestValue >= beta) {
                    alpha = (alpha + beta) / 2;
                    beta = std::min(bestValue + delta, VALUE_INFINITE);
                }
                else {
                    break;
                }

                delta += (delta / 4) + 5;

                assert(alpha >= -VALUE_INFINITE && beta <= VALUE_INFINITE);
            }

            std::stable_sort(rootMoves.begin(), rootMoves.begin() + PVIdx + 1);

            if(!mainThread) {
                break;
            }

            if(Signals.stop) {
                sync_cout << "info nodes " << Threads.nodes_searched()
                << " time " << Time.elapsed() << sync_endl;
            }

            else if(PVIdx + 1 == multiPV || Time.elapsed() > 3000) {
                sync_cout << UCI::pv(rootPos, rootDepth, alpha, beta) << sync_endl;
            }
        }

        if(!Signals.stop) {
            completedDepth = rootDepth;
        }

        if(!mainThread) {
            continue;
        }

        if(Limits.mate
            && bestValue >= VALUE_MATE_IN_MAX_PLY
         && VALUE_MATE - bestValue <= 2 * Limits.mate) {
            Signals.stop = true;
        }

        if(Limits.use_time_management()) {
            if(!Signals.stop && !Signals.stopOnPonderhit) {
                if(rootDepth > 4 * ONE_PLY && multiPV == 1) {
                    Time.pv_instability(mainThread->bestMoveChanges);
                }

                if(rootMoves.size() == 1
                    || Time.elapsed() > Time.available() * (mainThread->failedLow ? 641 : 315) / 640
                    || (mainThread->easyMovePlayed = (rootMoves[0].pv[0] == easyMove
                    && mainThread->bestMoveChanges < 0.03
                && Time.elapsed() > Time.available() / 8))) {
                    if(Limits.ponder) {
                        Signals.stopOnPonderhit = true;
                    }
                    else {
                        Signals.stop = true;
                    }
                }
            }

            if(rootMoves[0].pv.size() >= 3) {
                EasyMove.update(rootPos, rootMoves[0].pv);
            }
            else {
                EasyMove.clear();
            }
        }
    }

    if(!mainThread) {
        return;
    }

    if(EasyMove.stableCount < 6 || mainThread->easyMovePlayed) {
        EasyMove.clear();
    }
}


namespace {
    template <NodeType NT>
    Value search(Position& position, Stack* ss, Value alpha, Value beta, Depth depth, bool cutNode) {
        const bool RootNode = NT == Root;
        const bool PvNode = NT == PV || NT == Root;
        
        assert(-VALUE_INFINITE <= alpha && alpha < beta&& beta <= VALUE_INFINITE);
        assert(PvNode || (alpha == beta - 1));
        assert(DEPTH_ZERO < depth&& depth < DEPTH_MAX);

        Move pv[MAX_PLY + 1], quietsSearched[64];
        Undo undo[1];
        TTEntry* tte;
        Key posKey;
        Move ttMove, move, excludedMove, bestMove;
        Depth extension, newDepth, predictedDepth;
        Value bestValue;
        Value value = VALUE_ZERO;
        Value ttValue, eval, nullValue, futilityValue;
        bool ttHit, inCheck, givesCheck, singularExtensionNode, improving;
        bool isTactical, doFullDepthSearch;
        int moveCount, quietCount;

        Thread* thisThread = position.getThread();
        inCheck = bool(position.checkersTo(position.getSide()));
        moveCount = quietCount = ss->moveCount = 0;
        bestValue = -VALUE_INFINITE;
        ss->ply = (ss - 1)->ply + 1;

        if(thisThread->resetCalls.load(std::memory_order_relaxed)) {
            thisThread->resetCalls = false;
            thisThread->callsCount = 0;
        }
        if(++thisThread->callsCount > 4096) {
            for(Thread* thread : Threads) {
                thread->resetCalls = true;
            }

            check_time();
        }

        if(PvNode && thisThread->maxPly < ss->ply) {
            thisThread->maxPly = ss->ply;
        }

        if(!RootNode) {
            if(Signals.stop.load(std::memory_order_relaxed) || position.checkDraw() || ss->ply >= MAX_PLY) {
                return ss->ply >= MAX_PLY && !inCheck ? (Value)evaluate(position)
                                                      : DrawValue[position.getSide()];
            }
            alpha = std::max(-VALUE_MATE + (ss->ply), alpha);
            beta = std::min(VALUE_MATE - (ss->ply + 1), beta);

            if(alpha >= beta) {
                return alpha;
            }
        }

        assert(0 <= ss->ply && ss->ply < MAX_PLY);

        ss->currentMove = (ss + 1)->excludedMove = bestMove = NO_MOVE;
        (ss + 1)->skipEarlyPruning = false;
        (ss + 2)->killers[0] = (ss + 2)->killers[1] = NO_MOVE;

        excludedMove = ss->excludedMove;
        posKey = excludedMove ? position.getExclusionKey() : position.getPositionKey();
        tte = TT.probe(posKey, ttHit);
        ttValue = ttHit ? value_from_tt(tte->value(), ss->ply) : VALUE_NONE;
        ttMove = RootNode ? thisThread->rootMoves[thisThread->PVIdx].pv[0]
                          : ttHit ? tte->move() : NO_MOVE;

        if(!PvNode
            && ttHit
            && tte->depth() >= depth
            && ttValue != VALUE_NONE
        && (ttValue >= beta ? (tte->bound() & BOUND_LOWER) : (tte->bound() & BOUND_UPPER))) {
            ss->currentMove = ttMove;

            if(ttValue >= beta && ttMove && !checkTactical(&position, ttMove)) {
                update_stats(position, ss, ttMove, depth, nullptr, 0);
            }

            return ttValue;
        }

        if(inCheck) {
            ss->staticEval = eval = VALUE_NONE;
            goto moves_loop;
        }
        else if(ttHit) {
            if((ss->staticEval = eval = tte->eval()) == VALUE_NONE) {
                eval = ss->staticEval = (Value)evaluate(position);
            }

            if(ttValue != VALUE_NONE) {
                if(tte->bound() & (ttValue > eval ? BOUND_LOWER : BOUND_UPPER)) {
                    eval = ttValue;
                }
            }
        }
        else {
            eval = ss->staticEval =
                (ss - 1)->currentMove != NULL_MOVE ? (Value)evaluate(position)
                                                   : -(ss - 1)->staticEval + 2 * Tempo;

            tte->save(posKey, VALUE_NONE, BOUND_NONE, DEPTH_NONE, NO_MOVE, ss->staticEval, TT.generation());
        }

        if(ss->skipEarlyPruning) {
            goto moves_loop;
        }

        if(!PvNode
            && depth < 4 * ONE_PLY
            && eval + razor_margin[depth] <= alpha
         && ttMove == NO_MOVE) {
            if(depth <= ONE_PLY && eval + razor_margin[3 * ONE_PLY] <= alpha) {
                return qsearch<NonPV, false>(position, ss, alpha, beta, DEPTH_ZERO);
            }

            Value ralpha = alpha - razor_margin[depth];
            Value v = qsearch<NonPV, false>(position, ss, ralpha, ralpha + 1, DEPTH_ZERO);
            if(v <= ralpha) {
                return v;
            }
        }

        if(!RootNode
            && depth < 7 * ONE_PLY
            && eval - futility_margin(depth) >= beta
            && eval < VALUE_KNOWN_WIN  // Do not return unproven wins
         && position.checkNonPawnMaterial(position.getSide())) {
            return eval - futility_margin(depth);
        }

        if(!PvNode
            && eval >= beta
            && depth >= 2 * ONE_PLY
        && position.checkNonPawnMaterial(position.getSide())) {
            ss->currentMove = NULL_MOVE;

            assert(eval - beta >= 0);

            Depth R = ((823 + 67 * depth) / 256 + std::min((eval - beta) / valuePawnMg, 3)) * ONE_PLY;

            position.makeNullMove(undo);
            (ss + 1)->skipEarlyPruning = true;
            nullValue = depth - R < ONE_PLY ? -qsearch<NonPV, false>(position, ss + 1, -beta, -beta + 1, DEPTH_ZERO)
                : -search<NonPV>(position, ss + 1, -beta, -beta + 1, depth - R, !cutNode);
            (ss + 1)->skipEarlyPruning = false;
            position.undoNullMove(undo);

            if(nullValue >= beta) {
                if(nullValue >= VALUE_MATE_IN_MAX_PLY) {
                    nullValue = beta;
                }

                if(depth < 12 * ONE_PLY && abs(beta) < VALUE_KNOWN_WIN) {
                    return nullValue;
                }

                ss->skipEarlyPruning = true;
                Value v = depth - R < ONE_PLY ? qsearch<NonPV, false>(position, ss, beta - 1, beta, DEPTH_ZERO)
                                              : search<NonPV>(position, ss, beta - 1, beta, depth - R, false);
                ss->skipEarlyPruning = false;

                if(v >= beta) {
                    return nullValue;
                }
            }
        }

        if(!PvNode
            && depth >= 5 * ONE_PLY
        && abs(beta) < VALUE_MATE_IN_MAX_PLY) {
            Value rbeta = std::min(beta + 200, VALUE_INFINITE);
            Depth rdepth = depth - 4 * ONE_PLY;

            assert(rdepth >= ONE_PLY);
            assert((ss - 1)->currentMove != NO_MOVE);
            assert((ss - 1)->currentMove != NULL_MOVE);

            MovePicker mp(position, ttMove, thisThread->history, Value(pieceValue[position.getCapture()].value()));

            while((move = mp.next_move()) != NO_MOVE) {
                if(position.checkLegality(move)) {
                    ss->currentMove = move;
                    Position child = position;
                    child.makeMove(undo, move);

                    if(child.checkersTo(~child.getSide())) {
                        continue;
                    }
                    value = -search<NonPV>(child, ss + 1, -rbeta, -rbeta + 1, rdepth, !cutNode);

                    if(value >= rbeta) {
                        return value;
                    }
                }
            }
        }

        if(depth >= (PvNode ? 5 * ONE_PLY : 8 * ONE_PLY) && !ttMove
        && (PvNode || ss->staticEval + 256 >= beta)) {
            Depth d = depth - 2 * ONE_PLY - (PvNode ? DEPTH_ZERO : depth / 4);

            ss->skipEarlyPruning = true;
            search<PvNode ? PV : NonPV>(position, ss, alpha, beta, d, true);
            ss->skipEarlyPruning = false;

            tte = TT.probe(posKey, ttHit);
            ttMove = ttHit ? tte->move() : NO_MOVE;
        }

    moves_loop:

        Square prevSq = getTo((ss - 1)->currentMove);
        Move cm = thisThread->counterMoves[position.getPieceOnSquare(prevSq)][prevSq];
        const CounterMovesStats& cmh = CounterMovesHistory[position.getPieceOnSquare(prevSq)][prevSq];

        MovePicker mp(position, ttMove, depth, thisThread->history, cmh, cm, ss);

        value = bestValue;

        improving = ss->staticEval >= (ss - 2)->staticEval
            || ss->staticEval == VALUE_NONE || (ss - 2)->staticEval == VALUE_NONE;

        singularExtensionNode = !RootNode
            && depth >= 8 * ONE_PLY
            && ttMove != NO_MOVE
            && abs(ttValue) < VALUE_KNOWN_WIN
            && !excludedMove
            && (tte->bound() & BOUND_LOWER)
            && tte->depth() >= depth - 3 * ONE_PLY;

        while((move = mp.next_move()) != NO_MOVE) {
            if(!position.checkLegality(move)) continue;
            
            Position child = position;
            child.makeMove(undo, move);

            if(child.checkersTo(~child.getSide())) {
                continue;
            }
            if(move == excludedMove) {
                continue;
            }

            if(RootNode && !std::count(thisThread->rootMoves.begin() + thisThread->PVIdx,
            thisThread->rootMoves.end(), move)) {
                continue;
            }

            ss->moveCount = ++moveCount;

            if(RootNode && thisThread == Threads.main() && Time.elapsed() > 3000) {
                sync_cout << "info depth " << depth / ONE_PLY
                << " currmove " << UCI::move(move)
                << " currmovenumber " << moveCount + thisThread->PVIdx << sync_endl;
            }

            if(PvNode) {
                (ss + 1)->pv = nullptr;
            }

            extension = DEPTH_ZERO;
            isTactical = checkTactical(&position, move);

            givesCheck = position.givesCheck(move);
            if(givesCheck && position.seeSign(move) >= VALUE_ZERO) {
                extension = ONE_PLY;
            }

            if(singularExtensionNode
                && move == ttMove
                && !extension
            && position.checkLegality(move)) {
                Value rBeta = ttValue - 2 * depth / ONE_PLY;
                ss->excludedMove = move;
                ss->skipEarlyPruning = true;
                value = search<NonPV>(position, ss, rBeta - 1, rBeta, depth / 2, cutNode);
                ss->skipEarlyPruning = false;
                ss->excludedMove = NO_MOVE;

                if(value < rBeta) {
                    extension = ONE_PLY;
                }
            }

            newDepth = depth - ONE_PLY + extension;

            if(!RootNode
                && !isTactical
                && !inCheck
                && !givesCheck
                && !position.checkAdvancedPawnPush(move)
            && bestValue > VALUE_MATED_IN_MAX_PLY) {
                if(depth < 16 * ONE_PLY && moveCount >= FutilityMoveCounts[improving][depth]) {
                    continue;
                }

                if(depth <= 4 * ONE_PLY
                    && move != ss->killers[0]
                    && thisThread->history[position.getPieceOnSquare(getFrom(move))][getTo(move)] < VALUE_ZERO
                 && cmh[position.getPieceOnSquare(getFrom(move))][getTo(move)] < VALUE_ZERO) {
                    continue;
                }

                predictedDepth = newDepth - reduction<PvNode>(improving, depth, moveCount);

                if(predictedDepth < 7 * ONE_PLY) {
                    futilityValue = ss->staticEval + futility_margin(predictedDepth) + 256;

                    if(futilityValue <= alpha) {
                        bestValue = std::max(bestValue, futilityValue);
                        continue;
                    }
                }

                if(predictedDepth < 4 * ONE_PLY && position.seeSign(move) < VALUE_ZERO) {
                    continue;
                }
            }

            if(!RootNode && !position.checkLegality(move)) {
                ss->moveCount = --moveCount;
                continue;
            }

            ss->currentMove = move;

            if(depth >= 3 * ONE_PLY && moveCount > 1 && !isTactical) {
                Depth r = reduction<PvNode>(improving, depth, moveCount);

                if((!PvNode && cutNode)
                    || (thisThread->history[child.getPieceOnSquare(getTo(move))][getTo(move)] < VALUE_ZERO
                && cmh[child.getPieceOnSquare(getTo(move))][getTo(move)] <= VALUE_ZERO)) {
                    r += ONE_PLY;
                }

                if(thisThread->history[child.getPieceOnSquare(getTo(move))][getTo(move)] > VALUE_ZERO
                && cmh[child.getPieceOnSquare(getTo(move))][getTo(move)] > VALUE_ZERO) {
                    r = std::max(DEPTH_ZERO, r - ONE_PLY);
                }

                if(r && getMoveType(move) == NORMAL
                    && getPieceType(child.getPieceOnSquare(getTo(move))) != PAWN
                && child.see(fromAndTo(getTo(move), getFrom(move))) < VALUE_ZERO) {
                    r = std::max(DEPTH_ZERO, r - ONE_PLY);
                }

                Depth d = std::max(newDepth - r, ONE_PLY);

                value = -search<NonPV>(child, ss + 1, -(alpha + 1), -alpha, d, true);

                doFullDepthSearch = (value > alpha && r != DEPTH_ZERO);
            }
            else {
                doFullDepthSearch = !PvNode || moveCount > 1;
            }

            if(doFullDepthSearch) {
                value = newDepth < ONE_PLY ?
                givesCheck ? -qsearch<NonPV, true>(child, ss + 1, -(alpha + 1), -alpha, DEPTH_ZERO)
                : -qsearch<NonPV, false>(child, ss + 1, -(alpha + 1), -alpha, DEPTH_ZERO)
                : -search<NonPV>(child, ss + 1, -(alpha + 1), -alpha, newDepth, !cutNode);
            }

            if(PvNode && (moveCount == 1 || (value > alpha && (RootNode || value < beta)))) {
                (ss + 1)->pv = pv;
                (ss + 1)->pv[0] = NO_MOVE;

                value = newDepth < ONE_PLY ?
                    givesCheck ? -qsearch<PV, true>(child, ss + 1, -beta, -alpha, DEPTH_ZERO)
                    : -qsearch<PV, false>(child, ss + 1, -beta, -alpha, DEPTH_ZERO)
                    : -search<PV>(child, ss + 1, -beta, -alpha, newDepth, false);
            }

            assert(value > -VALUE_INFINITE && value < VALUE_INFINITE);

            if(Signals.stop.load(std::memory_order_relaxed)) {
                return VALUE_ZERO;
            }

            if(RootNode) {
                RootMove& rm = *std::find(thisThread->rootMoves.begin(), thisThread->rootMoves.end(), move);

                if(moveCount == 1 || value > alpha) {
                    rm.score = value;
                    rm.pv.resize(1);

                    assert((ss + 1)->pv);

                    for(Move* move = (ss + 1)->pv; *move != NO_MOVE; ++move) {
                        rm.pv.push_back(*move);
                    }

                    if(moveCount > 1 && thisThread == Threads.main()) {
                        ++static_cast<MainThread*>(thisThread)->bestMoveChanges;
                    }
                }
                else {
                    rm.score = -VALUE_INFINITE;
                }
            }

            if(value > bestValue) {
                bestValue = value;

                if(value > alpha) {
                    if(PvNode
                        && thisThread == Threads.main()
                        && EasyMove.get(child.getPositionKey())
                    && (move != EasyMove.get(child.getPositionKey()) || moveCount > 1)) {
                        EasyMove.clear();
                    }

                    bestMove = move;

                    if(PvNode && !RootNode) {
                        update_pv(ss->pv, move, (ss + 1)->pv);
                    }

                    if(PvNode && value < beta) {
                        alpha = value;
                    }
                    else {
                        assert(value >= beta);
                        break;
                    }
                }
            }

            if(!isTactical && move != bestMove && quietCount < 64) {
                quietsSearched[quietCount++] = move;
            }
        }

        if(!moveCount) {
            bestValue = excludedMove ? alpha
            : inCheck ? (-VALUE_MATE + ss->ply) : DrawValue[position.getSide()];
        }
        else if(bestMove && !checkTactical(&position, bestMove)) {
            update_stats(position, ss, bestMove, depth, quietsSearched, quietCount);
        }

        else if(depth >= 3 * ONE_PLY
            && !bestMove
            && !inCheck
            && !position.getCapture()
            && position.checkLegality((ss - 1)->currentMove)
        && position.checkLegality((ss - 2)->currentMove)) {
            Value bonus = Value((depth / ONE_PLY) * (depth / ONE_PLY) + depth / ONE_PLY - 1);
            Square prevPrevSq = getTo((ss - 2)->currentMove);
            CounterMovesStats& prevCmh = CounterMovesHistory[position.getPieceOnSquare(prevPrevSq)][prevPrevSq];
            prevCmh.update(position.getPieceOnSquare(prevSq), prevSq, bonus);
        }

        tte->save(posKey, value_to_tt(bestValue, ss->ply),
            bestValue >= beta ? BOUND_LOWER :
            PvNode && bestMove ? BOUND_EXACT : BOUND_UPPER,
            depth, bestMove, ss->staticEval, TT.generation());

        assert(bestValue > -VALUE_INFINITE && bestValue < VALUE_INFINITE);

        return bestValue;
    }

    template <NodeType NT, bool InCheck>
    Value qsearch(Position& position, Stack* ss, Value alpha, Value beta, Depth depth) {
        const bool PvNode = NT == PV;
        /*if(InCheck != bool(position.checkersTo(position.getSide()))) {
            std::cout << "InCheck is:" << InCheck << std::endl;
            std::cout << "check is:" << bool(position.checkersTo(position.getSide())) << std::endl;
            std::cout << "checkersTo\n";
            print_bb(position.checkersTo(position.getSide()));
            std::cout << "colour BB White\n";
            print_bb(position.getBitboardColour(position.getSide()));
            std::cout << "colour BB Black\n";
            print_bb(position.getBitboardColour(~position.getSide()));
            position.display();
        }*/
        assert(NT == PV || NT == NonPV);
        assert(InCheck == bool(position.checkersTo(position.getSide())));
        assert(alpha >= -VALUE_INFINITE && alpha < beta&& beta <= VALUE_INFINITE);
        assert(PvNode || (alpha == beta - 1));
        assert(depth <= DEPTH_ZERO);

        Move pv[MAX_PLY + 1];
        Undo undo[1];
        TTEntry* tte;
        Key posKey;
        Move ttMove, move, bestMove;
        Value bestValue = VALUE_ZERO;
        Value value = VALUE_ZERO;
        Value ttValue, futilityValue, futilityBase, oldAlpha;
        bool ttHit, givesCheck, evasionPrunable;
        Depth ttDepth;

        if(PvNode) {
            oldAlpha = alpha;
            (ss + 1)->pv = pv;
            ss->pv[0] = NO_MOVE;
        }

        ss->currentMove = bestMove = NO_MOVE;
        ss->ply = (ss - 1)->ply + 1;

        if(position.checkDraw() || ss->ply >= MAX_PLY) {
            return ss->ply >= MAX_PLY && !InCheck ? Value(evaluate(position))
            : DrawValue[position.getSide()];
        }

        assert(0 <= ss->ply && ss->ply < MAX_PLY);

        ttDepth = InCheck || depth >= DEPTH_QS_CHECKS ? DEPTH_QS_CHECKS : DEPTH_QS_NO_CHECKS;

        posKey = position.getPositionKey();
        tte = TT.probe(posKey, ttHit);
        ttMove = ttHit ? tte->move() : NO_MOVE;
        ttValue = ttHit ? value_from_tt(tte->value(), ss->ply) : VALUE_NONE;

        if(!PvNode
            && ttHit
            && tte->depth() >= ttDepth
            && ttValue != VALUE_NONE
        && (ttValue >= beta ? (tte->bound() & BOUND_LOWER) : (tte->bound() & BOUND_UPPER))) {
            ss->currentMove = ttMove;
            return ttValue;
        }

        if(InCheck) {
            ss->staticEval = VALUE_NONE;
            bestValue = futilityBase = -VALUE_INFINITE;
        }
        else {
            if(ttHit) {
                if((ss->staticEval = bestValue = tte->eval()) == VALUE_NONE) {
                    ss->staticEval = bestValue = Value(evaluate(position));
                }

                if(ttValue != VALUE_NONE) {
                    if(tte->bound() & (ttValue > bestValue ? BOUND_LOWER : BOUND_UPPER)) {
                        bestValue = ttValue;
                    }
                }
            }
            else {
                ss->staticEval = bestValue =
                (ss - 1)->currentMove != NULL_MOVE ? Value(evaluate(position))
                : -(ss - 1)->staticEval + 2 * Tempo;
            }

            if(bestValue >= beta) {
                if(!ttHit) {
                    tte->save(position.getPositionKey(), value_to_tt(bestValue, ss->ply), BOUND_LOWER,
                        DEPTH_NONE, NO_MOVE, ss->staticEval, TT.generation());
                }

                return bestValue;
            }

            if(PvNode && bestValue > alpha) {
                alpha = bestValue;
            }

            futilityBase = bestValue + 128;
        }

        MovePicker mp(position, ttMove, depth, position.getThread()->history, getTo((ss - 1)->currentMove));

        while((move = mp.next_move()) != NO_MOVE) {
            Position child = position;
            if(!position.checkLegality(move)) continue;

            child.makeMove(undo, move);
            if(child.checkersTo(~child.getSide())) {
                continue;
            }

            givesCheck = position.givesCheck(move);

            if(!InCheck
                && !givesCheck
                && futilityBase > -VALUE_KNOWN_WIN
            && !position.checkAdvancedPawnPush(move)) {
                assert(getMoveType(move) != ENPASSANT);

                futilityValue = futilityBase + pieceValue[position.getPieceOnSquare(getTo(move))].value();

                if(futilityValue <= alpha) {
                    bestValue = std::max(bestValue, futilityValue);
                    continue;
                }

                if(futilityBase <= alpha && position.see(move) <= VALUE_ZERO) {
                    bestValue = std::max(bestValue, futilityBase);
                    continue;
                }
            }

            evasionPrunable = InCheck && bestValue > VALUE_MATED_IN_MAX_PLY && !child.getCapture();

            if((!InCheck || evasionPrunable)
                && getMoveType(move) != PROMOTION
            && position.seeSign(move) < VALUE_ZERO) {
                continue;
            }

            ss->currentMove = move;

            value = givesCheck ? -qsearch<NT, true>(child, ss + 1, -beta, -alpha, depth - ONE_PLY)
                : -qsearch<NT, false>(child, ss + 1, -beta, -alpha, depth - ONE_PLY);

            assert(value > -VALUE_INFINITE && value < VALUE_INFINITE);

            if(value > bestValue) {
                bestValue = value;

                if(value > alpha) {
                    if(PvNode) {
                        update_pv(ss->pv, move, (ss + 1)->pv);
                    }

                    if(PvNode && value < beta) {
                        alpha = value;
                        bestMove = move;
                    }
                    else {
                        tte->save(posKey, value_to_tt(value, ss->ply), BOUND_LOWER,
                            ttDepth, move, ss->staticEval, TT.generation());

                        return value;
                    }
                }
            }
        }

        if(InCheck && bestValue == -VALUE_INFINITE) {
            return -VALUE_MATE + (ss->ply);
        }

        tte->save(posKey, value_to_tt(bestValue, ss->ply),
            PvNode && bestValue > oldAlpha ? BOUND_EXACT : BOUND_UPPER,
            ttDepth, bestMove, ss->staticEval, TT.generation());

        assert(bestValue > -VALUE_INFINITE && bestValue < VALUE_INFINITE);

        return bestValue;
    }

    Value value_to_tt(Value v, int ply) {
        assert(v != VALUE_NONE);

        return  v >= VALUE_MATE_IN_MAX_PLY ? v + ply
            : v <= VALUE_MATED_IN_MAX_PLY ? v - ply : v;
    }

    Value value_from_tt(Value v, int ply) {
        return  v == VALUE_NONE ? VALUE_NONE
            : v >= VALUE_MATE_IN_MAX_PLY ? v - ply
            : v <= VALUE_MATED_IN_MAX_PLY ? v + ply : v;
    }

    void update_pv(Move* pv, Move move, Move* childPv) {
        for(*pv++ = move; childPv && *childPv != NO_MOVE; )
            *pv++ = *childPv++;
        *pv = NO_MOVE;
    }

    void update_stats(const Position& position, Stack* ss, Move move, Depth depth, Move* quiets, int quietsCount) {
        if(ss->killers[0] != move) {
            ss->killers[1] = ss->killers[0];
            ss->killers[0] = move;
        }

        Value bonus = Value((depth / ONE_PLY) * (depth / ONE_PLY) + depth / ONE_PLY - 1);

        Square prevSq = getTo((ss - 1)->currentMove);
        CounterMovesStats& cmh = CounterMovesHistory[position.getPieceOnSquare(prevSq)][prevSq];
        Thread* thisThread = position.getThread();

        thisThread->history.update(position.getPieceOnSquare(getFrom(move)), getTo(move), bonus);

        if(position.checkLegality((ss - 1)->currentMove)) {
            thisThread->counterMoves.update(position.getPieceOnSquare(prevSq), prevSq, move);
            cmh.update(position.getPieceOnSquare(getFrom(move)), getTo(move), bonus);
        }

        for(int i = 0; i < quietsCount; ++i) {
            thisThread->history.update(position.getPieceOnSquare(getFrom(quiets[i])), getTo(quiets[i]), -bonus);

            if(position.checkLegality((ss - 1)->currentMove))
                cmh.update(position.getPieceOnSquare(getFrom(quiets[i])), getTo(quiets[i]), -bonus);
        }

        if((ss - 1)->moveCount == 1
            && !position.getCapture()
        && position.checkLegality((ss - 2)->currentMove)) {
            Square prevPrevSq = getTo((ss - 2)->currentMove);
            CounterMovesStats& prevCmh = CounterMovesHistory[position.getPieceOnSquare(prevPrevSq)][prevPrevSq];
            prevCmh.update(position.getPieceOnSquare(prevSq), prevSq, -bonus - 2 * (depth + 1) / ONE_PLY);
        }
    }

    void check_time() {
        static TimePoint lastInfoTime = now();

        int elapsed = Time.elapsed();
        TimePoint tick = Limits.startTime + elapsed;

        if(tick - lastInfoTime >= 1000) {
            lastInfoTime = tick;
            dbg_print();
        }

        if(Limits.ponder)
            return;

        if((Limits.use_time_management() && elapsed > Time.maximum() - 10)
            || (Limits.movetime && elapsed >= Limits.movetime)
            || (Limits.nodes && Threads.nodes_searched() >= Limits.nodes))
            Signals.stop = true;
    }

} // namespace

string UCI::pv(const Position& position, Depth depth, Value alpha, Value beta) {
    std::stringstream ss;
    int elapsed = Time.elapsed() + 1;
    const Search::RootMoveVector& rootMoves = position.getThread()->rootMoves;
    size_t PVIdx = position.getThread()->PVIdx;
    size_t multiPV = std::min((size_t)Options["MultiPV"], rootMoves.size());
    uint64_t nodes_searched = Threads.nodes_searched();

    for(size_t i = 0; i < multiPV; ++i) {
        bool updated = (i <= PVIdx);

        if(depth == ONE_PLY && !updated)
            continue;

        Depth d = updated ? depth : depth - ONE_PLY;
        Value v = updated ? rootMoves[i].score : rootMoves[i].previousScore;

        bool tb = TB::RootInTB && abs(v) < VALUE_MATE - MAX_PLY;
        v = tb ? TB::Score : v;

        if(ss.rdbuf()->in_avail())
            ss << "\n";

        ss << "info"
            << " depth " << d / ONE_PLY
            << " seldepth " << position.getThread()->maxPly
            << " multipv " << i + 1
            << " score " << UCI::value(v);

        if(!tb && i == PVIdx)
            ss << (v >= beta ? " lowerbound" : v <= alpha ? " upperbound" : "");

        ss << " nodes " << nodes_searched
            << " nps " << nodes_searched * 1000 / elapsed;

        if(elapsed > 1000) // Earlier makes little sense
            ss << " hashfull " << TT.hashfull();

        ss << " tbhits " << TB::Hits
            << " time " << elapsed
            << " pv";

        for(Move move : rootMoves[i].pv) {
            ss << " " << UCI::move(move);
        }
    }

    return ss.str();
}

void RootMove::insert_pv_in_tt(Position& position) {
    Undo undo[MAX_PLY], *ud = undo;
    bool ttHit;

    for(Move move : pv) {

        TTEntry* tte = TT.probe(position.getPositionKey(), ttHit);

        if(!ttHit || tte->move() != move)
            tte->save(position.getPositionKey(), VALUE_NONE, BOUND_NONE, DEPTH_NONE,
                move, VALUE_NONE, TT.generation());

        position.makeMove(ud++, move);
    }

    for(size_t i = pv.size(); i > 0; ) {
        position.undoMove(--ud, pv[--i]);
    }
}

bool RootMove::extract_ponder_from_tt(Position& position) {
    Undo undo[1];
    bool ttHit;

    assert(pv.size() == 1);

    position.makeMove(undo, pv[0]);
    TTEntry* tte = TT.probe(position.getPositionKey(), ttHit);
    position.undoMove(undo, pv[0]);

    if(ttHit) {
        Move move = tte->move();

        ExtMove moveList[MAX_MOVES];
        int moveCount = generateLegalMoves(position, moveList);

        for(int i = 0; i < moveCount; ++i) {
            if(moveList[i] == move) {
                return pv.push_back(move), true;
            }
        }
            
    }

    return false;
}