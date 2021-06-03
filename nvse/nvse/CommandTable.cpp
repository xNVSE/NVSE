#include "CommandTable.h"
#include "SafeWrite.h"
#include "GameAPI.h"
#include "GameData.h"
#include "GameObjects.h"
#include "GameEffects.h"
#include "GameExtraData.h"
#include "GameForms.h"
#include "GameProcess.h"
#include "GameRTTI.h"
#include "GameSettings.h"
#include "GameUI.h"
#include <string>
#include "Utilities.h"
#include "PluginManager.h"
#include "NiNodes.h"
#include <stdexcept>

#include "Commands_Console.h"
#include "Commands_Game.h"
#include "Commands_Input.h"
#include "Commands_Inventory.h"
#include "Commands_List.h"
#include "Commands_Math.h"
#include "Commands_MiscRef.h"
#include "Commands_Misc.h"
#include "Commands_Packages.h"
#include "Commands_Script.h"
#include "Commands_Scripting.h"
#include "Commands_UI.h"
#include "Commands_InventoryRef.h"
#include "Commands_Factions.h"
#include "Commands_Array.h"
#include "Commands_String.h"
#include "Commands_Algohol.h"

CommandTable g_consoleCommands;
CommandTable g_scriptCommands;

#if RUNTIME

// 1.4.0.525 runtime
UInt32 g_offsetConsoleCommandsStart = 0x0118E8E0;
UInt32 g_offsetConsoleCommandsLast = 0x011908C0;
UInt32 g_offsetScriptCommandsStart = 0x01190910;
UInt32 g_offsetScriptCommandsLast = 0x01196D10;
static const Cmd_Parse g_defaultParseCommand = (Cmd_Parse)0x005B1BA0;

#else

// 1.4.0.518 editor
UInt32 g_offsetConsoleCommandsStart = 0x00E9DB88;
UInt32 g_offsetConsoleCommandsLast = 0x00E9FB90;
UInt32 g_offsetScriptCommandsStart = 0x00E9FBB8;
UInt32 g_offsetScriptCommandsLast = 0x00EA5FB8;
static const Cmd_Parse g_defaultParseCommand = (Cmd_Parse)0x005C67E0;

#endif

struct PatchLocation
{
	UInt32	ptr;
	UInt32	offset;
	UInt32	type;
};

#if RUNTIME

static const PatchLocation kPatch_ScriptCommands_Start[] =
{
	{	0x005B1172, 0x00 },
	{	0x005B19B1, 0x00 },
	{	0x005B19CE, 0x04 },
	{	0x005B19F8, 0x08 },
	{	0x005BCC0A, 0x0C },
	{	0x005BCC2D, 0x00 },
	{	0x005BCC50, 0x04 },
	{	0x005BCC70, 0x0C },
	{	0x005BCC86, 0x0C },
	{	0x005BCCA6, 0x04 },
	{	0x005BCCB8, 0x04 },
	{	0x005BCCD4, 0x0C },
	{	0x005BCCE4, 0x04 },
	{	0x005BCCF4, 0x00 },
	{	0x005BCD13, 0x0C },
	{	0x005BCD23, 0x00 },
	{	0x005BCD42, 0x04 },
	{	0x005BCD54, 0x04 },
	{	0x005BCD70, 0x04 },
	{	0x005BCD80, 0x00 },
	{	0x005BCD9F, 0x00 },
	{	0x0068170B, 0x20 },
	{	0x00681722, 0x10 },
	{	0x00681752, 0x20 },
	{	0x00681AEB, 0x20 },
	{	0x00681CDE, 0x00 },
	{	0x006820FF, 0x14 },
	{	0x0068228D, 0x12 },
	{	0x006822FF, 0x14 },
	{	0x00682352, 0x14 },
	{	0x006823B2, 0x14 },
	{	0x0087A909, 0x12 },
	{	0x0087A948, 0x14 },
	{	0 },
};

static const PatchLocation kPatch_ScriptCommands_End[] =
{
	{	0x005AEA59, 0x08 },
	{	0 },
};

// 127B / 027B
// 1280 / 0280
static const PatchLocation kPatch_ScriptCommands_MaxIdx[] =
{
	{	0x00593909 + 1, 0 },
	{	0x005AEA57 + 6, 0 },
	{	0x005B115B + 3, 0 },
	{	0x005B19A0 + 3, (UInt32)(-0x1000) },
	{	0x005BCBDD + 6, (UInt32)(-0x1000) },
	{	0 },
};

void ApplyPatchEditorOpCodeDataList(void) {
}

#else

static const PatchLocation kPatch_ScriptCommands_Start[] =
{
	{	0x004072AF, 0x00 },
	{	0x004073FA, 0x00 },
	{	0x004A2374, 0x24 },
	{	0x004A23E8, 0x24 },
	{	0x004A2B9B, 0x00 },
	{	0x004A3CE2, 0x20 },
	{	0x004A3CF2, 0x10 },
	{	0x004A431A, 0x00 },
	{	0x004A474A, 0x20 },
	{	0x004A485F, 0x00 },
	{	0x004A4ED1, 0x00 },
	{	0x004A5134, 0x00 },
	{	0x004A58B4, 0x12 },
	{	0x004A58F5, 0x12 },
	{	0x004A5901, 0x14 },
	{	0x004A593E, 0x12 },
	{	0x004A5949, 0x14 },
	{	0x004A5A26, 0x12 },
	{	0x004A5A6D, 0x12 },
	{	0x004A5A79, 0x14 },
	{	0x004A5AD6, 0x12 },
	{	0x004A5B1D, 0x12 },
	{	0x004A5B29, 0x14 },
	{	0x004A5B7C, 0x12 },
	{	0x004A5BD9, 0x12 },
	{	0x004A5C28, 0x12 },
	{	0x004A5C34, 0x14 },
	{	0x004A600C, 0x14 },
	{	0x004A6704, 0x12 },
	{	0x004A6749, 0x12 },
	{	0x004A6755, 0x14 },
	{	0x004A684C, 0x12 },
	{	0x004A6A8F, 0x12 },
	{	0x004A6A9F, 0x14 },
	{	0x004A6BDF, 0x12 },
	{	0x004A6D30, 0x14 },
	{	0x004A6D74, 0x14 },
	{	0x004A703B, 0x12 },
	{	0x004A716D, 0x12 },
	{	0x004A71B5, 0x14 },
	{	0x004A7268, 0x14 },
	{	0x004A735A, 0x12 },
	{	0x004A7536, 0x14 },
	{	0x0059C532, 0x20 },
	{	0x0059C53B, 0x24 },
	{	0x0059C6BA, 0x24 },
	{	0x005C53F4, 0x04 },
	{	0x005C548D, 0x08 },
	{	0x005C6636, 0x00 },
	{	0x005C9499, 0x00 },
	{	0 },
};

static const PatchLocation kPatch_ScriptCommands_End[] =
{
	{	0x004A433B, 0x00 },
	{	0x0059C710, 0x24 },
	{	0x005C5372, 0x08 },
	{	0x005C541F, 0x04 },
	{	0 },
};

// 280 / 1280 / 27F
static const PatchLocation kPatch_ScriptCommands_MaxIdx[] =
{
	{	0x004A2B87 + 2,	(UInt32)(-0x1000) },
	{	0x0059C576 + 2,	(UInt32)(-0x1000) },
	{	0x005B1817 + 1,	0 },
	{	0x005C5370 + 6,	0 },

	{	0x004A439D + 2, (UInt32)(-0x1000) - 1 },
	{	0x004A43AD + 1, (UInt32)(-0x1000) - 1 },
	{	0x004A43B9 + 2, (UInt32)(-0x1000) - 1 },

	{	0x005C6625 + 1,	(UInt32)(-0x1000) - 1 },
	{	0x005C948B + 2,	(UInt32)(-0x1000) - 1 },

	{	0 },
};

#define OpCodeDataListAddress	0x004A4390	// The function containing the fith element of array kPatch_ScriptCommands_MaxIdx
#define hookOpCodeDataListAddress		0x004A5562	// call hookOpCodeDataList. This is the second reference to the function.

static void* OpCodeDataListFunc = (void*)OpCodeDataListAddress;

int __fastcall hookOpCodeDataList(UInt32 ECX, UInt32 EDX, UInt32 opCode) {  // Replacement for the vanilla version that truncate opCode by 1000
	_asm {
		mov eax, opCode
		add eax, 0x01000
		push eax
		call OpCodeDataListFunc	//	baseOpCodeDataList	// ecx and edx are still in place
	}
}

