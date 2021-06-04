#pragma once

#include "GameTypes.h"
#include "NiNodes.h"
#include "CommandTable.h"
#include "GameRTTI.h"
#include "GameScript.h"

struct ParamInfo;
class TESForm;
class TESObjectREFR;
struct BaseExtraList;

#define playerID	0x7
#define playerRefID 0x14

#if RUNTIME
	static const UInt32 s_Console__Print = 0x0071D0A0;
#endif

extern bool extraTraces;
extern bool alternateUpdate3D;
extern bool s_InsideOnActorEquipHook;
extern UInt32 s_CheckInsideOnActorEquipHook;

void Console_Print(const char * fmt, ...);

//typedef void * (* _FormHeap_Allocate)(UInt32 size);
//extern const _FormHeap_Allocate FormHeap_Allocate;
//
//typedef void (* _FormHeap_Free)(void * ptr);
//extern const _FormHeap_Free FormHeap_Free;

#if RUNTIME

#if USE_EXTRACT_ARGS_EX
typedef bool (*_ExtractArgs)(ParamInfo *paramInfo, void *scriptDataIn, UInt32 *scriptDataOffset, Script *scriptObj, ScriptEventList *eventList, ...);
#else
typedef bool (* _ExtractArgs)(ParamInfo * paramInfo, void * scriptData, UInt32 * arg2, TESObjectREFR * arg3, TESObjectREFR * arg4, Script * script, ScriptEventList * eventList, ...);
#endif
extern const _ExtractArgs ExtractArgs;

typedef TESForm * (* _CreateFormInstance)(UInt8 type);
extern const _CreateFormInstance CreateFormInstance;

bool IsConsoleMode();
bool GetConsoleEcho();
void SetConsoleEcho(bool doEcho);
const char * GetFullName(TESForm * baseForm);
const char* GetActorValueString(UInt32 actorValue); // should work now
UInt32 GetActorValueForString(const char* strActorVal, bool bForScript = false);

typedef char * (* _GetActorValueName)(UInt32 actorValueCode);
extern const _GetActorValueName GetActorValueName;
UInt32 GetActorValueMax(UInt32 actorValueCode);

typedef void (* _ShowMessageBox_Callback)(void);
extern const _ShowMessageBox_Callback ShowMessageBox_Callback;

// unk1 = 0
// unk2 = 0
// callback = may be NULL apparently
// unk4 = 0
// unk5 = 0x17 (why?)
// unk6 = 0
// unk7 = 0
// then buttons
// then NULL
typedef bool (* _ShowMessageBox)(const char * message, UInt32 unk1, UInt32 unk2, _ShowMessageBox_Callback callback, UInt32 unk4, UInt32 unk5, float unk6, float unk7, ...);
extern const _ShowMessageBox ShowMessageBox;

// set to scriptObj->refID after calling ShowMessageBox()
// GetButtonPressed checks this before returning a value, if it doesn't match it returns -1
typedef UInt32 * _ShowMessageBox_pScriptRefID;
extern const _ShowMessageBox_pScriptRefID ShowMessageBox_pScriptRefID;
typedef UInt8 * _ShowMessageBox_button;
extern const _ShowMessageBox_button ShowMessageBox_button;

// can be passed to QueueUIMessage to determine Vaultboy icon displayed
enum eEmotion {
	happy	= 0,
	sad		= 1,
	neutral = 2,
	pain	= 3
};

typedef bool (* _QueueUIMessage)(const char* msg, UInt32 emotion, const char* ddsPath, const char* soundName, float msgTime, bool maybeNextToDisplay);
extern const _QueueUIMessage QueueUIMessage;

const UInt32 kMaxMessageLength = 0x4000;

#if NVSE_CORE
bool ExtractArgsEx(ParamInfo * paramInfo, void * scriptData, UInt32 * scriptDataOffset, Script * scriptObj, ScriptEventList * eventList, ...);
extern bool ExtractFormatStringArgs(UInt32 fmtStringPos, char* buffer, ParamInfo * paramInfo, void * scriptDataIn, UInt32 * scriptDataOffset, Script * scriptObj, ScriptEventList * eventList, UInt32 maxParams, ...);
#endif

void ShowCompilerError(ScriptLineBuffer* lineBuf, const char* fmt, ...);

#else




typedef void (__cdecl *_ShowCompilerError)(ScriptBuffer* Buffer, const char* format, ...);
extern const _ShowCompilerError		ShowCompilerError;

bool DefaultCommandParseHook(UInt16 numParams, ParamInfo *paramInfo, ScriptLineBuffer *lineBuffer, ScriptBuffer *scriptBuffer);

#endif

typedef TESForm* (__cdecl* _GetFormByID)(const char* editorID);
extern const _GetFormByID GetFormByID;

struct NVSEStringVarInterface;
	// Problem: plugins may want to use %z specifier in format strings, but don't have access to StringVarMap
	// Could change params to ExtractFormatStringArgs to include an NVSEStringVarInterface* but
	//  this would break existing plugins
	// Instead allow plugins to register their NVSEStringVarInterface for use
	// I'm sure there is a better way to do this but I haven't found it
void RegisterStringVarInterface(NVSEStringVarInterface* intfc);

