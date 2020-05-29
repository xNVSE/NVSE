#include "Commands_Console.h"
#include "GameAPI.h"
#include "GameForms.h"
#include "GameScript.h"
#include "StringVar.h"

bool Cmd_PrintToConsole_Execute(COMMAND_ARGS)
{
	*result = 0;
	char buffer[kMaxMessageLength];

	if (ExtractFormatStringArgs(0, buffer, paramInfo, scriptData, opcodeOffsetPtr, scriptObj, eventList, 21))
	{
		if (strlen(buffer) < 512)
		{
			*result = 1;
			Console_Print("%s", buffer);
#if defined(_DEBUG)
			_MESSAGE("%s", buffer);
#endif
		}
		else
			Console_Print("PrintToConsole >> Max length exceeded (512 chars)");
	}

	return true;
}

static Bitfield<UInt32> ModDebugStates[8];

bool ModDebugState(Script * scriptObj) 
{
	UInt8 modIndex = scriptObj->GetModIndex();
	UInt8 modBit = modIndex % 32;
	modIndex /= 32;

	return (ModDebugStates[modIndex].IsSet(1 << modBit));
};

bool Cmd_SetDebugMode_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 bEnableDebug = 0;
	UInt32 modIndexArg = 0xFFFF;

	if (!ExtractArgs(EXTRACT_ARGS, &bEnableDebug, &modIndexArg))
		return true;

	if (modIndexArg == 0xFFFF)
		modIndexArg = scriptObj->GetModIndex();

	UInt8 modIndex = modIndexArg;
	if (modIndex > 0 && modIndex < 0xFF)
	{
		UInt8 modBit = modIndex % 32;			//which bit to toggle
		UInt8 arrIdx = modIndex / 32;			//index into bitfield array
		if (bEnableDebug)
			ModDebugStates[arrIdx].Set(1 << modBit);
		else
			ModDebugStates[arrIdx].UnSet(1 << modBit);

		if (IsConsoleMode())
			Console_Print("Debug statements toggled %s for mod %02X", (bEnableDebug ? "on" : "off"), modIndexArg);
	}						

	return true;
}

bool Cmd_DebugPrint_Execute(COMMAND_ARGS)
{
	*result = 0;

	UInt8 modIndex = scriptObj->GetModIndex();
	UInt8 modBit = modIndex % 32;
	modIndex /= 32;

	if (ModDebugStates[modIndex].IsSet(1 << modBit))
		Cmd_PrintToConsole_Execute(PASS_COMMAND_ARGS);

	return true;
}

bool Cmd_GetDebugMode_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 modIndexArg = 0xFFFF;

	if (!ExtractArgs(EXTRACT_ARGS, &modIndexArg))
		return true;

	if (modIndexArg == 0xFFFF)
		modIndexArg = scriptObj->GetModIndex();

	UInt8 modIndex = modIndexArg;
	UInt8 modBit = modIndex % 32;
	modIndex /= 32;

	if (IsConsoleMode())
		Console_Print("Debug statements %s for mod %02X", ((ModDebugStates[modIndex].IsSet(1 << modBit)) ? "on" : "off"), modIndexArg);
	if (ModDebugStates[modIndex].IsSet(1 << modBit)) {
		*result = 1;
	}

	return true;
}

bool Cmd_GetConsoleEcho_Execute(COMMAND_ARGS)
{
	*result = GetConsoleEcho() ? 1 : 0;
	return true;
};

bool Cmd_SetConsoleEcho_Execute(COMMAND_ARGS)
{
	*result = GetConsoleEcho() ? 1 : 0;
	UInt32 doEcho = 0;

	if (!ExtractArgs(EXTRACT_ARGS, &doEcho))
		return true;

	SetConsoleEcho(0!=doEcho);

	return true;
};

bool Cmd_HasConsoleOutputFilename_Execute(COMMAND_ARGS)
{
	*result = ConsoleManager::HasConsoleOutputFilename();

	return true;
}

bool Cmd_GetConsoleOutputFilename_Execute(COMMAND_ARGS)
{
	*result = 0;
	AssignToStringVar(PASS_COMMAND_ARGS, ConsoleManager::GetConsoleOutputFilename());
	
	return true;
}

