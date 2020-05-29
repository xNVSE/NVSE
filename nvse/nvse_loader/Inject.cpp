#include "Inject.h"

#pragma pack (push, 1)

struct HookLayout
{
	enum
	{
		kNumLibs = 16,
		kMaxLibNameLen = MAX_PATH
	};

	struct DoLoadLibrary
	{
		UInt8	push;		// 68
		UInt32	strAddr;	// address
		UInt8	indCall1;	// FF
		UInt8	indCall2;	// 15
		UInt32	callAddr;	// address

		void	Clear(void)
		{
			// nops
			push = 0x90;
			strAddr = 0x90909090;
			indCall1 = 0x90;
			indCall2 = 0x90;
			callAddr = 0x90909090;
		}

		void	Setup(UInt32 _strAddr, UInt32 _callAddr)
		{
			push = 0x68;
			strAddr = _strAddr;
			indCall1 = 0xFF;
			indCall2 = 0x15;
			callAddr = _callAddr;
		}
	};

	// code (entry point)
	UInt8			infLoop1;		// EB
	UInt8			infLoop2;		// FF
	DoLoadLibrary	loadLib[kNumLibs];
	UInt8			callMain1;		// FF
	UInt8			callMain2;		// 25
	UInt32			callMainAddr;	// address

	// data
	char			libNames[kMaxLibNameLen * kNumLibs];
	UInt32			mainAddr;

	void	Init(ProcHookInfo * hookInfo)
	{
#if 0
		infLoop1 = 0xEB;
		infLoop2 = 0xFE;
#else
		infLoop1 = 0x90;
		infLoop2 = 0x90;
#endif

		for(UInt32 i = 0; i < kNumLibs; i++)
			loadLib[i].Clear();

		callMain1 = 0xFF;
		callMain2 = 0x25;
		callMainAddr = 0;

		memset(libNames, 0, sizeof(libNames));

		mainAddr = 0;
	}
};

#pragma pack (pop, 1)

struct HookSetup
{
	HookLayout	m_data;

	HANDLE	m_proc;
	UInt32	m_base;
	UInt32	m_loadLib;

	UInt32	m_libIdx;
	UInt32	m_strOffset;

	bool	m_isInit;

	HookSetup()
	{
		m_proc = NULL;
		m_base = 0;
		m_loadLib = 0;

		m_libIdx = 0;
		m_strOffset = 0;

		m_isInit = false;
	}

	bool	Init(PROCESS_INFORMATION * info, ProcHookInfo * hookInfo)
	{
		bool	result = false;

		if(m_isInit) return true;

		m_loadLib =				hookInfo->loadLibAddr;
		UInt32 hookBaseAddr =	hookInfo->hookCallAddr;

		m_data.Init(hookInfo);

		m_proc = OpenProcess(
			PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, info->dwProcessId);
		if(m_proc)
		{
			m_base = (UInt32)VirtualAllocEx(m_proc, NULL, sizeof(m_data), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
			if(m_base)
			{
				UInt32	hookBaseCallAddr;
				UInt32	bytesTransferred = 0;

				_MESSAGE("remote memory = %08X", m_base);

				// update the call
				if(	ReadProcessMemory(m_proc, (void *)(hookBaseAddr + 1), &hookBaseCallAddr, sizeof(hookBaseCallAddr), &bytesTransferred) &&
					(bytesTransferred == sizeof(hookBaseCallAddr)))
				{
					// adjust for relcall
					hookBaseCallAddr += 5 + hookBaseAddr;

					_MESSAGE("old winmain = %08X", hookBaseCallAddr);

					m_data.mainAddr =			hookBaseCallAddr;
					m_data.callMainAddr =		GetRemoteOffset(&m_data.mainAddr);

					UInt32	newHookDst = m_base - hookBaseAddr - 5;
					if(	WriteProcessMemory(m_proc, (void *)(hookBaseAddr + 1), &newHookDst, sizeof(newHookDst), &bytesTransferred) &&
						(bytesTransferred == sizeof(newHookDst)))
					{
						m_isInit = true;
						result = true;
					}
					else
					{
						_ERROR("couldn't write memory (update winmain)");
					}
				}
				else
				{
					_ERROR("couldn't read memory (update winmain)");
				}
			}
			else
			{
				_ERROR("couldn't allocate memory in remote process");
			}
		}
		else
		{
			_ERROR("couldn't open process");
		}

		return result;
	}

	bool	AddLoadLibrary(const char * dllPath)
	{
		bool	result = false;

		if(m_libIdx < HookLayout::kNumLibs)
		{
			HookLayout::DoLoadLibrary	* lib = &m_data.loadLib[m_libIdx];
			char						* strDst = &m_data.libNames[m_strOffset];
			m_libIdx++;

#pragma warning (push)
#pragma warning (disable : 4996)
			strcpy(strDst, dllPath);
#pragma warning (pop)

			m_strOffset += strlen(dllPath) + 1;

			lib->Setup(
				GetRemoteOffset(strDst),
				m_loadLib);

			if(UpdateRemoteProc())
			{
				result = true;
			}
		}

		return result;
	}

	UInt32	GetRemoteOffset(void * data)
	{
		return m_base + ((UInt32)data) - ((UInt32)&m_data);
	}

	bool	UpdateRemoteProc(void)
	{
		UInt32	bytesTransferred;
		return	WriteProcessMemory(m_proc, (void *)m_base, &m_data, sizeof(m_data), &bytesTransferred) &&
			(bytesTransferred == sizeof(m_data));
	}
};

HookSetup	g_hookData;

bool DoInjectDLL(PROCESS_INFORMATION * info, const char * dllPath, ProcHookInfo * hookInfo)
{
	bool	result = false;

	if(g_hookData.Init(info, hookInfo))
	{
		if(g_hookData.AddLoadLibrary(dllPath))
		{
			result = true;
		}
		else
		{
			_ERROR("couldn't add library to list");
		}
	}
	else
	{
		_ERROR("couldn't init hook");
	}

	return result;
}
