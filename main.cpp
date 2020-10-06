// Inspired by Ethereal, Stockfish, Teki, and Winglet

#include <iostream>

#include "uci.h"
#include "defines.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "tt.h"

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);

    UCI::init(Options);
    lookups::init();
    Search::init();
    Threads.init();
    TT.resize(Options["Hash"]);

    UCI::loop(argc, argv);

    Threads.exit();
    return 0;
}