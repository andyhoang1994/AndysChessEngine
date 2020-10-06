#include <string>

#include "board.h"
#include "evaluate.h"
#include "history.h"
#include "search.h"
#include "thread.h"
#include "transposition.h"
#include "types.h"

// Default contempt values, UCI options can set them to other values
int ContemptDrawPenalty = 0;
int ContemptComplexity  = 0;

Thread* createThreadPool(int nthreads) {

    Thread *threads = calloc(nthreads, sizeof(Thread));

    for (int i = 0; i < nthreads; i++) {

        // Offset stacks so the root position may look backwards
        threads[i].evalStack  = &(threads[i]._evalStack[STACK_OFFSET]);
        threads[i].moveStack  = &(threads[i]._moveStack[STACK_OFFSET]);
        threads[i].pieceStack = &(threads[i]._pieceStack[STACK_OFFSET]);

        // Threads will know of each other
        threads[i].index = i;
        threads[i].threads = threads;
        threads[i].nthreads = nthreads;
    }

    return threads;
}

void resetThreadPool(Thread *threads) {

    // Reset the per-thread tables, used for move ordering
    // and evaluation caching. This is needed for ucinewgame
    // calls in order to ensure a deterministic behaviour

    for (int i = 0; i < threads->nthreads; i++) {

        memset(&threads[i].evtable, 0, sizeof(EvalTable));
        memset(&threads[i].pktable, 0, sizeof(PKTable));

        memset(&threads[i].killers, 0, sizeof(KillerTable));
        memset(&threads[i].cmtable, 0, sizeof(CounterMoveTable));

        memset(&threads[i].history, 0, sizeof(HistoryTable));
        memset(&threads[i].chistory, 0, sizeof(CaptureHistoryTable));
        memset(&threads[i].continuation, 0, sizeof(ContinuationTable));
    }
}

void newSearchThreadPool(Thread *threads, Board *board, Limits *limits, SearchInfo *info) {

    // Initialize each Thread in the Thread Pool. We need a reference
    // to the UCI seach parameters, access to the timing information,
    // somewhere to store the results of each iteration by the main, and
    // our own copy of the board. Also, we reset the seach statistics

    int contempt = MakeScore(ContemptDrawPenalty + ContemptComplexity, ContemptDrawPenalty);

    for (int i = 0; i < threads->nthreads; i++) {

        threads[i].limits = limits;
        threads[i].info   = info;

        threads[i].height    = 0;
        threads[i].pknndepth = 0;

        threads[i].nodes     = 0ull;
        threads[i].tbhits    = 0ull;

        memcpy(&threads[i].board, board, sizeof(Board));
        threads[i].contempt = board->turn == WHITE ? contempt : -contempt;
    }
}

uint64_t nodesSearchedThreadPool(Thread *threads) {

    // Sum up the node counters across each Thread. Threads have
    // their own node counters to avoid true sharing the cache

    uint64_t nodes = 0ull;

    for (int i = 0; i < threads->nthreads; i++)
        nodes += threads[i].nodes;

    return nodes;
}

uint64_t tbhitsThreadPool(Thread *threads) {

    // Sum up the tbhit counters across each Thread. Threads have
    // their own tbhit counters to avoid true sharing the cache

    uint64_t tbhits = 0ull;

    for (int i = 0; i < threads->nthreads; i++)
        tbhits += threads[i].tbhits;

    return tbhits;
}