void ApplyPatchEditorOpCodeDataList(void) {
	SInt32 RelativeAddress = (UInt32)(&hookOpCodeDataList) - hookOpCodeDataListAddress - 5 /* EIP after instruction that we modify*/;
	SafeWrite32(hookOpCodeDataListAddress+1, (UInt32)RelativeAddress);
}

#endif

static void ApplyPatch(const PatchLocation * patch, UInt32 newData)
{
	for(; patch->ptr; ++patch)
	{
		switch(patch->type)
		{
		case 0:
			SafeWrite32(patch->ptr, newData + patch->offset);
			break;

		case 1:
			SafeWrite16(patch->ptr, newData + patch->offset);
			break;
		}
	}
}

bool Cmd_Default_Parse(UInt32 numParams, ParamInfo * paramInfo, ScriptLineBuffer * lineBuf, ScriptBuffer * scriptBuf)
{
	return g_defaultParseCommand(numParams, paramInfo, lineBuf, scriptBuf);
}

#if RUNTIME

bool Cmd_GetNVSEVersion_Eval(COMMAND_ARGS_EVAL)
{
	*result = NVSE_VERSION_INTEGER;
	if (IsConsoleMode()) {
		Console_Print("NVSE version: %d", NVSE_VERSION_INTEGER);
	}
	return true;
}

bool Cmd_GetNVSEVersion_Execute(COMMAND_ARGS)
{
	return Cmd_GetNVSEVersion_Eval(thisObj, 0, 0, result);
}

bool Cmd_GetNVSERevision_Eval(COMMAND_ARGS_EVAL)
{
	*result = NVSE_VERSION_INTEGER_MINOR;
	if (IsConsoleMode()) {
		Console_Print("NVSE revision: %d", NVSE_VERSION_INTEGER_MINOR);
	}
	return true;
}

bool Cmd_GetNVSERevision_Execute(COMMAND_ARGS)
{
	return Cmd_GetNVSERevision_Eval(thisObj, 0, 0, result);
}

bool Cmd_GetNVSEBeta_Eval(COMMAND_ARGS_EVAL)
{
	*result = NVSE_VERSION_INTEGER_BETA;
	if (IsConsoleMode()) {
		Console_Print("NVSE beta: %d", NVSE_VERSION_INTEGER_BETA);
	}
	return true;
}

bool Cmd_GetNVSEBeta_Execute(COMMAND_ARGS)
{
	return Cmd_GetNVSEBeta_Eval(thisObj, 0, 0, result);
}

bool Cmd_DumpDocs_Execute(COMMAND_ARGS)
{
	if (IsConsoleMode()) {
		Console_Print("Dumping Command Docs");
	}
	g_scriptCommands.DumpCommandDocumentation();
	if (IsConsoleMode()) {
		Console_Print("Done Dumping Command Docs");
	}
	return true;
}

class Dumper {
	UInt32 m_sizeToDump;
public:
	Dumper(UInt32 sizeToDump = 512) : m_sizeToDump(sizeToDump) {}
	bool Accept(void *addr) {
		if (addr) {
			DumpClass(addr, m_sizeToDump);
		}
		return true;
	}
};

#endif

static ParamInfo kTestArgCommand_Params[] =
{
	{	"variable", kParamType_ScriptVariable, 0 },
};

static ParamInfo kTestDumpCommand_Params[] = 
{
	{	"form", kParamType_ObjectID, 1},
};

DEFINE_CMD_COND(GetNVSEVersion, returns the installed version of NVSE, 0, NULL);
DEFINE_CMD_COND(GetNVSERevision, returns the numbered revision of the installed version of NVSE, 0, NULL);
DEFINE_CMD_COND(GetNVSEBeta, returns the numbered beta of the installed version of NVSE, 0, NULL);
DEFINE_COMMAND(DumpDocs, , 0, 0, NULL);

#define ADD_CMD(command) Add(&kCommandInfo_ ## command )
#define ADD_CMD_RET(command, rtnType) Add(&kCommandInfo_ ## command, rtnType )
#define REPL_CMD(command) Replace(GetByName(command)->opcode, &kCommandInfo_ ## command )

CommandTable::CommandTable() { }
CommandTable::~CommandTable() { }

void CommandTable::Init(void)
{
	static CommandInfo* kCmdInfo_Unused_1;
#if RUNTIME
	kCmdInfo_Unused_1 = (CommandInfo*)0x0118E4F8;
#else
	kCmdInfo_Unused_1 = (CommandInfo*)0x00E9D7A0;
#endif

	// read in the console commands
	g_consoleCommands.SetBaseID(0x0100);
	g_consoleCommands.Read((CommandInfo *)g_offsetConsoleCommandsStart, (CommandInfo *)g_offsetConsoleCommandsLast);

	// read in the script commands
	g_scriptCommands.SetBaseID(0x1000);
	g_scriptCommands.Read((CommandInfo *)g_offsetScriptCommandsStart, (CommandInfo *)g_offsetScriptCommandsLast);

	// blocktype "Unused_1" becomes "Function"
	UInt16 onUnused_1Opcode = kCmdInfo_Unused_1->opcode;
	*kCmdInfo_Unused_1 = kCommandInfo_Function;
	kCmdInfo_Unused_1->opcode = onUnused_1Opcode;

	// pad to opcode 0x1400 to give Bethesda lots of room
	g_scriptCommands.PadTo(kNVSEOpcodeStart);

	// Add NVSE Commands
	g_scriptCommands.AddCommandsV1();
	g_scriptCommands.AddCommandsV3s();
	g_scriptCommands.AddCommandsV4();
	g_scriptCommands.AddCommandsV5();
	g_scriptCommands.AddCommandsV6();

#if _DEBUG
	g_scriptCommands.AddDebugCommands();
#endif

	// register plugins
	g_pluginManager.Init();

	// patch the code
	ApplyPatch(kPatch_ScriptCommands_Start, (UInt32)g_scriptCommands.GetStart());
	ApplyPatch(kPatch_ScriptCommands_End, (UInt32)g_scriptCommands.GetEnd());
	ApplyPatch(kPatch_ScriptCommands_MaxIdx, g_scriptCommands.GetMaxID());

	ApplyPatchEditorOpCodeDataList();

	_MESSAGE("max id = %08X", g_scriptCommands.GetMaxID());

	//_MESSAGE("console commands");
	//g_consoleCommands.Dump();
	//_MESSAGE("script commands");
	//g_scriptCommands.Dump();

	_MESSAGE("patched");
}

void CommandTable::Read(CommandInfo * start, CommandInfo * end)
{
	UInt32	numCommands = end - start;
	m_commands.reserve(m_commands.size() + numCommands);

	for(; start != end; ++start)
		Add(start);
}

void CommandTable::Add(CommandInfo * info, CommandReturnType retnType, UInt32 parentPluginOpcodeBase)
{
	UInt32	backCommandID = m_baseID + m_commands.size();	// opcode of the next command to add

	info->opcode = m_curID;

	if(m_curID == backCommandID)
	{
		// adding at the end?
		m_commands.push_back(*info);
	}
	else if(m_curID < backCommandID)
	{
		// adding to existing data?
		ASSERT(m_curID >= m_baseID);

		m_commands[m_curID - m_baseID] = *info;
	}
	else
	{
		HALT("CommandTable::Add: adding past the end");
	}

	m_curID++;

	CommandMetadata * metadata = &m_metadata[info->opcode];

	metadata->parentPlugin = parentPluginOpcodeBase;
	metadata->returnType = retnType;
}

bool CommandTable::Replace(UInt32 opcodeToReplace, CommandInfo* replaceWith)
{
	for (CommandList::iterator iter = m_commands.begin(); iter != m_commands.end(); ++iter)
	{
		if (iter->opcode == opcodeToReplace)
		{
			*iter = *replaceWith;
			iter->opcode = opcodeToReplace;
			return true;
		}
	}

	return false;
}

static CommandInfo kPaddingCommand =
{
	"", "",
	0,
	"command used for padding",
	0,
	0,
	NULL,

	Cmd_Default_Execute,
	Cmd_Default_Parse,
	NULL,
	NULL
};

void CommandTable::PadTo(UInt32 id, CommandInfo * info)
{
	if(!info) info = &kPaddingCommand;

	while(m_baseID + m_commands.size() < id)
	{
		info->opcode = m_baseID + m_commands.size();
		m_commands.push_back(*info);
	}

	m_curID = id;
}

void CommandTable::Dump(void)
{
	for(CommandList::iterator iter = m_commands.begin(); iter != m_commands.end(); ++iter)
	{
		_DMESSAGE("%08X %04X %s %s", iter->opcode, iter->needsParent, iter->longName, iter->shortName);
	}
}


