#include <iostream>
#include <string>
#include <sstream>
#include "commands.h"

void commandLoop(int argc, char* argv[]) {
    int command = 0;
    std::string token = "";
    std::string cmd = "";

    for(int i = 1; i < argc; ++i) cmd += std::string(argv[i]) + " ";

    do {
        if(argc == 1 && !getline(std::cin, cmd)) cmd = "quit";
        std::istringstream iss(cmd);
        token.clear();
        iss >> std::skipws >> token;
        if(token == "quit" || token == "q") break;
        if(token == "help" || token == "h") std::cout << "help is on the way" << std::endl;
    }
    while(token != "quit" && argc == 1);
}