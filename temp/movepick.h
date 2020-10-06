#pragma once

#include "defines.h"
#include "position.h"
#include "thread.h"

enum PickerType {
    NORMAL_PICKER,
    TACTICAL_PICKER
};

enum Stage {
    STAGE_TABLE,
    STAGE_GENERATE_TACTICAL, STAGE_GOOD_TACTICAL,
    STAGE_KILLER_1, STAGE_KILLER_2, STAGE_COUNTER_MOVE,
    STAGE_GENERATE_QUIET, STAGE_QUIET,
    STAGE_BAD_TACTICAL,
    STAGE_DONE,
};

struct MovePicker {
    int split;
    int tacticalSize;
    int quietSize;
    Stage stage;
    PickerType type;
    int threshold;
    int values[MAX_MOVES];
    Move moves[MAX_MOVES];
    Move tableMove;
    Move killer1;
    Move killer2;
    Move counter;
    Thread* thread;
};

void initMovePicker(MovePicker* mp, Thread* thread, Move ttMove);
void initSingularMovePicker(MovePicker* mp, Thread* thread, Move ttMove);
void initTacticalMovePicker(MovePicker* mp, Thread* thread, int threshold);
Move selectNextMove(MovePicker* mp, Position* position, bool skipQuiets);
