#pragma once

#include "utils.h"
#include "search.h"
#include "thread.h"

class TimeManagement {
public:
	void init(Search::LimitsType& limits, Colour us, int ply);
	void pv_instability(double bestMoveChanges) { unstablePvFactor = 1 + bestMoveChanges; }
	int available() const { return int(optimumTime * unstablePvFactor * 1.016); }
	int maximum() const { return maximumTime; }
	int elapsed() const { return int(Search::Limits.npmsec ? Threads.nodes_searched() : now() - startTime); }

	int64_t availableNodes;

private:
	TimePoint startTime;
	int optimumTime;
	int maximumTime;
	double unstablePvFactor;
};

extern TimeManagement Time;