void CommandTable::DumpAlternateCommandNames(void)
{
	for (CommandList::iterator iter= m_commands.begin(); iter != m_commands.end(); ++iter)
	{
		if (iter->shortName)
			_MESSAGE("%s", iter->shortName);
	}
}

const char* SimpleStringForParamType(UInt32 paramType)
{
	switch(paramType) {
		case kParamType_String: return "string";
		case kParamType_Integer: return "integer";
		case kParamType_Float: return "float";
		case kParamType_ObjectID: return "ref";
		case kParamType_ObjectRef: return "ref";
		case kParamType_ActorValue: return "actorValue";
		case kParamType_Actor: return "ref";
		case kParamType_SpellItem: return "ref";
		case kParamType_Axis: return "axis";
		case kParamType_Cell: return "ref";
		case kParamType_AnimationGroup: return "animGroup";
		case kParamType_MagicItem: return "ref";
		case kParamType_Sound: return "ref";
		case kParamType_Topic: return "ref";
		case kParamType_Quest: return "ref";
		case kParamType_Race: return "ref";
		case kParamType_Class: return "ref";
		case kParamType_Faction: return "ref";
		case kParamType_Sex: return "sex";
		case kParamType_Global: return "global";
		case kParamType_Furniture: return "ref";
		case kParamType_TESObject: return "ref";
		case kParamType_VariableName: return "string";
		case kParamType_QuestStage: return "short";
		case kParamType_MapMarker: return "ref";
		case kParamType_ActorBase: return "ref";
		case kParamType_Container: return "ref";
		case kParamType_WorldSpace: return "ref";
		case kParamType_CrimeType: return "crimeType";
		case kParamType_AIPackage: return "ref";
		case kParamType_CombatStyle: return "ref";
		case kParamType_MagicEffect: return "ref";
		case kParamType_FormType: return "formType";
		case kParamType_WeatherID: return "ref";
		case kParamType_NPC: return "ref";
		case kParamType_Owner: return "ref";
		case kParamType_EffectShader: return "ref";
		case kParamType_FormList:			return "ref";
		case kParamType_MenuIcon:			return "ref";
		case kParamType_Perk:				return "ref";
		case kParamType_Note:				return "ref";
		case kParamType_MiscellaneousStat:	return "ref";
		case kParamType_ImageSpaceModifier:	return "ref";
		case kParamType_ImageSpace:			return "ref";
		case kParamType_EncounterZone:		return "ref";
		case kParamType_Message:			return "ref";
		case kParamType_InvObjOrFormList:	return "ref";
		case kParamType_Alignment:			return "ref";
		case kParamType_EquipType:			return "ref";
		case kParamType_NonFormList:		return "ref";
		case kParamType_SoundFile:			return "ref";
		case kParamType_CriticalStage:		return "ref";
		case kParamType_LeveledOrBaseChar:	return "ref";
		case kParamType_LeveledOrBaseCreature:	return "ref";
		case kParamType_LeveledChar:		return "ref";
		case kParamType_LeveledCreature:	return "ref";
		case kParamType_LeveledItem:		return "ref";
		case kParamType_AnyForm:			return "ref";
		case kParamType_Reputation:			return "ref";
		case kParamType_Casino:				return "ref";
		case kParamType_CasinoChip:			return "ref";
		case kParamType_Challenge:			return "ref";
		case kParamType_CaravanMoney:		return "ref";
		case kParamType_CaravanCard:		return "ref";
		case kParamType_CaravanDeck:		return "ref";
		case kParamType_Region:				return "ref";

		// custom NVSE types
		// kParamType_StringVar is same as Integer
		case kParamType_Array:				return "array_var";

		default: return "<unknown>";
	}
}

const char* StringForParamType(UInt32 paramType)
{
	switch(paramType) {
		case kParamType_String:				return "String";
		case kParamType_Integer:			return "Integer";
		case kParamType_Float:				return "Float";
		case kParamType_ObjectID:			return "ObjectID";
		case kParamType_ObjectRef:			return "ObjectRef";
		case kParamType_ActorValue:			return "ActorValue";
		case kParamType_Actor:				return "Actor";
		case kParamType_SpellItem:			return "SpellItem";
		case kParamType_Axis:				return "Axis";
		case kParamType_Cell:				return "Cell";
		case kParamType_AnimationGroup:		return "AnimationGroup";
		case kParamType_MagicItem:			return "MagicItem";
		case kParamType_Sound:				return "Sound";
		case kParamType_Topic:				return "Topic";
		case kParamType_Quest:				return "Quest";
		case kParamType_Race:				return "Race";
		case kParamType_Class:				return "Class";
		case kParamType_Faction:			return "Faction";
		case kParamType_Sex:				return "Sex";
		case kParamType_Global:				return "Global";
		case kParamType_Furniture:			return "Furniture";
		case kParamType_TESObject:			return "Object";
		case kParamType_VariableName:		return "VariableName";
		case kParamType_QuestStage:			return "QuestStage";
		case kParamType_MapMarker:			return "MapMarker";
		case kParamType_ActorBase:			return "ActorBase";
		case kParamType_Container:			return "Container";
		case kParamType_WorldSpace:			return "WorldSpace";
		case kParamType_CrimeType:			return "CrimeType";
		case kParamType_AIPackage:			return "AIPackage";
		case kParamType_CombatStyle:		return "CombatStyle";
		case kParamType_MagicEffect:		return "MagicEffect";
		case kParamType_FormType:			return "FormType";
		case kParamType_WeatherID:			return "WeatherID";
		case kParamType_NPC:				return "NPC";
		case kParamType_Owner:				return "Owner";
		case kParamType_EffectShader:		return "EffectShader";
		case kParamType_FormList:			return "FormList";
		case kParamType_MenuIcon:			return "MenuIcon";
		case kParamType_Perk:				return "Perk";
		case kParamType_Note:				return "Note";
		case kParamType_MiscellaneousStat:	return "MiscStat";
		case kParamType_ImageSpaceModifier:	return "ImageSpaceModifier";
		case kParamType_ImageSpace:			return "ImageSpace";
		case kParamType_Double:				return "Double";
		case kParamType_ScriptVariable:		return "ScriptVar";
		case kParamType_Unhandled2E:		return "unk2E";
		case kParamType_EncounterZone:		return "EncounterZone";
		case kParamType_IdleForm:			return "IdleForm";
		case kParamType_Message:			return "Message";
		case kParamType_InvObjOrFormList:	return "InvObjectOrFormList";
		case kParamType_Alignment:			return "Alignment";
		case kParamType_EquipType:			return "EquipType";
		case kParamType_NonFormList:		return "NonFormList";
		case kParamType_SoundFile:			return "SoundFile";
		case kParamType_CriticalStage:		return "CriticalStage";
		case kParamType_LeveledOrBaseChar:	return "LeveledOrBaseChar";
		case kParamType_LeveledOrBaseCreature:	return "LeveledOrBaseCreature";
		case kParamType_LeveledChar:		return "LeveledChar";
		case kParamType_LeveledCreature:	return "LeveledCreature";
		case kParamType_LeveledItem:		return "LeveledItem";
		case kParamType_AnyForm:			return "AnyForm";
		case kParamType_Reputation:			return "Reputation";
		case kParamType_Casino:				return "Casino";
		case kParamType_CasinoChip:			return "CasinoChip";
		case kParamType_Challenge:			return "Challenge";
		case kParamType_CaravanMoney:		return "CaravanMoney";
		case kParamType_CaravanCard:		return "CaravanCard";
		case kParamType_CaravanDeck:		return "CaravanDeck";
		case kParamType_Region:				return "Region";

		// custom NVSE types
		// kParamType_StringVar is same as Integer
		case kParamType_Array:				return "ArrayVar";

		default: return "<unknown>";
	}
}

void CommandTable::DumpCommandDocumentation(UInt32 startWithID)
{
	_MESSAGE("NVSE Commands from: %#x", startWithID);

	_MESSAGE("<br><b>Function Quick Reference</b>");
	CommandList::iterator itEnd = m_commands.end();
	for (CommandList::iterator iter = m_commands.begin();iter != itEnd; ++iter) {
		if (iter->opcode >= startWithID) {
			iter->DumpFunctionDef();
		}
	}

	_MESSAGE("<hr><br><b>Functions In Detail</b>");
	for (CommandList::iterator iter = m_commands.begin();iter != itEnd; ++iter) {
		if (iter->opcode >= startWithID) {
			iter->DumpDocs();
		}
	}
}

