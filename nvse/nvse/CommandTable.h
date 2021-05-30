#pragma once

#include <unordered_map>
#include <vector>

class TESObjectREFR;
class Script;
struct ScriptEventList;
struct ScriptLineBuffer;
struct ScriptBuffer;
struct PluginInfo;

// for IsInventoryObjectType list, see GameForms.h

enum ParamType
{
	kParamType_String =					0x00,
	kParamType_Integer =				0x01,
	kParamType_Float =					0x02,
	kParamType_ObjectID =				0x03,	// GetItemCount				TESForm *, must pass IsInventoryObjectType and TESForm::Unk_3A
	kParamType_ObjectRef =				0x04,	// Activate					TESObjectREFR *, REFR-PFLA
	kParamType_ActorValue =				0x05,	// ModActorValue			UInt32 *, immediate UInt16
	kParamType_Actor =					0x06,	// ToggleAI					TESObjectREFR *, must pass IsActor (ACHR-ACRE)
	kParamType_SpellItem =				0x07,	// AddSpell					TESForm *, must be either SpellItem or book
	kParamType_Axis =					0x08,	// Rotate					char *, immediate char, X Y Z
	kParamType_Cell =					0x09,	// GetInCell				TESObjectCELL *, must be cell
	kParamType_AnimationGroup =			0x0A,	// PlayGroup				UInt32 *, immediate UInt16
	kParamType_MagicItem =				0x0B,	// Cast						MagicItem *
	kParamType_Sound =					0x0C,	// Sound					TESForm *, kFormType_TESSound
	kParamType_Topic =					0x0D,	// Say						TESForm *, kFormType_TESTopicog
	kParamType_Quest =					0x0E,	// ShowQuestVars			TESForm *, kFormType_TESQuest
	kParamType_Race =					0x0F,	// GetIsRace				TESForm *, kFormType_TESRace
	kParamType_Class =					0x10,	// GetIsClass				TESForm *, kFormType_TESClass
	kParamType_Faction =				0x11,	// Faction					TESForm *, kFormType_TESFaction
	kParamType_Sex =					0x12,	// GetIsSex					UInt32 *, immediate UInt16
	kParamType_Global =					0x13,	// GetGlobalValue			TESForm *, kFormType_TESGlobal
	kParamType_Furniture =				0x14,	// IsCurrentFurnitureObj	TESForm *, kFormType_TESFurniture or kFormType_BGSListForm
	kParamType_TESObject =				0x15,	// PlaceAtMe				TESObject *, must pass TESForm::Unk_3A
	kParamType_VariableName =			0x16,	// GetQuestVariable			only works in conditionals
	kParamType_QuestStage =				0x17,	// SetStage					handled like integer
	kParamType_MapMarker =				0x18,	// ShowMap					TESObjectREFR *, see ObjectRef
	kParamType_ActorBase =				0x19,	// SetEssential				TESActorBase * (NPC / creature)
	kParamType_Container =				0x1A,	// RemoveMe					TESObjectREFR *, see ObjectRef
	kParamType_WorldSpace =				0x1B,	// CenterOnWorld			TESWorldSpace *, kFormType_TESWorldSpace
	kParamType_CrimeType =				0x1C,	// GetCrimeKnown			UInt32 *, immediate UInt16
	kParamType_AIPackage =				0x1D,	// GetIsCurrentPackage		TESPackage *, kFormType_TESPackage
	kParamType_CombatStyle =			0x1E,	// SetCombatStyle			TESCombatStyle *, kFormType_TESCombatStyle
	kParamType_MagicEffect =			0x1F,	// HasMagicEffect			EffectSetting *
	kParamType_FormType =				0x20,	// GetIsUsedItemType		UInt8 *, immediate UInt16
	kParamType_WeatherID =				0x21,	// GetIsCurrentWeather		TESForm *, kFormType_TESWeather
	kParamType_NPC =					0x22,	// unused					TESNPC *, kFormType_TESNPC
	kParamType_Owner =					0x23,	// IsOwner					TESForm *, kFormType_TESNPC or kFormType_TESFaction
	kParamType_EffectShader =			0x24,	// PlayMagicShaderVisuals	TESForm *, kFormType_TESEffectShader
	kParamType_FormList	=				0x25,	// IsInList					kFormType_BGSListForm
	kParamType_MenuIcon =				0x26,	// unused					kFormType_BGSMenuIcon
	kParamType_Perk =					0x27,	// Add Perk					kFormType_BGSPerk
	kParamType_Note =					0x28,	// Add Note					kFormType_BGSNote
	kParamType_MiscellaneousStat =		0x29,	// ModPCMiscStat			UInt32 *, immediate UInt16
	kParamType_ImageSpaceModifier =		0x2A,	//							kFormType_TESImageSpaceModifier
	kParamType_ImageSpace =				0x2B,	//							kFormType_TESImageSpace
	kParamType_Double =					0x2C,	// 
	kParamType_ScriptVariable =			0x2D,	// 
	kParamType_Unhandled2E =			0x2E,	// 
	kParamType_EncounterZone =			0x2F,	//							kFormType_BGSEncounterZone
	kParamType_IdleForm =				0x30,	// 
	kParamType_Message =				0x31,	//							kFormType_BGSMessage
	kParamType_InvObjOrFormList =		0x32,	// AddItem					IsInventoryObjectType or kFormType_BGSListForm
	kParamType_Alignment =				0x33,	// GetIsAlignment			UInt32 *, immediate UInt16
	kParamType_EquipType =				0x34,	// GetIsUsedEquipType		UInt32 *, immediate UInt16
	kParamType_NonFormList =			0x35,	// GetIsUsedItem			TESForm::Unk_3A and not kFormType_BGSListForm
	kParamType_SoundFile =				0x36,	// PlayMusic				kFormType_BGSMusicType
	kParamType_CriticalStage =			0x37,	// SetCriticalStage			UInt32 *, immediate UInt16

