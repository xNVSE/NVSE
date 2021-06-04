#pragma once

#include "Utilities.h"
#include "GameForms.h"

struct ScriptEventList;
struct ScriptBuffer;

#if RUNTIME
#define SCRIPT_SIZE 0x54
static const UInt32 kScript_ExecuteFnAddr = 0x005AC1E0;
#elif EDITOR
#define SCRIPT_SIZE 0x48
static const UInt32 kScript_SetTextFnAddr = 0x005C27B0;
#endif

extern CRITICAL_SECTION	csGameScript;				// trying to avoid what looks like concurrency issues
extern const char* g_variableTypeNames[];
// 54 / 48
class Script : public TESForm
{
public:
	Script();
	~Script();

	// members

	struct RefVariable
	{
		String	name;		// 000 variable name/editorID (not used at run-time)
		TESForm	* form;		// 008
		UInt32	varIdx;		// 00C

		void	Resolve(ScriptEventList * eventList);
	};

	struct RefList : tList<RefVariable>
	{
		UInt32 GetIndex(Script::RefVariable* refVar);
	};

	enum VariableType
	{
		eVarType_Float = 0,			//ref is also zero
		eVarType_Integer,

		// NVSE, return values only
		eVarType_String,
		eVarType_Array,
		eVarType_Ref,

		eVarType_Invalid
	};
	
	struct VarInfoList : tList<VariableInfo>
	{
		VariableInfo* GetVariableByName(const char* name);
	};
	typedef Visitor<VarInfoList, VariableInfo> VarListVisitor;

	// 14
	struct ScriptInfo
	{
		UInt32	unusedVariableCount;		// 00 (18)
		UInt32	numRefs;	// 04 (1C)
		UInt32	dataLength;	// 08 (20)
		UInt32	varCount;	// 0C (24)
		UInt16	type;		// 10 (28)
		bool	compiled;	// 12 (2A)
		UInt8   unk13;      // 13 (2B)
	};

	enum {
		eType_Object = 0,
		eType_Quest = 1,
		eType_Magic = 0x100,
		eType_Unk = 0x10000,
	};
#if !RUNTIME
	UInt32			unk028;					//     /     / 028
#endif
	ScriptInfo		info;					// 018 / 018 / 02C
	char			* text;					// 02C / 02C / 040
	void			* data;					// 030 / 030 / 044
#if RUNTIME
	float			unk34;					// 034
	float			questDelayTimeCounter;	// 038      - init'd to fQuestDelayTime, decremented by frametime each frame
	float			secondsPassed;			// 03C      - only if you've modified fQuestDelayTime
	TESQuest*		quest;					// 040
#endif
	RefList	refList;				// 044 / 034 / 048 - ref variables and immediates
	VarInfoList	varList;				// 04C / 03C / 050 - local variable list
#if !RUNTIME
	void			* unk050;				//     /     / 050
	UInt8			unk054;					//	   /     / 054
	UInt8			pad055[3];
#endif

	RefVariable *	GetRefFromRefList(UInt32 refIdx);
	VariableInfo*	GetVariableInfo(UInt32 idx);

	UInt32			AddVariable(TESForm * form);
	void			CleanupVariables(void);

	UInt32			Type() const { return info.type; }
	bool			IsObjectScript() const {return info.type == eType_Object; }
	bool			IsQuestScript() const { return info.type == eType_Quest; }
	bool			IsMagicScript() const { return info.type == eType_Magic; }
	bool			IsUnkScript() const { return info.type == eType_Unk; }

	VariableInfo*	GetVariableByName(const char* varName);
	UInt32			GetVariableType(VariableInfo* var);

	bool			IsUserDefinedFunction() const;

	static bool	RunScriptLine(const char * text, TESObjectREFR * object = NULL);
	static bool	RunScriptLine2(const char * text, TESObjectREFR * object = NULL, bool bSuppressOutput = true);