void CommandInfo::DumpDocs() const
{
	_MESSAGE("<p><a name=\"%s\"></a><b>%s</b> ", longName, longName);
	_MESSAGE("<br><b>Alias:</b> %s<br><b>Parameters:</b>%d", (strlen(shortName) != 0) ? shortName : "none", numParams);
	if (numParams > 0) {
		for(UInt32 i = 0; i < numParams; i++)
		{
			ParamInfo	* param = &params[i];
			const char* paramTypeName = StringForParamType(param->typeID);
			if (param->isOptional != 0) {
				_MESSAGE("<br>&nbsp;&nbsp;&nbsp;<i>%s:%s</i> ", param->typeStr, paramTypeName);
			} else {
				_MESSAGE("<br>&nbsp;&nbsp;&nbsp;%s:%s ", param->typeStr, paramTypeName);
			}
		}
	}
	_MESSAGE("<br><b>Return Type:</b> FixMe<br><b>Opcode:</b> %#4x (%d)<br><b>Condition Function:</b> %s<br><b>Description:</b> %s</p>", opcode, opcode, eval ? "Yes" : "No",helpText);
}

void CommandInfo::DumpFunctionDef() const
{
	_MESSAGE("<br>(FixMe) %s<a href=\"#%s\">%s</a> ", needsParent > 0 ? "reference." : "", longName, longName);
	if (numParams > 0) {
		for(UInt32 i = 0; i < numParams; i++)
		{
			ParamInfo	* param = &params[i];
			const char* paramTypeName = StringForParamType(param->typeID);
			if (param->isOptional != 0) {
				_MESSAGE("<i>%s:%s</i> ", param->typeStr, paramTypeName);
			} else {
				_MESSAGE("%s:%s ", param->typeStr, paramTypeName);
			}
		}
	}
}


CommandInfo * CommandTable::GetByName(const char * name)
{
	for (CommandList::iterator iter = m_commands.begin(); iter != m_commands.end(); ++iter)
		if (!StrCompare(name, iter->longName) || (iter->shortName && !StrCompare(name, iter->shortName)))
			return &(*iter);

	return NULL;
}


CommandInfo* CommandTable::GetByOpcode(UInt32 opcode)
{
	const auto baseOpcode = m_commands.begin()->opcode;
	const auto arrayIndex = opcode - baseOpcode;
	if (arrayIndex >= m_commands.size())
	{
		return nullptr;
	}
	auto* const command = &m_commands[arrayIndex];
	if (command->opcode != opcode)
	{
		//_MESSAGE("ERROR: mismatched command opcodes when executing CommandTable::GetByOpcode (opcode: %X base: %X index: %d index opcode: %X)",
		//	opcode, baseOpcode, arrayIndex, command->opcode);
		return nullptr;
	}
	return command;

}

CommandReturnType CommandTable::GetReturnType(const CommandInfo* cmd)
{
	return m_metadata[cmd->opcode].returnType;
}

void CommandTable::SetReturnType(UInt32 opcode, CommandReturnType retnType)
{
	CommandInfo* cmdInfo = GetByOpcode(opcode);
	if (!cmdInfo)
		_MESSAGE("CommandTable::SetReturnType() - cannot locate command with opcode %04X", opcode);
	else {
		m_metadata[opcode].returnType = retnType;
	}
}

void CommandTable::RecordReleaseVersion(void)
{
	m_opcodesByRelease.push_back(GetCurID());
}

UInt32 CommandTable::GetRequiredNVSEVersion(const CommandInfo* cmd)
{
	UInt32  ver = 0;
	if (cmd) {
		if (cmd->opcode < m_opcodesByRelease[0])	// vanilla cmd
			ver = 0;
		else if (cmd->opcode >= kNVSEOpcodeTest)	// plugin cmd, we have no way of knowing
			ver = -1;
		else {
			for (UInt32 i = 0; i < m_opcodesByRelease.size(); i++) {
				if (cmd->opcode >= m_opcodesByRelease[i]) {
					ver = i;
				}
				else {
					break;
				}
			}
		}
	}

	return ver;
}

void CommandTable::RemoveDisabledPlugins(void)
{
	for (CommandList::iterator iter = m_commands.begin(); iter != m_commands.end(); ++iter)
	{
		// plugin failed to load but still registered some commands?
		// realistically the game is going to go down hard if this happens anyway
		if(g_pluginManager.LookupHandleFromBaseOpcode(m_metadata[iter->opcode].parentPlugin) == kPluginHandle_Invalid)
			Replace(iter->opcode, &kPaddingCommand);
	}
}

static const char * kNVSEname = "NVSE";

static PluginInfo g_NVSEPluginInfo =
{
	PluginInfo::kInfoVersion,
	kNVSEname,
	NVSE_VERSION_INTEGER,
};

PluginInfo * CommandTable::GetParentPlugin(const CommandInfo * cmd)
{
	if (!cmd->opcode || cmd->opcode<kNVSEOpcodeStart)
		return NULL;

	if (cmd->opcode < kNVSEOpcodeTest)
		return &g_NVSEPluginInfo;

	PluginInfo *info = g_pluginManager.GetInfoFromBase(m_metadata[cmd->opcode].parentPlugin);
	if (info)
		return info;

	return NULL;
}

void ImportConsoleCommand(const char * name)
{
	CommandInfo	* info = g_consoleCommands.GetByName(name);
	if(info)
	{
		CommandInfo	infoCopy = *info;

		std::string	newName;

		newName = std::string("con_") + name;

		infoCopy.shortName = "";
		infoCopy.longName = _strdup(newName.c_str());

		g_scriptCommands.Add(&infoCopy);

//		_MESSAGE("imported console command %s", name);
	}
	else
	{
		_WARNING("couldn't find console command (%s)", name);

		// pad it
		g_scriptCommands.Add(&kPaddingCommand);
	}
}

bool Cmd_tcmd_Execute(COMMAND_ARGS)
{
	return true;
}

bool Cmd_tcmd2_Execute(COMMAND_ARGS)
{

#if RUNTIME
	*result = 0;
	ScriptEventList::Var* variable;
	if (ExtractArgsEx(EXTRACT_ARGS_EX, &variable) && variable)
	{
		*result = 1;
		Console_Print("%d", variable->id);
	}
#endif
	return true;
}

bool Cmd_tcmd3_Execute(COMMAND_ARGS)
{
	return true;
}

DEFINE_COMMAND(tcmd, test, 0, 0, NULL);
DEFINE_CMD_ALT(tcmd2, testargcmd ,test, 0, 1, kTestArgCommand_Params);
DEFINE_CMD_ALT(tcmd3, testdump, dump info, 0, 1, kTestDumpCommand_Params);


// internal commands added at the end
void CommandTable::AddDebugCommands()
{
	ADD_CMD_RET(CloneForm, kRetnType_Form);
	ADD_CMD_RET(PlaceAtMeAndKeep, kRetnType_Form);
	ADD_CMD_RET(GetMe, kRetnType_Form);							// Tested, not suitable for publishing

	ADD_CMD(tcmd);
	ADD_CMD(tcmd2);
	ADD_CMD(tcmd3);

	ADD_CMD(DumpDocs);
}