	// added for dlc (1.1)
	kParamType_LeveledOrBaseChar =		0x38,	// AddNPCToLeveledList		NPC / LeveledCharacter
	kParamType_LeveledOrBaseCreature =	0x39,	// AddCreatureToLeveledList	Creature / LeveledCreature
	kParamType_LeveledChar =			0x3A,	// AddNPCToLeveledList		kFormType_TESLevCharacter
	kParamType_LeveledCreature =		0x3B,	// AddCreatureToLeveledList	kFormType_TESLevCreature
	kParamType_LeveledItem =			0x3C,	// AddItemToLeveledList		kFormType_TESLevItem
	kParamType_AnyForm =				0x3D,	// AddFormToFormList		any form

	// new vegas
	kParamType_Reputation =				0x3E,	//							kFormType_TESReputation
	kParamType_Casino =					0x3F,	//							kFormType_TESCasino
	kParamType_CasinoChip =				0x40,	//							kFormType_TESCasinoChips
	kParamType_Challenge =				0x41,	//							kFormType_TESChallenge
	kParamType_CaravanMoney =			0x42,	//							kFormType_TESCaravanMoney
	kParamType_CaravanCard =			0x43,	//							kFormType_TESCaravanCard
	kParamType_CaravanDeck =			0x44,	//							kFormType_TESCaravanDeck
	kParamType_Region =					0x45,	//							kFormType_TESRegion

	// custom NVSE types
	kParamType_StringVar =			0x01,
	kParamType_Array =				0x100,	// only usable with compiler override; StandardCompile() will report unrecognized param type
};

enum CommandReturnType : UInt8
{
	kRetnType_Default,
	kRetnType_Form,
	kRetnType_String,
	kRetnType_Array,
	kRetnType_ArrayIndex,
	kRetnType_Ambiguous,

	kRetnType_Max
};

struct ParamInfo
{
	const char	* typeStr;
	UInt32		typeID;		// ParamType
	UInt32		isOptional;	// do other bits do things?
};

#define USE_EXTRACT_ARGS_EX NVSE_CORE

#define COMMAND_ARGS		ParamInfo * paramInfo, void * scriptData, TESObjectREFR * thisObj, TESObjectREFR * containingObj, Script * scriptObj, ScriptEventList * eventList, double * result, UInt32 * opcodeOffsetPtr
#define COMMAND_ARGS_EX		ParamInfo *paramInfo, void *scriptData, UInt32 *opcodeOffsetPtr, Script *scriptObj, ScriptEventList *eventList
#define PASS_COMMAND_ARGS	paramInfo, scriptData, thisObj, containingObj, scriptObj, eventList, result, opcodeOffsetPtr
#define COMMAND_ARGS_EVAL	TESObjectREFR * thisObj, void * arg1, void * arg2, double * result
#define PASS_CMD_ARGS_EVAL	thisObj, arg1, arg2, result
#define EXTRACT_ARGS_EX		paramInfo, scriptData, opcodeOffsetPtr, scriptObj, eventList
#define PASS_FMTSTR_ARGS	paramInfo, scriptData, opcodeOffsetPtr, scriptObj, eventList
#if USE_EXTRACT_ARGS_EX
#define EXTRACT_ARGS		EXTRACT_ARGS_EX
#else
#define EXTRACT_ARGS		paramInfo, scriptData, opcodeOffsetPtr, thisObj, containingObj, scriptObj, eventList
#endif