// only records individual objects if there's a block that matches it
// ### how can it tell?
struct ScriptEventList
{
	enum
	{
		kEvent_OnAdd			= 0x00000001,
		kEvent_OnEquip			= 0x00000002,		// Called on Item and on Refr
		kEvent_OnActorEquip		= kEvent_OnEquip,	// presumably the game checks the type of the object
		kEvent_OnDrop			= 0x00000004,
		kEvent_OnUnequip		= 0x00000008,
		kEvent_OnActorUnequip	= kEvent_OnUnequip,
		kEvent_OnDeath			= 0x00000010,
		kEvent_OnMurder			= 0x00000020,
		kEvent_OnCombatEnd		= 0x00000040,		// See 0x008A083C
		kEvent_OnHit			= 0x00000080,		// See 0x0089AB12
		kEvent_OnHitWith		= 0x00000100,		// TESObjectWEAP*	0x0089AB2F
		kEvent_OnPackageStart	= 0x00000200,
		kEvent_OnPackageDone	= 0x00000400,
		kEvent_OnPackageChange	= 0x00000800,
		kEvent_OnLoad			= 0x00001000,
		kEvent_OnMagicEffectHit = 0x00002000,		// EffectSetting* 0x0082326F
		kEvent_OnSell			= 0x00004000,		// 0x0072FE29 and 0x0072FF05, linked to 'Barter Amount Traded' Misc Stat
		kEvent_OnStartCombat	= 0x00008000,

		kEvent_OnOpen			= 0x00010000,		// while opening some container, not all
		kEvent_OnClose			= 0x00020000,		// idem
		kEvent_SayToDone		= 0x00040000,		// in Func0050 0x005791C1 in relation to SayToTopicInfo (OnSayToDone? or OnSayStart/OnSayEnd?)
		kEvent_OnGrab			= 0x00080000,		// 0x0095FACD and 0x009604B0 (same func which is called from PlayerCharacter_func001B and 0021)
		kEvent_OnRelease		= 0x00100000,		// 0x0047ACCA in relation to container
		kEvent_OnDestructionStageChange	= 0x00200000,		// 0x004763E7/0x0047ADEE
		kEvent_OnFire			= 0x00400000,		// 0x008BAFB9 (references to package use item and use weapon are close)

		kEvent_OnTrigger		= 0x10000000,		// 0x005D8D6A	Cmd_EnterTrigger_Execute
		kEvent_OnTriggerEnter	= 0x20000000,		// 0x005D8D50	Cmd_EnterTrigger_Execute
		kEvent_OnTriggerLeave	= 0x40000000,		// 0x0062C946	OnTriggerLeave ?
		kEvent_OnReset			= 0x80000000		// 0x0054E5FB
	};

	struct Event
	{
		TESForm	* object;
		UInt32	eventMask;
	};

	struct VarEntry;

	struct Var
	{
		UInt32		id;
		VarEntry	* nextEntry;
		double		data;

		UInt32 GetFormId();
	};

	struct VarEntry
	{
		Var			* var;
		VarEntry	* next;
	};

	struct Struct010
	{
		UInt8 unk00[8];
	};

	typedef tList<Event> EventList;

	Script			* m_script;		// 00
	UInt32			  m_unk1;			// 04
	EventList		* m_eventList;	// 08
	VarEntry		* m_vars;		// 0C
	Struct010		* unk010;		// 10

	void	Dump(void);
	Var *	GetVariable(UInt32 id);
	UInt32	ResetAllVariables();

	void	Destructor();
	tList<Var>* GetVars() const;
};

ScriptEventList* EventListFromForm(TESForm* form);

typedef bool (* _MarkBaseExtraListScriptEvent)(TESForm* target, BaseExtraList* extraList, UInt32 eventMask);
extern const _MarkBaseExtraListScriptEvent MarkBaseExtraListScriptEvent;

typedef void (_cdecl * _DoCheckScriptRunnerAndRun)(TESObjectREFR* refr, BaseExtraList* extraList);
extern const _DoCheckScriptRunnerAndRun DoCheckScriptRunnerAndRun;

struct ExtractedParam
{
	// float/double types are kept as pointers
	// this avoids problems with storing invalid floats/doubles in to the fp registers which has a side effect
	// of corrupting data

	enum
	{
		kType_Unknown = 0,
		kType_String,		// str
		kType_Imm32,		// imm
		kType_Imm16,		// imm
		kType_Imm8,			// imm
		kType_ImmDouble,	// immDouble
		kType_Form,			// form
	};

	UInt8	type;
	bool	isVar;	// if true, data is stored in var, otherwise it's immediate

	union
	{
		// immediate
		UInt32			imm;
		const double	* immDouble;
		TESForm			* form;
		struct
		{
			const char	* buf;
			UInt32		len;
		} str;

		// variable
		struct 
		{
			ScriptEventList::Var	* var;
			ScriptEventList			* parent;
		} var;
	} data;
};

bool ExtractArgsRaw(ParamInfo * paramInfo, void * scriptDataIn, UInt32 * scriptDataOffset, Script * scriptObj, ScriptEventList * eventList, ...);

enum EActorVals {
	eActorVal_Aggression			= 0,
	eActorVal_Confidence			= 1,
	eActorVal_Energy				= 2,
	eActorVal_Responsibility		= 3,
	eActorVal_Mood					= 4,

