#include <algorithm>
#include <cassert>

#include "movegen.h"
#include "search.h"
#include "thread.h"
#include "uci.h"

using namespace Search;

ThreadPool Threads; // Global object

Thread::Thread() {
    resetCalls = exit = false;
    maxPly = callsCount = 0;
    history.clear();
    counterMoves.clear();
    idx = Threads.size();

    std::unique_lock<Mutex> lk(mutex);
    searching = true;
    nativeThread = std::thread(&Thread::idle_loop, this);
    sleepCondition.wait(lk, [&] { return !searching; });
}

Thread::~Thread() {
    mutex.lock();
    exit = true;
    sleepCondition.notify_one();
    mutex.unlock();
    nativeThread.join();
}

void Thread::wait_for_search_finished() {
    std::unique_lock<Mutex> lk(mutex);
    sleepCondition.wait(lk, [&] { return !searching; });
}

void Thread::wait(std::atomic_bool& condition) {
    std::unique_lock<Mutex> lk(mutex);
    sleepCondition.wait(lk, [&] { return bool(condition); });
}

void Thread::start_searching(bool resume) {

    std::unique_lock<Mutex> lk(mutex);

    if(!resume)
        searching = true;

    sleepCondition.notify_one();
}

void Thread::idle_loop() {

    while(!exit) {
        std::unique_lock<Mutex> lk(mutex);

        searching = false;

        while(!searching && !exit) {
            sleepCondition.notify_one();
            sleepCondition.wait(lk);
        }

        lk.unlock();

        if(!exit) {
            search();
        }
    }
}

void ThreadPool::init() {
    push_back(new MainThread);
    read_uci_options();
}

void ThreadPool::exit() {
    while(size())
        delete back(), pop_back();
}

void ThreadPool::read_uci_options() {
    size_t requested = Options["Threads"];

    assert(requested > 0);

    while(size() < requested)
        push_back(new Thread);

    while(size() > requested)
        delete back(), pop_back();
}

int64_t ThreadPool::nodes_searched() {
    int64_t nodes = 0;
    for(Thread* th : *this)
        nodes += th->rootPos.nodes_searched();
    return nodes;
}

void ThreadPool::start_thinking(const Position& position, const LimitsType& limits,
    UndoStackPtr& states) {

    main()->wait_for_search_finished();

    Signals.stopOnPonderhit = Signals.stop = false;

    main()->rootMoves.clear();
    main()->rootPos = position;
    Limits = limits;
    if(states.get()) {
        SetupUndo = std::move(states);
        assert(!states.get());
    }

    ExtMove moveList[MAX_MOVES];
    int moveCount = generateLegalMoves(position, moveList);

    for(int i = 0; i < moveCount; ++i) {
        if(limits.searchmoves.empty()
            || std::count(limits.searchmoves.begin(), limits.searchmoves.end(), moveList[i])) {
            main()->rootMoves.push_back(RootMove(moveList[i]));
        }
    }

    main()->start_searching();
}
