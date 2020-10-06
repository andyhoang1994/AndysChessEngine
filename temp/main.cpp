// Andy's Chess Engine.cpp : This file contains the 'main' function. Program execution begins and ends there.
// Inspired by Stockfish, Teki, and Winglet

#include <iostream>
#include "defines.h"
#include "position.h"
#include "commands.h"

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);
    lookups::init();

    commandLoop(argc, argv);

    return 0;
}