	eActorVal_Strength				= 5,
	eActorVal_Perception			= 6,
	eActorVal_Endurance				= 7,
	eActorVal_Charisma				= 8,
	eActorVal_Intelligence			= 9,
	eActorVal_Agility				= 10,
	eActorVal_Luck					= 11,
	eActorVal_SpecialStart = eActorVal_Strength,
	eActorVal_SpecialEnd = eActorVal_Luck,

	eActorVal_ActionPoints			= 12,
	eActorVal_CarryWeight			= 13,
	eActorVal_CritChance			= 14,
	eActorVal_HealRate				= 15,
	eActorVal_Health				= 16,
	eActorVal_MeleeDamage			= 17,
	eActorVal_DamageResistance		= 18,
	eActorVal_PoisonResistance		= 19,
	eActorVal_RadResistance			= 20,
	eActorVal_SpeedMultiplier		= 21,
	eActorVal_Fatigue				= 22,
	eActorVal_Karma					= 23,
	eActorVal_XP					= 24,

	eActorVal_Head					= 25,
	eActorVal_Torso					= 26,
	eActorVal_LeftArm				= 27,
	eActorVal_RightArm				= 28,
	eActorVal_LeftLeg				= 29,
	eActorVal_RightLeg				= 30,
	eActorVal_Brain					= 31,
	eActorVal_BodyPartStart = eActorVal_Head,
	eActorVal_BodyPartEnd = eActorVal_Brain,

	eActorVal_Barter				= 32,
	eActorVal_BigGuns				= 33,
	eActorVal_EnergyWeapons			= 34,
	eActorVal_Explosives			= 35,
	eActorVal_Lockpick				= 36,
	eActorVal_Medicine				= 37,
	eActorVal_MeleeWeapons			= 38,
	eActorVal_Repair				= 39,
	eActorVal_Science				= 40,
	eActorVal_Guns					= 41,
	eActorVal_Sneak					= 42,
	eActorVal_Speech				= 43,
	eActorVal_Survival				= 44,
	eActorVal_Unarmed				= 45,
	eActorVal_SkillsStart = eActorVal_Barter,
	eActorVal_SkillsEnd = eActorVal_Unarmed,

	eActorVal_InventoryWeight		= 46,
	eActorVal_Paralysis				= 47,
	eActorVal_Invisibility			= 48,
	eActorVal_Chameleon				= 49,
	eActorVal_NightEye				= 50,
	eActorVal_Turbo					= 51,
	eActorVal_FireResistance		= 52,
	eActorVal_WaterBreathing		= 53,
	eActorVal_RadLevel				= 54,
	eActorVal_BloodyMess			= 55,
	eActorVal_UnarmedDamage			= 56,
	eActorVal_Assistance			= 57,

	eActorVal_ElectricResistance	= 58,
	eActorVal_FrostResistance		= 59,

	eActorVal_EnergyResistance		= 60,
	eActorVal_EMPResistance			= 61,
	eActorVal_Var1Medical			= 62,
	eActorVal_Var2					= 63,
	eActorVal_Var3					= 64,
	eActorVal_Var4					= 65,
	eActorVal_Var5					= 66,
	eActorVal_Var6					= 67,
	eActorVal_Var7					= 68,
	eActorVal_Var8					= 69,
	eActorVal_Var9					= 70,
	eActorVal_Var10					= 71,

	eActorVal_IgnoreCrippledLimbs	= 72,
	eActorVal_Dehydration			= 73,
	eActorVal_Hunger				= 74,
	eActorVal_Sleepdeprevation		= 75,
	eActorVal_Damagethreshold		= 76,
	eActorVal_FalloutMax = eActorVal_Damagethreshold,
	eActorVal_NoActorValue = 256,
};

class ConsoleManager
{
public:
#if RUNTIME
	MEMBER_FN_PREFIX(ConsoleManager);
	DEFINE_MEMBER_FN(Print, void, s_Console__Print, const char * fmt, va_list args);
#endif

	ConsoleManager();
	~ConsoleManager();

	void	* scriptContext;			// 00
	UInt32	unk004[(0x0810-0x04) >> 2];	// 004
	char	COFileName[260];			// 810

	static ConsoleManager * GetSingleton(void);
	
	static char * GetConsoleOutputFilename(void);
	static bool HasConsoleOutputFilename(void);
};

// A plugin author requested the ability to use OBSE format specifiers to format strings with the args
// coming from a source other than script.
// So changed ExtractFormattedString to take an object derived from following class, containing the args
// Probably doesn't belong in GameAPI.h but utilizes a bunch of stuff defined here and can't think of a better place for it
class FormatStringArgs
{
public:
	enum argType {
		kArgType_Float,
		kArgType_Form		// TESForm*
	};

	virtual bool Arg(argType asType, void * outResult) = 0;	// retrieve next arg
	virtual bool SkipArgs(UInt32 numToSkip) = 0;			// skip specified # of args
	virtual bool HasMoreArgs() = 0;
	virtual char *GetFormatString() = 0;						// return format string
};

// concrete class used for extracting script args
class ScriptFormatStringArgs : public FormatStringArgs
{
public:
	virtual bool Arg(argType asType, void* outResult);
	virtual bool SkipArgs(UInt32 numToSkip);
	virtual bool HasMoreArgs();
	virtual char *GetFormatString();

