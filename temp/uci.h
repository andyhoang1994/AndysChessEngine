#pragma once

#include "defines.h"
#include "position.h"
#include "thread.h"

struct Limits {
    time_ms start, time, inc, mtg, timeLimit;
    int limitedByNone, limitedByTime, limitedBySelf;
    int limitedByDepth, limitedByMoves, depthLimit, multiPV;
    Move searchMoves[MAX_MOVES], excludedMoves[MAX_MOVES];
};

struct UCIGoStruct {
    int multiPV;
    char str[512];
    Position* board;
    Thread* threads;
};

void* uciGo(void* cargo);
void uciSetOption(char* str, Thread** threads, int* multiPV, int* chess960);
void uciPosition(char* str, Position* board, int chess960);

void uciReport(Thread* threads, int alpha, int beta, int value);
void uciReportCurrentMove(Position* board, Move move, int currmove, int depth);

int strEquals(char* str1, char* str2);
int strStartsWith(char* str, char* key);
int strContains(char* str, char* key);
int getInput(char* str);
