
#include "history.h"
#include "movegen.h"
#include "thread.h"
#include "defines.h"

void updateHistoryHeuristics(Thread* thread, Move* moves, int length, int depth) {

    int entry, bonus, colour = thread->position.getSide();
    uint16_t bestMove = moves[length - 1];

    // Extract information from last move
    uint16_t counter = thread->moveStack[thread->height - 1];
    int cmPiece = thread->pieceStack[thread->height - 1];
    int cmTo = getTo(counter);

    // Extract information from two moves ago
    uint16_t follow = thread->moveStack[thread->height - 2];
    int fmPiece = thread->pieceStack[thread->height - 2];
    int fmTo = getTo(follow);

    // Update Killer Moves (Avoid duplicates)
    if(thread->killers[thread->height][0] != bestMove) {
        thread->killers[thread->height][1] = thread->killers[thread->height][0];
        thread->killers[thread->height][0] = bestMove;
    }

    // Update Counter Moves (BestMove refutes the previous move)
    if(counter != NON_MOVE && counter != NULL_MOVE)
        thread->cmtable[!colour][cmPiece][cmTo] = bestMove;

    // If the 1st quiet move failed-high below depth 4, we don't update history tables
    // Depth 0 gives no bonus in any case
    if(length == 1 && depth <= 3) return;

    // Cap update size to avoid saturation
    bonus = MIN(depth * depth, HistoryMax);

    for(int i = 0; i < length; i++) {

        // Apply a malus until the final move
        int delta = (moves[i] == bestMove) ? bonus : -bonus;

        // Extract information from this move
        int to = getTo(moves[i]);
        int from = getFrom(moves[i]);
        int piece = pieceType(thread->board.squares[from]);

        // Update Butterfly History
        entry = thread->history[colour][from][to];
        entry += HistoryMultiplier * delta - entry * abs(delta) / HistoryDivisor;
        thread->history[colour][from][to] = entry;

        // Update Counter Move History
        if(counter != NON_MOVE && counter != NULL_MOVE) {
            entry = thread->continuation[0][cmPiece][cmTo][piece][to];
            entry += HistoryMultiplier * delta - entry * abs(delta) / HistoryDivisor;
            thread->continuation[0][cmPiece][cmTo][piece][to] = entry;
        }

        // Update Followup Move History
        if(follow != NON_MOVE && follow != NULL_MOVE) {
            entry = thread->continuation[1][fmPiece][fmTo][piece][to];
            entry += HistoryMultiplier * delta - entry * abs(delta) / HistoryDivisor;
            thread->continuation[1][fmPiece][fmTo][piece][to] = entry;
        }
    }
}

void updateKillerMoves(Thread* thread, Move move) {

    // Avoid saving the same Killer Move twice
    if(thread->killers[thread->height][0] == move) return;

    thread->killers[thread->height][1] = thread->killers[thread->height][0];
    thread->killers[thread->height][0] = move;
}


void updateCaptureHistories(Thread* thread, Move best, Move* moves, int length, int depth) {

    const int bonus = MIN(depth * depth, HistoryMax);

    for(int i = 0; i < length; i++) {

        const int to = getTo(moves[i]);
        const int from = getFrom(moves[i]);
        const int delta = moves[i] == best ? bonus : -bonus;

        int piece = pieceType(thread->board.squares[from]);
        int captured = pieceType(thread->board.squares[to]);

        if(MoveType(moves[i]) == ENPASS_MOVE) captured = PAWN;
        if(MoveType(moves[i]) == PROMOTION_MOVE) captured = PAWN;

        assert(PAWN <= piece && piece <= KING);
        assert(PAWN <= captured && captured <= QUEEN);

        int entry = thread->chistory[piece][to][captured];
        entry += HistoryMultiplier * delta - entry * abs(delta) / HistoryDivisor;
        thread->chistory[piece][to][captured] = entry;
    }
}

