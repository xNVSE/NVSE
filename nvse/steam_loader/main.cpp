#include <string>
#include <direct.h>
#include "loader_common/IdentifyEXE.h"
#include "nvse/nvse_version.h"
#include "nvse/SafeWrite.h"
#include "nvse/Hooks_Memory.h"
#include <tlhelp32.h>
#include <intrin.h>

IDebugLog	gLog("nvse_steam_loader.log");

static void OnAttach();

std::string GetCWD() {
	char	cwd[4096];
	ASSERT(_getcwd(cwd, sizeof(cwd)));
	return cwd;
}

std::string GetAppPath() {
	char	appPath[4096];
	ASSERT(GetModuleFileName(GetModuleHandle(NULL), appPath, sizeof(appPath)));
	return appPath;
}

BOOL WINAPI DllMain(HANDLE procHandle, const DWORD reason, LPVOID reserved) {
	if(reason == DLL_PROCESS_ATTACH) { OnAttach(); }
	return TRUE;
}

typedef int (__stdcall *_HookedWinMain)(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
_HookedWinMain g_hookedWinMain = nullptr;
std::string g_dllPath;

int __stdcall OnHook(const HINSTANCE hInstance, const HINSTANCE hPrevInstance, const LPSTR lpCmdLine, const int nCmdShow) {
	_MESSAGE("OnHook: thread = %d", GetCurrentThreadId());

	if(!LoadLibrary(g_dllPath.c_str())) { _ERROR("couldn't load dll"); } // load main dll

	_MESSAGE("calling winmain"); // call original winmain

	const int result = g_hookedWinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);

	_MESSAGE("returned from winmain (%d)", result);

	return result;
}