void CommandTable::AddCommandsV1()
{
	// record return type of vanilla commands which return forms
	g_scriptCommands.SetReturnType(0x1025, kRetnType_Form);		// PlaceAtMe
	g_scriptCommands.SetReturnType(0x10CD, kRetnType_Form);		// GetActionRef
	g_scriptCommands.SetReturnType(0x10CE, kRetnType_Form);		// GetSelf
	g_scriptCommands.SetReturnType(0x10CF, kRetnType_Form);		// GetContainer
	g_scriptCommands.SetReturnType(0x10E8, kRetnType_Form);		// GetCombatTarget
	g_scriptCommands.SetReturnType(0x10E9, kRetnType_Form);		// GetPackageTarget
	g_scriptCommands.SetReturnType(0x1113, kRetnType_Form);		// GetParentRef
	g_scriptCommands.SetReturnType(0x116B, kRetnType_Form);		// GetLinkedRef
	g_scriptCommands.SetReturnType(0x11BD, kRetnType_Form);		// PlaceAtMeHealthPercent
	g_scriptCommands.SetReturnType(0x11CF, kRetnType_Form);		// GetPlayerGrabbedRef
	g_scriptCommands.SetReturnType(0x124E, kRetnType_Form);		// GetOwnerLastTarget
	g_scriptCommands.SetReturnType(0x1265, kRetnType_Form);		// ObjectUnderTheReticule

	RecordReleaseVersion();

	// beta 1
	ADD_CMD(GetNVSEVersion);
	ADD_CMD(GetNVSERevision);
	ADD_CMD(GetNVSEBeta);
	ADD_CMD_RET(GetBaseObject, kRetnType_Form);
	ADD_CMD(GetWeight);
	ADD_CMD(GetHealth);
	ADD_CMD(GetValue);
	ADD_CMD(SetWeight);
	ADD_CMD(SetHealth);
	ADD_CMD(SetBaseItemValue);
	ADD_CMD(GetType);
	ADD_CMD_RET(GetRepairList, kRetnType_Form);
	ADD_CMD(GetEquipType);
	ADD_CMD_RET(GetWeaponAmmo, kRetnType_Form);
	ADD_CMD(GetWeaponClipRounds);
	ADD_CMD(GetAttackDamage);
	ADD_CMD(GetWeaponType);
	ADD_CMD(GetWeaponMinSpread);
	ADD_CMD(GetWeaponSpread);
	ADD_CMD(GetWeaponProjectile);
	ADD_CMD(GetWeaponSightFOV);
	ADD_CMD(GetWeaponMinRange);
	ADD_CMD(GetWeaponMaxRange);
	ADD_CMD(GetWeaponAmmoUse);
	ADD_CMD(GetWeaponActionPoints);
	ADD_CMD(GetWeaponCritDamage);
	ADD_CMD(GetWeaponCritChance);
	ADD_CMD(GetWeaponCritEffect);
	ADD_CMD(GetWeaponFireRate);
	ADD_CMD(GetWeaponAnimAttackMult);
	ADD_CMD(GetWeaponRumbleLeftMotor);
	ADD_CMD(GetWeaponRumbleRightMotor);
	ADD_CMD(GetWeaponRumbleDuration);
	ADD_CMD(GetWeaponRumbleWavelength);
	ADD_CMD(GetWeaponAnimShotsPerSec);
	ADD_CMD(GetWeaponAnimReloadTime);
	ADD_CMD(GetWeaponAnimJamTime);
	ADD_CMD(GetWeaponSkill);
	ADD_CMD(GetWeaponResistType);
	ADD_CMD(GetWeaponFireDelayMin);
	ADD_CMD(GetWeaponFireDelayMax);
	ADD_CMD(GetWeaponAnimMult);
	ADD_CMD(GetWeaponReach);
	ADD_CMD(GetWeaponIsAutomatic);
	ADD_CMD(GetWeaponHandGrip);
	ADD_CMD(GetWeaponReloadAnim);
	ADD_CMD(GetWeaponBaseVATSChance);
	ADD_CMD(GetWeaponAttackAnimation);
	ADD_CMD(GetWeaponNumProjectiles);
	ADD_CMD(GetWeaponAimArc);
	ADD_CMD(GetWeaponLimbDamageMult);
	ADD_CMD(GetWeaponSightUsage);
	ADD_CMD(GetWeaponHasScope);
	ImportConsoleCommand("SetGameSetting");
	ImportConsoleCommand("SetINISetting");
	ImportConsoleCommand("GetINISetting");
	ImportConsoleCommand("RefreshINI");
	ImportConsoleCommand("Save");
	ImportConsoleCommand("SaveINI");
	ImportConsoleCommand("QuitGame");
	ImportConsoleCommand("LoadGame");
	ImportConsoleCommand("CloseAllMenus");
	ImportConsoleCommand("SetVel");
	ADD_CMD(ListGetCount);
	ADD_CMD_RET(ListGetNthForm, kRetnType_Form);
	ADD_CMD(ListGetFormIndex);
	ADD_CMD(ListAddForm);
	ADD_CMD(ListAddReference);
	ADD_CMD(ListRemoveNthForm);
	ADD_CMD(ListRemoveForm);
	ADD_CMD(ListReplaceNthForm);
	ADD_CMD(ListReplaceForm);
	ADD_CMD(ListClear);
	ADD_CMD_RET(GetEquippedObject, kRetnType_Form);
	ADD_CMD(GetEquippedCurrentHealth);
	ADD_CMD(CompareNames);
	ADD_CMD(SetName);
	ADD_CMD(GetHotkeyItem);
	ADD_CMD(GetNumItems);
	ADD_CMD_RET(GetInventoryObject, kRetnType_Form);
	ADD_CMD(SetEquippedCurrentHealth);
	ADD_CMD(GetCurrentHealth);
	ADD_CMD(SetCurrentHealth);
	ADD_CMD(IsKeyPressed);
	ADD_CMD(TapKey);
	ADD_CMD(HoldKey);
	ADD_CMD(ReleaseKey);
	ADD_CMD(DisableKey);
	ADD_CMD(EnableKey);
	ADD_CMD(GetNumKeysPressed);
	ADD_CMD(GetKeyPress);
	ADD_CMD(GetNumMouseButtonsPressed);
	ADD_CMD(GetMouseButtonPress);
	ADD_CMD(GetControl);
	ADD_CMD(GetAltControl);
	ADD_CMD(MenuTapKey);
	ADD_CMD(MenuHoldKey);
	ADD_CMD(MenuReleaseKey);
	ADD_CMD(DisableControl);
	ADD_CMD(EnableControl);
	ADD_CMD(TapControl);
	ADD_CMD(SetControl);
	ADD_CMD(SetAltControl);
	ADD_CMD(SetIsControl);
	ADD_CMD(IsControl);
	ADD_CMD(IsKeyDisabled);
	ADD_CMD(IsControlDisabled);
	ADD_CMD(IsControlPressed);
	ADD_CMD(IsPersistent);
	ADD_CMD_RET(GetParentCell, kRetnType_Form);
	ADD_CMD_RET(GetParentWorldspace, kRetnType_Form);
	ADD_CMD_RET(GetTeleportCell, kRetnType_Form);
	ADD_CMD_RET(GetLinkedDoor, kRetnType_Form);
	ADD_CMD_RET(GetFirstRef, kRetnType_Form);
	ADD_CMD_RET(GetNextRef, kRetnType_Form);
	ADD_CMD(GetNumRefs);
	ADD_CMD_RET(GetFirstRefInCell, kRetnType_Form);
	ADD_CMD(GetNumRefsInCell);
	ADD_CMD(GetRefCount);
	ADD_CMD(SetRefCount);
	ADD_CMD(GetArmorAR);
	ADD_CMD(IsPowerArmor);
	ADD_CMD(SetIsPowerArmor);
	ADD_CMD(SetRepairList);
	ADD_CMD(IsQuestItem);
	ADD_CMD(SetQuestItem);
	ADD_CMD_RET(GetObjectEffect, kRetnType_Form);
	ADD_CMD(SetWeaponAmmo);
	ADD_CMD(SetWeaponClipRounds);
	ADD_CMD(SetAttackDamage);
	ADD_CMD(SetWeaponType);
	ADD_CMD(SetWeaponMinSpread);
	ADD_CMD(SetWeaponSpread);
	ADD_CMD(SetWeaponProjectile);
	ADD_CMD(SetWeaponSightFOV);
	ADD_CMD(SetWeaponMinRange);
	ADD_CMD(SetWeaponMaxRange);
	ADD_CMD(SetWeaponAmmoUse);
	ADD_CMD(SetWeaponActionPoints);
	ADD_CMD(SetWeaponCritDamage);
	ADD_CMD(SetWeaponCritChance);
	ADD_CMD(SetWeaponCritEffect);
	ADD_CMD(SetWeaponAnimAttackMult);
	ADD_CMD(SetWeaponAnimMult);
	ADD_CMD(SetWeaponReach);
	ADD_CMD(SetWeaponIsAutomatic);
	ADD_CMD(SetWeaponHandGrip);
	ADD_CMD(SetWeaponReloadAnim);
	ADD_CMD(SetWeaponBaseVATSChance);
	ADD_CMD(SetWeaponAttackAnimation);
	ADD_CMD(SetWeaponNumProjectiles);
	ADD_CMD(SetWeaponAimArc);
	ADD_CMD(SetWeaponLimbDamageMult);
	ADD_CMD(SetWeaponSightUsage);
	ADD_CMD(GetNumericGameSetting);
	ADD_CMD(SetNumericGameSetting);
	ADD_CMD(GetNumericIniSetting);
	ADD_CMD(SetNumericIniSetting);
	ADD_CMD(Label);
	ADD_CMD(Goto);
	ADD_CMD(PrintToConsole);
	ADD_CMD(DebugPrint);
	ADD_CMD(SetDebugMode);
	ADD_CMD(GetDebugMode);

	// beta 2
	ADD_CMD(GetUIFloat);
	ADD_CMD(SetUIFloat);
	ADD_CMD(SetUIString);
	ADD_CMD_RET(GetCrosshairRef, kRetnType_Form);
	ADD_CMD(GetGameRestarted);
	ImportConsoleCommand("ToggleMenus");
	ImportConsoleCommand("TFC");	// changed from ToggleFreeCamera
	ImportConsoleCommand("TCL");	// changed from ToggleCollision
	ADD_CMD(GetGameLoaded);
	ADD_CMD(GetWeaponItemMod);
	ADD_CMD(IsModLoaded);
	ADD_CMD(GetModIndex);
	ADD_CMD(GetNumLoadedMods);
	ADD_CMD(GetSourceModIndex);
	ADD_CMD(GetDebugSelection);
	ADD_CMD(GetArmorDT);
	ADD_CMD(SetArmorAR);
	ADD_CMD(SetArmorDT);

	// beta 3
	ADD_CMD(IsScripted);
	ADD_CMD_RET(GetScript, kRetnType_Form);
	ADD_CMD(RemoveScript);
	ADD_CMD(SetScript);
	ADD_CMD(IsFormValid);
	ADD_CMD(IsReference);

	// beta 4 - compat with 1.2.0.285

	// beta 5
	ADD_CMD(GetWeaponRequiredStrength);
	ADD_CMD(GetWeaponRequiredSkill);
	ADD_CMD(SetWeaponRequiredStrength);
	ADD_CMD(SetWeaponRequiredSkill);
	ADD_CMD(SetWeaponResistType);
	ADD_CMD(SetWeaponSkill);
	ADD_CMD(GetAmmoSpeed);
	ADD_CMD(GetAmmoConsumedPercent);
	ADD_CMD(GetAmmoCasing);
	ADD_CMD(GetPlayerCurrentAmmoRounds);
	ADD_CMD(SetPlayerCurrentAmmoRounds);
	ADD_CMD(GetPlayerCurrentAmmo);

	// beta 6 - compat with 1.2.0.314
	ADD_CMD_RET(GetOpenKey, kRetnType_Form);
	ADD_CMD(Exp);
	ADD_CMD(Log10);
	ADD_CMD(Floor);
	ADD_CMD(Ceil);
	ADD_CMD(LeftShift);
	ADD_CMD(RightShift);
	ADD_CMD(LogicalAnd);
	ADD_CMD(LogicalOr);
	ADD_CMD(LogicalXor);
	ADD_CMD(LogicalNot);
	ADD_CMD(Pow);
	ADD_CMD(Fmod);
	ADD_CMD(Rand);

	// beta 7 - compat with 1.2.0.352
	
	// beta 8 - rewrite loader to work around steam bugs
	
	// beta 9 - compat with 1.3.0.452
	ADD_CMD(SortUIListBox);
	ADD_CMD_RET(GetOwner, kRetnType_Form);

	// beta 10 - compat with editor 1.3.0.452

	RecordReleaseVersion();

	// 2 beta 1
	ADD_CMD(GetLocalRefIndex);
	ADD_CMD_RET(BuildRef, kRetnType_Form);
	ADD_CMD(SetNameEx);
	ADD_CMD(MessageEx);
	ADD_CMD(MessageBoxEx);
	ADD_CMD_RET(TempCloneForm, kRetnType_Form);
	ADD_CMD(IsClonedForm);
	ADD_CMD_RET(GetParentCellOwner, kRetnType_Form);
	ADD_CMD(GetOwningFactionRequiredRank);
	ADD_CMD(GetParentCellOwningFactionRequiredRank);
	
	// 2 beta 2
	ADD_CMD(SetUIStringEx);

	// 2 beta 3
	// 2 beta 4

	// 2 beta 5
	ImportConsoleCommand("SetUFOCamSpeedMult");
	ImportConsoleCommand("TDT");
	ADD_CMD(SetWeaponFireRate);
	ADD_CMD(GetWeaponLongBursts);
	ADD_CMD(SetWeaponLongBursts);

	// 2 beta 6 - compat with 1.4.0.525
	ADD_CMD(GetWeaponFlags1);
	ADD_CMD(GetWeaponFlags2);
	ADD_CMD(SetWeaponFlags1);
	ADD_CMD(SetWeaponFlags2);
	ADD_CMD(GetActorBaseFlagsLow);
	ADD_CMD(SetActorBaseFlagsLow);
	ADD_CMD(GetActorBaseFlagsHigh);
	ADD_CMD(SetActorBaseFlagsHigh);
	ADD_CMD(ClearBit);
	ADD_CMD(SetBit);

	// 2 beta 7 - quick fix for InterfaceManager ptr

	// 2 beta 8 - compat with editor 1.4.0.518

	// 2 beta 9 - quick fix for IsControlDisabled

	// 2 beta 10 - compat with nogore runtime 1.4.0.525
	ADD_CMD(GetEquippedWeaponModFlags);
	ADD_CMD(SetEquippedWeaponModFlags);
	ADD_CMD(GetWeaponItemModEffect);
	ADD_CMD(GetWeaponItemModValue1);
	ADD_CMD(GetWeaponItemModValue2);

	// 2 beta 11 - fixed TESObjectWEAP struct (mod value 2 off by one)
	// 2 beta 12 - fixed GetWeaponItemModEffect etc. using 0-based indexing
}

