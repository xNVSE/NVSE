#include "SafeWrite.h"
#include "Hooks_Script.h"

#include <filesystem>
#include <ranges>

#include "GameForms.h"
#include "GameScript.h"
#include "ScriptUtils.h"
#include "CommandTable.h"
#include <stack>
#include <string>
#include <fstream>
#include <iostream>
#include <regex>
#include <set>

#include "GameAPI.h"
#include "GameData.h"
#include "NVSECompiler.h"
#include "NVSECompilerUtils.h"
#include "NVSELexer.h"
#include "NVSEParser.h"
#include "NVSETreePrinter.h"
#include "NVSETypeChecker.h"
#include "PluginManager.h"

// a size of ~1KB should be enough for a single line of code
char s_ExpressionParserAltBuffer[0x500] = {0};

struct ScriptAndScriptBuffer
{
	Script* script;
	ScriptBuffer* scriptBuffer;
};

// Skip code checking for mismatched parenthesis, since: 
// 1) it can be faulty (can't handle "(" strings or similar)
// 2) the custom NVSE script processing parts should already be handling this fine (did a few quick tests).
void PatchMismatchedParenthesisCheck()
{
#if EDITOR
	WriteRelJump(0x5C933C, 0x5C934A);
#elif RUNTIME
	WriteRelJump(0x5B0A3F, 0x5B0A91);
#endif
}

#if RUNTIME
void PatchRuntimeScriptCompile();
const UInt32 ExtractStringPatchAddr = 0x005ADDCA; // ExtractArgs: follow first jz inside loop then first case of following switch: last call in case.
const UInt32 ExtractStringRetnAddr = 0x005ADDE3;

static const UInt32 kResolveRefVarPatchAddr = 0x005AC530;	  // Second jnz, just after reference to TlsData. In second to last call before main switch in ExtractArgs.
static const UInt32 kResolveNumericVarPatchAddr = 0x005A9168; //		From previous sub, third call before the end.
static const UInt32 kEndOfLineCheckPatchAddr = 0;			  // not yet supported at run-time

// incremented on each recursive call to Activate, limit of 5 hard-coded
static UInt32 *kActivationRecurseDepth = (UInt32 *)0x011CA424;

static const UInt32 kExpressionParserBufferOverflowHookAddr_1 = 0x005935D0; // find ref to aInfixtopostfixError, then enter previous call. In sub, first reference to buffer at -0X48
static const UInt32 kExpressionParserBufferOverflowRetnAddr_1 = 0x005935D7;

static const UInt32 kExpressionParserBufferOverflowHookAddr_2 = 0x005937DE; // same sub, second reference to same buffer
static const UInt32 kExpressionParserBufferOverflowRetnAddr_2 = 0x005937E5;

static const UInt32 kExtractArgsEndProcAddr = 0x005AE0FA;		   // returns true from ExtractArgs()
static const UInt32 kExtractArgsReadNumArgsPatchAddr = 0x005ACCD4; // FalloutNV uses a different sequence, see the end of this file
static const UInt32 kExtractArgsReadNumArgsRetnAddr = 0x005ACCE9;  // FalloutNV uses a different sequence, see the end of this file
static const UInt32 kExtractArgsNoArgsPatchAddr = 0x005ACD07;	   // jle kExtractArgsEndProcAddr (if num args == 0)

static const UInt32 kScriptRunner_RunHookAddr = 0x005E0D51; // Start from Script::Execute, second call after pushing "all" arguments, take 3rd call from the end (present twice)
static const UInt32 kScriptRunner_RunRetnAddr = kScriptRunner_RunHookAddr + 5;
static const UInt32 kScriptRunner_RunCallAddr = 0x00702FC0;	   // overwritten call
static const UInt32 kScriptRunner_RunEndProcAddr = 0x005E113A; // retn 0x20

bool OverrideWithExtractArgsEx(ParamInfo *paramInfo, void *scriptData, int *opcodeOffsetPtr, TESObjectREFR *thisObj, TESObjectREFR *containingObj, Script *scriptObj, ScriptEventList *eventList, ...)
{
	va_list args;
	va_start(args, eventList);
	const auto result = vExtractArgsEx(paramInfo, scriptData, reinterpret_cast<UInt32 *>(opcodeOffsetPtr), scriptObj, eventList, args, true);
#if _DEBUG
	if (!result)
		_MESSAGE("ExtractArgsEx Failed!");
#endif
	va_end(args);
	return result;
}

void __stdcall DoExtractString(ScriptEventList *eventList, char *dest, char *scriptData, UInt32 dataLen)
{
	dest[dataLen] = 0;
	if (dataLen)
	{
		memcpy(dest, scriptData, dataLen);
		if ((dataLen > 1) && (*dest == '$') && eventList && eventList->m_script)
		{
			VariableInfo *varInfo = eventList->m_script->GetVariableByName(dest + 1);
			if (varInfo)
			{
				ScriptLocal *var = eventList->GetVariable(varInfo->idx);
				if (var)
				{
					StringVar *strVar = g_StringMap.Get((int)var->data);
					if (strVar)
					{
						dataLen = strVar->GetLength();
						if (dataLen)
							memcpy(dest, strVar->GetCString(), dataLen + 1);
						else
							*dest = 0;
					}
				}
			}
		}
		if ((dataLen == 2) && (*dest == '%') && ((dest[1] | 0x20) == 'e'))
			*dest = 0;
	}
}

static __declspec(naked) void ExtractStringHook(void)
{
	// This hooks immediately before a call to memcpy().
	// Original code copies a string literal from script data to buffer
	// If string is of format $localVariableName, replace literal with contents of string var

	__asm
	{
		push	dword ptr [ebp+0x20]
		call	DoExtractString
		mov		eax, 0x5ADDE3
		jmp		eax
	}
}

static __declspec(naked) void ExpressionParserBufferOverflowHook_1(void)
{
	__asm {
		lea	edx, s_ExpressionParserAltBuffer // swap buffers
		push edx
		lea eax, [ebp - 0x54]
		jmp[kExpressionParserBufferOverflowRetnAddr_1]
	}
}

static __declspec(naked) void ExpressionParserBufferOverflowHook_2(void)
{
	__asm {
		lea eax, s_ExpressionParserAltBuffer
		push eax
		mov ecx, [ebp - 0x58]
		jmp[kExpressionParserBufferOverflowRetnAddr_2]
	}
}

