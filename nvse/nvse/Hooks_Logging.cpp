#include "Hooks_Logging.h"
#include "SafeWrite.h"
#include <cstdarg>

#include "GameAPI.h"
#include "Utilities.h"

#if RUNTIME

#define kPatchSCOF 0x0071DE73
#define kStrCRLF 0x0101F520
#define kBufferSCOF 0x0071DE11

static FILE * s_errorLog = NULL;
static int ErrorLogHook(const char * fmt, const char * fmt_alt, ...)
{
	va_list	args;
	bool alt;
	if(0xFFFF < (UInt32)fmt)
	{
		va_start(args, fmt);
		alt = false;
	}
	else
	{
		va_start(args, fmt_alt);
		alt = true;
	}

	if(!alt)
	{
		vfprintf(s_errorLog, fmt, args);
	}
	else
	{
		vfprintf(s_errorLog, fmt_alt, args);
	}
	fputc('\n', s_errorLog);

#if _DEBUG
	char buf[0x1000];

	if (!alt)
		vsnprintf(buf, sizeof buf, fmt, args);
	else
		vsnprintf(buf, sizeof buf, fmt_alt, args);
	if (*(UInt32*)buf == 'IRCS')
	{
		int breakpointHere = 0;
		Console_Print("%s", buf);
	}
#endif

	
	va_end(args);

	return 0;
}

static FILE * s_havokErrorLog = NULL;
static int HavokErrorLogHook(int severity, const char * fmt, ...)
{
	va_list	args;

	va_start(args, fmt);
	vfprintf(s_havokErrorLog, fmt, args);
	fputc('\n', s_havokErrorLog);

	va_end(args);

	return 0;
}

void Hook_Logging_Init(void)
{
	UInt32	enableGameErrorLog = 0;
	UInt32	disableSCOFfixes = 0;

	if(GetNVSEConfigOption_UInt32("Logging", "EnableGameErrorLog", &enableGameErrorLog) && enableGameErrorLog)
	{
		fopen_s(&s_errorLog, "falloutnv_error.log", "w");
		fopen_s(&s_havokErrorLog, "falloutnv_havok.log", "w");

		WriteRelJump(0x005B5E40, (UInt32)ErrorLogHook);

		WriteRelCall(0x006259D3, (UInt32)HavokErrorLogHook);
		WriteRelCall(0x00625A23, (UInt32)HavokErrorLogHook);
		WriteRelCall(0x00625A63, (UInt32)HavokErrorLogHook);
		WriteRelCall(0x00625A92, (UInt32)HavokErrorLogHook);
	}

	if(!GetNVSEConfigOption_UInt32("FIXES", "DisableSCOFfixes", &disableSCOFfixes) || !disableSCOFfixes)
	{
		SafeWrite8(kPatchSCOF + 1, 1);					// Only write one char.
		SafeWrite32(kPatchSCOF + 2 + 1, kStrCRLF + 1);	// Skip \r
	}
}

#endif