void getCaptureHistories(Thread* thread, Move* moves, int* scores, int start, int length) {

    static const int MVVAugment[] = { 0, 2400, 2400, 4800, 9600 };

    for(int i = start; i < start + length; i++) {

        const int to = getTo(moves[i]);
        const int from = getFrom(moves[i]);

        int piece = pieceType(thread->board.squares[from]);
        int captured = pieceType(thread->board.squares[to]);

        if(MoveType(moves[i]) == ENPASS_MOVE) captured = PAWN;
        if(MoveType(moves[i]) == PROMOTION_MOVE) captured = PAWN;

        assert(PAWN <= piece && piece <= KING);
        assert(PAWN <= captured && captured <= QUEEN);

        scores[i] = 64000 + thread->chistory[piece][to][captured];
        if(MovePromoPiece(moves[i]) == QUEEN) scores[i] += 64000;
        scores[i] += MVVAugment[captured];

        assert(scores[i] >= 0);
    }
}


void getHistory(Thread* thread, Move move, int* hist, int* cmhist, int* fmhist) {

    // Extract information from this move
    int to = getTo(move);
    int from = getFrom(move);
    int piece = pieceType(thread->board.squares[from]);

    // Extract information from last move
    uint16_t counter = thread->moveStack[thread->height - 1];
    int cmPiece = thread->pieceStack[thread->height - 1];
    int cmTo = getTo(counter);

    // Extract information from two moves ago
    uint16_t follow = thread->moveStack[thread->height - 2];
    int fmPiece = thread->pieceStack[thread->height - 2];
    int fmTo = getTo(follow);

    // Set basic Butterfly history
    *hist = thread->history[thread->board.turn][from][to];

    // Set Counter Move History if it exists
    if(counter == NON_MOVE || counter == NULL_MOVE) *cmhist = 0;
    else *cmhist = thread->continuation[0][cmPiece][cmTo][piece][to];

    // Set Followup Move History if it exists
    if(follow == NON_MOVE || follow == NULL_MOVE) *fmhist = 0;
    else *fmhist = thread->continuation[1][fmPiece][fmTo][piece][to];
}

void getHistoryScores(Thread* thread, Move* moves, int* scores, int start, int length) {

    // Extract information from last move
    uint16_t counter = thread->moveStack[thread->height - 1];
    int cmPiece = thread->pieceStack[thread->height - 1];
    int cmTo = getTo(counter);

    // Extract information from two moves ago
    uint16_t follow = thread->moveStack[thread->height - 2];
    int fmPiece = thread->pieceStack[thread->height - 2];
    int fmTo = getTo(follow);

    for(int i = start; i < start + length; i++) {

        // Extract information from this move
        int to = getTo(moves[i]);
        int from = getFrom(moves[i]);
        int piece = pieceType(thread->board.squares[from]);

        // Start with the basic Butterfly history
        scores[i] = thread->history[thread->board.turn][from][to];

        // Add Counter Move History if it exists
        if(counter != NON_MOVE && counter != NULL_MOVE)
            scores[i] += thread->continuation[0][cmPiece][cmTo][piece][to];

        // Add Followup Move History if it exists
        if(follow != NON_MOVE && follow != NULL_MOVE)
            scores[i] += thread->continuation[1][fmPiece][fmTo][piece][to];
    }
}

void getRefutationMoves(Thread* thread, Move* killer1, Move* killer2, Move* counter) {

    // Extract information from last move
    uint16_t previous = thread->moveStack[thread->height - 1];
    int cmPiece = thread->pieceStack[thread->height - 1];
    int cmTo = getTo(previous);

    // Set Killer Moves by height
    *killer1 = thread->killers[thread->height][0];
    *killer2 = thread->killers[thread->height][1];

    // Set Counter Move if one exists
    if(previous == NON_MOVE || previous == NULL_MOVE) *counter = NON_MOVE;
    else *counter = thread->cmtable[!thread->board.turn][cmPiece][cmTo];
}
