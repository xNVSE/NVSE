#pragma once

#define SCRIPT_SIZE 0x54
static const UInt32 kScript_ExecuteFnAddr = 0x005AC1E0;

extern CRITICAL_SECTION	csGameScript;				// trying to avoid what looks like concurrency issues


// 54 / 48
class Script : public TESForm
{
public:
	Script();
	~Script();

	// members

	struct RefVariable
	{
		String		name;		// 00 variable name/editorID (not used at run-time)
		TESForm		*form;		// 08
		UInt32		varIdx;		// 0C always zero in editor

		void	Resolve(ScriptEventList * eventList);
	};

	struct RefVarList : tList<RefVariable>
	{
		RefVariable *GetRefVariableByName(const char *name) const;
		UInt32 GetIndex(RefVariable *refVar);
	};

	enum
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
		VariableInfo* GetVariableByName(const char* name) const;
	};

	// 14
	struct ScriptInfo
	{
		UInt32	unk0;			// 00 (18)
		UInt32	numRefs;		// 04 (1C)
		UInt32	dataLength;		// 08 (20)
		UInt32	varCount;		// 0C (24)
		UInt16	type;			// 10 (28)
		UInt16	unk12;			// 12 (2A)
	};

	enum
	{
		eType_Object =	0,
		eType_Quest =	1,
		eType_Magic =	0x100,
		eType_Unk =		0x10000,
	};
#if !RUNTIME
	UInt32			unk028;					//     /     / 028
#endif
	ScriptInfo		info;					// 018 / 018 / 02C
	char			*text;					// 02C / 02C / 040
	UInt8			*data;					// 030 / 030 / 044
#if RUNTIME
	float			unk34;					// 034
	float			questDelayTimeCounter;	// 038      - init'd to fQuestDelayTime, decremented by frametime each frame
	float			secondsPassed;			// 03C      - only if you've modified fQuestDelayTime
	TESQuest		*quest;					// 040
#endif
	RefVarList		refList;				// 044 / 034 / 048 - ref variables and immediates
	VarInfoList		varList;				// 04C / 03C / 050 - local variable list
#if !RUNTIME
	void			*unk050;				//     /     / 050
	UInt8			unk054;					//	   /     / 054
	UInt8			pad055[3];
#endif

	RefVariable*	GetRefVariable(UInt32 reqIdx) const;
	VariableInfo*	GetVariableInfo(UInt32 idx) const;
	VariableInfo*	GetVariableInfo(const char* name) const;

	UInt32			AddVariable(TESForm *form);
	void			CleanupVariables(void);

	UInt32			Type() const {return info.type;}
	bool			IsObjectScript() const {return info.type == eType_Object;}
	bool			IsQuestScript() const {return info.type == eType_Quest;}
	bool			IsMagicScript() const {return info.type == eType_Magic;}
	bool			IsUnkScript() const {return info.type == eType_Unk;}

	UInt32			GetVariableType(VariableInfo *var);
	ScriptVar		*AddVariable(ScriptEventList *eventList, UInt32 ownerID, UInt8 modIdx);
	UInt32			GetDataLength();

	static bool	RunScriptLine(const char *text, TESObjectREFR *object = NULL);
	static bool	RunScriptLine2(const char *text, TESObjectREFR *object = NULL, bool bSuppressOutput = true);

	// no changed flags (TESForm flags)
	MEMBER_FN_PREFIX(Script);
	// arg3 appears to be true for result scripts (runs script even if dataLength <= 4)
	DEFINE_MEMBER_FN(Execute, bool, kScript_ExecuteFnAddr, TESObjectREFR* thisObj, ScriptEventList* eventList, TESObjectREFR* containingObj, bool arg3);
	DEFINE_MEMBER_FN(Constructor, Script *, 0x005AA0F0);
	DEFINE_MEMBER_FN(SetText, void, 0x005ABE50, const char * text);
	DEFINE_MEMBER_FN(Run, bool, 0x005AC400, void * scriptContext, bool unkAlwaysOne, TESObjectREFR * object);
	DEFINE_MEMBER_FN(Destructor, void, 0x005AA1A0);

	ScriptEventList	*CreateEventList();
};
STATIC_ASSERT(sizeof(Script) == SCRIPT_SIZE);

struct ScriptRunner
{
	UInt32				unk00;			// 00
	TESForm				*baseForm;		// 04
	ScriptEventList		*eventList;		// 08
	UInt32				unk0C;			// 0C
	UInt32				unk10;			// 10
	Script				*script;		// 14
	UInt32				unk18;			// 18	= 6 after failed to evaluate expression
	UInt32				unk1C;			// 1C
	UInt32				stackDepth;		// 20
	UInt32				stack[10];		// 24
	UInt32				stack2Depth;	// 4C
	UInt32				stack2[10];		// 50
	UInt32				stack3[10];		// 78
	UInt8				byteA0;			// A0
	UInt8				byteA1;			// A1	is set during runLine if CmdExecute.byt025 is not NULL
	UInt8				padA2[2];		// A2
};
STATIC_ASSERT(sizeof(ScriptRunner) == 0xA4);

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
	UInt32				unk418;				// 418

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
	template <typename tData> struct Node
	{
		tData		* data;
		Node<tData>	* next;
	};

	char					*scriptText;		// 000
	UInt32					textOffset;			// 004 
	UInt32					unk008;				// 008
	String					scriptName;			// 00C
	UInt32					unk014;				// 014
	UInt16					unk018;				// 018
	UInt16					unk01A;				// 01A
	UInt32					curLineNumber;		// 01C 
	UInt8					*scriptData;		// 020 pointer to 0x4000-byte array
	UInt32					dataOffset;			// 024
	UInt32					unk028;				// 028
	UInt32					numRefs;			// 02C
	UInt32					unk030;				// 030
	UInt32					varCount;			// 034 script->varCount
	UInt8					scriptType;			// 038 script->type
	UInt8					unk039;				// 039 script->unk35
	UInt8					unk03A[2];
	Script::VarInfoList		vars;				// 03C
	Script::RefVarList		refVars;			// 044 probably ref vars
	UInt32					unk04C;				// 04C num lines?
	Node<ScriptLineBuffer>	lines;				// 050
	// nothing else initialized

	// convert a variable or form to a RefVar, add to refList if necessary
	Script::RefVariable* ResolveRef(const char* refName);
	UInt32 GetRefIdx(Script::RefVariable *refVar);
	UInt32 GetVariableType(VariableInfo *varInfo, Script::RefVariable *refVar);
};

UInt32 GetDeclaredVariableType(const char* varName, const char* scriptText);	// parses scriptText to determine var type
Script* GetScriptFromForm(TESForm* form);