void CommandTable::AddCommandsV3s()
{
	RecordReleaseVersion();

	// 3 alpha 01 sub 1 - submission hlp
	ADD_CMD(HasOwnership);
	ADD_CMD(IsOwned);
	ADD_CMD(SetOwningFactionRequiredRank);
	ADD_CMD_RET(GetDialogueTarget, kRetnType_Form);
	ADD_CMD_RET(GetDialogueSubject, kRetnType_Form);
	ADD_CMD_RET(GetDialogueSpeaker, kRetnType_Form);
	ADD_CMD_RET(SetPackageLocationReference, kRetnType_Form);
	ADD_CMD(GetAgeClass);
	ADD_CMD(RemoveMeIR);
	ADD_CMD(CopyIR);
	ADD_CMD_RET(CreateTempRef, kRetnType_Form);
	ADD_CMD_RET(GetFirstRefForItem, kRetnType_Form);
	ADD_CMD_RET(GetNextRefForItem, kRetnType_Form);
	ADD_CMD(AddItemOwnership);
	ADD_CMD(AddItemHealthPercentOwner);
	ADD_CMD(GetTokenValue);
	ADD_CMD(SetTokenValue);
	ADD_CMD_RET(GetTokenRef, kRetnType_Form);
	ADD_CMD(SetTokenRef);
	ADD_CMD(SetTokenValueAndRef);
	ADD_CMD(GetPaired);
	ADD_CMD(GetRespawn);
	ADD_CMD(SetRespawn);
	ADD_CMD(GetPermanent);
	ADD_CMD(SetPermanent);
	ADD_CMD_RET(GetBaseForm, kRetnType_Form);
	ADD_CMD(IsRefInList);

#ifdef RUNTIME
	// 3 beta 02 sub 5 - submission hlp
	GetByName("GetFactionRank")->eval = &Cmd_GetFactionRank_Eval;
	GetByName("GetFactionRank")->execute = &Cmd_GetFactionRank_Execute;
#endif

	// 3 beta 03 - forgotten ?
	ADD_CMD(SetOpenKey);

	// 3 beta 03 - submission hlp
	ADD_CMD_RET(GetCurrentPackage, kRetnType_Form);
	ADD_CMD(GetPackageLocation);
	ADD_CMD(GetPackageCount);
	ADD_CMD_RET(GetNthPackage, kRetnType_Form);
	ADD_CMD(SetNthPackage);
	ADD_CMD(AddPackageAt);
	ADD_CMD(RemovePackageAt);
	ADD_CMD(RemoveAllPackages);

	// 3 beta 04 - submission hlp
	ADD_CMD(ClearOpenKey);

	// 3 beta 05 - submission hlp
	ADD_CMD(SetPackageTargetReference);
	ADD_CMD(SetPackageTargetCount);
	ADD_CMD(GetPackageTargetCount);
	ADD_CMD(SetPackageLocationRadius);
	ADD_CMD(GetPackageLocationRadius);
}

