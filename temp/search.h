#pragma once

#include "defines.h"

struct SearchInfo {
    int depth, values[MAX_PLY];
    Move bestMoves[MAX_PLY], ponderMoves[MAX_PLY];
    double startTime, idealUsage, maxAlloc, maxUsage;
    int pvFactor;
};

struct PVariation {
    uint16_t line[MAX_PLY];
    int length;
};

void initSearch();
void getBestMove(Thread* threads, Position* position, Limits* limits, Move* best, Move* ponder);
void* iterativeDeepening(void* vthread);
void aspirationWindow(Thread* thread);
int search(Thread* thread, PVariation* pv, int alpha, int beta, int depth);
int qsearch(Thread* thread, PVariation* pv, int alpha, int beta);
int staticExchangeEvaluation(Position* position, Move move, int threshold);
int singularity(Thread* thread, MovePicker* mp, int ttValue, int depth, int beta);

static const int WindowDepth = 5;
static const int WindowSize = 10;
static const int WindowTimerMS = 2500;

static const int CurrmoveTimerMS = 2500;

static const int BetaPruningDepth = 8;
static const int BetaMargin = 85;

static const int NullMovePruningDepth = 2;

static const int ProbCutDepth = 5;
static const int ProbCutMargin = 80;

static const int FutilityMargin = 65;
static const int FutilityMarginNoHistory = 210;
static const int FutilityPruningDepth = 8;
static const int FutilityPruningHistoryLimit[] = { 12000, 6000 };

static const int CounterMovePruningDepth[] = { 3, 2 };
static const int CounterMoveHistoryLimit[] = { 0, -1000 };

static const int FollowUpMovePruningDepth[] = { 3, 2 };
static const int FollowUpMoveHistoryLimit[] = { -2000, -4000 };

static const int LateMovePruningDepth = 8;
static const int LateMovePruningCounts[2][9] = {
    {  0,  3,  4,  6, 10, 14, 19, 25, 31},
    {  0,  5,  7, 11, 17, 26, 36, 48, 63},
};

static const int SEEPruningDepth = 9;
static const int SEEQuietMargin = -55;
static const int SEETacticalMargin = -17;
static const int SEEPieceValues[] = {
     100,  450,  450,  675,
    1300,    0,    0,    0,
};

static const int HistexLimit = 10000;

static const int QSSeeMargin = 110;
static const int QSDeltaMargin = 150;

static const int SingularQuietLimit = 6;
static const int SingularTacticalLimit = 3;