	ScriptFormatStringArgs(UInt32 _numArgs, UInt8* _scriptData, Script* _scriptObj, ScriptEventList* _eventList, void* scriptDataIn);
	UInt32 GetNumArgs();
	UInt8* GetScriptData();
	bool m_bad = false;
private:
	UInt32			numArgs;
	UInt8			* scriptData;
	Script			* scriptObj;
	ScriptEventList		* eventList;
	void* scriptDataIn;
};
bool SCRIPT_ASSERT(bool expr, Script* script, const char * errorMsg, ...);

bool ExtractSetStatementVar(Script* script, ScriptEventList* eventList, void* scriptDataIn, double* outVarData, bool* makeTemporary, UInt32* opcodeOffsetPtr, UInt8* outModIndex = NULL);
bool ExtractFormattedString(FormatStringArgs& args, char* buffer);

class ChangesMap;
class InteriorCellNewReferencesMap;
class ExteriorCellNewReferencesMap;
class NumericIDBufferMap;

//
struct ToBeNamed
{
	char		m_path[0x104];	// 0000
	BSFile*		m_file;			// 0104
	UInt32		m_unk0108;		// 0108
	UInt32		m_offset;		// 010C
};

// Form type class: use to preload some information for created objects (?) refr and Cells
struct formTypeClassData
{
	typedef UInt8 EncodedID[3];	// Codes the refID on 3 bytes, as used in changed forms and save refID mapping

	struct Data01 // Applies to CELL where changeFlags bit30 (Detached CELL) and bit29 (CHANGE_CELL_EXTERIOR_CHAR) are set
	{
		UInt16	worldspaceIndex;	// 00 Index into visitedWorldspaces		goes into unk000
		UInt8	coordX;				// 02	goes into unk004
		UInt8	coordY;				// 03	goes into unk008, paired with 002
		UInt8	detachTime;			// 04	goes into unk00C
	};

	struct Data02 // Applies to CELL where changeFlags bit30 (Detached CELL) and bit 28 (CHANGE_CELL_EXTERIOR_SHORT) are set and changeFlags bit29 is clear
	{
		UInt16	worldspaceIndex;	// 00 Index into visitedWorldspaces		goes into unk000
		UInt16	coordX;				// 02	goes into unk004
		UInt16	coordY;				// 03	goes into unk008, paired with 002
		UInt32	detachTime;			// 04	goes into unk00C
	};

	// The difference between the two preceding case seems to be how big the data (coordinates?) are

	struct Data03 // Applies to CELL where changeFlags bit30 (Detached CELL) is set and changeFlags bit28 and bit29 are clear
	{
		UInt32	detachTime;	// 00	goes into unk00C. Null goes into unk000, 004 and 008
	};

	struct Data04 // Applies to references where changeFlags bit3 (CHANGE_REFR_CELL_CHANGED) is clear and
					// either bit1 (CHANGE_REFR_MOVE) or bit2 (CHANGE_REFR_HAVOK_MOVE) is set
	{
		EncodedID	cellOrWorldspaceID;	// 000	goes into unk000, Null goes into unk004, 008, 00C, 010 and byt02C
		float		posX;	// 003	goes into unk014
		float		posY;	// 007	goes into unk018, associated with unk003
		float		posZ;	// 00B	goes into unk01C, associated with unk003	(pos?)
		float		rotX;	// 00F	goes into unk020
		float		rotY;	// 013	goes into unk024, associated with unk00F
		float		rotZ;	// 017	goes into unk028, associated with unk00F	(rot?)
	};

	struct Data05 // Applies to created objects (ie 0xFFnnnnnn)
	{
		EncodedID	cellOrWorldspaceID;	// 000	goes into unk000
		float		posX;	// 003	goes into unk014
		float		posY;	// 007	goes into unk018, associated with unk003
		float		posZ;	// 00B	goes into unk01C, associated with unk003	(pos?)
		float		rotX;	// 00F	goes into unk020
		float		rotY;	// 013	goes into unk024, associated with unk024
		float		rotZ;	// 017	goes into unk028, associated with unk028	(rot?)
		UInt8		flags;	// 01B	goes into unk02C	bit0 always set, bit1 = ESP or persistent, bit2 = Byt081 true
		EncodedID	baseFormID;	// 01C	goes into unk004, Null goes into unk008, 00C and 010
	};

	struct Data06 // Applies to references whose changeFlags bit3 (CHANGE_REFR_CELL_CHANGED) is set
	{
		EncodedID	cellOrWorldspace;		// 000	goes into unk000
		float		posX;					// 003	goes into unk014
		float		posY;					// 007	goes into unk018, associated with unk003
		float		posZ;					// 00B	goes into unk01C, associated with unk003	(pos?)
		float		rotX;					// 00F	goes into unk020
		float		rotY;					// 013	goes into unk024, associated with unk00F
		float		rotZ;					// 017	goes into unk028, associated with unk00F	(rot?)
		EncodedID	newCellOrWorldspaceID;	// 01C	goes into unk008
		SInt16		coordX;					// 01E	goes into unk00C
		SInt16		coordY;					// 020	goes into unk010, Null goes into unk004 and byt02C
	};

	struct Data00  // Every other cases (no data)
	{
	};

