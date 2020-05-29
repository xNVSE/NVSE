#pragma once

#include "CommandTable.h"
#include "ParamInfos.h"

bool ModDebugState(Script * scriptObj);

DEFINE_CMD_ALT(PrintToConsole, printc, Print formatted string to console, 0, 21, kParams_FormatString)
DEFINE_CMD_ALT(DebugPrint, dbprintc, prints a formatted string to the console if debug mode is enabled for calling script, 0, 21, kParams_FormatString);
DEFINE_CMD_ALT(SetDebugMode, dbmode, toggles debug mode for all scripts belonging to a mod, 0, 2, kParams_OneInt_OneOptionalInt);
DEFINE_CMD_ALT(GetDebugMode, GetDbMode, returns whether debug mode is set for the mod, 0, 1, kParams_OneOptionalInt);
DEFINE_CMD_ALT(SetConsoleEcho, , toggles wether the engine echoes to the console and return previous state, 0, 1, kParams_OneInt);
DEFINE_CMD_ALT(GetConsoleEcho, , returns whether the engine echoes to the console, 0, 1, kParams_OneOptionalInt);

DEFINE_CMD_ALT(HasConsoleOutputFilename, HasCOF, "return if there is a Console Output Filename active", 0, 0, NULL);
DEFINE_CMD_ALT(GetConsoleOutputFilename, GetCOF, "returns the name of the Console Output Filename", 0, 0, NULL);
