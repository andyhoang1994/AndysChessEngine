
#include "defines.h"
#include "history.h"
#include "movegen.h"
#include "movepick.h"
#include "position.h"
#include "thread.h"

static Move popMove(int* size, Move* moves, int* values, int index) {
    Move popped = moves[index];
    moves[index] = moves[-- * size];
    values[index] = values[*size];
    return popped;
}

static int getBestMoveIndex(MovePicker* mp, int start, int end) {
    int best = start;

    for(int i = start + 1; i < end; i++) {
        if(mp->values[i] > mp->values[best]) {
            best = i;
        }
    }

    return best;
}


void initMovePicker(MovePicker* mp, Thread* thread, Move ttMove) {
    // Start with the table move
    mp->stage = STAGE_TABLE;
    mp->tableMove = ttMove;

    // Lookup our refutations (killers and counter moves)
    getRefutationMoves(thread, &mp->killer1, &mp->killer2, &mp->counter);

    // General housekeeping
    mp->threshold = 0;
    mp->thread = thread;
    mp->type = NORMAL_PICKER;
}

void initSingularMovePicker(MovePicker* mp, Thread* thread, Move ttMove) {
    // Simply skip over the TT move
    initMovePicker(mp, thread, ttMove);
    mp->stage = STAGE_GENERATE_TACTICAL;

}

void initTacticalMovePicker(MovePicker* mp, Thread* thread, int threshold) {

    // Start with just the tactical moves
    mp->stage = STAGE_GENERATE_TACTICAL;

    // Skip all of the special (refutation and table) moves
    mp->tableMove = mp->killer1 = mp->killer2 = mp->counter = NON_MOVE;

    // General housekeeping
    mp->threshold = threshold;
    mp->thread = thread;
    mp->type = TACTICAL_PICKER;
}

Move selectNextMove(MovePicker* mp, Position* position, bool skipQuiets) {

    int best; uint16_t bestMove;

    switch(mp->stage) {

    case STAGE_TABLE:

        // Play table move if it is pseudo legal
        mp->stage = STAGE_GENERATE_TACTICAL;
        if(checkPseudoLegal(*position, mp->tableMove))
            return mp->tableMove;

        /* fallthrough */

    case STAGE_GENERATE_TACTICAL:

        // Generate and evaluate tactical moves. mp->split sets a break point
        // to seperate the tactical from the quiet moves, so that we can skip
        // some of the tactical moves during STAGE_GOOD_TACTICAL and return later
        mp->tacticalSize = mp->split = generateTacticalMoves(*position, mp->moves);
        getCaptureHistories(mp->thread, mp->moves, mp->values, 0, mp->tacticalSize);
        mp->stage = STAGE_GOOD_TACTICAL;

        /* fallthrough */

    case STAGE_GOOD_TACTICAL:

        // Check to see if there are still more tactical moves
        if(mp->tacticalSize) {

            // Grab the next best move index
            best = getBestMoveIndex(mp, 0, mp->tacticalSize);

            // Values below zero are flagged as failing an SEE (bad tactical)
            if(mp->values[best] >= 0) {

                // Skip moves which fail to beat our SEE margin. We flag those moves
                // as failed with the value (-1), and then repeat the selection process
                if(!staticExchangeEvaluation(position, mp->moves[best], mp->threshold)) {
                    mp->values[best] = -1;
                    return selectNextMove(mp, position, skipQuiets);
                }

                // Reduce effective move list size
                bestMove = popMove(&mp->tacticalSize, mp->moves, mp->values, best);

                // Don't play the table move twice
                if(bestMove == mp->tableMove)
                    return selectNextMove(mp, position, skipQuiets);

                // Don't play the refutation moves twice
                if(bestMove == mp->killer1) mp->killer1 = NON_MOVE;
                if(bestMove == mp->killer2) mp->killer2 = NON_MOVE;
                if(bestMove == mp->counter) mp->counter = NON_MOVE;

                return bestMove;
            }
        }

        // Jump to bad tactical moves when skipping quiets
        if(skipQuiets) {
            mp->stage = STAGE_BAD_TACTICAL;
            return selectNextMove(mp, position, skipQuiets);
        }

        mp->stage = STAGE_KILLER_1;

        /* fallthrough */

    case STAGE_KILLER_1:

        // Play killer move if not yet played, and pseudo legal
        mp->stage = STAGE_KILLER_2;
        if(!skipQuiets
            && mp->killer1 != mp->tableMove
            && moveIsPseudoLegal(position, mp->killer1))
            return mp->killer1;

        /* fallthrough */

    case STAGE_KILLER_2:

        // Play killer move if not yet played, and pseudo legal
        mp->stage = STAGE_COUNTER_MOVE;
        if(!skipQuiets
            && mp->killer2 != mp->tableMove
            && moveIsPseudoLegal(position, mp->killer2))
            return mp->killer2;

        /* fallthrough */

    case STAGE_COUNTER_MOVE:

        // Play counter move if not yet played, and pseudo legal
        mp->stage = STAGE_GENERATE_QUIET;
        if(!skipQuiets
            && mp->counter != mp->tableMove
            && mp->counter != mp->killer1
            && mp->counter != mp->killer2
            && moveIsPseudoLegal(position, mp->counter))
            return mp->counter;

        /* fallthrough */

    case STAGE_GENERATE_QUIET:

        // Generate and evaluate all quiet moves when not skipping them
        if(!skipQuiets) {
            mp->quietSize = genAllQuietMoves(position, mp->moves + mp->split);
            getHistoryScores(mp->thread, mp->moves, mp->values, mp->split, mp->quietSize);
        }

        mp->stage = STAGE_QUIET;

        /* fallthrough */

    case STAGE_QUIET:

        // Check to see if there are still more quiet moves
        if(!skipQuiets && mp->quietSize) {

            // Select next best quiet and reduce the effective move list size
            best = getBestMoveIndex(mp, mp->split, mp->split + mp->quietSize) - mp->split;
            bestMove = popMove(&mp->quietSize, mp->moves + mp->split, mp->values + mp->split, best);

            // Don't play a move more than once
            if(bestMove == mp->tableMove
                || bestMove == mp->killer1
                || bestMove == mp->killer2
                || bestMove == mp->counter)
                return selectNextMove(mp, position, skipQuiets);

            return bestMove;
        }

        // Out of quiet moves, only bad quiets remain
        mp->stage = STAGE_BAD_TACTICAL;

        /* fallthrough */

    case STAGE_BAD_TACTICAL:

        // Check to see if there are still more tactical moves
        if(mp->tacticalSize && mp->type != TACTICAL_PICKER) {

            // Reduce effective move list size
            bestMove = popMove(&mp->tacticalSize, mp->moves, mp->values, 0);

            // Don't play a move more than once
            if(bestMove == mp->tableMove
                || bestMove == mp->killer1
                || bestMove == mp->killer2
                || bestMove == mp->counter)
                return selectNextMove(mp, position, skipQuiets);

            return bestMove;
        }

        mp->stage = STAGE_DONE;

        /* fallthrough */

    case STAGE_DONE:
        return NON_MOVE;

    default:
        assert(0);
        return NON_MOVE;
    }
}
