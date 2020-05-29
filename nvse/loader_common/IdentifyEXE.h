#pragma once
#include <string>

enum
{
	kProcType_Steam,
	kProcType_Normal,

	kProcType_Packed,

	kProcType_Unknown
};

struct ProcHookInfo
{
	UInt64	version;
	UInt32	procType;
	UInt32	hookCallAddr;
	UInt32	loadLibAddr;
	bool	noGore;
};

bool IdentifyEXE(const char * procName, bool isEditor, std::string * dllSuffix, ProcHookInfo * hookInfo);
