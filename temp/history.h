#pragma once

#include "defines.h"

static const int HistoryMax = 400;
static const int HistoryMultiplier = 32;
static const int HistoryDivisor = 512;

void updateHistoryHeuristics(Thread* thread, Move* moves, int length, int depth);
void updateKillerMoves(Thread* thread, Move move);

void updateCaptureHistories(Thread* thread, Move best, Move* moves, int length, int depth);
void getCaptureHistories(Thread* thread, Move* moves, int* scores, int start, int length);

void getHistory(Thread* thread, Move move, int* hist, int* cmhist, int* fmhist);
void getHistoryScores(Thread* thread, Move* moves, int* scores, int start, int length);
void getRefutationMoves(Thread* thread, Move* killer1, Move* killer2, Move* counter);
