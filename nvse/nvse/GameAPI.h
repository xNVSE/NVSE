#pragma once

#define playerID	0x7
#define playerRefID 0x14

static const UInt32 s_Console__Print = 0x0071D0A0;

extern bool extraTraces;

void Console_Print(const char * fmt, ...);

//typedef void * (* _FormHeap_Allocate)(UInt32 size);
//extern const _FormHeap_Allocate FormHeap_Allocate;
//
//typedef void (* _FormHeap_Free)(void * ptr);
//extern const _FormHeap_Free FormHeap_Free;

#if RUNTIME

typedef bool (* _ExtractArgs)(ParamInfo * paramInfo, void * scriptData, UInt32 * arg2, TESObjectREFR * arg3, TESObjectREFR * arg4, Script * script, ScriptEventList * eventList, ...);
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

// unk1 = 0
// unk3 = 0, "UIVATSInsufficientAP" (sound?)
// duration = 2
// unk5 = 0
typedef bool (*_QueueUIMessage)(const char *msgText, UInt32 iconType, const char *iconPath, const char *soundPath, float displayTime, UInt8 unk5);
extern const _QueueUIMessage QueueUIMessage;

enum
{
	kMaxMessageLength = 0x4000
};

#if NVSE_CORE
bool ExtractArgsEx(ParamInfo * paramInfo, void * scriptData, UInt32 * scriptDataOffset, Script * scriptObj, ScriptEventList * eventList, ...);
extern bool ExtractFormatStringArgs(UInt32 fmtStringPos, char* buffer, ParamInfo * paramInfo, void * scriptDataIn, UInt32 * scriptDataOffset, Script * scriptObj, ScriptEventList * eventList, UInt32 maxParams, ...);
#endif

void ShowCompilerError(ScriptLineBuffer* lineBuf, const char* fmt, ...);

#else

typedef TESForm * (__cdecl * _GetFormByID)(const char* editorID);
extern const _GetFormByID GetFormByID;

typedef void (__cdecl *_ShowCompilerError)(ScriptBuffer* Buffer, const char* format, ...);
extern const _ShowCompilerError		ShowCompilerError;

#endif

// can be passed to QueueUIMessage to determine Vaultboy icon displayed
enum eEmotion {
	neutral = 0,
	happy = 1,
	sad = 2,
	pain = 3
};

struct NVSEStringVarInterface;
	// Problem: plugins may want to use %z specifier in format strings, but don't have access to StringVarMap
	// Could change params to ExtractFormatStringArgs to include an NVSEStringVarInterface* but
	//  this would break existing plugins
	// Instead allow plugins to register their NVSEStringVarInterface for use
	// I'm sure there is a better way to do this but I haven't found it
void RegisterStringVarInterface(NVSEStringVarInterface* intfc);

struct ScriptVar
{
	UInt32				id;
	ListNode<ScriptVar>	*next;
	double				data;
};

// only records individual objects if there's a block that matches it
// ### how can it tell?
struct ScriptEventList
{
	enum
	{
		kEvent_OnAdd =						1,
		kEvent_OnEquip =					2,
		kEvent_OnActorEquip =				2,
		kEvent_OnDrop =						4,
		kEvent_OnUnequip =					8,
		kEvent_OnActorUnequip =				8,

		kEvent_OnDeath =					0x10,
		kEvent_OnMurder =					0x20,
		kEvent_OnCombatEnd =				0x40,			// See 0x008A083C
		kEvent_OnHit =						0x80,			// See 0x0089AB12

		kEvent_OnHitWith =					0x100,			// TESObjectWEAP*	0x0089AB2F
		kEvent_OnPackageStart =				0x200,
		kEvent_OnPackageDone =				0x400,
		kEvent_OnPackageChange =			0x800,

		kEvent_OnLoad =						0x1000,
		kEvent_OnMagicEffectHit =			0x2000,			// EffectSetting* 0x0082326F
		kEvent_OnSell =						0x4000,			// 0x0072FE29 and 0x0072FF05, linked to 'Barter Amount Traded' Misc Stat
		kEvent_OnStartCombat =				0x8000,

		kEvent_OnOpen =						0x10000,		// while opening some container, not all
		kEvent_OnClose =					0x20000,
		kEvent_SayToDone =					0x40000,		// in Func0050 0x005791C1 in relation to SayToTopicInfo (OnSayToDone? or OnSayStart/OnSayEnd?)
		kEvent_OnGrab =						0x80000,		// 0x0095FACD and 0x009604B0 (same func which is called from PlayerCharacter_func001B and 0021)

		kEvent_OnRelease =					0x100000,		// 0x0047ACCA in relation to container
		kEvent_OnDestructionStageChange =	0x200000,		// 0x004763E7/0x0047ADEE
		kEvent_OnFire =						0x400000,		// 0x008BAFB9 (references to package use item and use weapon are close)