	union Data
	{
		Data00	data00;
		Data01	data01;
		Data02	data02;
		Data03	data03;
		Data04	data04;
		Data05	data05;
		Data06	data06;
	};

	Data data;	// 00
};

struct PreloadCELLdata	// Unpacked and decoded version of Data01, 02 end 03
{
	UInt32	worldspaceID;	// 000
	SInt32	coordX;			// 004
	SInt32	coordY;			// 008
	UInt32	detachTime;		// 00C
};

struct PreloadREFRdata	// Unpacked and decoded version of Data04, 05 and 06
{
	UInt32	cellOrWorldspaceID;		// 000
	UInt32	baseFormID;				// 004
	UInt32	newCellOrWorldspaceID;	// 008
	SInt32	coordX;					// 00C
	SInt32	coordY;					// 010
	float	posXcoordX;				// 014
	float	posYcoordY;				// 018
	float	posZ;					// 01C
	float	rotX;					// 020
	float	rotY;					// 024
	float	rotZ;					// 028
	UInt8	flg02C;					// 02C
};

union preloadData
{
	PreloadCELLdata	cell;
	PreloadREFRdata	refr;
};

class BGSLoadGameBuffer
{
	BGSLoadGameBuffer();
	~BGSLoadGameBuffer();

	virtual UInt8			GetSaveFormVersion(void);	// replaced in descendant 
	virtual TESForm*		getForm(void);				// only implemented in descendants
	virtual TESObjectREFR*	getREFR(void);				// only implemented in descendants
	virtual Actor*			getActor(void);				// only implemented in descendants

	char*	chunk;			// 004
	UInt32	chunkSize;		// 008
	UInt32	chunkConsumed;	// 00C
};

struct BGSFormChange
{
	UInt32	changeFlags;
	UInt32	unk004;			// Pointer to the changed record or the save record ?
};

struct	BGSSaveLoadChangesMap
{
	NiTPointerMap<BGSFormChange> BGSFormChangeMap;
	// most likely more
};

// 030
class BGSLoadFormBuffer: public BGSLoadGameBuffer
{
	BGSLoadFormBuffer();
	~BGSLoadFormBuffer();

	typedef UInt8 EncodedID[3];
	struct Header	// 00C
	{
		EncodedID	encodeID;			// 00
		UInt32		changeFlags;		// 03
		UInt8		codedTypeAndLength;	// 07
		UInt8		formVersion;		// 08
		UInt8		pad009[3];			// 09
	};

	UInt32			refID;				// 010
	Header			header;				// 014
	UInt32			bufferSize;			// 020
	TESForm*		form;				// 024
	UInt32			flg028;				// 028	bit1 form invalid
	BGSFormChange*	currentFormChange;	// 02C
};

class BGSSaveFormBuffer: public BGSLoadGameBuffer
{
	BGSSaveFormBuffer();
	~BGSSaveFormBuffer();

};	// in BGSSaveGameBuffer there is a 010, which look like a counter (ChunkCount ?), then the Header

// 1C8 - only explicitly marked things are verified
class TESSaveLoadGame
{
public:
	TESSaveLoadGame();
	~TESSaveLoadGame();

	struct CreatedObject {
		UInt32			refID;
		CreatedObject	* next;
	};

	ChangesMap					* changesMap;		// 000
	UInt32						unk004;				// 004
	InteriorCellNewReferencesMap	* intRefMap;	// 008
	ExteriorCellNewReferencesMap	* extRefMap00C;	// 00C
	ExteriorCellNewReferencesMap	* extRefMap010;	// 010
	void						* saveLoadBuffer;	// 014
	UInt32						unk018;				// 018
	UInt8						unk01C;				// 01C
	UInt8						pad01D[3];
	NiTArray<TESObjectREFR*>	* arr020;			// 020
	UInt32						unk024;				// 024
	UInt32						unk028;				// 028
	CreatedObject				createdObjectList;	// 02C data is formID - verified
	NiTPointerMap<void*>		* map034;			// 034
	UInt32				unk034[(0x58-0x44) >> 2];	// 044
	NumericIDBufferMap			* idMap058;			// 058
	NumericIDBufferMap			* idMap05C;			// 05C
	NumericIDBufferMap			* idMap060;			// 060
	NumericIDBufferMap			* idMap064;			// 064
	UInt32						unk068;				// 068
	UInt32						unk06C;				// 06C
	UInt32						unk070;				// 070
	UInt8						unk074;				// 074
	UInt8						unk075;				//     init to 0x7D
	UInt8						pad076[2];
	NiTArray<UInt32>			* array078;			// 078 NiTLargePrimitiveArray<?>
	NiTArray<UInt32>			* array07C;			// 07C NiTLargePrimitiveArray<?>	
	UInt8						unk080;				// 080 version of save?
	UInt8						unk081;
	UInt8						pad082[2];
	UInt32				unk084[(0xAC-0x84) >> 2];	// 084
	UInt8						unk0AC;				// 0AC
	UInt8						unk0AD;				// 0AD
	UInt8						unk0AE;				// 0AE
	UInt8						unk0AF;				// 0AF
	UInt32				unk0B0[(0x1C8-0x0B0) >> 2];	// 0B0

	static TESSaveLoadGame* Get();

