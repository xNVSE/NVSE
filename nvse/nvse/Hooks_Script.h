#pragma once

struct CommandInfo;
struct ScriptLineBuffer;
struct ScriptBuffer;
class TESObjectREFR;
class Script;
struct ScriptEventList;
struct ParamInfo;

void Hook_Script_Init();
bool ParseNestedFunction(CommandInfo* cmd, ScriptLineBuffer* lineBuf, ScriptBuffer* scriptBuf);
bool ParsingExpression();	// returns true if we are parsing an NVSE expression

void RegisterLoopStart(UInt8* offsetPtr);
bool HandleLoopEnd(UInt32 offsetToEnd);

#if !RUNTIME

void Hook_Compiler_Init();

#endif


// v0020: Scripts can optionally request that cmds within a block be parsed by NVSE's Cmd_Expression_Parse rather than
// Cmd_Default_Parse by e.g. 'begin _gamemode', 'begin _onHit {some expression evaluating to an actor}', etc.
// This allows NVSE constructs like arrays, strings, user-defined function calls, enhanced expressions to be used
// with any command, i.e. effectively integrates them into the scripting language proper.
// CompilerOverride provides an interface for managing this in the editor.
namespace CompilerOverride
{
	// sigil values stored in bytecode where # of args is stored.
	// ExtractArgs() recognizes these and invokes hook
	// must be 16-bit negative value
	enum Mode : UInt16 {
		kOverride_Command		= 0xFFFF,		// an ordinary command within a block
		kOverride_BlockType		= 0xFFFE,		// a block type e.g. 'gamemode'
	};

	void InitHooks();

	// is current block being compiled with override in effect?
	bool IsActive();

	bool Cmd_Plugin_Default_Parse(UInt32 numParams, ParamInfo* params, ScriptLineBuffer* lineBuf, ScriptBuffer* scriptBuf);
};

// interface for extracting arguments compiled with the compiler override above.
// the associated hooks only get invoked if actually needed, so there's no cost if the override is not used.
// Annoyingly, it is occassionally possible for multiple scripts to execute concurrently, so we need to account for that.
namespace ExtractArgsOverride
{
	// represents a script currently executing with the compiler override in effect.
	// This is needed so we can access the calling and containing objects when evaluating expressions passed as cmd args.
	struct ExecutingScriptContext
	{
	public:
		ExecutingScriptContext(TESObjectREFR* thisObj, TESObjectREFR* container, UInt16 opcode);

		TESObjectREFR*	callingRef;
		TESObjectREFR*	containerRef;
		UInt16			cmdOpcode;
		DWORD			threadID;
	};

	// on entry to a block compiled with override in effect, records the context in a thread-safe manner and returns it
	// a cmd invoking this method is inserted at the beginning of the block so that the context is accessible to
	// any cmds within that block
	ExecutingScriptContext* PushContext(TESObjectREFR* thisObj, TESObjectREFR* containerObj, UInt8* scriptData, UInt32* offsetPtr);

	// a cmd invoking this method is inserted at the end of a block compiled with the override
	// pops the context, returns true if something goes horrendously wrong
	bool PopContext();

	// get the topmost execution context for the current thread.
	ExecutingScriptContext* GetCurrentContext();

	// Extract arguments compiled with override.
	// bConvertTESForms is true if called from ExtractArgs(), false from ExtractArgsEx()
	// numArgs are the # of args as extracted from script data, will be negative 16-bit value, one of CompilerOverride::Mode
	bool ExtractArgs(ParamInfo* paramInfo, UInt8* scriptData, Script* scriptObj, ScriptEventList* eventList, UInt32* opcodeOffsetPtr,
		va_list varArgs, bool bConvertTESForms, UInt16 numArgs);

	bool ExtractFormattedString(ParamInfo* paramInfo, UInt8* scriptData, Script* scriptObj, ScriptEventList* eventList,
		UInt32* opcodeOffsetPtr, va_list varArgs, UInt32 fmtStringPos, char* fmtStringOut, UInt32 maxParams);

	void Init_Hooks();
};


	