bool __cdecl ExpressionEvalRunCommandHook(double* commandResult,
	UInt8* scriptData,
	UInt32* opcodeOffsetPtr,
	TESObjectREFR* ref,
	TESObjectREFR* containingObj,
	Script* scriptObj,
	ScriptEventList* scriptEventList,
	char bigEndian)
{
	// Do not use temporary stack buffer for script data when command is called from an if/set statement, instead set script data as correct ptr and opcode offset also as correct offset
	auto* _ebp = GetParentBasePtr(_AddressOfReturnAddress());

	UInt8* exprSubsetPtr = *reinterpret_cast<UInt8**>(_ebp - 0x44);
	UInt8* exprSubset = reinterpret_cast<UInt8*>(_ebp - 0x250);
	UInt32 numBytesParsed = *reinterpret_cast<UInt32*>(_ebp - 0x30);
	UInt8* pos = *reinterpret_cast<UInt8**>(_ebp + 0x8);

	*opcodeOffsetPtr = pos - scriptObj->data + (exprSubsetPtr - exprSubset - numBytesParsed);
	// deliberately not passing the stack allocated scriptData passed here but the script data from the script object
	return CdeclCall<bool>(0x5AC7A0, commandResult, scriptObj->data, opcodeOffsetPtr, ref, containingObj, scriptObj, scriptEventList, bigEndian);
}


void* g_heapManager = reinterpret_cast<void*>(0x11F6238);

size_t Heap_MemorySize(void* memory)
{
	return ThisStdCall<size_t>(0xAA44C0, g_heapManager, memory);
}

namespace RemoveScriptDataLimit
{
	void __fastcall RemoveScriptDataSizeLimit(const ScriptLineBuffer* lineBuffer, ScriptBuffer* scriptBuffer)
	{
		const size_t dataSize = Heap_MemorySize(scriptBuffer->scriptData);
		if (scriptBuffer->dataOffset + lineBuffer->dataOffset + 10 < dataSize)
			return;
		const size_t newMemSize = dataSize * 2;
		void* newMem = FormHeap_Allocate(newMemSize);
		memset(newMem, 0, newMemSize);
		memcpy(newMem, scriptBuffer->scriptData, scriptBuffer->dataOffset);
		FormHeap_Free(scriptBuffer->scriptData);
		scriptBuffer->scriptData = static_cast<UInt8*>(newMem);
	}

	__declspec(naked) void Hook()
	{
		static UInt32 const skipReturn = 0x5B0FB4;
		__asm
		{
			// edx already contains scriptBuffer
			mov		ecx, [ebp + 0x10]  // ecx = lineBuffer
			call	RemoveScriptDataSizeLimit

			// No need to set up any registers.
			jmp		skipReturn
		}
	}

	// Remove script data size limit at runtime.
	// Based off Kormakur's fix for GECK.
	// https://github.com/lStewieAl/Geck-Extender/commit/57753e9c8bc6695c02a6b9a3a06e86fd16ea589d
	void WriteHook()
	{
		WriteRelJump(0x5B0F68, UInt32(Hook));
	}
}

namespace PatchHelpCommand {
	void __cdecl HandlePrintCommands(char* searchParameter) {
		const auto* end = g_scriptCommands.GetEnd();
		for (const auto* cur = g_scriptCommands.GetStart(); cur != end; ++cur) {
			auto* parent = g_scriptCommands.GetParentPlugin(cur);

			if (!searchParameter[0] || SubStrCI(cur->helpText, searchParameter) || SubStrCI(cur->longName, searchParameter) || SubStrCI(cur->shortName, searchParameter) || (parent && SubStrCI(parent->name, searchParameter))) {
				char versionBuf[50] = "";

				if (parent) {
					const auto* updateInfo = g_scriptCommands.GetUpdateInfoForOpCode(cur->opcode);

					if (!_stricmp(parent->name, "nvse")) {
						if (updateInfo) {
							const auto ver = std::get<1>(*updateInfo);
							sprintf(versionBuf, "[%s %lu.%lu.%lu] ", parent->name, ver >> 24 & 0xFF, ver >> 16 & 0xFF, ver >> 4 & 0xFF);
						}
						else {
							sprintf(versionBuf, "[%s %lu.%lu.%lu] ", parent->name, NVSE_VERSION_INTEGER, NVSE_VERSION_INTEGER_MINOR, NVSE_VERSION_INTEGER_BETA);
						}
					}
					else {
						if (updateInfo) {
							sprintf(versionBuf, "[%s %lu] ", parent->name, std::get<1>(*updateInfo));
						}
						else {
							sprintf(versionBuf, "[%s %lu] ", parent->name, parent->version);
						}
					}
				}

				if (cur->helpText && strlen(cur->helpText)) {
					if (cur->shortName && strlen(cur->shortName)) {
						Console_Print("%s%s (%s) -> %s", versionBuf, cur->longName, cur->shortName, cur->helpText);
					}
					else {
						Console_Print("%s%s -> %s", versionBuf, cur->longName, cur->helpText);
					}
				}
				else if (cur->shortName && strlen(cur->shortName)) {
					Console_Print("%s%s (%s)", versionBuf, cur->longName, cur->shortName);
				}
				else {
					Console_Print("%s%s", versionBuf, cur->longName);
				}
			}
		}
	}

	__declspec(naked) void Hook() {
		static uint32_t rtnAddr = 0x5BCDB6;

		_asm {
			lea eax, [ebp - 0x214]
			push eax
			call HandlePrintCommands
			add esp, 4
			jmp rtnAddr
		}
	}

	void WriteHooks() {
		WriteRelJump(0x5BCBC2, reinterpret_cast<UInt32>(&Hook));
	}
}

namespace Serialization {
	char __fastcall HookScriptLocals_SaveGame(ScriptEventList *eventList, void* ecx, UInt32 varId) {
		auto* ebp = GetParentBasePtr(_AddressOfReturnAddress());
		const auto* var = *reinterpret_cast<ScriptLocal**>(ebp - 0x14);

		if (auto* script = eventList->m_script) {
			auto* varName = script->GetVariableInfo(varId)->name.CStr();

			// Skip saving of __temp vars
			if (!_strnicmp(varName, "__temp", 6)) {
				*static_cast<UInt32*>(_AddressOfReturnAddress()) = 0x5A9E91;
				return 0;
			}
		}

		return ThisStdCall<char>(0x5A9480, eventList, varId);
	}

	void WriteHooks() {
		WriteRelCall(0x5A9E11, reinterpret_cast<UInt32>(HookScriptLocals_SaveGame));
	}
}