	MEMBER_FN_PREFIX(TESSaveLoadGame);
#if RUNTIME
	DEFINE_MEMBER_FN(AddCreatedForm, UInt32, 0x00861780, TESForm * pForm);
#endif
};

#if RUNTIME
const UInt32 _SaveGameManager_ConstructSavegameFilename = 0x0084FF90;
const UInt32 _SaveGameManager_ConstructSavegamePath		= 0x0084FF30;
#endif

template <typename T_Key, typename T_Data>
class NiTMap : public NiTMapBase<T_Key, T_Data>
{
public:
	NiTMap();
	~NiTMap();
};

class BGSCellNumericIDArrayMap;
class BGSLoadGameSubBuffer;
class BGSReconstructFormsInFileMap;
class BGSReconstructFormsInAllFilesMap;

class BGSSaveLoadGame	// 0x011DDF38
{
public:
	BGSSaveLoadGame();
	~BGSSaveLoadGame();

	typedef UInt32	RefID;
	typedef UInt32	IndexRefID;
	struct RefIDIndexMapping	// reversible map between refID and loaded form index
	{
		NiTMap<RefID, IndexRefID>	* map000;	// 000
		NiTMap<IndexRefID, RefID>	* map010;	// 010
		UInt32			            countRefID;	// 020
	};

	struct SaveChapters	// 06E	chapter table in save
	{
		struct RefIDArray	// List of all refID referenced in save for tranlation in RefIDIndexMapping
		{
			UInt32	count;	// 000
			RefID	IDs[1];	// 004
		};

		RefIDArray	*arr000;	// 000
		RefIDArray	*arr004;	// 004

	};

	struct Struct010
	{
		NiTPointerMap<UInt32>						* map000;	// 000
		BGSCellNumericIDArrayMap					* map010;	// 010
		NiTPointerMap<BGSCellNumericIDArrayMap*>	* map020;	// 020
	};

	BGSSaveLoadChangesMap					* changesMap;			// 000
	BGSSaveLoadChangesMap					* previousChangeMap;	// 004
	RefIDIndexMapping						* refIDmapping;			// 008
	RefIDIndexMapping						* visitedWorldspaces;	// 00C
	Struct010								* sct010;				// 010
	NiTMap<TESForm *, BGSLoadGameSubBuffer> * maps014[3];			// 014	0 = changed Animations, 2 = changed Havok Move
	NiTMap<UInt32, UInt32>					* map018;				// 018	
	BSSimpleArray<char *>					* strings;				// 01C
	BGSReconstructFormsInAllFilesMap*		rfiafMap;				// 020
	BSSimpleArray<BGSLoadFormBuffer *>		changedForms;			// 024
	NiTPointerMap<Actor*>					map0034;				// 034 Either dead or not dead actors
	UInt8									saveMods[255];			// 044
	UInt8									loadedMods[255];		// 143

	UInt16									pad242;					// 242
	UInt32									flg244;					// 244 bit 6 block updating player position/rotation from save, bit 2 set during save
	UInt8									formVersion;			// 248
	UInt8									pad249[3];				// 249

};

#if RUNTIME
class SaveGameManager
{
public:
	SaveGameManager();
	~SaveGameManager();

	static SaveGameManager* GetSingleton();
	MEMBER_FN_PREFIX(SaveGameManager);
	DEFINE_MEMBER_FN(ConstructSavegameFilename, void, _SaveGameManager_ConstructSavegameFilename, 
					 const char* filename, char* outputBuf, bool bTempFile);
	DEFINE_MEMBER_FN(ConstructSavegamePath, void, _SaveGameManager_ConstructSavegamePath, char* outputBuf);

	struct SaveGameData
	{
		const char	* name;		// 00
		UInt32		unk04;		// 04
		UInt32		saveNumber;	// 08 index?
		const char	* pcName;	// 0C
		const char	* pcTitle;	// 10
		const char	* location;	// 14
		const char	* time;		// 18
	};

	tList<SaveGameData>		* saveList;		// 00
	UInt32					numSaves;		// 04
	UInt32					unk08;			// 08
	UInt8					unk0C;			// 0C	flag for either opened or writable or useSeparator (|)
	UInt8					unk0D;
	UInt8					unk0E;
	UInt8					unk0F;
/*
	const char				* unk10;		// 10 name of most recently loaded/saved game?
	UInt32					unk14;			// 14 init to -1
	UInt8					unk18;			// 18
	UInt8					pad19[3];
	UInt8					unk20;			// 20 init to 1
	UInt8					unk21;
	UInt8					pad22[2];
	UInt32					unk24;			// 24
	UInt32					unk28;			// 28
*/
};

std::string GetSavegamePath();

bool vExtractArgsEx(ParamInfo* paramInfo, void* scriptDataIn, UInt32* scriptDataOffset, Script* scriptObj, ScriptEventList* eventList, va_list args, bool incrementOffsetPtr = false);

#endif

class ButtonIcon;

const UInt32 FontArraySize = 8;

class FontManager
{
public:
	FontManager();
	~FontManager();

	// 3C
	struct FontInfo {
		FontInfo();
		~FontInfo();

		struct Data03C {
			UInt32	unk000;	// 000
			UInt16	wrd004;	// 004	Init'd to 0
			UInt16	wrd006;	// 006	Init'd to 0x0FFFF
		};	// 0008

