#include "Hooks_Logging.h"
#include "SafeWrite.h"
#include <cstdarg>
#include <unordered_set>

#include "GameAPI.h"
#include "GameData.h"
#include "Hooks_Gameplay.h"
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
	auto* scriptContext = OtherHooks::GetExecutingScriptContext();
	const auto retnAddress = reinterpret_cast<UInt32>(_ReturnAddress());

	if (scriptContext && scriptContext->command && (scriptContext->command->opcode == 4418 || scriptContext->command->opcode == 4261) && retnAddress == 0x5E2383) // shut up Dispel and RemoveMe
	{
		// context: Dispel and RemoveMe return false deliberately if called within own script effect to act as a "Return" statement
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

	if (g_warnScriptErrors && scriptContext && scriptContext->script && scriptContext->curDataPtr && g_myMods.contains(scriptContext->script->GetModIndex()))
	{
		char buf[0x400];
		if (!alt)
			vsnprintf(buf, sizeof buf, fmt, args);
		else
			vsnprintf(buf, sizeof buf, fmt_alt, args);
		const auto inRunLine = _L(, retnAddress >= 0x5E1550 && retnAddress <= 0x5E23A4);
		if (inRunLine() || *reinterpret_cast<UInt32*>(buf) == 'IRCS')
		{
			const auto modName = DataHandler::Get()->GetNthModName(scriptContext->script->GetModIndex());
			if (modName)
			{
				const static auto noWarnModules = { "FalloutNV.esm", "DeadMoney.esm", "HonestHearts.esm", "CaravanPack.esm", "OldWorldBlues.esm", "LonesomeRoad.esm", "GunRunnersArsenal.esm", "MercenaryPack.esm", "ClassicPack.esm", "TribalPack.esm" };
				const auto isOfficial = ra::any_of(noWarnModules, _L(const char* name, _stricmp(name, modName) == 0));
				if (!isOfficial)
				{
					ScriptParsing::ScriptIterator iter;
					auto* lineDataStart = scriptContext->script->data + *scriptContext->curDataPtr - 4;
					const auto defaultIter = _L(,ScriptParsing::ScriptIterator(scriptContext->script, scriptContext->script->data + *scriptContext->curDataPtr - 4));
					// handles reference functions, no way to know if it's a coincidence that the opcode before it matches it or not so we have to parse the whole script.
					if (*reinterpret_cast<UInt16*>(lineDataStart - 4) == static_cast<UInt16>(ScriptParsing::ScriptStatementCode::ReferenceFunction)
						&& scriptContext->script->GetRefFromRefList(*reinterpret_cast<UInt16*>(lineDataStart - 2)))
					{
						ScriptParsing::ScriptAnalyzer analyzer(scriptContext->script);
						const auto it = ra::find_if(analyzer.lines, _L(auto& line, line->context.refIdx && line->context.startData == lineDataStart + 4));
						if (it != analyzer.lines.end())
							iter = it->get()->context;
						else
							iter = defaultIter();
					}
					else
					{
						iter = defaultIter();
					}
					if (iter.curData)
					{
						const auto line = ScriptParsing::ScriptAnalyzer::ParseLine(iter);
						if (line)
						{
							ShowRuntimeError(scriptContext->script, "%s\nDecompiled Line: %s", buf, line->ToString().c_str());
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
		std::string filemode = "w";	//write and overwrite mode
		UInt32	bAppendErrorLogs = 0;
		if (GetNVSEConfigOption_UInt32("Logging", "bAppendErrorLogs", &bAppendErrorLogs) && bAppendErrorLogs) {
			filemode = "a";	// append mode
		}
		fopen_s(&s_errorLog, "falloutnv_error.log", filemode.c_str());
		fopen_s(&s_havokErrorLog, "falloutnv_havok.log", filemode.c_str());
		if (bAppendErrorLogs)
		{
			time_t my_time = time(nullptr);
			//format: Day Month Date hh : mm:ss Year
			printf("\n%s", ctime(&my_time));
		}

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
