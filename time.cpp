
#include <algorithm>
#include <cfloat>
#include <cmath>

#include "search.h"
#include "time.h"
#include "uci.h"

TimeManagement Time; // Our global time management object

namespace {
    enum TimeType { OptimumTime, MaxTime };

    const int MoveHorizon = 50;
    const double MaxRatio = 6.93;
    const double StealRatio = 0.36;

    double move_importance(int ply) {

        const double XScale = 8.27;
        const double XShift = 59.;
        const double Skew = 0.179;

        return pow((1 + exp((ply - XShift) / XScale)), -Skew) + DBL_MIN;
    }

    template<TimeType T>
    int remaining(int myTime, int movesToGo, int ply, int slowMover) {
        const double TMaxRatio = (T == OptimumTime ? 1 : MaxRatio);
        const double TStealRatio = (T == OptimumTime ? 0 : StealRatio);

        double moveImportance = (move_importance(ply) * slowMover) / 100;
        double otherMovesImportance = 0;

        for(int i = 1; i < movesToGo; ++i)
            otherMovesImportance += move_importance(ply + 2 * i);

        double ratio1 = (TMaxRatio * moveImportance) / (TMaxRatio * moveImportance + otherMovesImportance);
        double ratio2 = (moveImportance + TStealRatio * otherMovesImportance) / (moveImportance + otherMovesImportance);

        return int(myTime * std::min(ratio1, ratio2));
    }

}

void TimeManagement::init(Search::LimitsType& limits, Colour us, int ply)
{
    int minThinkingTime = Options["Minimum Thinking Time"];
    int moveOverhead = Options["Move Overhead"];
    int slowMover = Options["Slow Mover"];
    int npmsec = Options["nodestime"];

    if(npmsec) {
        if(!availableNodes) {
            availableNodes = npmsec * limits.time[us];
        }

        // Convert from millisecs to nodes
        limits.time[us] = (int)availableNodes;
        limits.inc[us] *= npmsec;
        limits.npmsec = npmsec;
    }

    startTime = limits.startTime;
    unstablePvFactor = 1;
    optimumTime = maximumTime = std::max(limits.time[us], minThinkingTime);

    const int MaxMTG = limits.movestogo ? std::min(limits.movestogo, MoveHorizon) : MoveHorizon;

    for(int hypMTG = 1; hypMTG <= MaxMTG; ++hypMTG) {
        int hypMyTime = limits.time[us]
            + limits.inc[us] * (hypMTG - 1)
            - moveOverhead * (2 + std::min(hypMTG, 40));

        hypMyTime = std::max(hypMyTime, 0);

        int t1 = minThinkingTime + remaining<OptimumTime>(hypMyTime, hypMTG, ply, slowMover);
        int t2 = minThinkingTime + remaining<MaxTime    >(hypMyTime, hypMTG, ply, slowMover);

        optimumTime = std::min(t1, optimumTime);
        maximumTime = std::min(t2, maximumTime);
    }

    if(Options["Ponder"]) {
        optimumTime += optimumTime / 4;
    }
}