//Macro to make CommandInfo definitions a bit less tedious

#define DEFINE_CMD_FULL(name, altName, description, refRequired, numParams, paramInfo, parser) \
	extern bool Cmd_ ## name ## _Execute(COMMAND_ARGS); \
	static CommandInfo (kCommandInfo_ ## name) = { \
	#name, \
	#altName, \
	0, \
	#description, \
	refRequired, \
	numParams, \
	paramInfo, \
	HANDLER(Cmd_ ## name ## _Execute), \
	parser, \
	NULL, \
	0 \
	};

#define DEFINE_CMD_ALT(name, altName, description, refRequired, numParams, paramInfo) \
	DEFINE_CMD_FULL(name, altName, description, refRequired, numParams, paramInfo, Cmd_Default_Parse)	

#define DEFINE_CMD_ALT_EXP(name, altName, description, refRequired, paramInfo) \
	DEFINE_CMD_FULL(name, altName, description, refRequired, (sizeof(paramInfo) / sizeof(ParamInfo)), paramInfo, Cmd_Expression_Parse)	

#define DEFINE_COMMAND(name, description, refRequired, numParams, paramInfo) \
	DEFINE_CMD_FULL(name, , description, refRequired, numParams, paramInfo, Cmd_Default_Parse)	

#define DEFINE_CMD(name, description, refRequired, paramInfo) \
	DEFINE_COMMAND(name, description, refRequired, (sizeof(paramInfo) / sizeof(ParamInfo)), paramInfo)

#define DEFINE_COMMAND_EXP(name, description, refRequired, paramInfo) \
	DEFINE_CMD_ALT_EXP(name, , description, refRequired, paramInfo)	

#define DEFINE_COMMAND_PLUGIN(name, description, refRequired, numParams, paramInfo) \
	DEFINE_CMD_FULL(name, , description, refRequired, numParams, paramInfo, NULL)

#define DEFINE_COMMAND_ALT_PLUGIN(name, altName, description, refRequired, numParams, paramInfo) \
	DEFINE_CMD_FULL(name, altName, description, refRequired, numParams, paramInfo, NULL)

// for commands which can be used as conditionals
#define DEFINE_CMD_ALT_COND_ANY(name, altName, description, refRequired, paramInfo, parser) \
	extern bool Cmd_ ## name ## _Execute(COMMAND_ARGS); \
	extern bool Cmd_ ## name ## _Eval(COMMAND_ARGS_EVAL); \
	static CommandInfo (kCommandInfo_ ## name) = { \
	#name,	\
	#altName,		\
	0,		\
	#description,	\
	refRequired,	\
	(sizeof(paramInfo) / sizeof(ParamInfo)),	\
	paramInfo,	\
	HANDLER(Cmd_ ## name ## _Execute),	\
	parser,	\
	HANDLER_EVAL(Cmd_ ## name ## _Eval),	\
	1	\
	};

#define DEFINE_CMD_ALT_COND(name, altName, description, refRequired, paramInfo) \
	DEFINE_CMD_ALT_COND_ANY(name, altName, description, refRequired, paramInfo, Cmd_Default_Parse)

#define DEFINE_CMD_ALT_COND_PLUGIN(name, altName, description, refRequired, paramInfo) \
	DEFINE_CMD_ALT_COND_ANY(name, altName, description, refRequired, paramInfo, NULL)

#define DEFINE_CMD_COND(name, description, refRequired, paramInfo) \
	DEFINE_CMD_ALT_COND(name, , description, refRequired, paramInfo)

typedef bool (* Cmd_Execute)(COMMAND_ARGS);
bool Cmd_Default_Execute(COMMAND_ARGS);

typedef bool (* Cmd_Parse)(UInt32 numParams, ParamInfo * paramInfo, ScriptLineBuffer * lineBuf, ScriptBuffer * scriptBuf);
bool Cmd_Default_Parse(UInt32 numParams, ParamInfo * paramInfo, ScriptLineBuffer * lineBuf, ScriptBuffer * scriptBuf);

typedef bool (* Cmd_Eval)(COMMAND_ARGS_EVAL);
bool Cmd_Default_Eval(COMMAND_ARGS_EVAL);


#ifdef RUNTIME
#define HANDLER(x)	x
#define HANDLER_EVAL(x)	x
#else
#define HANDLER(x)	Cmd_Default_Execute
#define HANDLER_EVAL(x)	Cmd_Default_Eval
#endif

struct CommandInfo
{
	const char	* longName;		// 00
	const char	* shortName;	// 04
	UInt32		opcode;			// 08
	const char	* helpText;		// 0C
	UInt16		needsParent;	// 10
	UInt16		numParams;		// 12
	ParamInfo	* params;		// 14

	// handlers
	Cmd_Execute	execute;		// 18
	Cmd_Parse	parse;			// 1C
	Cmd_Eval	eval;			// 20

	UInt32		flags;			// 24		might be more than one field (reference to 25 as a byte)

	void	DumpFunctionDef() const;
	void	DumpDocs() const;
};

const UInt32 kNVSEOpcodeStart	= 0x1400;
const UInt32 kNVSEOpcodeTest	= 0x2000;

struct CommandMetadata
{
	CommandMetadata() :parentPlugin(kNVSEOpcodeStart), returnType(kRetnType_Default) { }

	UInt32				parentPlugin;
	CommandReturnType	returnType;
};

class CommandTable
{
public:
	CommandTable();
	~CommandTable();

	static void	Init(void);

	void	Read(CommandInfo * start, CommandInfo * end);
	void	Add(CommandInfo * info, CommandReturnType retnType = kRetnType_Default, UInt32 parentPluginOpcodeBase = 0);
	void	PadTo(UInt32 id, CommandInfo * info = NULL);
	bool	Replace(UInt32 opcodeToReplace, CommandInfo* replaceWith);

	CommandInfo *	GetStart(void)	{ return &m_commands[0]; }
	CommandInfo *	GetEnd(void)	{ return GetStart() + m_commands.size(); }
	CommandInfo *	GetByName(const char * name);
	CommandInfo *	GetByOpcode(UInt32 opcode);

	void	SetBaseID(UInt32 id)	{ m_baseID = id; m_curID = id; }
	UInt32	GetMaxID(void)			{ return m_baseID + m_commands.size(); }
	void	SetCurID(UInt32 id)		{ m_curID = id; }
	UInt32	GetCurID(void)			{ return m_curID; }

	void	Dump(void);
	void	DumpAlternateCommandNames(void);
	void	DumpCommandDocumentation(UInt32 startWithID = kNVSEOpcodeStart);

	CommandReturnType	GetReturnType(const CommandInfo * cmd);
	void				SetReturnType(UInt32 opcode, CommandReturnType retnType);

	UInt32				GetRequiredNVSEVersion(const CommandInfo * cmd);
	PluginInfo *		GetParentPlugin(const CommandInfo * cmd);

private:
	// add commands for each release (will help keep track of commands)
	void AddCommandsV1();
	void AddCommandsV3s();
	void AddCommandsV4();
	void AddCommandsV5();
	void AddCommandsV6();
	void AddDebugCommands();

	typedef std::vector <CommandInfo>				CommandList;
	typedef UnorderedMap<UInt32, CommandMetadata>	CmdMetadataList;

	CommandList		m_commands;
	CmdMetadataList	m_metadata;

	UInt32		m_baseID;
	UInt32		m_curID;

	std::vector<UInt32>	m_opcodesByRelease;	// maps an NVSE major version # to opcode of first command added to that release, beginning with v0008

	void	RecordReleaseVersion(void);
	void	RemoveDisabledPlugins(void);
};

extern CommandTable	g_consoleCommands;
extern CommandTable	g_scriptCommands;

namespace PluginAPI
{
	const CommandInfo* GetCmdTblStart();
	const CommandInfo* GetCmdTblEnd();
	const CommandInfo* GetCmdByOpcode(UInt32 opcode);
	const CommandInfo* GetCmdByName(const char* name);
	UInt32 GetCmdRetnType(const CommandInfo* cmd);
	UInt32 GetReqVersion(const CommandInfo* cmd);
	const PluginInfo* GetCmdParentPlugin(const CommandInfo* cmd);
	const PluginInfo* GetPluginInfoByName(const char *pluginName);
}
