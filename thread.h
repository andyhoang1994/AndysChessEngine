#pragma once

#include <atomic>
#include <thread>

#include "evaluate.h"
#include "defines.h"
#include "position.h"
#include "search.h"
#include "tt.h"


#include <atomic>
#include <bitset>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include "movepick.h"
#include "position.h"
#include "search.h"

class Thread {
	std::thread nativeThread;
	Mutex mutex;
	ConditionVariable sleepCondition;
	bool exit, searching;

public:
	Thread();
	virtual ~Thread();
	virtual void search();
	void idle_loop();
	void start_searching(bool resume = false);
	void wait_for_search_finished();
	void wait(std::atomic_bool& b);

	size_t idx, PVIdx;
	int maxPly, callsCount;

	Position rootPos;
	Search::RootMoveVector rootMoves;
	Depth rootDepth;
	HistoryStats history;
	MovesStats counterMoves;
	Depth completedDepth;
	std::atomic_bool resetCalls;
};

struct MainThread : public Thread {
	virtual void search();

	bool easyMovePlayed, failedLow;
	double bestMoveChanges;
};

struct ThreadPool : public std::vector<Thread*> {
	void init();
	void exit();

	MainThread* main() { return static_cast<MainThread*>(at(0)); }
	void start_thinking(const Position&, const Search::LimitsType&, Search::UndoStackPtr&);
	void read_uci_options();
	int64_t nodes_searched();
};

extern ThreadPool Threads;