void Hook_Script_Init()
{
	WriteRelJump(ExtractStringPatchAddr, (UInt32)&ExtractStringHook);

	// patch the "apple bug"
	// game caches information about the most recently retrieved RefVariable for the current executing script
	// if same refIdx requested twice in a row returns previously returned ref without
	// bothering to check if form stored in ref var has changed
	// this fixes it by overwriting a conditional jump with an unconditional one
	SafeWrite8(kResolveRefVarPatchAddr, 0xEB);

	// game also caches information about the most recently retrieved local numeric variable for
	// currently executing script. Causes issues with function scripts. As above, overwrite conditional jump with unconditional
	SafeWrite8(kResolveNumericVarPatchAddr, 0xEB);

	// hook code in the vanilla expression parser's subroutine to fix the buffer overflow
	WriteRelJump(kExpressionParserBufferOverflowHookAddr_1, (UInt32)&ExpressionParserBufferOverflowHook_1);
	WriteRelJump(kExpressionParserBufferOverflowHookAddr_2, (UInt32)&ExpressionParserBufferOverflowHook_2);

	// (jazzisparis) increase the string buffer size to 0x400
	SafeWrite8(0x593DB6, 7);
	SafeWrite16(0x593E5A, 0xF8B4);

	// Handle kParamType_Double in vanilla ExtractArgs
	SafeWrite8(0x5B3C30, 1);
	*(UInt16 *)0x118CF34 = 1;

	// hook ExtractArgs() to handle commands normally compiled with Cmd_Default_Parse which were instead compiled with Cmd_Expression_Parse
	ExtractArgsOverride::Init_Hooks();

#if USE_EXTRACT_ARGS_EX
	WriteRelJump(0x5ACCB0, UInt32(OverrideWithExtractArgsEx));
	// Never let commands return false and stop scripts
	//SafeWriteBuf(0x5ACBC0, "\x90\x90", 2);
#endif
	UInt32 bAllowScriptsToCrash = 1;
	GetNVSEConfigOption_UInt32("RELEASE", "bAllowScriptsToCrash", &bAllowScriptsToCrash);
	// Prevent scripts from stopping execution if a command returns false or there's a missing reference.
	if (!bAllowScriptsToCrash)
	{
		SafeWrite8(0x5E1024, 0xEB); // replace jnz with jmp
		SafeWrite8(0x5E133B, 0xEB);
	}
	
	//WriteRelJump(0x5949D4, reinterpret_cast<UInt32>(ExpressionEvalRunCommandHook));
	WriteRelCall(0x5949D4, reinterpret_cast<UInt32>(ExpressionEvalRunCommandHook));

	// Patch command output for vanilla help command to output parent plugins / associated versions
	PatchHelpCommand::WriteHooks();
	Serialization::WriteHooks();

	PatchRuntimeScriptCompile();

	PatchMismatchedParenthesisCheck();
}

#else // CS-stuff

#include "PluginManager.h"

static const UInt32 kEndOfLineCheckPatchAddr = 0x005C7EBC; // find aGameMode, down +0x1C, follow offset, follow first jle, the following jnb	(F3:0x005C16A2)
static const UInt32 kCopyStringArgHookAddr = 0x005C7A70;   // find function referencing aInvalidOwnerSF, follow the jz before the switch  67 cases, take first choice of following case (F3:0x005C038C)

static const UInt32 kBeginScriptCompilePatchAddr = 0x005C9859; // calls CompileScript()	// Find reference to aScriptDDS, then follow the 5th call below. It is the second call in this proc (F3:0x005C3099)
static const UInt32 kBeginScriptCompileCallAddr = 0x005C96E0;  // bool __fastcall CompileScript(unk, unk, Script*, ScriptBuffer*)	(F3:0x005C96E0)
static const UInt32 kBeginScriptCompileRetnAddr = 0x005C985E;  // next op. (F3:0x005C309E)

static const UInt32 kExpressionParserBufferOverflowHookAddr_1 = 0x005B1AD9; // First call before reference to aInfixToPostfix, then first reference to var_44	(F3:0x005B7049)
static const UInt32 kExpressionParserBufferOverflowRetnAddr_1 = 0x005B1ADE; // Two ops below	(F3:0x005B704E)

static const UInt32 kExpressionParserBufferOverflowHookAddr_2 = 0x005B1C6C; // Same proc, next reference to var-44	(F3:0x005B71F5)
static const UInt32 kExpressionParserBufferOverflowRetnAddr_2 = 0x005B1C73; // Two ops below also	(F3:0x005B71FC)

static __declspec(naked) void ExpressionParserBufferOverflowHook_1(void)
{
	__asm {
		lea	eax, s_ExpressionParserAltBuffer
		push eax
		jmp[kExpressionParserBufferOverflowRetnAddr_1]
	}
}

static __declspec(naked) void ExpressionParserBufferOverflowHook_2(void)
{
	__asm {
		lea	edx, s_ExpressionParserAltBuffer
		mov	byte ptr[edi], 0x20
		jmp[kExpressionParserBufferOverflowRetnAddr_2]
	}
}

#endif // RUNTIME

// Patch compiler check on end of line when calling commands from within other commands
// TODO: implement for run-time compiler
// Hook differs for 1.0/1.2 CS versions as the patched code differs
#if EDITOR

void PatchEndOfLineCheck(bool bDisableCheck)
{

	if (bDisableCheck)
		SafeWrite8(kEndOfLineCheckPatchAddr, 0xEB); // unconditional (short) jump
	else
		SafeWrite8(kEndOfLineCheckPatchAddr, 0x73); // conditional jnb (short)
}

#elif RUNTIME

void PatchEndOfLineCheck(bool bDisableCheck)
{
	if (bDisableCheck)
		SafeWrite8(0x5B3A8D, 0xEB);
	else
		SafeWrite8(0x5B3A8D, 0x73);
}

#endif

static bool s_bParsingExpression = false;

bool ParsingExpression()
{
	return s_bParsingExpression;
}

bool g_patchedEol = false;

bool ParseNestedFunction(CommandInfo *cmd, ScriptLineBuffer *lineBuf, ScriptBuffer *scriptBuf)
{
	bool patcher = false;
	// disable check for end of line
	if (!g_patchedEol)
	{
		g_patchedEol = patcher = true;
		PatchEndOfLineCheck(true);
	}

	s_bParsingExpression = true;
	bool bParsed = cmd->parse(cmd->numParams, cmd->params, lineBuf, scriptBuf);
	s_bParsingExpression = false;

	// re-enable EOL check
	if (patcher)
	{
		PatchEndOfLineCheck(false);
		g_patchedEol = false;
	}

	return bParsed;
}

static std::stack<UInt8 *> s_loopStartOffsets;

void RegisterLoopStart(UInt8 *offsetPtr)
{
	s_loopStartOffsets.push(offsetPtr);
}

bool HandleLoopEnd(UInt32 offsetToEnd)
{
	if (s_loopStartOffsets.empty())
		return false;

	UInt8 *startPtr = s_loopStartOffsets.top();
	s_loopStartOffsets.pop();

	*((UInt32 *)startPtr) = offsetToEnd;
	return true;
}


// ScriptBuffer::currentScript not used due to GECK treating it as external ref script
std::stack<Script*> g_currentScriptStack;
std::stack<std::unordered_map<std::string, UInt32>> g_currentCompilerPluginVersions;
std::stack<std::vector<size_t>> g_currentScriptRestorePoundChar;

enum PrecompileResult : uint8_t 
{
	kPrecompile_Failure,
	kPrecompile_Success,
	kPrecompile_SpecialCompile // only occurs if a plugin overtakes the compilation process.
};

