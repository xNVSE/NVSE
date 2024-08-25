#include <string>
#include <direct.h>
#include "loader_common/IdentifyEXE.h"
#include "nvse/nvse_version.h"
#include "nvse/SafeWrite.h"
#include "nvse/Hooks_Memory.h"
#include "common/IFileStream.h"
#include <tlhelp32.h>
#include <intrin.h>
#include "PluginPreload.h"

IDebugLog	gLog("nvse_steam_loader.log");

static void OnAttach(void);

std::string GetCWD(void)
{
	char	cwd[4096];

	ASSERT(_getcwd(cwd, sizeof(cwd)));

	return cwd;
}

std::string GetAppPath(void)
{
	char	appPath[4096];

	ASSERT(GetModuleFileName(GetModuleHandle(NULL), appPath, sizeof(appPath)));

	return appPath;
}

BOOL WINAPI DllMain(HANDLE procHandle, DWORD reason, LPVOID reserved)
{
	if(reason == DLL_PROCESS_ATTACH)
	{
		OnAttach();
	}

	return TRUE;
}

typedef int (__stdcall * _HookedWinMain)(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
_HookedWinMain g_hookedWinMain = NULL;
std::string g_dllPath;

int __stdcall OnHook(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	_MESSAGE("OnHook: thread = %d", GetCurrentThreadId());

	// load main dll
	if(!LoadLibrary(g_dllPath.c_str()))
	{
		_ERROR("couldn't load dll");
	}

	// call original winmain
	_MESSAGE("calling winmain");


	int result = g_hookedWinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);

	_MESSAGE("returned from winmain (%d)", result);

	return result;
}

void DumpThreads(void)
{
	HANDLE	snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, GetCurrentProcessId());
	if(snapshot != INVALID_HANDLE_VALUE)
	{
		THREADENTRY32	info;

		info.dwSize = sizeof(info);

		if(Thread32First(snapshot, &info))
		{
			do 
			{
				if(info.th32OwnerProcessID == GetCurrentProcessId())
				{
					UInt32	eip = 0xFFFFFFFF;

					HANDLE	thread = OpenThread(THREAD_GET_CONTEXT, FALSE, info.th32ThreadID);
					if(thread)
					{
						CONTEXT	ctx;

						ctx.ContextFlags = CONTEXT_CONTROL;

						GetThreadContext(thread, &ctx);

						eip = ctx.Eip;

						CloseHandle(thread);
					}

					_MESSAGE("thread %d pri %d eip %08X%s",
						info.th32ThreadID,
						info.tpBasePri,
						eip,
						(info.th32ThreadID == GetCurrentThreadId()) ? " current thread" : "");
				}

				info.dwSize = sizeof(info);
			}
			while(Thread32Next(snapshot, &info));
		}

		CloseHandle(snapshot);
	}
}

bool hookInstalled = false;

void InstallHook(void * retaddr)
{
	if(hookInstalled)
		return;
	else
		hookInstalled = true;

	_MESSAGE("InstallHook: thread = %d retaddr = %08X", GetCurrentThreadId(), retaddr);

//	DumpThreads();

	std::string appPath = GetAppPath();
	_MESSAGE("appPath = %s", appPath.c_str());

	std::string		dllSuffix;
	ProcHookInfo	procHookInfo;

	if(!IdentifyEXE(appPath.c_str(), false, &dllSuffix, &procHookInfo))
	{
		_ERROR("unknown exe");
		return;
	}

	// build full path to our dll
	std::string	dllPath;

	g_dllPath = GetCWD() + "\\nvse_" + dllSuffix + ".dll";

	_MESSAGE("dll = %s", g_dllPath.c_str());

	// hook winmain call
	UInt32	hookBaseAddr = procHookInfo.hookCallAddr;
	UInt32	hookBaseCallAddr = *((UInt32 *)(hookBaseAddr + 1));

	hookBaseCallAddr += 5 + hookBaseAddr;	// adjust for relcall

	_MESSAGE("old winmain = %08X", hookBaseCallAddr);

	g_hookedWinMain = (_HookedWinMain)hookBaseCallAddr;

	UInt32	newHookDst = ((UInt32)OnHook) - hookBaseAddr - 5;

	SafeWrite32(hookBaseAddr + 1, newHookDst);

	PreloadPlugins();

	Hooks_Memory_PreloadCommit(procHookInfo.noGore);
}

typedef void (__stdcall * _GetSystemTimeAsFileTime)(LPFILETIME * fileTime);

_GetSystemTimeAsFileTime GetSystemTimeAsFileTime_Original = NULL;
_GetSystemTimeAsFileTime * _GetSystemTimeAsFileTime_IAT = NULL;

typedef void (__stdcall * _GetStartupInfoA)(STARTUPINFOA * startupInfo);