		struct FontData {
			float	flt000;				// 000
			UInt32	fontTextureCount;	// 004
			UInt32	unk008;
			char	unk00C[8][0x024];	// array of 8 Font Texture Name (expected in Textures\Fonts\*.tex)
		};

		struct TextReplaced {
			String	str000;	// 000	Init'd to ""
			UInt32	unk008;	// 008	Init'd to arg1
			UInt32	unk00C;	// 00C	Init'd to arg2
			UInt32	unk010;	// 010	Init'd to arg3
			UInt32	unk014;	// 014	Init'd to arg4
			UInt32	unk018;	// 018	Init'd to 0
			UInt8	byt01C;	// 01C	Init'd to arg5
			UInt8	fill[3];	
		};	// 020

		UInt16						unk000;			// 000	Init'd to 0, loaded successfully in OBSE (word bool ?)
		UInt16						pad002;			// 002
		char						* path;			// 004	Init'd to arg2, passed to OpenBSFile
		UInt32						id;				// 008	1 based, up to 8 apparently
		NiObject					* unk00C[8];	// 00C	in OBSE: NiTexturingProperty			* textureProperty
		float						unk02C;			// 02C	Those two values seem to be computed by looping through the characters in the font (max height/weight ?)
		float						unk030;			// 030
		UInt32						unk034;			// 038	in OBSE: NiD3DShaderConstantMapEntry	* unk34;
		FontData					* fontData;		// 038	Init'd to 0, might be the font content, at Unk004 we have the count of font texture
		Data03C						dat03C;			// 03C
		BSSimpleArray<ButtonIcon>	unk044;			// 044

		static FontInfo * Load(const char* path, UInt32 ID);
		bool GetName(char* out);	// filename stripped of path and extension
	};	// 054

	FontInfo	* fontInfos[FontArraySize];		// 00 indexed by FontInfo::ID - 1; access inlined at each point in code where font requested
	UInt8		unk20;				// 20
	UInt8		pad15[3];

	static FontManager * GetSingleton();
};

void Debug_DumpFontNames(void);

//class NiMemObject
//{
//	NiMemObject();
//	~NiMemObject();
//
//};

//class NiRefObject: public NiMemObject
//{
//	NiRefObject();
//	~NiRefObject();
//
//	virtual void		Destructor(bool freeThis);	// 00
//	virtual void		Free(void);					// 01 calls Destructor(true);
//
////	void		** _vtbl;		// 000
//	UInt32		m_uiRefCount;	// 004 - name known (in OBSE)
//};

enum Coords
{
	kCoords_X = 0,	// 00
	kCoords_Y,		// 01
	kCoords_Z,		// 02
	kCoords_Max		// 03
};

struct NavMeshVertex
{
	float coords[kCoords_Max];	// 000
};	// 00C

enum Vertices
{
	kVertices_0 = 0,	// 00
	kVertices_1,		// 01
	kVertices_2,		// 02
	kVertices_Max		// 03
};

enum Sides
{
	kSides_0_1 = 0,	// 00
	kSides_1_2,		// 01
	kSides_2_0,		// 02
	kSides_Max		// 03
};

struct NavMeshTriangle
{
	SInt16	verticesIndex[kVertices_Max];	// 000
	SInt16	sides[kSides_Max];				// 006
	UInt32	flags;							// 00C
};	// Alloc'd by 0x10

struct NavMeshInfo;

struct EdgeExtraInfo
{
	struct Connection
	{
		NavMeshInfo*	navMeshInfo;
		SInt16			triangle;
	};

	UInt32	unk000;			// 00
	Connection connectTo;	// 04
};	// Alloc'd by 0x0C

struct NavMeshTriangleDoorPortal
{
	TESObjectREFR	* door;	// 00
	UInt16			unk004;	// 04
	UInt16			pad006;	// 06
};	// Alloc'd to 0x08

struct NavMeshCloseDoorInfo
{
	UInt32	unk000;	// 00
	UInt32	unk004;	// 04
};	// Alloc'd to 0x08

struct NavMeshPOVData;
struct ObstacleData;
struct ObstacleUndoData;

struct NavMeshStaticAvoidNode
{
	UInt32	unk000;	// 00
	UInt32	unk004;	// 04
	UInt32	unk008;	// 08
	UInt32	unk00C;	// 0C
	UInt32	unk010;	// 10
	UInt32	unk014;	// 14
	UInt32	unk018;	// 18
	UInt32	unk01C;	// 1C
	UInt32	unk020;	// 20
	UInt32	unk024;	// 24
};	// Alloc'd to 0x28


// represents the currently executing script context
class ScriptRunner
{
public:
	static const UInt32	kStackDepth = 10;

	enum
	{
		kStackFlags_IF = 1 << 0,
		kStackFlags_ELSEIF = 1 << 1,
		/* ELSE and ENDIF modify the above flags*/
	};