static bool compilerConsoleOpened = false;
PrecompileResult __stdcall HandleBeginCompile(ScriptBuffer* buf, Script* script)
{
	// empty out the loop stack
	while (!s_loopStartOffsets.empty())
		s_loopStartOffsets.pop();

	g_currentScriptStack.push(script);
	g_currentCompilerPluginVersions.emplace();
	g_currentScriptRestorePoundChar.emplace();

	// Initialize these, just in case.
	buf->errorCode = 0;
	script->info.compiled = false;

	// See if new compiler should override script compiler
	// First token on first line should be 'name'
	if (!_strnicmp(buf->scriptText, "name", 4)) {
		CompInfo("\n========================================\n\n");
		
		// Just convert script buffer to a string
		auto program = std::string(buf->scriptText);

		NVSELexer lexer(program);
		NVSEParser parser(lexer);

		if (auto astOpt = parser.Parse(); astOpt.has_value()) {
			auto ast = std::move(astOpt.value());

			auto tc = NVSETypeChecker(&ast, script);
			bool typeCheckerPass = tc.check();
			
			auto tp = NVSETreePrinter();
			ast.Accept(&tp);

			if (!typeCheckerPass) {
				return PrecompileResult::kPrecompile_Failure;
			}

			try {
				NVSECompiler comp{script, buf->partialScript, ast};
				comp.Compile();

				// Only set script name if not partial
				if (!buf->partialScript) {
					buf->scriptName = String();
					buf->scriptName.Set(comp.scriptName.c_str());
				}

				printf("Script compiled successfully.\n");
			} catch (std::runtime_error &er) {
				CompErr("Script compilation failed: %s\n", er.what());
				return PrecompileResult::kPrecompile_Failure;
			}
		} else {
			return PrecompileResult::kPrecompile_Failure;
		}
	}

	else {
		auto& pluginVersions = g_currentCompilerPluginVersions.top();
		auto& replacements = g_currentScriptRestorePoundChar.top();

		// Default to 6.3.6 for scriptrunner unless specified
		if (buf->partialScript) {
			pluginVersions["nvse"] = MAKE_NEW_VEGAS_VERSION(6, 3, 6);
		}

		// Pre-process comments for version tags
		std::regex versionRegex(R"lit(#[Vv][Ee][Rr][Ss][Ii][Oo][Nn]\("([^"]+)",\s*(\d+)(?:,\s+(\d+))?(?:,\s+(\d+))?\))lit");
		const auto scriptText = std::string(buf->scriptText);

		auto iter = std::sregex_iterator(scriptText.begin(), scriptText.end(), versionRegex);
		while (iter != std::sregex_iterator()) {
			const auto &match = *iter;
			if (match.empty()) {
				continue;
			}

			// Change # to ; during compilation
			const auto matchPos = match.position();
			replacements.emplace_back(matchPos);
			buf->scriptText[matchPos] = ';';

			std::string pluginName = match[1];
			std::ranges::transform(pluginName.begin(), pluginName.end(), pluginName.begin(), [](unsigned char c) { return std::tolower(c); });

			if (!_stricmp(pluginName.c_str(), "nvse")) {
				int major = std::stoi(match[2]);
				int minor = match[3].matched ? std::stoi(match[3]) : 0;
				int beta = match[4].matched ? std::stoi(match[4]) : 0;

				pluginVersions[pluginName] = MAKE_NEW_VEGAS_VERSION(major, minor, beta);
			}
			else { // handle versions for plugins
				auto* pluginInfo = g_pluginManager.GetInfoByName(pluginName.c_str());
				if (!pluginInfo) [[unlikely]] {
					CompErr("Script compilation failed: No plugin with name %s could be found.\n", pluginName.c_str());
					return PrecompileResult::kPrecompile_Failure;
				}

				pluginVersions[pluginName] = std::stoi(match[2]);
			}

			++iter;
		}

		ScriptAndScriptBuffer msg{ script, buf };
		PluginManager::Dispatch_Message(0, NVSEMessagingInterface::kMessage_ScriptPrecompile,
			&msg, sizeof(ScriptAndScriptBuffer), NULL);

		if (buf->errorCode >= 1) [[unlikely]] // if plugin reports an error in compiling the script.
			return PrecompileResult::kPrecompile_Failure;
	}

	if (script->info.compiled) // if plugin compiled the script for us
	{
		// Copying some code from Script::FinalizeCompilation (runtime: 0x5AAF20, editor: 0x5C5100)
		// We can't assume ScriptBuffer has extracted anything, since compilation was done via plugin instead.
#if RUNTIME
		script->InitItem();
		script->MarkForDeletion(false);
		script->SetAltered(true);
#else
		script->Unk_1A();
		script->Unk_29(false);
		script->Unk_2A(true);
#endif

		return PrecompileResult::kPrecompile_SpecialCompile;
	}

	// Preprocess the script:
	//  - check for usage of array variables in Set statements (disallowed)
	//  - check loop structure integrity
	//  - check for use of ResetAllVariables on scripts containing string/array vars
	if (g_currentScriptStack.size() == 1)
	{
		// Avoid preprocessing a lambda inside a script, since the lambda should already be preprocessed.
		if (!PrecompileScript(buf)) [[unlikely]]
		{
			buf->errorCode = 1;
			return PrecompileResult::kPrecompile_Failure;
		}
	}
	return PrecompileResult::kPrecompile_Success;
}

void PostScriptCompile()
{
	if (!g_currentScriptStack.empty()) // could be empty here at runtime if the ScriptBuffer or Script are nullptr.
	{
		// Replace replaced ';' chars with '#' again for saving
		const auto scriptText = g_currentScriptStack.top()->text;
		for (const auto value : g_currentScriptRestorePoundChar.top()) {
			scriptText[value] = '#';
		}

		g_currentScriptStack.pop();
		g_currentCompilerPluginVersions.pop();
		g_currentScriptRestorePoundChar.pop();

		// Avoid clearing the variables map after parsing a lambda script that belongs to a parent script.
		if (g_currentScriptStack.empty())
			g_variableDefinitionsMap.clear(); // must be the parent script we just removed.
	}
}

#if RUNTIME

namespace Runtime // double-clarify
{
	PrecompileResult __fastcall HandleBeginCompile_SetNotCompiled(Script* script, ScriptBuffer* buf, bool isCompiled)
	{
		return HandleBeginCompile(buf, script);
		}

	__declspec(naked) void HookBeginScriptCompile()
	{
		const static auto retnAddr = 0x5AEBB6;
		const static auto failOrSpecialCompileAddr = 0x5AEDA0;
		__asm
		{
			mov		edx, [ebp + 0xC] //scriptBuffer
			call	HandleBeginCompile_SetNotCompiled
			test	al, al
			jz		fail
			cmp		al, kPrecompile_Success
			je		success
			// else, special compile success.
			// We want to prevent regular compilation from happening, hence skipping to the end.
			// Also need to set result to 1 (success)
			mov		al, 1

			fail:
			// fail, or a plugin custom-compiled the script
			jmp failOrSpecialCompileAddr //jump here to land in HookEndScriptCompile.

				success :
			jmp retnAddr
		}
	}

	bool __fastcall HookEndScriptCompile_Call(UInt32 success, void* edx)
		{
		if (success)
		{
			auto* ebp = GetParentBasePtr(_AddressOfReturnAddress());
			auto* buf = *reinterpret_cast<ScriptBuffer**>(ebp + 0xC);

			ScriptAndScriptBuffer data{ g_currentScriptStack.top(), buf };
			PluginManager::Dispatch_Message(0, NVSEMessagingInterface::kMessage_ScriptCompile,
				&data, sizeof(ScriptAndScriptBuffer), nullptr);
		}
		PostScriptCompile();
		return success;
	}

	__declspec(naked) void HookEndScriptCompile()
	{
		__asm
		{
			// pass the result, al.
			movzx ecx, al
			call HookEndScriptCompile_Call
			// al still contains the result since the function returns it.

			// do what we overwrote
			mov esp, ebp
			pop ebp
			ret 8
		} 
	}

	char __cdecl HandleParseCommandToken(ScriptParseToken* parseToken) {
		if (const auto* commandInfo = g_scriptCommands.GetByName(parseToken->tokenString, &g_currentCompilerPluginVersions.top())) {
			parseToken->tokenType = 'X';
			parseToken->cmdOpcode = commandInfo->opcode;
			return 1;
		}

		return 0;
	}

	__declspec(naked) void HookParseCommandToken() {
		_asm {
			add esp, 8

			push edx
			call HandleParseCommandToken
			add esp, 4

			test eax, eax
			jz ret_0

			// Jump to original return, AL still contains 1
			push 0x5B1A84
			retn

			// Skip to console command loop
			ret_0 :
			push 0x5B1A08
			retn
		}
	}
}

void PatchRuntimeScriptCompile()
{
	// Hooks a Script::SetIsCompiled call at the start of the function.
	WriteRelCall(0x5AEBB1, reinterpret_cast<UInt32>(Runtime::HookBeginScriptCompile));

	// Hooks the end (epilogue) of the function.
	WriteRelCall(0x5AEDA0, reinterpret_cast<UInt32>(Runtime::HookEndScriptCompile));

	RemoveScriptDataLimit::WriteHook();

	UInt32 bAlwaysPrintScriptCompilationError = false;
	GetNVSEConfigOption_UInt32("RELEASE", "bAlwaysPrintScriptCompilationError", &bAlwaysPrintScriptCompilationError);
	if (bAlwaysPrintScriptCompilationError)
	{
		// Replace Console_PrintIfOpen with non-conditional print, in PrintScriptCompileError. 
		WriteRelCall(0x5AEB36, (UInt32)Console_Print);
	}

	WriteRelJump(0x5B19BA, reinterpret_cast<UInt32>(&Runtime::HookParseCommandToken));
}

#endif

#if EDITOR

namespace CompilerOverride
{
	static Mode s_currentMode = kOverride_BlockType;
	static const UInt16 kOpcode_Begin = 0x0010;
	static const UInt16 kOpcode_Dot = 0x001C;

	static bool __stdcall DoCmdDefaultParseHook(UInt32 numParams, ParamInfo *params, ScriptLineBuffer *lineBuf, ScriptBuffer *scriptBuf)
	{
		// record the mode so ExtractArgs hook knows what to expect
		lineBuf->Write16(s_currentMode);

		// parse args
		ExpressionParser parser(scriptBuf, lineBuf);
		bool bResult = parser.ParseArgs(params, numParams, false);
		return bResult;
	}

	static __declspec(naked) void Hook_Cmd_Default_Parse(void)
	{
		static UInt32 numParams;
		static ParamInfo *params;
		static ScriptLineBuffer *lineBuf;
		static ScriptBuffer *scriptBuf;
		static UInt8 result;

		__asm {
			// grab args
			mov	eax, [esp + 4]
			mov[numParams], eax
			mov	eax, [esp + 8]
			mov[params], eax
			mov	eax, [esp + 0xC]
			mov[lineBuf], eax
			mov eax, [esp + 0x10]
			mov[scriptBuf], eax

				// call override parse routine
			pushad
			push scriptBuf
			push lineBuf
			push params
			push numParams
			call DoCmdDefaultParseHook

										// store result
			mov[result], al

					// clean up
			popad

				// return result
			xor eax, eax
			mov al, [result]
			retn
		}
	}

	void __stdcall ToggleOverride(bool bOverride)
	{
		static const UInt32 patchLoc = 0x005C67E0; // editor default parse routine	(g_defaultParseCommand in CommandTable)	(F3:0x005C01F0)

		// ToggleOverride() only gets invoked when we parse a begin or end statement, so set mode accordingly
		s_currentMode = kOverride_BlockType;

		// overwritten instructions
		static const UInt8 s_patchedInstructions[5] = {0x81, 0xEC, 0x30, 0x02, 0x00}; // same first five bytes as Oblivion
		if (bOverride)
		{
			WriteRelJump(patchLoc, (UInt32)&Hook_Cmd_Default_Parse);
		}
		else
		{
			for (UInt32 i = 0; i < sizeof(s_patchedInstructions); i++)
			{
				SafeWrite8(patchLoc + i, s_patchedInstructions[i]);
			}
		}
	}

	static const UInt32 stricmpAddr = 0x00C5C61A;		 // doesn't clean stack						// the proc called below	(F3:0x00BBBDCF)
	static const UInt32 stricmpPatchAddr_1 = 0x005C8D49; // stricmp(cmdInfo->longName, blockName)	// proc containing reference to aSyntaxError__2, second call above	(F3:0x005C2581)
	static const UInt32 stricmpPatchAddr_2 = 0x005C8D5D; // stricmp(cmdInfo->altName, blockName)		// same proc, first call above that same reference (F3:0x005C2595)

	// stores offset into ScriptBuffer's data buffer at which to store the jump offset for the current block
	static const UInt32 *kBeginBlockDataOffsetPtr = (const UInt32 *)0x00ED9D54; // same proc, address moved into before reference to aInvalidBlockTy (mov kBeginBlockDataOffsetPtr, ecx) (F3:0x00FC6D38)

	// target of an overwritten jump after parsing of block args
	// copies block args to ScriptBuffer after doing some bounds-checking
	static const UInt32 kCopyBlockArgsAddr = 0x005C92C9; // same proc, jump before reference to aSyntaxError__3	(F3:0x005C2D5D)

	// address of an overwritten call to a void function(void) which does nothing.
	// after ScriptLineBuffer data has been copied to ScriptBuffer and CompileLine() is about to return true
	static const UInt32 nullCallAddr = 0x005C95EB; // same proc, last call to nullsub(_3) before the endp	(F3:0x005C2E29, nullsub_19)

	// address at which the block len is calculated when compiling 'end' statement
	static const UInt32 storeBlockLenHookAddr = 0x005C8F8A; // same proc of course, 3 ops BEFORE next addr (mov ecx, [eax])	(F3:0x005C27C3)
	static const UInt32 storeBlockLenRetnAddr = 0x005C8F90; // same proc, mov kBeginBlockDataOffsetPtr, ebx after reference to aSyntaxError__3	(F3:0x005C27C9)

	static bool s_overridden; // is true if compiler override in effect

	bool IsActive()
	{
		return s_overridden;
	}

	static __declspec(naked) void CompareBlockName(void)
	{
		// edx: token
		// ecx: CommandInfo longName or altName

		__asm {
			push edx // we may be incrementing edx here, so preserve its original value
			push eax

					// first, toggle override off if it's already activated
			test[s_overridden], 1
			jz TestForUnderscore
			pushad
			push 0
			call ToggleOverride
			popad

			TestForUnderscore :
				// if block name preceded by '_', enable compiler override
			mov al, byte ptr[edx]
				cmp al, '_'
				setz al
				mov[s_overridden], al
				test al, al
				jz DoStricmp
					// skip the '_' character
				add edx, 1

				DoStricmp:
				// do the comparison
			pop eax
				push edx
				push eax
				call[stricmpAddr]
				add esp, 8
				pop edx

				// a match?
				test eax, eax
				jnz Done

					// a match. override requested?
				test[s_overridden], 1
				jz Done

				// toggle the override
				pushad
				push 1
				call ToggleOverride
				popad

				Done :
				// return the result of stricmp(). caller will clean stack
			retn
		}
	}

	static void __stdcall ProcessBeginOrEnd(ScriptBuffer *buf, UInt32 opcode)
	{
		ASSERT(opcode == 0x10 || opcode == 0x11);

		// make sure we've got enough room in the buffer
		if (buf->dataOffset + 4 >= 0x4000)
		{
			g_ErrOut.Show("Error: Max script length exceeded. Please edit and recompile.");
		}
		else
		{
			const char *cmdName = "@PushExecutionContext";
			SInt32 offset = 0;

			if (opcode == 0x11)
			{
				// 'end'. Need to rearrange so we pop context before the 'end' statement
				cmdName = "@PopExecutionContext";
				offset = -4;

				// move 'end' state forward by 4 bytes (i.e. at current offset)
				*((UInt32 *)(buf->scriptData + buf->dataOffset)) = 0x00000011;
			}

			CommandInfo *cmdInfo = g_scriptCommands.GetByName(cmdName, &g_currentCompilerPluginVersions.top());
			ASSERT(cmdInfo != NULL);

			// write a call to our cmd
			*((UInt16 *)(buf->scriptData + buf->dataOffset + offset)) = cmdInfo->opcode;
			buf->dataOffset += sizeof(UInt16);

			// write length of params
			*((UInt16 *)(buf->scriptData + buf->dataOffset + offset)) = 0;
			buf->dataOffset += sizeof(UInt16);
		}
	}

	static __declspec(naked) void OnCompileSuccessHook(void)
	{
		// esi: ScriptLineBuffer
		// edi: ScriptBuffer
		// eax: volatile
		__asm {
			// is override in effect?
			test[s_overridden], 1
			jz Done

				// have we just parsed a begin or end statement?
			movzx eax, word ptr[esi + 0x410] // cmd opcode for this line
			cmp eax, 0x10 // 0x10 == 'begin'
			jz Begin
			cmp eax, 0x11 // 0x11 == 'end'
			jz End
			jmp Done		 // not a block statement

			Begin :
								 // commands following start of block should have access to script context
			mov[s_currentMode], kOverride_Command
				jmp Process

				End :
								 // expect a new block or end of script, no execution context available
			mov[s_currentMode], kOverride_BlockType

				Process :
				// got a begin or end statement, handle it
			pushad
				push eax // opcode
				push edi // ScriptBuffer
				call ProcessBeginOrEnd
				popad

				Done :
			retn
		}
	}

	static __declspec(naked) void StoreBlockLenHook(void)
	{
		__asm {
			// overwritten code
			mov ecx, [eax]
			sub edx, ecx

				// override in effect?
			test[s_overridden], 1
			jz Done

				// add 4 bytes to block len to account for 'push context' cmd
			add edx, 4

			Done:
				// (overwritten) store block len
			mov[eax], edx
				jmp[storeBlockLenRetnAddr]
		}
	}

	void InitHooks()
	{
		// overwrite calls to compare block name when parsing 'begin'
		WriteRelCall(stricmpPatchAddr_1, (UInt32)&CompareBlockName);
		WriteRelCall(stricmpPatchAddr_2, (UInt32)&CompareBlockName);

		// overwrite a call to a null sub just before returning true from CompileScriptLine()
		WriteRelCall(nullCallAddr, (UInt32)&OnCompileSuccessHook);

		// hook code that records the jump offset for the current block
		WriteRelJump(storeBlockLenHookAddr, (UInt32)&StoreBlockLenHook);
	}

	bool Cmd_Plugin_Default_Parse(UInt32 numParams, ParamInfo *params, ScriptLineBuffer *lineBuf, ScriptBuffer *scriptBuf)
	{
		bool bOverride = IsActive();
		if (bOverride)
		{
			ToggleOverride(false);
		}

		bool result = Cmd_Default_Parse(numParams, params, lineBuf, scriptBuf);

		if (bOverride)
		{
			ToggleOverride(true);
		}

		return result;
	}
}

void MarkModAsMyMod(Script* script)
{
	std::set<std::string> myMods;
	const auto myModsFileName = "MyGECKMods.txt";
	if (std::filesystem::exists(myModsFileName))
	{
		std::ifstream is(myModsFileName);
		std::string curMod;
		while (std::getline(is, curMod))
		{
			myMods.emplace(std::move(curMod));
		}
	}
	std::ofstream os(myModsFileName);
	std::string esmFileName;
	if (script->mods.Head() && script->mods.Head()->data && ValidString(script->mods.Head()->data->name))
	{
		esmFileName = script->mods.Head()->data->name;
	}
	else if (DataHandler::Get()->activeFile && ValidString(DataHandler::Get()->activeFile->name))
	{
		esmFileName = DataHandler::Get()->activeFile->name;
	}
	myMods.emplace(std::move(esmFileName));
	for (auto& line : myMods)
	{
		os << line << std::endl;
	}
}

void __fastcall PostScriptCompileSuccess(Script* script, ScriptBuffer* scriptBuffer)
{
	ScriptAndScriptBuffer msg{ script, scriptBuffer };
	PluginManager::Dispatch_Message(0, NVSEMessagingInterface::kMessage_ScriptCompile, &msg, sizeof(ScriptAndScriptBuffer), nullptr);
#if EDITOR
	MarkModAsMyMod(script);
#endif
}

// This hook is inconsistent with runtime - this hooks calls to Script::Compile instead of hooking inside of it
//	- If Script->Compile is called in the GECK outside of the vanilla code this will NOT be hit
//	- Not sure if this is a problem, maybe all plugins should be compiling scripts via NVSE and we can clean up the runtime hooks as well?
static char __fastcall CompileScriptHook(void* context, void* edx, Script* script, ScriptBuffer *buf) {
	if (buf == nullptr || buf->scriptText == nullptr) {
		return 1;
	}

	auto precompileResult = HandleBeginCompile(buf, script);
	char result = 0;

	if (precompileResult == kPrecompile_Success) {
		// Let original compiler take over
		result = ThisStdCall<char>(0x5C96E0, context, script, buf);
	}

	// Special compilation, new compiler
	else if (precompileResult == kPrecompile_SpecialCompile) {
		result = 1;
	}

	PostScriptCompile();
	if (result) {
		PostScriptCompileSuccess(script, buf);
	}

	CompilerOverride::ToggleOverride(false);
	return result;
}

// replace special characters ("%q" -> '"', "%r" -> '\n')
UInt32 __stdcall CopyStringArg(char *dest, const char *src, UInt32 len, ScriptLineBuffer *lineBuf)
{
	if (!src || !len || !dest)
		return len;

	std::string str(src);
	UInt32 pos = 0;

	while ((pos = str.find('%', pos)) != -1 && pos < str.length() - 1)
	{
		char toInsert = 0;
		switch (str[pos + 1])
		{
		case '%':
			pos += 2;
			continue;
		case 'r':
		case 'R':
			toInsert = '\n';
			break;
		case 'q':
		case 'Q':
			toInsert = '"';
			break;
		default:
			pos += 1;
			continue;
		}

		str.insert(pos, 1, toInsert); // insert char at current pos
		str.erase(pos + 1, 2);		  // erase format specifier
		pos += 1;
	}

	// copy the string to script data
	memcpy(dest, str.c_str(), str.length());

	// write length of string
	lineBuf->dataOffset -= 2;
	lineBuf->Write16(str.length());

	return str.length();
}

// major code differences between CS versions here so hooks differ significantly
static __declspec(naked) void __cdecl CopyStringArgHook(void)
{
	// overwrite call to memcpy()

	// On entry:
	//	eax: dest buffer
	//	edx: src string
	//	edi: string len
	//  esi: ScriptLineBuffer* (must be preserved)
	// edi must be updated to reflect length of modified string (added to dataOffset on return)

	__asm
	{
		push	esi

		push	esi
		push	edi
		push	edx
		push	eax
		call	CopyStringArg

		mov		edi, eax
		pop		esi

		retn
	}
}

char __cdecl HandleParseCommandToken(ScriptParseToken* parseToken) {
	if (const auto* commandInfo = g_scriptCommands.GetByName(parseToken->tokenString, &g_currentCompilerPluginVersions.top())) {
		parseToken->tokenType = 'X';
		parseToken->cmdOpcode = commandInfo->opcode;
		return 1;
	}

	return 0;

	// return CdeclCall<char>(0x5C53B0, parseToken);
}

__declspec(naked) void HookParseCommandToken() {
	_asm {
		add esp, 8

		push esi
		call HandleParseCommandToken
		add esp, 4

		test eax, eax
		jz ret_0

		// Return 1 (inside AL already)
		pop edi
		pop esi
		pop ebx
		retn

		// Skip to console command loop
		ret_0:
		push 0x5C5425
		retn
	}
}

void Hook_Compiler_Init()
{
	// hook beginning of compilation process
	WriteRelCall(0x5C9859, reinterpret_cast<UInt32>(CompileScriptHook));
	WriteRelCall(0x5C99AC, reinterpret_cast<UInt32>(CompileScriptHook));

	// hook copying of string argument to compiled data
	// lets us modify the string before its copied
	WriteRelCall(kCopyStringArgHookAddr, (UInt32)&CopyStringArgHook);

	// hook code in the vanilla expression parser's subroutine to fix the buffer overflow
	WriteRelJump(kExpressionParserBufferOverflowHookAddr_1, (UInt32)&ExpressionParserBufferOverflowHook_1);
	WriteRelJump(kExpressionParserBufferOverflowHookAddr_2, (UInt32)&ExpressionParserBufferOverflowHook_2);

	CompilerOverride::InitHooks();

	PatchMismatchedParenthesisCheck();

	// WriteRelCall(0x5C55C0, reinterpret_cast<UInt32>(&HookParseCommandToken));
	// WriteRelCall(0x5C63D4, reinterpret_cast<UInt32>(&HookParseCommandToken));
	// WriteRelCall(0x5C64AC, reinterpret_cast<UInt32>(&HookParseCommandToken));

	WriteRelJump(0x5C53FD, reinterpret_cast<UInt32>(&HookParseCommandToken));
}

#else // run-time

#include "common/ICriticalSection.h"

namespace ExtractArgsOverride
{
	static std::vector<ExecutingScriptContext *> s_stack;
	static ICriticalSection s_critSection;

	ExecutingScriptContext::ExecutingScriptContext(TESObjectREFR *thisObj, TESObjectREFR *container, UInt16 opcode)
	{
		callingRef = thisObj;
		containerRef = container;
		cmdOpcode = opcode;
		threadID = GetCurrentThreadId();
	}

	ExecutingScriptContext *PushContext(TESObjectREFR *thisObj, TESObjectREFR *containerObj, UInt8 *scriptData, UInt32 *offsetPtr)
	{
		UInt16 *opcodePtr = (UInt16 *)(scriptData + *offsetPtr - 4);
		ExecutingScriptContext *context = new ExecutingScriptContext(thisObj, containerObj, *opcodePtr);

		s_critSection.Enter();
		s_stack.push_back(context);
		s_critSection.Leave();

		return context;
	}

	bool PopContext()
	{
		DWORD curThreadID = GetCurrentThreadId();
		bool popped = false;

		s_critSection.Enter();
		for (std::vector<ExecutingScriptContext *>::reverse_iterator iter = s_stack.rbegin(); iter != s_stack.rend(); ++iter)
		{
			ExecutingScriptContext *context = *iter;
			if (context->threadID == curThreadID)
			{
				delete context;
				s_stack.erase((++iter).base());
				popped = true;
				break;
			}
		}
		s_critSection.Leave();

		return popped;
	}

	ExecutingScriptContext *GetCurrentContext()
	{
		ExecutingScriptContext *context = NULL;
		DWORD curThreadID = GetCurrentThreadId();

		s_critSection.Enter();
		for (SInt32 i = s_stack.size() - 1; i >= 0; i--)
		{
			if (s_stack[i]->threadID == curThreadID)
			{
				context = s_stack[i];
				break;
			}
		}
		s_critSection.Leave();

		return context;
	}

	static bool __stdcall DoExtractExtendedArgs(CompilerOverride::Mode mode, UInt32 _ebp, va_list varArgs)
	{
		static const UInt8 stackOffset = 0x00;

		// grab the args to ExtractArgs()
		_ebp += stackOffset;
		ParamInfo *paramInfo = *(ParamInfo **)(_ebp + 0x08);
		UInt8 *scriptData = *(UInt8 **)(_ebp + 0x0C);
		UInt32 *opcodeOffsetPtr = *(UInt32 **)(_ebp + 0x10);
		Script *scriptObj = *(Script **)(_ebp + 0x1C);
		ScriptEventList *eventList = *(ScriptEventList **)(_ebp + 0x20);

		// extract
		return ExtractArgs(paramInfo, scriptData, scriptObj, eventList, opcodeOffsetPtr, varArgs, true, mode);
	}

	bool ExtractArgs(ParamInfo *paramInfo, UInt8 *scriptData, Script *scriptObj, ScriptEventList *eventList, UInt32 *opcodeOffsetPtr,
					 va_list varArgs, bool bConvertTESForms, UInt16 numArgs)
	{
		// get thisObj and containerObj from currently executing script context, if available
		TESObjectREFR *thisObj = NULL;
		TESObjectREFR *containingObj = NULL;
		if (numArgs == CompilerOverride::kOverride_Command)
		{
			// context should be available
			ExecutingScriptContext *context = GetCurrentContext();
			if (!context)
			{
				g_ErrOut.Show("ERROR: Could not get execution context in ExtractArgsOverride::ExtractArgs");
				return false;
			}
			else
			{
				thisObj = context->callingRef;
				containingObj = context->containerRef;
			}
		}

		// extract
		ExpressionEvaluator eval(paramInfo, scriptData, thisObj, containingObj, scriptObj, eventList, NULL, opcodeOffsetPtr);
		if (eval.ExtractDefaultArgs(varArgs, bConvertTESForms))
		{
			return true;
		}
		else
		{
			DEBUG_PRINT("ExtractArgsOverride::ExtractArgs returns false");
			return false;
		}
	}

	bool ExtractFormattedString(ParamInfo *paramInfo, UInt8 *scriptData, Script *scriptObj, ScriptEventList *eventList,
								UInt32 *opcodeOffsetPtr, va_list varArgs, UInt32 fmtStringPos, char *fmtStringOut, UInt32 maxParams)
	{
		ExecutingScriptContext *context = GetCurrentContext();
		if (!context)
		{
			g_ErrOut.Show("ERROR: Could not get execution context in ExtractFormattedString()");
			return false;
		}
		*opcodeOffsetPtr += 2;
		ExpressionEvaluator eval(paramInfo, scriptData, context->callingRef, context->containerRef, scriptObj, eventList,
								 NULL, opcodeOffsetPtr);
		return eval.ExtractFormatStringArgs(varArgs, fmtStringPos, fmtStringOut, maxParams);
	}

	/*

	FalloutNV
	.text:005ACCC6 lea     eax, [ebp+arg_1C]               ; Load Effective Address
	.text:005ACCC9 mov     [ebp+var_C], eax
	.text:005ACCCC mov     ecx, [ebp+opcodeOffsetPtr]
	.text:005ACCCF mov     edx, [ecx]
	.text:005ACCD1 mov     eax, [ebp+scriptData]
	.text:005ACCD4 kExtractArgsReadNumArgsPatchAddr:
	.text:005ACCD4 mov     cx, [eax+edx]																jmp     kExtractArgsNumArgsHookAddr
	.text:005ACCD8 mov     [ebp+var_4], cx
	.text:005ACCDC kExtractArgsReadNumArgsRetnAddr:
	.text:005ACCDC mov     edx, [ebp+opcodeOffsetPtr]
	.text:005ACCDF mov     eax, [edx]
	.text:005ACCE1 add     eax, 2                          ; Add
	.text:005ACCE4 mov     ecx, [ebp+opcodeOffsetPtr]
	.text:005ACCE7 mov     [ecx], eax
	.text:005ACCE9 xor     edx, edx                        ; Logical Exclusive OR
	.text:005ACCEB mov     [ebp+var_8], dx
	.text:005ACCEF jmp     short loc_5ACCFD                ; Jump
	.text:005ACCF1 ; ---------------------------------------------------------------------------
	.text:005ACCF1
	.text:005ACCF1 loop:                                   ; CODE XREF: ExtractArgs:loc_5AE0ECj
	.text:005ACCF1 mov     ax, [ebp+var_8]
	.text:005ACCF5 add     ax, 1                           ; Add
	.text:005ACCF9 mov     [ebp+var_8], ax
	.text:005ACCFD
	.text:005ACCFD loc_5ACCFD:                             ; CODE XREF: ExtractArgs+3Fj
	.text:005ACCFD movsx   ecx, [ebp+var_8]                ; Move with Sign-Extend
	.text:005ACD01 movsx   edx, [ebp+var_4]                ; Move with Sign-Extend
	.text:005ACD05 cmp     ecx, edx                        ; Compare Two Operands
	.text:005ACD07 jge     kExtractArgsEndProcAddr         ; current argNum >= argCount, we are done

	*/

	static __declspec(naked) void ExtractExtendedArgsHook(void)
	{
		static UInt32 _ebp;
		static UInt8 bResult;

		__asm {
			// restore missing ops
			mov     cx, [eax + edx]
			mov[ebp - 4], cx
			mov     edx, [ebp + 0x10]
			mov     eax, [edx]
			add     eax, 2
			mov[edx], eax // original code used ecx, but we need to preserve it, so we reuse edx

				//saves context
			mov[_ebp], ebp
			pushad
			test	cx, cx
			jl		ourcode // < 0 indicates compiled with Cmd_Expression_Parse rather than Cmd_Default_Parse
			popad
			jmp		kExtractArgsReadNumArgsRetnAddr
			ourcode :
			lea		eax, [ebp + 0x24] // va_list
				push	eax
				mov		eax, [_ebp] // caller context
				push	eax
				movsx	eax, cx // num args
				push	eax
				call	DoExtractExtendedArgs
				mov[bResult], al

				popad
				movzx	eax, [bResult]
				jmp[kExtractArgsEndProcAddr]
		}
	}

	// these hooks don't get invoked unless they are required (no run-time penalty if compiler override is not used)
	static void Init_Hooks()
	{
		WriteRelJump(kExtractArgsReadNumArgsPatchAddr, (UInt32)&ExtractExtendedArgsHook); // numArgs < 0, execute hook
	}
} // namespace

#endif // run-time
