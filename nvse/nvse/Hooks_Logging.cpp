#include "Hooks_Logging.h"
#include "SafeWrite.h"
#include <cstdarg>
#include <unordered_set>

#include "GameAPI.h"
#include "GameData.h"
#include "Hooks_Other.h"
#include "ScriptAnalyzer.h"
#include "Utilities.h"

#if RUNTIME

#define kPatchSCOF 0x0071DE73
#define kStrCRLF 0x0101F520
#define kBufferSCOF 0x0071DE11


static FILE * s_errorLog = NULL;
static int ErrorLogHook(const char * fmt, const char * fmt_alt, ...)
{
	auto& scriptContext = OtherHooks::g_currentScriptContext;
	const auto retnAddress = reinterpret_cast<UInt32>(_ReturnAddress());

	if (scriptContext.command && scriptContext.command->opcode == 4418 && retnAddress == 0x5E2383) // shut up Dispel
	{
		// context: Dispel returns false deliberately if called within own script effect to act as a "Return" statement
		return 0;
	}
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
	auto intRep = *(UInt32*)buf;
	if (intRep == 'IRCS' || intRep == 'ssiM' || intRep == 'liaF' || intRep == 'avnI')
	{
		int breakpointHere = 0;
		Console_Print("%s", buf);
	}
#endif

	if (g_warnScriptErrors)
	{
		if (retnAddress >= 0x5E1550 && retnAddress <= 0x5E23A4) // script::runline
		{
			char buf[0x400];
			if (!alt)
				vsnprintf(buf, sizeof buf, fmt, args);
			else
				vsnprintf(buf, sizeof buf, fmt_alt, args);
			if (scriptContext.script && scriptContext.lineNumberPtr)
			{
				const auto modName = DataHandler::Get()->GetNthModName(scriptContext.script->GetModIndex());
				if (modName)
				{
					const static auto noWarnModules = { "FalloutNV.esm", "DeadMoney.esm", "HonestHearts.esm", "CaravanPack.esm", "OldWorldBlues.esm", "LonesomeRoad.esm", "GunRunnersArsenal.esm", "MercenaryPack.esm", "ClassicPack.esm", "TribalPack.esm" };
					const auto isOfficial = ra::any_of(noWarnModules, _L(const char* name, _stricmp(name, modName) == 0));
					if (!isOfficial)
					{
						ScriptParsing::ScriptAnalyzer analyzer(scriptContext.script, false);
						const auto line = analyzer.ParseLine(*scriptContext.lineNumberPtr - 1);
						if (line)
						{
							ShowRuntimeError(scriptContext.script, "%s\nDecompiled Line: %s", buf, line->ToString().c_str());
						}
					}
				}
			}
		}
		
	}
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