	// members
	/*00*/ TESObjectREFR* containingObj; // set when executing scripts on inventory objects
	/*04*/ TESForm* callingRefBaseForm;
	/*08*/ ScriptEventList* eventList;
	/*0C*/ UInt32 unk0C;
	/*10*/ UInt32 unk10; // pointer? set to NULL before executing an instruction
	/*14*/ Script* script;
	/*18*/ UInt32 unk18; // set to 6 after a failed expression evaluation
	/*1C*/ UInt32 unk1C; // set to Expression::errorCode
	/*20*/ UInt32 ifStackDepth;
	/*24*/ UInt32 ifStack[kStackDepth];	// stores flags
	/*4C*/ UInt32 unk4C[(0xA0 - 0x4C) >> 2];
	/*A0*/ UInt8 invalidReferences;	// set when the dot operator fails to resolve a reference (inside the error message handler)
	/*A1*/ UInt8 unkA1;	// set when the executing CommandInfo's 2nd flag bit (+0x25) is set
	/*A2*/ UInt16 padA2;
};
STATIC_ASSERT(sizeof(ScriptRunner) == 0xA4);

// Gets the real script data ptr, as it can be a pointer to a buffer on the stack in case of vanilla expressions in set and if statements
UInt8* GetScriptDataPosition(Script* script, void* scriptDataIn, const UInt32* opcodeOffsetPtrIn);

/* I need to port NiTypes 

class NavMesh: public TESForm
{
	NavMesh();
	~NavMesh();

	struct NavMeshGridCells
	{
		UInt32					cellCount;	// 00
		BSSimpleArray<UInt16>	cells[1];	// 04
	};	// 4 + cellCount*0x10

	struct NavMeshGrid
	{
		UInt32	size;					// 000 = 0
		float	unk004;					// 004
		float	unk008;					// 008
		float	flt00C;					// 00C Init'd to MAXFLOAT
		float	unk010;					// 010
		float	unk014;					// 014
		float	unk018;					// 018
		float	unk01C;					// 01C
		float	unk020;					// 020
		NavMeshGridCells	* cells;	// 024 = 0, array of size size*size
	};

	TESChildCell								childCell;				// 018
	NiRefObject									niro;					// 01C
	TESObjectCELL								* cell;					// 024
	BSSimpleArray<NavMeshVertex>				vertices;				// 028
	BSSimpleArray<NavMeshTriangle>				triangles;				// 038
	BSSimpleArray<EdgeExtraInfo>				edgesExtraInfo;			// 048
	BSSimpleArray<NavMeshTriangleDoorPortal>	trianglesDoorPortal;	// 058
	BSSimpleArray<NavMeshClosedDoorInfo>		closedDoorsInfo;		// 068
	BSSimpleArray<UInt16>						arr07NVCA;				// 078
	NiTMap<ushort,NavMeshPOVData>				povDataMap;				// 088
	BSSimpleArray<UInt8>						arr098;					// 098
	NavMeshGrid									grid;					// 0A8
	BSSimpleArray<NiTPointer<ObstacleUndoData>>	obstaclesUndoData;		// 0D0
	NiTMap<ushort,NiPointer<ObstacleData>>		* obstaclesData;		// 0E0
	BSSimpleArray<UInt8>						arr0E4;					// 0E4
	BSSimpleArray<NavMeshStaticAvoidNode>		staticAvoidNodes;		// 0F4
};

class NavMeshInfoMap: public TESForm
{
	// 1C is NiTPointerMap indexed by NavMesh refID
	// 2C is a map of map indexed by Worldspace/Cell refID
};

*/



template <typename T, const UInt32 ConstructorPtr = 0, typename ...Args>
T* New(Args&& ...args)
{
	auto* alloc = FormHeap_Allocate(sizeof(T));
	if constexpr (ConstructorPtr)
	{
		ThisStdCall(ConstructorPtr, alloc, std::forward<Args>(args)...);
	}
	else
	{
		memset(alloc, 0, sizeof(T));
	}
	return static_cast<T*>(alloc);
}

template <typename T, const UInt32 DestructorPtr = 0, typename ...Args>
void Delete(T* t, Args&& ...args)
{
	if constexpr (DestructorPtr)
	{
		ThisStdCall(DestructorPtr, t, std::forward<Args>(args)...);
	}
	FormHeap_Free(t);
}

template<typename T>
using game_unique_ptr = std::unique_ptr<T, std::function<void(T*)>>;

template <typename T, const UInt32 ConstructorPtr = 0, const UInt32 DestructorPtr = 0, typename ...ConstructorArgs>
game_unique_ptr<T> MakeUnique(ConstructorArgs&& ...args)
{
	auto* obj = New<T, ConstructorPtr>(std::forward(args)...);
	return game_unique_ptr<T>(obj, [](T* t) { Delete<T, DestructorPtr>(t); });
}

UInt32 GetNextFreeFormID(UInt32 formId);

struct Timer
{
	UInt8 disableCounter;		// 00
	UInt8 gap01[3];				// 01
	float fpsClamp;				// 04
	float fpsClampRemainder;	// 08
	float secondsPassed;		// 0C
	float lastSecondsPassed;	// 10
	UInt32 msPassed;			// 14
	UInt32 unk18;				// 18
	byte isChangeTimeMultSlowly;// 1C
	byte unk1D;					// 1D
	byte unk1E;					// 1E
	byte unk1F;					// 1F
};

struct TimeGlobal : Timer
{
	float unk20;  // 020
	float unk24;  // 024
	float unk28;  // 028
};

extern TimeGlobal* g_timeGlobal;
extern float* g_globalTimeMult;

Script* GetReferencedQuestScript(UInt32 refIdx, ScriptEventList* baseEventList);
