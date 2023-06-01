#include <tlhelp32.h>
#include <direct.h>
#include <string>
#include "common/IFileStream.h"
#include "loader_common/IdentifyEXE.h"
#include "loader_common/Error.h"
#include "Options.h"
#include "Inject.h"
#include "nvse/nvse_version.h"

IDebugLog	gLog("nvse_loader.log");

static bool InjectDLL(PROCESS_INFORMATION * info, const char * dllPath, ProcHookInfo * hookInfo);
static bool InjectDLLThread(PROCESS_INFORMATION * info, const char * dllPath, bool sync);
static bool DoInjectDLLThread(PROCESS_INFORMATION * info, const char * dllPath, bool sync);
static void PrintModuleInfo(UInt32 procID);

int main(int argc, char ** argv)
{
	gLog.SetPrintLevel(IDebugLog::kLevel_FatalError);
	gLog.SetLogLevel(IDebugLog::kLevel_DebugMessage);

	FILETIME	now;
	GetSystemTimeAsFileTime(&now);

	_MESSAGE("nvse loader %08X %08X%08X", PACKED_NVSE_VERSION, now.dwHighDateTime, now.dwLowDateTime);

	if(!g_options.Read(argc, argv))
	{
		PrintLoaderError("Couldn't read arguments.");
		g_options.PrintUsage();

		return 1;
	}

	if(g_options.m_optionsOnly)
	{
		g_options.PrintUsage();
		return 0;
	}

	if(g_options.m_verbose)
		gLog.SetPrintLevel(IDebugLog::kLevel_VerboseMessage);

	if(g_options.m_launchCS)
		_MESSAGE("launching editor");

	// create the process
	bool	dllHasFullPath = false;

	const char	* procName = g_options.m_launchCS ? "GECK.exe" : "FalloutNV.exe";
	const char	* baseDllName = g_options.m_launchCS ? "nvse_editor" : "nvse";

	char	currentWorkingDirectory[4096];
	ASSERT(_getcwd(currentWorkingDirectory, sizeof(currentWorkingDirectory)));

	std::string procPath = std::string(currentWorkingDirectory) + "\\" + std::string(procName);

	if(g_options.m_altEXE.size())
	{
		procPath = g_options.m_altEXE;
		_MESSAGE("launching alternate exe (%s)", procPath.c_str());
	}
	
	_MESSAGE("procPath = %s", procPath.c_str());

	{
		IFileStream	fileCheck;
		if(!fileCheck.Open(procPath.c_str()))
		{
			PrintLoaderError("Couldn't find %s.", procName);
			return 1;
		}
	}

	_MESSAGE("launching: %s (%s)", procName, procPath.c_str());

	if(g_options.m_altDLL.size())
	{
		baseDllName = g_options.m_altDLL.c_str();
		_MESSAGE("launching alternate dll (%s)", baseDllName);

		dllHasFullPath = true;
	}

	std::string		dllSuffix;
	ProcHookInfo	procHookInfo;

	if(!IdentifyEXE(procPath.c_str(), g_options.m_launchCS, &dllSuffix, &procHookInfo))
	{
		_ERROR("unknown exe");
		return 1;
	}

	_MESSAGE("hook call addr = %08X", procHookInfo.hookCallAddr);
	_MESSAGE("load lib addr = %08X", procHookInfo.loadLibAddr);

	if(g_options.m_crcOnly)
		return 0;

	// build dll path
	std::string	dllPath;
	if(dllHasFullPath)
	{
		dllPath = baseDllName;
	}
	else
	{
		dllPath = std::string(currentWorkingDirectory) + "\\" + baseDllName + "_" + dllSuffix + ".dll";
	}

	_MESSAGE("dll = %s", dllPath.c_str());

	// check to make sure the dll exists
	{
		IFileStream	tempFile;

		if(!tempFile.Open(dllPath.c_str()))
		{
			PrintLoaderError("Couldn't find xNVSE DLL (%s). Please make sure you have installed xNVSE correctly and are running it from your game folder.", dllPath.c_str());
			return 1;
		}
	}

	if(procHookInfo.procType == kProcType_Steam)
	{
		// same for standard and nogore
		char appIDStr[128];
		UInt32 appID = 22380;

		if(g_options.m_appID)
		{
			_MESSAGE("using alternate appid %d", g_options.m_appID);
			appID = g_options.m_appID;
		}

		sprintf_s(appIDStr, sizeof(appIDStr), "%d", appID);

		// set this no matter what to work around launch issues
		SetEnvironmentVariable("SteamGameId", appIDStr);

		if(g_options.m_skipLauncher)
		{
			SetEnvironmentVariable("SteamAppID", appIDStr);
		}
	}

	STARTUPINFO			startupInfo = { 0 };
	PROCESS_INFORMATION	procInfo = { 0 };

	startupInfo.cb = sizeof(startupInfo);

	if(!CreateProcess(
		procPath.c_str(),
		NULL,	// no args
		NULL,	// default process security
		NULL,	// default thread security
		TRUE,	// don't inherit handles
		CREATE_SUSPENDED,
		NULL,	// no new environment
		NULL,	// no new cwd
		&startupInfo, &procInfo))
	{
		PrintLoaderError("Launching %s failed (%d).", procPath.c_str(), GetLastError());
		return 1;
	}

	_MESSAGE("main thread id = %d", procInfo.dwThreadId);

	bool	injectionSucceeded = false;

	switch(procHookInfo.procType)
	{
		case kProcType_Steam:
			if(g_options.m_skipLauncher)
			{
				std::string	steamHookDllPath = std::string(currentWorkingDirectory) + "\\nvse_steam_loader.dll";

				if(InjectDLLThread(&procInfo, steamHookDllPath.c_str(), true))
				{
					injectionSucceeded = true;
				}
			}
			else
			{
				injectionSucceeded = true;
			}
			break;

		case kProcType_Normal:
			if(InjectDLL(&procInfo, dllPath.c_str(), &procHookInfo))
			{
				injectionSucceeded = true;
			}
			break;

		default:
			HALT("impossible");
	}

	if(!injectionSucceeded)
	{
		PrintLoaderError("Couldn't inject DLL.");

		_ERROR("terminating process");

		TerminateProcess(procInfo.hProcess, 0);
	}
	else
	{
		_MESSAGE("launching");

		ResumeThread(procInfo.hThread);

		if(g_options.m_moduleInfo)
		{
			Sleep(1000 * 3);	// wait 3 seconds

			PrintModuleInfo(procInfo.dwProcessId);
		}

		if(g_options.m_waitForClose)
			WaitForSingleObject(procInfo.hProcess, INFINITE);
	}

	CloseHandle(procInfo.hProcess);
	CloseHandle(procInfo.hThread);

	return 0;
}

