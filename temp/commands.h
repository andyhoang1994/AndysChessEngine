#ifndef COMMANDS_H
#define COMMANDS_H
	#include <string>
	#include "movegen.h"
	#include "position.h"

	std::string doCommand(const char* buff);
	void commandLoop(int argc, char* argv[]);
#endif