#include "CommandTable.h"
#include "Hooks_DirectInput8Create.h"
#include "Hooks_SaveLoad.h"
#include "Hooks_Editor.h"
#include "Hooks_Logging.h"
#include "Hooks_Gameplay.h"
#include "Hooks_Script.h"
#include "Hooks_Animation.h"
#include "Hooks_Dialog.h"
#include "Hooks_Other.h"
#include "ThreadLocal.h"
#include "SafeWrite.h"
#include "Utilities.h"
#include "Commands_Input.h"
#include "GameAPI.h"
#include "EventManager.h"

#if RUNTIME
IDebugLog	gLog("nvse.log");

#else
IDebugLog	gLog("nvse_editor.log");

#endif
UInt32 logLevel = IDebugLog::kLevel_Message;

#if RUNTIME

// fix dinput code so it doesn't acquire the keyboard/mouse in exclusive mode
// bBackground Mouse works on startup, but when gaining focus that setting is ignored
// there's probably a better way to fix the bug but this is good enough
void PatchCoopLevel(void)
{
	SafeWrite8(0x00A227A1 + 1, 0x16);
	SafeWrite8(0x00A229CB + 1, 0x06);
	SafeWrite8(0x00A23CAD + 1, 0x06);
}

#endif

UInt32 waitForDebugger;
UInt32 createHookWindow;
UInt32 et;
UInt32 au3D;

void WaitForDebugger(void)
{
	_MESSAGE("Waiting for debugger");
	while(!IsDebuggerPresent())
	{
		Sleep(10);
	}
	Sleep(1000 * 2);
}

void NVSE_Initialize(void)
{
#ifndef _DEBUG
	__try {
#endif
		FILETIME	now;
		GetSystemTimeAsFileTime(&now);

#if RUNTIME
		UInt32 bMousePatch;
		if (GetNVSEConfigOption_UInt32("DEBUG", "EscapeMouse", &bMousePatch) && bMousePatch)
			PatchCoopLevel();
		
		_MESSAGE("NVSE runtime: initialize (version = %d.%d.%d %08X %08X%08X)",
			NVSE_VERSION_INTEGER, NVSE_VERSION_INTEGER_MINOR, NVSE_VERSION_INTEGER_BETA, RUNTIME_VERSION,
			now.dwHighDateTime, now.dwLowDateTime);
#else
		_MESSAGE("NVSE editor: initialize (version = %d.%d.%d %08X %08X%08X)",
			NVSE_VERSION_INTEGER, NVSE_VERSION_INTEGER_MINOR, NVSE_VERSION_INTEGER_BETA, EDITOR_VERSION,
			now.dwHighDateTime, now.dwLowDateTime);
#endif
		_MESSAGE("imagebase = %08X", GetModuleHandle(NULL));

#ifdef _DEBUG
		logLevel = IDebugLog::kLevel_DebugMessage;
		if (GetNVSEConfigOption_UInt32("DEBUG", "LogLevel", &logLevel) && logLevel)
			if (logLevel>IDebugLog::kLevel_DebugMessage)
				logLevel = IDebugLog::kLevel_DebugMessage;
		if (GetNVSEConfigOption_UInt32("DEBUG", "AlternateUpdate3D", &au3D) && au3D)
			alternateUpdate3D = true;
		// SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);

#if RUNTIME
		if (GetNVSEConfigOption_UInt32("RUNTIME DEBUG", "WaitForDebugger", &waitForDebugger) && waitForDebugger)
			WaitForDebugger();
		GetNVSEConfigOption_UInt32("FIXES", "EnablePrintDuringOnEquip", &s_CheckInsideOnActorEquipHook);
#else
		if (GetNVSEConfigOption_UInt32("EDITOR DEBUG", "WaitForDebugger", &waitForDebugger) && waitForDebugger)
			WaitForDebugger();
#endif

#else
		if (GetNVSEConfigOption_UInt32("RELEASE", "AlternateUpdate3D", &au3D) && au3D)
			alternateUpdate3D = true;
		if (GetNVSEConfigOption_UInt32("RELEASE", "LogLevel", &logLevel) && logLevel)
			if (logLevel>IDebugLog::kLevel_DebugMessage)
				logLevel = IDebugLog::kLevel_DebugMessage;
#endif
		_memcpy = memcpy;
		_memmove = memmove;

		gLog.SetLogLevel((IDebugLog::LogLevel)logLevel);

		MersenneTwister::init_genrand(GetTickCount());
		CommandTable::Init();

#if RUNTIME
		Commands_Input_Init();
		Hook_DirectInput8Create_Init();
		Hook_Gameplay_Init();
		Hook_SaveLoad_Init();
		Hook_Logging_Init();
		ThreadLocalData::Init();
		Hook_Script_Init();
		Hook_Animation_Init();
		OtherHooks::Hooks_Other_Init();
		EventManager::Init();

		Hook_Dialog_Init();
#endif

#if EDITOR
		Hook_Editor_Init();
		Hook_Compiler_Init();
		FixEditorFont();
		PatchDefaultCommandParser();
#if 0
		FixErrorReportBug();

		// Disable check on vanilla opcode range for use of commands as conditionals
		PatchConditionalCommands();
#endif

		// Add "string_var" as alias for "long"
		CreateTokenTypedefs();

		// Allow use of special characters '$', '[', and ']' in string params to script commands
		PatchIsAlpha();

#ifdef _DEBUG
		if (GetNVSEConfigOption_UInt32("EDITOR DEBUG", "CreateHookWindow", &createHookWindow) && createHookWindow)
			CreateHookWindow();
#endif
#endif
		FlushInstructionCache(GetCurrentProcess(), NULL, 0);

#ifndef _DEBUG
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		_ERROR("exception");
	}
#endif

	_MESSAGE("init complete");
}

extern "C" 
{
	// entrypoint
	void StartNVSE(void)
	{
		NVSE_Initialize();
	}

	BOOL WINAPI DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved)
	{
		if (dwReason == DLL_PROCESS_ATTACH)
		{
			NVSE_Initialize();
		}
		return TRUE;
	}
};