	// no changed flags (TESForm flags)
	MEMBER_FN_PREFIX(Script);
#if RUNTIME
	// arg3 appears to be true for result scripts (runs script even if dataLength <= 4)
	DEFINE_MEMBER_FN(Execute, bool, kScript_ExecuteFnAddr, TESObjectREFR* thisObj, ScriptEventList* eventList, TESObjectREFR* containingObj, bool arg3);
	DEFINE_MEMBER_FN(Constructor, Script *, 0x005AA0F0);
	DEFINE_MEMBER_FN(SetText, void, 0x005ABE50, const char * text);
	DEFINE_MEMBER_FN(Run, bool, 0x005AC400, void * scriptContext, bool unkAlwaysOne, TESObjectREFR * object);
	DEFINE_MEMBER_FN(Destructor, void, 0x005AA1A0);
#endif
	ScriptEventList	* CreateEventList();
	UInt32 GetVarCount() const;
	UInt32 GetRefCount() const;

	tList<VariableInfo>* GetVars();
	tList<RefVariable>* GetRefList();
};

STATIC_ASSERT(sizeof(Script) == SCRIPT_SIZE);

struct ConditionEntry
{
	struct Data
	{
		union Param
		{
			float	number;
			TESForm	* form;
		};

		// ### TODO: this
		UInt32		operatorAndFlags;	// 00
		float		comparisonValue;	// 04
		UInt16		functionIndex;		// 08 is opcode & 0x0FFF
		UInt16		unk0A;
		Param		param1;				// 0C
		Param		param2;				// 10
		UInt32		unk14;
	};

	Data			* data;
	ConditionEntry	* next;
};

// 6C
struct QuestStageItem
{
	UInt32			unk00;			// 00
	ConditionEntry	conditionList;	// 04
	Script			resultScript;	// 0C
	UInt32			unk5C;			// 5C disk offset to log text records? consistent within a single quest
	UInt8			index;			// 60 sequential
	bool			hasLogText;		// 61
	UInt8			unk62[2];		// 62 pad?
	UInt32			logDate;		// 64
	TESQuest		* owningQuest;	// 68;
};

#if RUNTIME
STATIC_ASSERT(sizeof(QuestStageItem) == (SCRIPT_SIZE + 0x1C));
#endif

// 41C
struct ScriptLineBuffer
{
	static const UInt32	kBufferSize = 0x200;

	UInt32				lineNumber;			// 000 counts blank lines too
	char				paramText[0x200];	// 004 portion of line text following command
	UInt32				paramTextLen;		// 204
	UInt32				lineOffset;			// 208
	UInt8				dataBuf[0x200];		// 20C
	UInt32				dataOffset;			// 40C
	UInt32				cmdOpcode;			// 410 not initialized. Opcode of command being parsed
	UInt32				callingRefIndex;	// 414 not initialized. Zero if cmd not invoked with dot syntax
	UInt32				errorCode;			// 418

	// these write data and update dataOffset
	bool Write(const void* buf, UInt32 bufsize);
	bool WriteFloat(double buf);
	bool WriteString(const char* buf);
	bool Write32(UInt32 buf);
	bool Write16(UInt16 buf);
	bool WriteByte(UInt8 buf);
};

// size 0x58? Nothing initialized beyond 0x50.
struct ScriptBuffer
{	
	enum RuntimeMode
	{
		kEditor = 0,
		kGameConsole = 1,
	};

	char		* scriptText;		// 000
	UInt32			textOffset;			// 004 
	RuntimeMode		runtimeMode;		// 008
	String			scriptName;			// 00C
	UInt32			errorCode;			// 014
	bool			partialScript;		// 018
	UInt8			pad019[3];			// 018
	UInt32			curLineNumber;		// 01C
	UInt8			* scriptData;		// 020 pointer to 0x4000-byte array
	UInt32			dataOffset;			// 024
	Script::ScriptInfo	info;				// 028
	Script::VarInfoList	vars;		// 03C
	Script::RefList	refVars;	// 044 probably ref vars
	Script*			currentScript;				// 04C num lines?
	tList<ScriptLineBuffer>	lines;		// 050
	// nothing else initialized

	// convert a variable or form to a RefVar, add to refList if necessary
	Script::RefVariable* ResolveRef(const char* refName, Script* script);
	UInt32	GetRefIdx(Script::RefVariable* ref);
	VariableInfo* GetVariableByName(const char* name);
	UInt32	GetVariableType(VariableInfo* varInfo, Script::RefVariable* refVar, Script* script);
};
static_assert(sizeof(ScriptBuffer) == 0x58);


Script::VariableType VariableTypeNameToType(const char* name);

UInt32 GetDeclaredVariableType(const char* varName, const char* scriptText, Script* script);	// parses scriptText to determine var type
Script* GetScriptFromForm(TESForm* form);

