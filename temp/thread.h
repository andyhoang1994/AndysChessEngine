#pragma once

#include "position.h"
#include "evalcache.h"
#include "search.h"
#include "transposition.h"
#include "defines.h"

enum {
    STACK_OFFSET = 4,
    STACK_SIZE = MAX_PLY + STACK_OFFSET
};

struct Thread {

    Position position;
    PVariation pv;
    Limits* limits;
    SearchInfo* info;

    int multiPV, values[MAX_MOVES];
    uint16_t bestMoves[MAX_MOVES];
    uint16_t ponderMoves[MAX_MOVES];

    int contempt, pknndepth;
    int depth, seldepth, height;
    uint64_t nodes, tbhits;

    int* evalStack, _evalStack[STACK_SIZE];
    uint16_t* moveStack, _moveStack[STACK_SIZE];
    int* pieceStack, _pieceStack[STACK_SIZE];

    Undo undoStack[STACK_SIZE];
    bool pknnchanged[STACK_SIZE];

    ALIGN64 EvalTable evtable;
    ALIGN64 PKTable pktable;

    ALIGN64 KillerTable killers;
    ALIGN64 CounterMoveTable cmtable;

    ALIGN64 HistoryTable history;
    ALIGN64 CaptureHistoryTable chistory;
    ALIGN64 ContinuationTable continuation;

    ALIGN64 float pknnlayer1[STACK_SIZE][PKNETWORK_LAYER1];

    int index, nthreads;
    Thread* threads;
    jmp_buf jbuffer;
};


Thread* createThreadPool(int nthreads);
void resetThreadPool(Thread* threads);
void newSearchThreadPool(Thread* threads, Position* board, Limits* limits, SearchInfo* info);
uint64_t nodesSearchedThreadPool(Thread* threads);
uint64_t tbhitsThreadPool(Thread* threads);