void CommandTable::AddCommandsV4()
{
	RecordReleaseVersion();

	// 4.1 beta 01 - submission hlp
	ADD_CMD(SetEyes);
	ADD_CMD_RET(GetEyes, kRetnType_Form);
	ADD_CMD(SetHair);
	ADD_CMD_RET(GetHair, kRetnType_Form);
	ADD_CMD(GetHairLength);
	ADD_CMD(SetHairLength);
	ADD_CMD(GetHairColor);
	ADD_CMD(SetHairColor);
	ADD_CMD(GetNPCWeight);
	ADD_CMD(SetNPCWeight);
	ADD_CMD(GetNPCHeight);
	ADD_CMD(SetNPCHeight);
	ADD_CMD(Update3D);

#ifdef RUNTIME
	// 4.1 beta 01 - submission hlp
	GetByName("ModFactionRank")->execute = &Cmd_ModFactionRank_Execute;
#endif

	// 4.1 beta 01 - subset of CommandScripts from OBSE 21 beta 4
	ADD_CMD(GetVariable);
	ADD_CMD(HasVariable);
	ADD_CMD(GetRefVariable);
	ADD_CMD_RET(GetArrayVariable, kRetnType_Array);		// corrected in version 4.5 Beta 7
	ADD_CMD(CompareScripts);
	ADD_CMD(ResetAllVariables);
	ADD_CMD(GetNumExplicitRefs);
	ADD_CMD_RET(GetNthExplicitRef, kRetnType_Form);
	ADD_CMD(RunScript);
	ADD_CMD_RET(GetCurrentScript, kRetnType_Form);
	ADD_CMD_RET(GetCallingScript, kRetnType_Form);

	// 4.1 beta 01 - general merged into scripting
	ADD_CMD(Let);
	ADD_CMD(eval);
	ADD_CMD(While);
	ADD_CMD(Loop);
	ADD_CMD(ForEach);
	ADD_CMD(Continue);
	ADD_CMD(Break);
	ADD_CMD_RET(ToString, kRetnType_String);
	ADD_CMD(Print);
	ADD_CMD(testexpr);
	ADD_CMD_RET(TypeOf, kRetnType_String);
	ADD_CMD(Function);
	ADD_CMD_RET(Call, kRetnType_Ambiguous);
	ADD_CMD(SetFunctionValue);
	ADD_CMD_RET(GetUserTime, kRetnType_Array);	// corrected in version 4.2 Beta 4 alpha 1
	ADD_CMD_RET(GetModLocalData, kRetnType_Ambiguous);	// corrected in version 5.0 Beta 3
	ADD_CMD(SetModLocalData);							// restored in version 5.0 Beta 3
	ADD_CMD(ModLocalDataExists);
	ADD_CMD(RemoveModLocalData);
	ADD_CMD_RET(GetAllModLocalData, kRetnType_Array);	// corrected in version 4.5 Beta 6
	ADD_CMD(Internal_PushExecutionContext);
	ADD_CMD(Internal_PopExecutionContext);

	// 4.1 beta 01 - Arrays (except "NI" stuff)
	ADD_CMD_RET(ar_Construct, kRetnType_Array);
	ADD_CMD(ar_Size);
	ADD_CMD(ar_Dump);
	ADD_CMD(ar_DumpID);
	ADD_CMD(ar_Erase);
	ADD_CMD_RET(ar_Sort, kRetnType_Array);
	ADD_CMD_RET(ar_CustomSort, kRetnType_Array);
	ADD_CMD_RET(ar_SortAlpha, kRetnType_Array);
	ADD_CMD_RET(ar_Find, kRetnType_ArrayIndex);
	ADD_CMD_RET(ar_First, kRetnType_ArrayIndex);
	ADD_CMD_RET(ar_Last, kRetnType_ArrayIndex);
	ADD_CMD_RET(ar_Next, kRetnType_ArrayIndex);
	ADD_CMD_RET(ar_Prev, kRetnType_ArrayIndex);
	ADD_CMD_RET(ar_Keys, kRetnType_Array);
	ADD_CMD(ar_HasKey);
	ADD_CMD_RET(ar_BadStringIndex, kRetnType_String);
	ADD_CMD(ar_BadNumericIndex);
	ADD_CMD_RET(ar_Copy, kRetnType_Array);
	ADD_CMD_RET(ar_DeepCopy, kRetnType_Array);
	ADD_CMD_RET(ar_Null, kRetnType_Array);
	ADD_CMD(ar_Resize);
	ADD_CMD(ar_Insert);
	ADD_CMD(ar_InsertRange);
	ADD_CMD(ar_Append);
	ADD_CMD_RET(ar_List, kRetnType_Array);
	ADD_CMD_RET(ar_Map, kRetnType_Array);
	ADD_CMD_RET(ar_Range, kRetnType_Array);


	// 4.1 beta 01 - StringVar
	ADD_CMD(sv_Destruct);
	ADD_CMD_RET(sv_Construct, kRetnType_String);
	ADD_CMD(sv_Set);
	ADD_CMD(sv_Compare);
	ADD_CMD(sv_Length);
	ADD_CMD(sv_Erase);
	ADD_CMD_RET(sv_SubString, kRetnType_String);
	ADD_CMD(sv_ToNumeric);
	ADD_CMD(sv_Insert);
	ADD_CMD(sv_Count);
	ADD_CMD(sv_Find);
	ADD_CMD(sv_Replace);
	ADD_CMD(sv_GetChar);
	ADD_CMD_RET(sv_Split, kRetnType_Array);
	ADD_CMD_RET(sv_Percentify, kRetnType_String);
	ADD_CMD_RET(sv_ToUpper, kRetnType_String);
	ADD_CMD_RET(sv_ToLower, kRetnType_String);

	ADD_CMD(IsLetter);
	ADD_CMD(IsDigit);
	ADD_CMD(IsPrintable);
	ADD_CMD(IsPunctuation);
	ADD_CMD(IsUpperCase);
	ADD_CMD(CharToAscii);

	ADD_CMD(ToUpper);
	ADD_CMD(ToLower);
	ADD_CMD_RET(AsciiToChar, kRetnType_String);
	ADD_CMD_RET(NumToHex, kRetnType_String);
	ADD_CMD(ToNumber);

	ADD_CMD_RET(GetNthModName, kRetnType_String);
	ADD_CMD_RET(GetName, kRetnType_String);
	ADD_CMD_RET(GetKeyName, kRetnType_String);
	ADD_CMD_RET(GetFormIDString, kRetnType_String);
	ADD_CMD_RET(GetRawFormIDString, kRetnType_String);
	ADD_CMD_RET(GetFalloutDirectory, kRetnType_String);
	ADD_CMD_RET(ActorValueToString, kRetnType_String);
	ADD_CMD_RET(ActorValueToStringC, kRetnType_String);

	// 4 beta 01 - using strings with forms
	ADD_CMD_RET(GetModelPath, kRetnType_String);
	ADD_CMD_RET(GetIconPath, kRetnType_String);
	ADD_CMD_RET(GetBipedModelPath, kRetnType_String);
	ADD_CMD_RET(GetBipedIconPath, kRetnType_String);
	ADD_CMD_RET(GetTexturePath, kRetnType_String);
	ADD_CMD(SetModelPathEX);
	ADD_CMD(SetIconPathEX);
	ADD_CMD(SetBipedIconPathEX);
	ADD_CMD(SetBipedModelPathEX);
	ADD_CMD(SetTexturePath);

	ADD_CMD_RET(GetNthFactionRankName, kRetnType_String);
	ADD_CMD(SetNthFactionRankNameEX);

	ADD_CMD_RET(GetStringGameSetting, kRetnType_String);
	ADD_CMD(SetStringGameSettingEX);

	// 4.2 beta 02
	ADD_CMD_RET(GetRace, kRetnType_Form);
	ADD_CMD_RET(GetRaceName, kRetnType_String);
	ImportConsoleCommand("SCOF");
	ADD_CMD(PickOneOf);

	// 4.2 beta 03 alpha 5
	ADD_CMD(IsPlayerSwimming);
	ADD_CMD(GetTFC);

	// 4.2 beta 03 alpha 6	- Algohol OBSE plugin by emtim (with permission)

	//	Vector3 commands
	ADD_CMD( V3Length );
	ADD_CMD( V3Normalize );
	//ADD_CMD( V3Dotproduct );
	ADD_CMD( V3Crossproduct );

	//	Quaternion commands
	ADD_CMD( QFromEuler );
	ADD_CMD( QFromAxisAngle );
	ADD_CMD( QNormalize );
	ADD_CMD( QMultQuatQuat );
	ADD_CMD( QMultQuatVector3 );
	ADD_CMD( QToEuler );
	ADD_CMD( QInterpolate );

	// 4.2 beta 04 alpha 1
	ADD_CMD(IsPlayable);
	ADD_CMD(SetIsPlayable);
	ADD_CMD(GetEquipmentSlotsMask);
	ADD_CMD(SetEquipmentSlotsMask);
	ImportConsoleCommand("SQV");		// requires ConsoleEcho to be turned on!
	ADD_CMD(GetConsoleEcho);
	ADD_CMD(SetConsoleEcho);

	// 4.2 beta 04 alpha 5
	ADD_CMD_RET(GetScopeModelPath, kRetnType_String);
	ADD_CMD(SetScopeModelPath);

	// 4.3 and 4.4 sk�pped

	// 4.5 beta 01 none added

	// 4.5 beta 02
	ADD_CMD(EndVATScam);	// Provided by Queued

	// 4.5 beta 06
	ADD_CMD(EquipItem2);	// EquipItem2 is broken in the sense that the item is equipped after equipitem returns so which item is equipped is not detected :(
	ADD_CMD(EquipMe);
	ADD_CMD(UnequipMe);
	ADD_CMD(IsEquipped);
	ADD_CMD_RET(GetInvRefsForItem, kRetnType_Array);
	ADD_CMD(SetHotkeyItem);
	ADD_CMD(ClearHotkey);
	ADD_CMD(PrintDebug);
	ADD_CMD(SetVariable);
	ADD_CMD(SetRefVariable);
	ImportConsoleCommand("ShowVars");		// requires ConsoleEcho to be turned on!
	ADD_CMD_RET(GetStringIniSetting, kRetnType_String);
	ADD_CMD(SetStringIniSetting);

	// 4.5 beta 07

	ADD_CMD(GetPerkRank);										// Tested
	ADD_CMD(GetAltPerkRank);									// Tested
	ADD_CMD(GetEquipmentBipedMask);								// Tested
	ADD_CMD(SetEquipmentBipedMask);								// Tested
	ADD_CMD_RET(GetRefs, kRetnType_Array);						// Tested
	ADD_CMD_RET(GetRefsInCell, kRetnType_Array);				// Tested
	ADD_CMD(GetBaseNumFactions);								// Tested
	ADD_CMD_RET(GetBaseNthFaction, kRetnType_Form);				// Tested
	ADD_CMD(GetBaseNthRank);									// Tested
	ADD_CMD(GetNumRanks);										// Tested
	ADD_CMD_RET(GetRaceHairs, kRetnType_Array);					// Tested
	ADD_CMD_RET(GetRaceEyes, kRetnType_Array);					// Tested
	ADD_CMD_RET(GetBaseSpellListSpells, kRetnType_Array);		// Tested
	ADD_CMD_RET(GetBaseSpellListLevSpells, kRetnType_Array);	// Tested but no data
	ADD_CMD_RET(GetBasePackages, kRetnType_Array);				// Tested
	ADD_CMD_RET(GetBaseFactions, kRetnType_Array);				// Tested
	ADD_CMD_RET(GetBaseRanks, kRetnType_Array);					// Tested
	ADD_CMD_RET(GetActiveFactions, kRetnType_Array);			// Tested
	ADD_CMD_RET(GetActiveRanks, kRetnType_Array);				// Tested
	ADD_CMD_RET(GetFactionRankNames, kRetnType_Array);			// Tested
	ADD_CMD_RET(GetFactionRankFemaleNames, kRetnType_Array);	// Tested
	ADD_CMD_RET(GetHeadParts, kRetnType_Array);					// Tested
	ADD_CMD_RET(GetLevCreatureRefs, kRetnType_Array);			// Tested
	ADD_CMD_RET(GetLevCharacterRefs, kRetnType_Array);			// Tested
	ADD_CMD_RET(GetListForms, kRetnType_Array);					// Tested
	ADD_CMD(GenericAddForm);									// Tested									
	ADD_CMD_RET(GenericReplaceForm, kRetnType_Form);			// Tested
	ADD_CMD_RET(GenericDeleteForm, kRetnType_Form);				// Tested
	ADD_CMD(IsPluginInstalled);									// Tested
	ADD_CMD(GetPluginVersion);									// Tested
	ADD_CMD_RET(GenericGetForm, kRetnType_Form);				// Tested
	ImportConsoleCommand("INV");								// Tested
	ADD_CMD_RET(GetNthDefaultForm, kRetnType_Form);				// Tested
	ADD_CMD(SetNthDefaultForm);									// Tested
	ADD_CMD_RET(GetDefaultForms, kRetnType_Array);				// Tested

	// 4.5 beta 08 private. Fix only

	// 4.6 beta 01
	ADD_CMD(GetGridsToLoad);
	ADD_CMD(OutputLocalMapPicturesOverride);
	ADD_CMD(SetOutputLocalMapPicturesGrids);

	// Events
	ADD_CMD(SetEventHandler);
	ADD_CMD(RemoveEventHandler);
	ADD_CMD_RET(GetCurrentEventName, kRetnType_String);
	ADD_CMD(DispatchEvent);

	ADD_CMD(GetInGrid);
	ADD_CMD(GetInGridInCell);									// Name is bad, but in line with others

	// 4.6 beta 02 : Fixes only

	// 4.6 beta 03
	ADD_CMD(AddSpellNS);
	ADD_CMD(GetFlagsLow);
	ADD_CMD(SetFlagsLow);
	ADD_CMD(GetFlagsHigh);
	ADD_CMD(SetFlagsHigh);
	ADD_CMD(HasConsoleOutputFilename);
	ADD_CMD_RET(GetConsoleOutputFilename, kRetnType_String);
	ADD_CMD(PrintF);
	ADD_CMD(PrintDebugF);
	ImportConsoleCommand("TFIK");
	ADD_CMD(IsLoadDoor);
	ADD_CMD(GetDoorTeleportX);
	ADD_CMD(GetDoorTeleportY);
	ADD_CMD(GetDoorTeleportZ);
	ADD_CMD(GetDoorTeleportRot);
	ADD_CMD(SetDoorTeleport);

	// 4.6 beta 04 : Never public
	ADD_CMD(GenericCheckForm);
	ADD_CMD(GetEyesFlags);
	ADD_CMD(SetEyesFlags);
	ADD_CMD(GetHairFlags);
	ADD_CMD(SetHairFlags);

}