		kEvent_OnTrigger =					0x10000000,		// 0x005D8D6A	Cmd_EnterTrigger_Execute
		kEvent_OnTriggerEnter =				0x20000000,		// 0x005D8D50	Cmd_EnterTrigger_Execute
		kEvent_OnTriggerLeave =				0x40000000,		// 0x0062C946	OnTriggerLeave ?
		kEvent_OnReset =					0x80000000		// 0x0054E5FB
	};

	struct Event
	{
		TESForm		*object;
		UInt32		eventMask;
	};

	struct Struct10
	{
		bool	effectStart;
		bool	effectFinish;
		UInt8	unk03[6];
	};

	typedef tList<Event> EventList;
	typedef tList<ScriptVar> VarList;

	Script			*m_script;		// 00
	UInt32			m_unk1;			// 04
	EventList		*m_eventList;	// 08
	VarList			*m_vars;		// 0C
	Struct10		*unk010;		// 10

	void Dump(void);
	ScriptVar *GetVariable(UInt32 id) const;
	UInt32 ResetAllVariables();
};

ScriptEventList* EventListFromForm(TESForm* form);

typedef bool (* _MarkBaseExtraListScriptEvent)(TESForm* target, BaseExtraList* extraList, UInt32 eventMask);
extern const _MarkBaseExtraListScriptEvent MarkBaseExtraListScriptEvent;

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
			ScriptVar			*var;
			ScriptEventList		*parent;
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

// 914
class ConsoleManager
{
public:
#if RUNTIME
	MEMBER_FN_PREFIX(ConsoleManager);
	DEFINE_MEMBER_FN(Print, void, s_Console__Print, const char * fmt, va_list args);
#endif

	ConsoleManager();
	~ConsoleManager();

	struct TextNode
	{
		TextNode	*next;
		TextNode	*prev;
		String		text;
	};

	struct TextList
	{
		TextNode	*first;
		TextNode	*last;
		UInt32		count;

		TextList* Append(TextNode* newList);
	};

	struct RecordedCommand
	{
		char buf[100];
	};

	void* scriptContext;
	TextList printedLines;
	TextList inputHistory;
	unsigned int historyIndex;
	unsigned int unk020;
	unsigned int printedCount;
	unsigned int unk028;
	unsigned int lineHeight;
	int textXPos;
	int textYPos;
	UInt8 isConsoleOpen;
	UInt8 unk39;
	UInt8 isBatchRecording;
	UInt8 unk3B;
	unsigned int numRecordedCommands;
	RecordedCommand recordedCommands[20];
	char scofPath[260];

	static ConsoleManager * GetSingleton(void);
	void AppendToSentHistory(const char* src);
};
STATIC_ASSERT(sizeof(ConsoleManager) == 0x914);

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
	virtual std::string GetFormatString() = 0;						// return format string
};

// concrete class used for extracting script args
class ScriptFormatStringArgs : public FormatStringArgs
{
public:
	virtual bool Arg(argType asType, void* outResult);
	virtual bool SkipArgs(UInt32 numToSkip);
	virtual bool HasMoreArgs();
	virtual std::string GetFormatString();

	ScriptFormatStringArgs(UInt32 _numArgs, UInt8* _scriptData, Script* _scriptObj, ScriptEventList* _eventList);
	UInt32 GetNumArgs();
	UInt8* GetScriptData();

private:
	UInt32			numArgs;
	UInt8			* scriptData;
	Script			* scriptObj;
	ScriptEventList	* eventList;
	std::string fmtString;
};
bool SCRIPT_ASSERT(bool expr, Script* script, const char * errorMsg, ...);

bool ExtractSetStatementVar(Script* script, ScriptEventList* eventList, void* scriptDataIn, double* outVarData, UInt8* outModIndex = NULL, bool shortPath = false);
bool ExtractFormattedString(FormatStringArgs& args, char* buffer);

class ChangesMap;
class InteriorCellNewReferencesMap;
class ExteriorCellNewReferencesMap;
class NumericIDBufferMap;

class NiBinaryStream
{
public:
	NiBinaryStream();
	~NiBinaryStream();

	virtual void	Destructor(bool freeMemory);		// 00
	virtual void	Unk_01(void);						// 04
	virtual void	SeekCur(SInt32 delta);				// 08
	virtual void	GetBufferSize(void);				// 0C
	virtual void	InitReadWriteProcs(bool useAlt);	// 10

//	void	** m_vtbl;		// 000
	UInt32	m_offset;		// 004
	void	* m_readProc;	// 008 - function pointer
	void	* m_writeProc;	// 00C - function pointer
};

class NiFile: public NiBinaryStream
{
public:
	NiFile();
	~NiFile();