void DumpThreads() {
	if (const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, GetCurrentProcessId()); snapshot != INVALID_HANDLE_VALUE) {
		THREADENTRY32	info;

		info.dwSize = sizeof(info);
		if (Thread32First(snapshot, &info)) {
			do {
				if(info.th32OwnerProcessID == GetCurrentProcessId()) {
					UInt32	eip = 0xFFFFFFFF;

					if (const HANDLE thread = OpenThread(THREAD_GET_CONTEXT, FALSE, info.th32ThreadID); thread) {
						CONTEXT ctx;
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
			} while(Thread32Next(snapshot, &info));
		}
		CloseHandle(snapshot);
	}
}

bool hookInstalled = false;

void InstallHook(void *retaddr) {
	if (hookInstalled) { return; }

	hookInstalled = true;
	_MESSAGE("InstallHook: thread = %d retaddr = %08X", GetCurrentThreadId(), retaddr);

//	DumpThreads();
	const std::string appPath = GetAppPath();
	_MESSAGE("appPath = %s", appPath.c_str());

	std::string		dllSuffix;
	ProcHookInfo	procHookInfo = { };

	if(!IdentifyEXE(appPath.c_str(), false, &dllSuffix, &procHookInfo)) { _ERROR("unknown exe"); return; }

	std::string	dllPath; // build full path to our dll

	g_dllPath = GetCWD() + "\\nvse_" + dllSuffix + ".dll";

	_MESSAGE("dll = %s", g_dllPath.c_str());

	UInt32	hookBaseAddr = procHookInfo.hookCallAddr; // hook winmain call
	UInt32 hookBaseCallAddr = *reinterpret_cast<UInt32 *>(hookBaseAddr + 1); // UInt32	hookBaseCallAddr = *((UInt32 *)(hookBaseAddr + 1));

	hookBaseCallAddr += 5 + hookBaseAddr;	// adjust for relcall

	_MESSAGE("old winmain = %08X", hookBaseCallAddr);

	g_hookedWinMain = reinterpret_cast<_HookedWinMain>(hookBaseCallAddr); // g_hookedWinMain = (_HookedWinMain)hookBaseCallAddr;

	const auto newHookDst = reinterpret_cast<UInt32>(OnHook) - hookBaseAddr - 5; // ((UInt32)OnHook) - hookBaseAddr - 5;

	SafeWrite32(hookBaseAddr + 1, newHookDst);

	Hooks_Memory_PreloadCommit(procHookInfo.noGore);
}

typedef void (__stdcall * _GetSystemTimeAsFileTime)(LPFILETIME * fileTime);

_GetSystemTimeAsFileTime GetSystemTimeAsFileTime_Original = nullptr;
_GetSystemTimeAsFileTime * _GetSystemTimeAsFileTime_IAT = nullptr;

typedef void (__stdcall * _GetStartupInfoA)(STARTUPINFOA * startupInfo);

_GetStartupInfoA GetStartupInfoA_Original = nullptr;
_GetStartupInfoA * _GetStartupInfoA_IAT = nullptr;

void __stdcall GetSystemTimeAsFileTime_Hook(LPFILETIME * fileTime) {
	InstallHook(_ReturnAddress());
	GetSystemTimeAsFileTime_Original(fileTime);
}

void __stdcall GetStartupInfoA_Hook(STARTUPINFOA * startupInfo) {
	InstallHook(_ReturnAddress());
	GetStartupInfoA_Original(startupInfo);
}

void * GetIATAddr(UInt8 * base, const char * searchDllName, const char * searchImportName) {
	const auto *dosHeader = reinterpret_cast<IMAGE_DOS_HEADER *>(base);
	const auto *ntHeader = reinterpret_cast<IMAGE_NT_HEADERS *>(base + dosHeader->e_lfanew);
	
	const auto *importTable = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR *>(base + ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
	for(; importTable->Characteristics; ++importTable) {
		// find the DLL, if found
		if(const auto dllName = reinterpret_cast<const char *>(base + importTable->Name); !_stricmp(dllName, searchDllName)) {
			auto *thunkData = reinterpret_cast<IMAGE_THUNK_DATA *>(base + importTable->OriginalFirstThunk);
			auto *iat = reinterpret_cast<UInt32 *>(base + importTable->FirstThunk);

			for(; thunkData->u1.Ordinal; ++thunkData, ++iat) {
				if (IMAGE_SNAP_BY_ORDINAL(thunkData->u1.Ordinal)) { continue; }
				
				// found the import
				if(const auto	*importInfo = reinterpret_cast<IMAGE_IMPORT_BY_NAME *>(base + thunkData->u1.AddressOfData); !_stricmp(importInfo->Name, searchImportName)) { return iat; }
			}
			return nullptr;
		}
	}
	return nullptr;
}

static void OnAttach() {
	gLog.SetPrintLevel(IDebugLog::kLevel_Error);
	gLog.SetLogLevel(IDebugLog::kLevel_DebugMessage);

	FILETIME	now;
	GetSystemTimeAsFileTime(&now);

	_MESSAGE("nvse loader %08X (steam) %08X%08X", PACKED_NVSE_VERSION, now.dwHighDateTime, now.dwLowDateTime);

	UInt32	oldProtect;

	_GetSystemTimeAsFileTime_IAT = static_cast<_GetSystemTimeAsFileTime *>(GetIATAddr(reinterpret_cast<UInt8 *>(GetModuleHandle(nullptr)), "kernel32.dll", "GetSystemTimeAsFileTime"));
	if(_GetSystemTimeAsFileTime_IAT) {
		_MESSAGE("GetSystemTimeAsFileTime IAT = %08X", _GetSystemTimeAsFileTime_IAT);

		VirtualProtect(_GetSystemTimeAsFileTime_IAT, 4, PAGE_EXECUTE_READWRITE, &oldProtect);

		_MESSAGE("original GetSystemTimeAsFileTime = %08X", *_GetSystemTimeAsFileTime_IAT);
		GetSystemTimeAsFileTime_Original = *_GetSystemTimeAsFileTime_IAT;
		*_GetSystemTimeAsFileTime_IAT = GetSystemTimeAsFileTime_Hook;
		_MESSAGE("patched GetSystemTimeAsFileTime = %08X", *_GetSystemTimeAsFileTime_IAT);

		UInt32 junk;
		VirtualProtect(_GetSystemTimeAsFileTime_IAT, 4, oldProtect, &junk);
	}
	else { _ERROR("couldn't read IAT"); }
	
	// win8 automatically initializes the stack cookie, so the previous hook doesn't get hit
	_GetStartupInfoA_IAT = static_cast<_GetStartupInfoA *>(GetIATAddr(reinterpret_cast<UInt8 *>(GetModuleHandle(nullptr)), "kernel32.dll", "GetStartupInfoA"));
	if(_GetStartupInfoA_IAT) {
		_MESSAGE("GetStartupInfoA IAT = %08X", _GetStartupInfoA_IAT);

		VirtualProtect(_GetStartupInfoA_IAT, 4, PAGE_EXECUTE_READWRITE, &oldProtect);

		_MESSAGE("original GetStartupInfoA = %08X", *_GetStartupInfoA_IAT);
		GetStartupInfoA_Original = *_GetStartupInfoA_IAT;
		*_GetStartupInfoA_IAT = GetStartupInfoA_Hook;
		_MESSAGE("patched GetStartupInfoA = %08X", *_GetStartupInfoA_IAT);

		UInt32 junk;
		VirtualProtect(_GetStartupInfoA_IAT, 4, oldProtect, &junk);
	}
	else { _ERROR("couldn't read IAT"); }
}