void CommandTable::AddCommandsV5()
{
	RecordReleaseVersion();

	// 5.0 beta 01
	ADD_CMD(ar_Packed);
	ADD_CMD(GetActorFIKstatus);
	ADD_CMD(SetActorFIKstatus);
	ADD_CMD(GetBit);

	// 5.0 beta 02
	ADD_CMD(HasEffectShader);

	// 5.0 beta 03
	ADD_CMD_RET(GetCurrentQuestObjectiveTeleportLinks, kRetnType_Array);
	
	// Port of trig functions from OBSE
	ADD_CMD(ATan2);
	ADD_CMD(Sinh);
	ADD_CMD(Cosh);
	ADD_CMD(Tanh);
	ADD_CMD(dSin);
	ADD_CMD(dCos);
	ADD_CMD(dTan);
	ADD_CMD(dASin);
	ADD_CMD(dACos);
	ADD_CMD(dATan);
	ADD_CMD(dATan2);
	ADD_CMD(dSinh);
	ADD_CMD(dCosh);
	ADD_CMD(dTanh);

	// 5.1 beta 01
	ADD_CMD_RET(GetNthAnimation, kRetnType_String);
	ADD_CMD(AddAnimation);
	ADD_CMD(DelAnimation);
	ADD_CMD(DelAnimations);
	ADD_CMD_RET(GetClass, kRetnType_Form);
	ADD_CMD_RET(GetNameOfClass, kRetnType_String);
	ADD_CMD(ShowLevelUpMenu);


}

void CommandTable::AddCommandsV6()
{
	RecordReleaseVersion();
	// 6.0 beta 08
	ADD_CMD(GetUIFloatAlt);
	ADD_CMD(SetUIFloatAlt);
	ADD_CMD(SetUIStringAlt);
	
	// 6.1 beta 00
	ADD_CMD(CallAfterSeconds);
	ADD_CMD(CallWhile);
	ADD_CMD(CallForSeconds);
	ADD_CMD(ar_DumpF);
}

namespace PluginAPI
{
	const CommandInfo* GetCmdTblStart() { return g_scriptCommands.GetStart(); }
	const CommandInfo* GetCmdTblEnd() { return g_scriptCommands.GetEnd(); }
	const CommandInfo* GetCmdByOpcode(UInt32 opcode) { return g_scriptCommands.GetByOpcode(opcode); }
	const CommandInfo* GetCmdByName(const char* name) { return g_scriptCommands.GetByName(name); }
	UInt32 GetCmdRetnType(const CommandInfo* cmd) { return g_scriptCommands.GetReturnType(cmd); }
	UInt32 GetReqVersion(const CommandInfo* cmd) { return g_scriptCommands.GetRequiredNVSEVersion(cmd); }
	const PluginInfo* GetCmdParentPlugin(const CommandInfo* cmd) { return g_scriptCommands.GetParentPlugin(cmd); }
	const PluginInfo* GetPluginInfoByName(const char *pluginName) {	return g_pluginManager.GetInfoByName(pluginName); }
}