static bool InjectDLL(PROCESS_INFORMATION * info, const char * dllPath, ProcHookInfo * hookInfo)
{
	bool	result = false;

	// wrap DLL injection in SEH, if it crashes print a message
	__try {
		result = DoInjectDLL(info, dllPath, hookInfo);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		PrintLoaderError("DLL injection failed. In most cases, this is caused by an overly paranoid software firewall or antivirus package. Disabling either of these may solve the problem.");
		result = false;
	}

	return result;
}

static void PrintModuleInfo(UInt32 procID)
{
	HANDLE	snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, procID);
	if(snap != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32	module;

		module.dwSize = sizeof(module);

		if(Module32First(snap, &module))
		{
			do 
			{
				_MESSAGE("%08Xx%08X %08X %s %s", module.modBaseAddr, module.modBaseSize, module.hModule, module.szModule, module.szExePath);
			}
			while(Module32Next(snap, &module));
		}
		else
		{
			_ERROR("PrintModuleInfo: Module32First failed (%d)", GetLastError());
		}

		CloseHandle(snap);
	}
	else
	{
		_ERROR("PrintModuleInfo: CreateToolhelp32Snapshot failed (%d)", GetLastError());
	}
}

static bool InjectDLLThread(PROCESS_INFORMATION * info, const char * dllPath, bool sync)
{
	bool	result = false;

	// wrap DLL injection in SEH, if it crashes print a message
	__try {
		result = DoInjectDLLThread(info, dllPath, sync);
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		PrintLoaderError("DLL injection failed. In most cases, this is caused by an overly paranoid software firewall or antivirus package. Disabling either of these may solve the problem.");
		result = false;
	}

	return result;
}

static bool DoInjectDLLThread(PROCESS_INFORMATION * info, const char * dllPath, bool sync)
{
	bool	result = false;

	HANDLE	process = OpenProcess(
		PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, info->dwProcessId);
	if(process)
	{
		UInt32	hookBase = (UInt32)VirtualAllocEx(process, NULL, 8192, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if(hookBase)
		{
			// safe because kernel32 is loaded at the same address in all processes
			// (can change across restarts)
			UInt32	loadLibraryAAddr = (UInt32)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA");

			_MESSAGE("hookBase = %08X", hookBase);
			_MESSAGE("loadLibraryAAddr = %08X", loadLibraryAAddr);

			UInt32	bytesWritten;
			WriteProcessMemory(process, (LPVOID)(hookBase + 5), dllPath, strlen(dllPath) + 1, &bytesWritten);

			UInt8	hookCode[5];

			hookCode[0] = 0xE9;
			*((UInt32 *)&hookCode[1]) = loadLibraryAAddr - (hookBase + 5);

			WriteProcessMemory(process, (LPVOID)(hookBase), hookCode, sizeof(hookCode), &bytesWritten);

			HANDLE	thread = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)hookBase, (void *)(hookBase + 5), 0, NULL);
			if(thread)
			{
				if(sync)
				{
					switch(WaitForSingleObject(thread, 1000 * 60))	// timeout = one minute
					{
						case WAIT_OBJECT_0:
							_MESSAGE("hook thread complete");
							result = true;
							break;

						case WAIT_ABANDONED:
							_ERROR("Process::InstallHook: waiting for thread = WAIT_ABANDONED");
							break;

						case WAIT_TIMEOUT:
							_ERROR("Process::InstallHook: waiting for thread = WAIT_TIMEOUT");
							break;
					}
				}
				else
					result = true;

				CloseHandle(thread);
			}
			else
				_ERROR("CreateRemoteThread failed (%d)", GetLastError());

			VirtualFreeEx(process, (LPVOID)hookBase, 0, MEM_RELEASE);
		}
		else
			_ERROR("Process::InstallHook: couldn't allocate memory in target process");

		CloseHandle(process);
	}
	else
		_ERROR("Process::InstallHook: couldn't get process handle");

	return result;
}