_GetStartupInfoA GetStartupInfoA_Original = NULL;
_GetStartupInfoA * _GetStartupInfoA_IAT = NULL;

void __stdcall GetSystemTimeAsFileTime_Hook(LPFILETIME * fileTime)
{
	InstallHook(_ReturnAddress());

	GetSystemTimeAsFileTime_Original(fileTime);
}

void __stdcall GetStartupInfoA_Hook(STARTUPINFOA * startupInfo)
{
	InstallHook(_ReturnAddress());

	GetStartupInfoA_Original(startupInfo);
}

void * GetIATAddr(UInt8 * base, const char * searchDllName, const char * searchImportName)
{
	IMAGE_DOS_HEADER		* dosHeader = (IMAGE_DOS_HEADER *)base;
	IMAGE_NT_HEADERS		* ntHeader = (IMAGE_NT_HEADERS *)(base + dosHeader->e_lfanew);
	IMAGE_IMPORT_DESCRIPTOR	* importTable =
		(IMAGE_IMPORT_DESCRIPTOR *)(base + ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

	for(; importTable->Characteristics; ++importTable)
	{
		const char	* dllName = (const char *)(base + importTable->Name);

		if(!_stricmp(dllName, searchDllName))
		{
			// found the dll

			IMAGE_THUNK_DATA	* thunkData = (IMAGE_THUNK_DATA *)(base + importTable->OriginalFirstThunk);
			UInt32				* iat = (UInt32 *)(base + importTable->FirstThunk);

			for(; thunkData->u1.Ordinal; ++thunkData, ++iat)
			{
				if(!IMAGE_SNAP_BY_ORDINAL(thunkData->u1.Ordinal))
				{
					IMAGE_IMPORT_BY_NAME	* importInfo = (IMAGE_IMPORT_BY_NAME *)(base + thunkData->u1.AddressOfData);

					if(!_stricmp((char *)importInfo->Name, searchImportName))
					{
						// found the import
						return iat;
					}
				}
			}

			return NULL;
		}
	}

	return NULL;
}

static void OnAttach(void)
{
	gLog.SetPrintLevel(IDebugLog::kLevel_Error);
	gLog.SetLogLevel(IDebugLog::kLevel_DebugMessage);

	FILETIME	now;
	GetSystemTimeAsFileTime(&now);

	_MESSAGE("nvse loader %08X (steam) %08X%08X", PACKED_NVSE_VERSION, now.dwHighDateTime, now.dwLowDateTime);

	UInt32	oldProtect;

	_GetSystemTimeAsFileTime_IAT = (_GetSystemTimeAsFileTime *)GetIATAddr((UInt8 *)GetModuleHandle(NULL), "kernel32.dll", "GetSystemTimeAsFileTime");
	if(_GetSystemTimeAsFileTime_IAT)
	{
		_MESSAGE("GetSystemTimeAsFileTime IAT = %08X", _GetSystemTimeAsFileTime_IAT);

		VirtualProtect((void *)_GetSystemTimeAsFileTime_IAT, 4, PAGE_EXECUTE_READWRITE, &oldProtect);

		_MESSAGE("original GetSystemTimeAsFileTime = %08X", *_GetSystemTimeAsFileTime_IAT);
		GetSystemTimeAsFileTime_Original = *_GetSystemTimeAsFileTime_IAT;
		*_GetSystemTimeAsFileTime_IAT = GetSystemTimeAsFileTime_Hook;
		_MESSAGE("patched GetSystemTimeAsFileTime = %08X", *_GetSystemTimeAsFileTime_IAT);

		UInt32 junk;
		VirtualProtect((void *)_GetSystemTimeAsFileTime_IAT, 4, oldProtect, &junk);
	}
	else
	{
		_ERROR("couldn't read IAT");
	}
	
	// win8 automatically initializes the stack cookie, so the previous hook doesn't get hit
	_GetStartupInfoA_IAT = (_GetStartupInfoA *)GetIATAddr((UInt8 *)GetModuleHandle(NULL), "kernel32.dll", "GetStartupInfoA");
	if(_GetStartupInfoA_IAT)
	{
		_MESSAGE("GetStartupInfoA IAT = %08X", _GetStartupInfoA_IAT);

		VirtualProtect((void *)_GetStartupInfoA_IAT, 4, PAGE_EXECUTE_READWRITE, &oldProtect);

		_MESSAGE("original GetStartupInfoA = %08X", *_GetStartupInfoA_IAT);
		GetStartupInfoA_Original = *_GetStartupInfoA_IAT;
		*_GetStartupInfoA_IAT = GetStartupInfoA_Hook;
		_MESSAGE("patched GetStartupInfoA = %08X", *_GetStartupInfoA_IAT);

		UInt32 junk;
		VirtualProtect((void *)_GetStartupInfoA_IAT, 4, oldProtect, &junk);
	}
	else
	{
		_ERROR("couldn't read IAT");
	}
}