	virtual UInt32	SetOffset(UInt32 newOffset, UInt32 arg2);	// 14
	virtual UInt32	GetFilename(void);	// 18
	virtual UInt32	GetSize();			// 1C

	UInt32	m_bufSize;	// 010
	UInt32	m_unk014;	// 014 - Total read in buffer
	UInt32	m_unk018;	// 018 - Consumed from buffer
	UInt32	m_unk01C;	// 01C
	void*	m_buffer;	// 020
	FILE*	m_File;		// 024
};

// 158
class BSFile : public NiFile
{
public:
	BSFile();
	~BSFile();

	virtual bool	Reset(bool arg1, bool arg2);	// 20
	virtual bool	Unk_09(UInt32 arg1);	// 24
	virtual UInt32	Unk_0A();	// 28
	virtual UInt32	Unk_0B(String *string, UInt32 arg2);	// 2C
	virtual UInt32	Unk_0C(void *ptr, UInt32 arg2);	// 30
	virtual UInt32	ReadBufDelim(void *bufferPtr, UInt32 bufferSize, short delim);		// 34
	virtual UInt32	Unk_0E(void *ptr, UInt8 arg2);	// 38
	virtual UInt32	Unk_0F(void *ptr, UInt8 arg2);	// 3C
	virtual bool	IsReadable();	// 40
	virtual UInt32	ReadBuf(void *bufferPtr, UInt32 numBytes);	// 44
	virtual UInt32	WriteBuf(void *bufferPtr, UInt32 numBytes);	// 48

	UInt32		m_modeReadWriteAppend;	// 028
	UInt8		m_good;					// 02C
	UInt8		pad02D[3];				// 02D
	UInt8		m_unk030;				// 030
	UInt8		pad031[3];				// 031
	UInt32		m_unk034;				// 034
	UInt32		m_unk038;				// 038 - init'd to FFFFFFFF
	UInt32		m_unk03C;				// 038
	UInt32		m_unk040;				// 038
	char		m_path[0x104];			// 044
	UInt32		m_unk148;				// 148
	UInt32		m_unk14C;				// 14C
	UInt32		m_fileSize;				// 150
	UInt32		m_unk154;				// 154
};

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
public:
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
public:
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
#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
	DEFINE_MEMBER_FN(AddCreatedForm, UInt32, 0x00861780, TESForm * pForm);
#elif RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525ng
	DEFINE_MEMBER_FN(AddCreatedForm, UInt32, 0x00861330, TESForm * pForm);
#elif EDITOR
#else
#error
#endif
};

#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
const UInt32 _SaveGameManager_ConstructSavegameFilename = 0x0084FF90;
const UInt32 _SaveGameManager_ConstructSavegamePath		= 0x0084FF30;
#elif RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525ng
const UInt32 _SaveGameManager_ConstructSavegameFilename = 0x0084F9E0;
const UInt32 _SaveGameManager_ConstructSavegamePath		= 0x0084F980;
#elif EDITOR
#else
#error
#endif

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
		NiTMapBase<RefID, IndexRefID>	* map000;	// 000
		NiTMapBase<IndexRefID, RefID>	* map010;	// 010
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
	NiTMapBase<TESForm *, BGSLoadGameSubBuffer> * maps014[3];			// 014	0 = changed Animations, 2 = changed Havok Move
	NiTMapBase<UInt32, UInt32>					* map018;				// 018	
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
	
	static BGSSaveLoadGame* GetSingleton() { return *(BGSSaveLoadGame * *)0x11DDF38; };
};
STATIC_ASSERT(sizeof(BGSSaveLoadGame) == 0x254);

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
	UInt32 numSaves;						// 04
	UInt32 unk08;							// 08
	UInt8 unk0C;							// 0C
	UInt8 unk0D;							// 0D
	UInt8 unk0E;							// 0E
	UInt8 autoSaveTimer;					// 0F
	UInt8 forceSaveTimer;					// 10
	UInt8 systemSaveTimer;					// 11
	UInt8 unk12;							// 12
	UInt8 unk13;							// 13
	UInt32 list14;							// 14
	SInt32 unk18;							// 18
	UInt8 unk1C;							// 1C
	UInt8 unk1D;							// 1D
	UInt8 pad1E[2];							// 1E
	UInt32 unk20;							// 20
	UInt8 unk24;							// 24
	UInt8 unk25;							// 25
	UInt8 unk26;							// 26
	UInt8 unk27;							// 27
	void* func28;							// 28
	void* func2C;							// 2C
	void* str30;							// 30
	void* unk34;							// 34
	
	void ReloadCurrentSave() { ThisStdCall(0x8512F0, this); };
	bool Save(char* name) { return ThisStdCall(0x8503B0, this, name, -1, 0); };
};

std::string GetSavegamePath();

#endif

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
class ObstacleData;
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
