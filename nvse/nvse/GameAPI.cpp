#include "GameAPI.h"
#include "GameRTTI.h"
#include "GameForms.h"
#include "GameObjects.h"
#include "GameTypes.h"
#include "CommandTable.h"
#include "GameScript.h"
#include "Hooks_Other.h"
#include "StringVar.h"

#if NVSE_CORE
#include "Hooks_Script.h"
#include "ScriptUtils.h"
#endif

UInt8* g_lastScriptData;

static NVSEStringVarInterface* s_StringVarInterface = NULL;
bool extraTraces = false;
bool alternateUpdate3D = false;

// arg1 = 1, ignored if canCreateNew is false, passed to 'init' function if a new object is created
typedef void * (* _GetSingleton)(bool canCreateNew);

#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525

const _ExtractArgs ExtractArgs = (_ExtractArgs)0x005ACCB0;

const _FormHeap_Allocate FormHeap_Allocate = (_FormHeap_Allocate)0x00401000;
const _FormHeap_Free FormHeap_Free = (_FormHeap_Free)0x00401030;

const _LookupFormByID LookupFormByID = (_LookupFormByID)0x004839C0;
const _CreateFormInstance CreateFormInstance = (_CreateFormInstance)0x00465110;

const _GetSingleton ConsoleManager_GetSingleton = (_GetSingleton)0x0071B160;
bool * bEchoConsole = (bool*)0x011F158C;

const _QueueUIMessage QueueUIMessage = (_QueueUIMessage)0x007052F0;	// Called from Cmd_AddSpell_Execute

const _ShowMessageBox ShowMessageBox = (_ShowMessageBox)0x00703E80;
const _ShowMessageBox_Callback ShowMessageBox_Callback = (_ShowMessageBox_Callback)0x005B4A70;
const _ShowMessageBox_pScriptRefID ShowMessageBox_pScriptRefID = (_ShowMessageBox_pScriptRefID)0x011CAC64;
const _ShowMessageBox_button ShowMessageBox_button = (_ShowMessageBox_button)0x0118C684;

const _GetActorValueName GetActorValueName = (_GetActorValueName)0x00066EAC0;	// See Cmd_GetActorValue_Eval
const UInt32 * g_TlsIndexPtr = (UInt32 *)0x0126FD98;
const _MarkBaseExtraListScriptEvent MarkBaseExtraListScriptEvent = (_MarkBaseExtraListScriptEvent)0x005AC750;
const _DoCheckScriptRunnerAndRun DoCheckScriptRunnerAndRun = (_DoCheckScriptRunnerAndRun)0x005AC190;

SaveGameManager ** g_saveGameManager = (SaveGameManager**)0x011DE134;

#elif RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525ng

const _ExtractArgs ExtractArgs = (_ExtractArgs)0x005ACE60;

const _FormHeap_Allocate FormHeap_Allocate = (_FormHeap_Allocate)0x00401000;
const _FormHeap_Free FormHeap_Free = (_FormHeap_Free)0x0042F4D0;

const _LookupFormByID LookupFormByID = (_LookupFormByID)0x004849B0;
const _CreateFormInstance CreateFormInstance = (_CreateFormInstance)0x00465F10;

const _GetSingleton ConsoleManager_GetSingleton = (_GetSingleton)0x0071B110;
bool * bEchoConsole = (bool*)0x011F158C;

const _QueueUIMessage QueueUIMessage = (_QueueUIMessage)0x00705220;	// Called from Cmd_AddSpell_Execute

const _ShowMessageBox ShowMessageBox = (_ShowMessageBox)0x00703DB0;
const _ShowMessageBox_Callback ShowMessageBox_Callback = (_ShowMessageBox_Callback)0x005B4B10;
const _ShowMessageBox_pScriptRefID ShowMessageBox_pScriptRefID = (_ShowMessageBox_pScriptRefID)0x011CAC64;
const _ShowMessageBox_button ShowMessageBox_button = (_ShowMessageBox_button)0x0118C684;

const _GetActorValueName GetActorValueName = (_GetActorValueName)0x0066E620;	// See Cmd_GetActorValue_Eval
const UInt32 * g_TlsIndexPtr = (UInt32 *)0x0126FD98;
const _MarkBaseExtraListScriptEvent MarkBaseExtraListScriptEvent = (_MarkBaseExtraListScriptEvent)0x005AC900;
const _DoCheckScriptRunnerAndRun DoCheckScriptRunnerAndRun = (_DoCheckScriptRunnerAndRun)0x005AC340;

SaveGameManager ** g_saveGameManager = (SaveGameManager**)0x011DE134;

#elif EDITOR

//	FormMap* g_FormMap = (FormMap *)0x009EE18C;		// currently unused
//	DataHandler ** g_dataHandler = (DataHandler **)0x00A0E064;
//	TES** g_TES = (TES**)0x00A0ABB0;
	const _LookupFormByID LookupFormByID = (_LookupFormByID)0x004F9620;	// Call between third reference to RTTI_TESWorldspace and RuntimeDynamicCast
	const _GetFormByID GetFormByID = (_GetFormByID)(0x004F9650); // Search for aNonPersistentR and aPlayer (third call below aPlayer, second is LookupFomrByID)
	const _FormHeap_Allocate FormHeap_Allocate = (_FormHeap_Allocate)0x00401000;
	const _FormHeap_Free FormHeap_Free = (_FormHeap_Free)0x0000401180;
	const _ShowCompilerError ShowCompilerError = (_ShowCompilerError)0x005C5730;	// Called with aNonPersistentR (still same sub as the other one)

#else

#error RUNTIME_VERSION unknown

#endif

#if RUNTIME

struct TLSData
{
	// thread local storage

	UInt32	pad000[(0x260 - 0x000) >> 2];	// 000
	NiNode			* lastNiNode;			// 260	248 in FOSE
	TESObjectREFR	* lastNiNodeREFR;		// 264	24C in FOSE
	UInt8			consoleMode;			// 268
	UInt8			pad269[3];				// 269
	// 25C is used as do not head track the player , 
	// 2B8 is used to init QueudFile::unk0018, 
	// 28C might count the recursive calls to Activate, limited to 5.
};

STATIC_ASSERT(offsetof(TLSData, consoleMode) == 0x268);

static TLSData * GetTLSData()
{
	UInt32 TlsIndex = *g_TlsIndexPtr;
	TLSData * data = NULL;

	__asm {
		mov		ecx,	[TlsIndex]
		mov		edx,	fs:[2Ch]	// linear address of thread local storage array
		mov		eax,	[edx+ecx*4]
		mov		[data], eax
	}

	return data;
}

bool IsConsoleMode()
{
	TLSData * tlsData = GetTLSData();

	if(tlsData)
		return tlsData->consoleMode != 0;

	return false;
}

bool GetConsoleEcho()
{
	return *bEchoConsole != 0;
}

void SetConsoleEcho(bool doEcho)
{
	*bEchoConsole = doEcho ? 1 : 0;
}

const char * GetFullName(TESForm * baseForm)
{
	if(baseForm)
	{
		TESFullName* fullName = baseForm->GetFullName();
		if(fullName && fullName->name.m_data)
		{
			if (fullName->name.m_dataLen)
				return fullName->name.m_data;
		}
	}

	return "<no name>";
}

ConsoleManager * ConsoleManager::GetSingleton(void)
{
	return (ConsoleManager *)ConsoleManager_GetSingleton(true);
}

char * ConsoleManager::GetConsoleOutputFilename(void)
{
	return GetSingleton()->COFileName;
};

bool ConsoleManager::HasConsoleOutputFilename(void) {
	return 0 != GetSingleton()->COFileName[0];
};

bool s_InsideOnActorEquipHook = false;
UInt32 s_CheckInsideOnActorEquipHook = 1;

void Console_Print(const char * fmt, ...)
{
	if (!s_CheckInsideOnActorEquipHook || !s_InsideOnActorEquipHook) {
		ConsoleManager	* mgr = ConsoleManager::GetSingleton();
		if(mgr)
		{
			va_list	args;

			va_start(args, fmt);

			CALL_MEMBER_FN(mgr, Print)(fmt, args);

			va_end(args);
		}
	}
}

TESSaveLoadGame * TESSaveLoadGame::Get()
{
	return (TESSaveLoadGame *)0x011DE45C;
}

SaveGameManager* SaveGameManager::GetSingleton()
{
	return *g_saveGameManager;
}

std::string GetSavegamePath()
{
	char path[0x104];
	CALL_MEMBER_FN(SaveGameManager::GetSingleton(), ConstructSavegamePath)(path);
	return path;
}

// ExtractArgsEx code
ScriptEventList* ResolveExternalVar(ScriptEventList* in_EventList, Script* in_Script, UInt8* &scriptData)
{
	ScriptEventList* refEventList = NULL;
	UInt16 varIdx = *((UInt16*)++scriptData);
	scriptData += 2;

	Script::RefVariable* refVar = in_Script->GetVariable(varIdx);
	if (refVar)
	{
		refVar->Resolve(in_EventList);
		TESForm* refObj = refVar->form;
		if (refObj)
		{
			if (refObj->typeID == kFormType_Reference)
			{
				TESObjectREFR* refr = DYNAMIC_CAST(refObj, TESForm, TESObjectREFR);
				if (refr)
					refEventList = refr->GetEventList();
			}
			else if (refObj->typeID == kFormType_Quest)
			{
				TESQuest* quest = DYNAMIC_CAST(refObj, TESForm, TESQuest);
				if (quest)
					refEventList = quest->scriptEventList;
			}
		}
	}

	return refEventList;
}

TESGlobal* ResolveGlobalVar(ScriptEventList* in_EventList, Script* in_Script, UInt8* &scriptData)
{
	TESGlobal* global = NULL;
	UInt16 varIdx = *((UInt16*)++scriptData);
	scriptData += 2;

	Script::RefVariable* globalRef = in_Script->GetVariable(varIdx);
	if (globalRef)
		global = (TESGlobal*)DYNAMIC_CAST(globalRef->form, TESForm, TESGlobal);

	return global;
}

void ScriptEventList::Destructor()
{
// OBLIVION	ThisStdCall(0x004FB4E0, this);
	if (m_eventList)
		m_eventList->RemoveAll();
	while (m_vars) {
		if (m_vars->var) {
			FormHeap_Free(m_vars->var);
		}
		VarEntry* next = m_vars->next;
		FormHeap_Free(m_vars);
		m_vars = next;
	}
}

static bool ExtractFloat(double& out, UInt8* &scriptData, Script* scriptObj, ScriptEventList* eventList)
{
	//extracts one float arg

	bool ret = false;
	if (*scriptData == 'r')		//reference to var in another script
	{
		eventList = ResolveExternalVar(eventList, scriptObj, scriptData);
		if (!eventList)			//couldn't resolve script ref
			return false;
	}	

	switch (*scriptData)
	{
	case 'G':		//global var
	{
		TESGlobal* global = ResolveGlobalVar(eventList, scriptObj, scriptData);
		if (global)
		{
			out = global->data;
			ret = true;
		}
		break;
	}
	case 'z':		//literal double
	{
		out = *((double*)++scriptData);
		scriptData += sizeof(double);
		ret = true;
		break;
	}
	case 'f':
	case 's':		//local var
	{
		UInt16 varIdx = *((UInt16*)++scriptData);
		scriptData += 2;
		ScriptEventList::Var* var = eventList->GetVariable(varIdx);
		if (var)
		{
			out = var->data;
			ret = true;
		}
		break;
	}
	}
	return ret;
}

TESForm* ExtractFormFromFloat(UInt8* &scriptData, Script* scriptObj, ScriptEventList* eventList)
{
	TESForm* outForm = NULL;
	if (*scriptData == 'r')		//doesn't work as intended yet so refs must be local vars
	{
		eventList = ResolveExternalVar(eventList, scriptObj, scriptData);
		if (!eventList)
			return NULL;
	}

	UInt16 varIdx = *(UInt16*)++scriptData;
	scriptData += 2;

	ScriptEventList::Var* var = eventList->GetVariable(varIdx);
	if (var)
		outForm = LookupFormByID(*((UInt64 *)&var->data));

	return outForm;
}

TESForm* ResolveForm(UInt8* &scriptData, Script* scriptObj, ScriptEventList* eventList)
{
	TESForm* outForm = NULL;
	char argType = *scriptData;
	UInt16	varIdx = *((UInt16 *)(scriptData+1));
//	scriptData += 2;

	switch (argType)
	{
	case 'r':
		{
			Script::RefVariable	* var = scriptObj->GetVariable(varIdx);
			if(var)
			{
				var->Resolve(eventList);
				outForm = var->form;
				scriptData += 3;
			}
		}
		break;
	case 'f':
			outForm = ExtractFormFromFloat(scriptData, scriptObj, eventList);
			break;
	}
	return outForm;
}

static const char* StringFromStringVar(UInt32 strID)
{
#if NVSE_CORE
	StringVar* strVar = g_StringMap.Get(strID);
	return strVar ? strVar->GetCString() : "";
#else
	if (s_StringVarInterface)
		return s_StringVarInterface->GetString(strID);
	else
		return "";
#endif
}

static const char* ResolveStringArgument(ScriptEventList* eventList, const char* stringArg)
{
	const char* result = stringArg;

	if (stringArg && stringArg[0] == '$')
	{
		VariableInfo* varInfo = eventList->m_script->GetVariableByName(stringArg + 1);
		if (varInfo)
		{
			ScriptEventList::Var* var = eventList->GetVariable(varInfo->idx);
			if (var)
				result = StringFromStringVar(var->data);
		}
	}

	return result;
}

static bool v_ExtractArgsEx(SInt16 numArgs, ParamInfo * paramInfo, UInt8* &scriptData, Script * scriptObj, ScriptEventList * eventList, va_list args)
{
#if NVSE_CORE
	if (numArgs < 0) {
		UInt32 opcodeOffsetPtr = 0;
		if (ExtractArgsOverride::ExtractArgs(paramInfo, scriptData, scriptObj, eventList, &opcodeOffsetPtr, args, false, numArgs)) {
			scriptData += opcodeOffsetPtr;
			return true;
		}
		else {
			DEBUG_PRINT("v_ExtractArgsEx returns false");
			return false;
		}
	}
#endif

	for(UInt32 i = 0; i < numArgs; i++)
	{
		ParamInfo	* info = &paramInfo[i];

		//DEBUG_MESSAGE("ParamType: %d Type: %d Param: %s scriptData: %08x", info->typeID, *scriptData, info->typeStr, scriptData);	

		switch(info->typeID)
		{
			case kParamType_String:
			{
				char	* out = va_arg(args, char *);

				UInt16	len = *((UInt16 *)scriptData);
				scriptData += 2;

				memcpy(out, scriptData, len);
				scriptData += len;

				out[len] = 0;

				const char* resolved = ResolveStringArgument(eventList, out);
				if (resolved != out)
				{
					UInt32 resolvedLen = strlen(resolved);
					memcpy(out, resolved, resolvedLen);
					out[resolvedLen] = 0;
				}
			}
			break;

			case kParamType_Integer:
			case kParamType_QuestStage:
			{
				UInt32	* out = va_arg(args, UInt32 *);
				UInt8	type = *scriptData;
				switch(type)
				{
					case 0x6E: // "n"
						*out = *((UInt32 *)++scriptData);
						scriptData += sizeof(UInt32);
						break;
					case 0x7A: // "z"
						*out = *((double *)++scriptData);
						scriptData += sizeof(double);
						break;					
					case 0x66: // "f"
					case 0x72: // "r"
					case 0x73: // "s"
					case 0x47: // "G"
					{
						double data = 0;
						if (ExtractFloat(data, scriptData, scriptObj, eventList))
							*out = data;
						else
							return false;

						break;
					}

					default:
						_ERROR("ExtractArgsEx: unknown integer type (%02X)", type);
						return false;
				}
			}
			break;

			case kParamType_Float:
			{
				float	* out = va_arg(args, float *);
				UInt8	type = *scriptData;
				switch(type)
				{
					case 0x7A: // "z"
						*out = *((double *)++scriptData);
						scriptData += sizeof(double);
						break;

					case 0x72: // "r"
					case 0x66: // "f"
					case 0x73: // "s"
					case 0x47: // "G"
					{
						double data = 0;
						if (ExtractFloat(data, scriptData, scriptObj, eventList))
							*out = data;
						else
							return false;
						
						break;
					}

					default:
						_ERROR("ExtractArgsEx: unknown float type (%02X)", type);
						return false;
				}
			}
			break;

			case kParamType_ObjectID:
			case kParamType_ObjectRef:
			case kParamType_Actor:
			case kParamType_SpellItem:
			case kParamType_Cell:
			case kParamType_MagicItem:
			case kParamType_Sound:
			case kParamType_Topic:
			case kParamType_Quest:
			case kParamType_Race:
			case kParamType_Class:
			case kParamType_Faction:
			case kParamType_Global:
			case kParamType_Furniture:
			case kParamType_TESObject:
			case kParamType_MapMarker:
			case kParamType_ActorBase:
			case kParamType_Container:
			case kParamType_WorldSpace:
			case kParamType_AIPackage:
			case kParamType_CombatStyle:
			case kParamType_MagicEffect:
			case kParamType_WeatherID:
			case kParamType_NPC:
			case kParamType_Owner:
			case kParamType_EffectShader:
			case kParamType_FormList:
			case kParamType_MenuIcon:
			case kParamType_Perk:
			case kParamType_Note:
			case kParamType_ImageSpaceModifier:
			case kParamType_ImageSpace:
			case kParamType_EncounterZone:
			case kParamType_Message:
			case kParamType_InvObjOrFormList:
			case kParamType_NonFormList:
			case kParamType_SoundFile:
			case kParamType_LeveledOrBaseChar:
			case kParamType_LeveledOrBaseCreature:
			case kParamType_LeveledChar:
			case kParamType_LeveledCreature:
			case kParamType_LeveledItem:
			case kParamType_AnyForm:
			case kParamType_Reputation:
			case kParamType_Casino:
			case kParamType_CasinoChip:
			case kParamType_Challenge:
			case kParamType_CaravanMoney:
			case kParamType_CaravanCard:
			case kParamType_CaravanDeck:
			case kParamType_Region:
			{
				TESForm	** out = va_arg(args, TESForm **);
				TESForm* form = ResolveForm(scriptData, scriptObj, eventList);
				if (!form)
					return false;

				*out = form;
			}
			break;

			case kParamType_ActorValue:
			case kParamType_AnimationGroup:
			case kParamType_Sex:
			case kParamType_CrimeType:
			case kParamType_MiscellaneousStat:
			case kParamType_Alignment:
			case kParamType_EquipType:
			case kParamType_CriticalStage:
			{
				UInt32	* out = va_arg(args, UInt32 *);

				*out = *((UInt16 *)scriptData);
				scriptData += 2;
			}
			break;

			case kParamType_Axis:
			case kParamType_FormType:
			{
				UInt32	* out = va_arg(args, UInt32 *);

				*out = *((UInt8 *)scriptData);
				scriptData += 1;
			}
			break;

			// known unhandled types
			case kParamType_VariableName:
				// fall through to error reporter

			default:
				_ERROR("Unhandled type encountered. Arg #%d numArgs = %d paramType = %d paramStr = %s",
					i, numArgs, info->typeID, info->typeStr);
				HALT("unhandled type");
				break;
		}
	}

	return true;
}

bool ExtractArgsRaw(ParamInfo * paramInfo, void * scriptDataIn, UInt32 * scriptDataOffset, Script * scriptObj, ScriptEventList * eventList, ...)
{
	va_list	args;
	va_start(args, eventList);

	UInt8	* scriptData = ((UInt8 *)scriptDataIn) + *scriptDataOffset;

	UInt32	numArgs = *((UInt16 *)scriptData);
	scriptData += sizeof(UInt16);

	for(UInt32 i = 0; i < numArgs; i++)
	{
		ParamInfo		* info = &paramInfo[i];
		ExtractedParam	* dst = va_arg(args, ExtractedParam *);

		dst->type = ExtractedParam::kType_Unknown;
		dst->isVar = false;

		switch(info->typeID)
		{
			case kParamType_String:
				dst->type = ExtractedParam::kType_String;
				dst->data.str.len = *((UInt16 *)scriptData);
				scriptData += sizeof(UInt16);
				dst->data.str.buf = (const char *)scriptData;
				scriptData += dst->data.str.len;
				break;

			case kParamType_ActorValue:
			case kParamType_AnimationGroup:
			case kParamType_Sex:
			case kParamType_CrimeType:
			case kParamType_MiscellaneousStat:
			case kParamType_Alignment:
			case kParamType_EquipType:
			case kParamType_CriticalStage:
				dst->type = ExtractedParam::kType_Imm16;
				dst->data.imm = *((UInt16 *)scriptData);
				scriptData += sizeof(UInt16);
				break;

			case kParamType_Axis:
			case kParamType_FormType:
				dst->type = ExtractedParam::kType_Imm8;
				dst->data.imm = *((UInt8 *)scriptData);
				scriptData += sizeof(UInt8);
				break;

			case kParamType_Integer:
			case kParamType_QuestStage:
			case kParamType_Float:
			{
				UInt8	type = *scriptData++;
				switch(type)
				{
					case 'n':	// 6E
						dst->type = ExtractedParam::kType_Imm32;
						dst->data.imm = *((UInt32 *)scriptData);
						scriptData += sizeof(UInt32);
						break;

					case 'z':	// 7A
						dst->type = ExtractedParam::kType_ImmDouble;
						dst->data.immDouble = (double *)scriptData;
						scriptData += sizeof(double);
						break;

					case 'f':	// 66
					case 'r':	// 72
					case 's':	// 73
					case 'G':	// 47
					{
						ScriptEventList	* srcEventList = eventList;
						UInt8			varType = *scriptData++;

						// remote reference?
						if(varType == 'r')
						{
							// swap the event list
							UInt16	varIdx = *((UInt16 *)scriptData);
							scriptData += 2;

							Script::RefVariable	* var = scriptObj->GetVariable(varIdx);
							if(var)
							{
								TESForm	* eventListSrc = var->form;
								switch(eventListSrc->typeID)
								{
									case kFormType_Reference:
										srcEventList = ((TESObjectREFR *)eventListSrc)->GetEventList();
										break;

									case kFormType_Quest:
										srcEventList = ((TESQuest *)eventListSrc)->scriptEventList;
										break;

									default:
										_ERROR("ExtractArgsRaw: unknown remote reference in number var (%02X)", eventListSrc->typeID);
										return false;
								}
							}
							else
							{
								_ERROR("ExtractArgsRaw: couldn't find remote reference in number var");
								return false;
							}
						}
						
						else
						{
							// ###

							//default:
								_ERROR("ExtractArgsRaw: unknown number var type (%02X)", type);
								return false;
						}
					}
					break;

					default:
						_ERROR("ExtractArgsRaw: unknown number type (%02X)", type);
						return false;
				}
			}
			break;

			case kParamType_ObjectID:
			case kParamType_ObjectRef:
			case kParamType_Actor:
			case kParamType_SpellItem:
			case kParamType_Cell:
			case kParamType_MagicItem:
			case kParamType_Sound:
			case kParamType_Topic:
			case kParamType_Quest:
			case kParamType_Race:
			case kParamType_Class:
			case kParamType_Faction:
			case kParamType_Global:
			case kParamType_Furniture:
			case kParamType_TESObject:
			case kParamType_MapMarker:
			case kParamType_ActorBase:
			case kParamType_Container:
			case kParamType_WorldSpace:
			case kParamType_AIPackage:
			case kParamType_CombatStyle:
			case kParamType_MagicEffect:
			case kParamType_WeatherID:
			case kParamType_NPC:
			case kParamType_Owner:
			case kParamType_EffectShader:
			case kParamType_FormList:
			case kParamType_MenuIcon:
			case kParamType_Perk:
			case kParamType_Note:
			case kParamType_ImageSpaceModifier:
			case kParamType_ImageSpace:
			case kParamType_EncounterZone:
			case kParamType_Message:
			case kParamType_InvObjOrFormList:
			case kParamType_NonFormList:
			case kParamType_SoundFile:
			case kParamType_LeveledOrBaseChar:
			case kParamType_LeveledOrBaseCreature:
			case kParamType_LeveledChar:
			case kParamType_LeveledCreature:
			case kParamType_LeveledItem:
			case kParamType_AnyForm:
			case kParamType_Reputation:
			case kParamType_Casino:
			case kParamType_CasinoChip:
			case kParamType_Challenge:
			case kParamType_CaravanMoney:
			case kParamType_CaravanCard:
			case kParamType_CaravanDeck:
			case kParamType_Region:
			{
				UInt8	type = *scriptData++;
				switch(type)
				{
					case 'r':	// constant
					{
						UInt16	varIdx = *((UInt16 *)scriptData);
						scriptData += 2;

						Script::RefVariable	* var = scriptObj->GetVariable(varIdx);
						ASSERT(var);

						var->Resolve(eventList);

						dst->type = ExtractedParam::kType_Form;
						dst->data.form = var->form;
					}
					break;

					case 'f':	// variable
					{
						UInt16	varIdx = *((UInt16 *)scriptData);
						scriptData += 2;

						dst->type = ExtractedParam::kType_Form;
						dst->isVar = true;
						dst->data.var.var = eventList->GetVariable(varIdx);
						dst->data.var.parent = eventList;
					}
					break;

					default:
						_ERROR("ExtractArgsRaw: unknown form type (%02X)", type);
						return false;
				}
			}
			break;

			case kParamType_VariableName:
				// unhandled, fall through

			default:
				_ERROR("ExtractArgsRaw: unhandled type encountered, arg %d type %02X", i, info->typeID);
				HALT("unhandled type");
				break;
		}
	}

	return true;
}

bool ExtractArgsEx(ParamInfo * paramInfo, void * scriptDataIn, UInt32 * scriptDataOffset, Script * scriptObj, ScriptEventList * eventList, ...)
{
	va_list	args;
	va_start(args, eventList);

	UInt8	* scriptData = ((UInt8 *)scriptDataIn) + *scriptDataOffset;
	UInt32	numArgs = *((UInt16 *)scriptData);
	scriptData += 2;

	//DEBUG_MESSAGE("scriptData:%08x numArgs:%d paramInfo:%08x scriptObj:%08x eventList:%08x", scriptData, numArgs, paramInfo, scriptObj, eventList);

	bool bExtracted = v_ExtractArgsEx(numArgs, paramInfo, scriptData, scriptObj, eventList, args);
	va_end(args);
	return bExtracted;
}

void ScriptEventList::Dump(void)
{
	UInt32 nEvents = m_eventList->Count();

	for(SInt32 n = 0; n < nEvents; ++n)
	{
		Event* pEvent = m_eventList->GetNthItem(n);
		if(pEvent)
		{
			Console_Print("%08X (%s) %08X", pEvent->object, GetObjectClassName(pEvent->object), pEvent->eventMask);
		}
	}
}

UInt32 ScriptEventList::ResetAllVariables()
{
	UInt32 numVars = 0;
	for (VarEntry * entry = m_vars; entry; entry = entry->next)
		if (entry->var)
		{
			entry->var->data = 0.0;
			numVars++;
		}

	return numVars;
}

ScriptEventList::Var * ScriptEventList::GetVariable(UInt32 id)
{
	for(VarEntry * entry = m_vars; entry; entry = entry->next)
		if(entry->var && entry->var->id == id)
			return entry->var;

	return NULL;
}

ScriptEventList* EventListFromForm(TESForm* form)
{
	ScriptEventList* eventList = NULL;
	TESObjectREFR* refr = DYNAMIC_CAST(form, TESForm, TESObjectREFR);
	if (refr)
		eventList = refr->GetEventList();
	else
	{
		TESQuest* quest = DYNAMIC_CAST(form, TESForm, TESQuest);
		if (quest)
			eventList = quest->scriptEventList;
	}

	return eventList;
}

static void ConvertLiteralPercents(std::string* str)
{
	UInt32 idx = 0;
	while ((idx = str->find('%', idx)) != -1)
	{
		str->insert(idx, "%");
		idx += 2;
	}
}

static void SkipArgs(UInt8* &scriptData)
{
	switch (*scriptData)
	{
	case 'z':
		scriptData += sizeof(double) + 1;
		break;
	case 'r':
		scriptData += 6;
		break;
	default:
		scriptData += 3;
		break;
	}
}

static void OmitFormatStringArgs(std::string str, FormatStringArgs& args)
{
	//skip any args omitted by the %{ specifier
	UInt32 strIdx = 0;
	while ((strIdx = str.find('%', strIdx)) != -1 && args.HasMoreArgs())
	{
		switch(str[++strIdx])
		{
		case '%':
		case 'q':
		case 'Q':
		case 'r':
		case 'R':
			break;
		case 'c':
		case 'C':
			args.SkipArgs(2);
			break;
		default:
			args.SkipArgs(1);
		}
		strIdx++;
	}
}

//static bool ExtractFormattedString(UInt32 &numArgs, char* buffer, UInt8* &scriptData, Script* scriptObj, ScriptEventList* eventList)
bool ExtractFormattedString(FormatStringArgs& args, char* buffer)
{
	//extracts args based on format string, prints formatted string to buffer
	static const int maxArgs = 20;
	double f[maxArgs] = {0.0};
	UInt32 argIdx = 0;

	std::string fmtString = args.GetFormatString();
	UInt32 strIdx = 0;

	//extract args
	while ((strIdx = fmtString.find('%', strIdx)) != -1)
	{
		char argType = fmtString.at(strIdx+1);
		switch (argType)
		{
		case '%':										//literal %
			strIdx += 2;
			break;
					case 'z':
					case 'Z':										//string variable
						{
							fmtString.erase(strIdx, 2);
							double strID = 0;
							if (!args.Arg(args.kArgType_Float, &strID))
								return false;
			
							const char* toInsert = StringFromStringVar(strID);
			//#if NVSE_CORE
			//				StringVar* insStr = NULL;
			//				insStr = g_StringMap.Get(strID);
			//
			//				if (insStr)
			//					toInsert = insStr->GetCString();
			//#else			// called from a plugin command
			//				if (s_StringVarInterface)
			//					toInsert = s_StringVarInterface->GetString(strID);
			//#endif
							if (toInsert && toInsert[0])
								fmtString.insert(strIdx, toInsert);
							else
								fmtString.insert(strIdx, "NULL");
						}
						break;
		case 'r':										//newline
		case 'R':
			fmtString.erase(strIdx, 2);
			fmtString.insert(strIdx, "\n");
			break;
		case 'e':
		case 'E':										//workaround for CS not accepting empty strings
			fmtString.erase(strIdx, 2);
			break;
		case 'a':
		case 'A':										//character specified by ASCII code
			{
				fmtString.erase(strIdx, 2);
				double fCharCode = 0;
				if (args.Arg(args.kArgType_Float, &fCharCode))
					fmtString.insert(strIdx, 1, (char)fCharCode);
				else
					return false;
			}
			break;
		case 'n':										// name of obj/ref
		case 'N':
			{
				fmtString.erase(strIdx, 2);
				TESForm* form = NULL;
				if (!args.Arg(args.kArgType_Form, &form))
					return false;

				std::string strName(GetFullName(form));
				ConvertLiteralPercents(&strName);
				fmtString.insert(strIdx, strName);
				strIdx += strName.length();
			}
			break;
		case 'i':											//formID
		case 'I':
			{
				fmtString.erase(strIdx, 2);
				TESForm* form = NULL;
				if (!(args.Arg(args.kArgType_Form, &form)))
					return false;
				else if (!form)
					fmtString.insert(strIdx, "00000000");
				else
				{			
					char formID[9];
					sprintf_s(formID, 9, "%08X", form->refID);
					fmtString.insert(strIdx, formID);
				}
			}
			break;
		case 'c':											//named component of another object
		case 'C':											//2 args - object and index
			{
				TESForm* form = NULL;
				if (!args.Arg(args.kArgType_Form, &form))
					return false;

				fmtString.erase(strIdx, 2);
				if (!form)
					fmtString.insert(strIdx, "NULL");
				else
				{
					double objIdx = 0;

					if (!args.Arg(args.kArgType_Float, &objIdx))
						return false;
					else
					{
						std::string strName("");

						switch(form->typeID)
						{
#if 0
							case kFormType_Spell:
							case kFormType_Enchantment:
							case kFormType_Ingredient:
							case kFormType_AlchemyItem:
							{
								MagicItem* magItm = DYNAMIC_CAST(form, TESForm, MagicItem);
								if (!magItm)
									strName = "NULL";
								else
								{
									strName = magItm->list.GetNthEIName(objIdx);
									EffectItem* effItem = magItm->list.ItemAt(objIdx);
									if (effItem && effItem->HasActorValue())
									{
										UInt32 valIdx = strName.find(' ');
										if (valIdx != -1)
										{
											strName.erase(valIdx + 1, strName.length() - valIdx);
											strName.insert(valIdx + 1, std::string(GetActorValueString(effItem->actorValueOrOther)));
										}
									}
								}
							}
							break;
#endif

							case kFormType_Ammo:
							{
								TESAmmo	* ammo = DYNAMIC_CAST(form, TESForm, TESAmmo);

								if(!ammo)
									strName = "NULL";	// something is very wrong
								else switch((int)objIdx)
								{
									default:
									case 0:	// full name
										strName = GetFullName(ammo);
										break;

									case 1:	// short name
										strName = ammo->shortName.CStr();
										break;

									case 2:	// abbrev
										strName = ammo->abbreviation.CStr();
										break;
								}
							}
							break;

#if 1	// to be tested
							case kFormType_Faction:
							{
								TESFaction* fact = DYNAMIC_CAST(form, TESForm, TESFaction);
								if (!fact)
									strName = "NULL";
								else
								{
									strName = fact->GetNthRankName(objIdx);
								}
							}
							break;
#endif

							default:
								strName = "unknown";
								break;
						}

						ConvertLiteralPercents(&strName);

						fmtString.insert(strIdx, strName);
						strIdx += strName.length();
					}
				}
			}
			break;
		case 'k':
		case 'K':											//DX code
			{
				double keycode = 0;
				fmtString.erase(strIdx, 2);
				if (!args.Arg(args.kArgType_Float, &keycode))
					return false;

				const char* desc = GetDXDescription(keycode);
				fmtString.insert(strIdx, desc);

			}
			break;
		case 'v':
		case 'V':											//actor value
			{
				double actorVal = eActorVal_FalloutMax;
				fmtString.erase(strIdx, 2);
				if (!args.Arg(args.kArgType_Float, &actorVal))
					return false;

				std::string valStr(GetActorValueString(actorVal));
				if (valStr.length())
				{
					for (UInt32 idx = 1; idx < valStr.length(); idx++)
						if (isupper(valStr[idx]))
						{								//insert spaces to make names more presentable
							valStr.insert(idx, " ");
							idx += 2;
						}
				}
				fmtString.insert(strIdx, valStr);
			}
			break;
		case 'p':
		case 'P':											//pronouns
			{
				fmtString.erase(strIdx, 2);
				char pronounType = fmtString[strIdx];
				fmtString.erase(strIdx, 1);
				TESForm* form = NULL;
				if (!args.Arg(args.kArgType_Form, &form))
					return false;

				if (!form)
					fmtString.insert(strIdx, "NULL");
				else
				{			
					TESObjectREFR* refr = DYNAMIC_CAST(form, TESForm, TESObjectREFR);
					if (refr)
						form = refr->baseForm;

					short objType = 0;
					if (form->typeID == kFormType_NPC)
					{
						TESActorBaseData* actorBase = DYNAMIC_CAST(form, TESForm, TESActorBaseData);
						objType = (actorBase->IsFemale()) ? 2 : 1;
					}

					switch (pronounType)
					{
					case 'o':
					case 'O':
						if (objType == 1)
							fmtString.insert(strIdx, "him");
						else if (objType == 2)
							fmtString.insert(strIdx, "her");
						else
							fmtString.insert(strIdx, "it");
						break;
					case 's':
					case 'S':
						if (objType == 1)
							fmtString.insert(strIdx, "he");
						else if (objType == 2)
							fmtString.insert(strIdx, "she");
						else
							fmtString.insert(strIdx, "it");
						break;
					case 'p':
					case 'P':
						if (objType == 1)
							fmtString.insert(strIdx, "his");
						else if (objType == 2)
							fmtString.insert(strIdx, "her");
						else
							fmtString.insert(strIdx, "its");
						break;
					default:
						fmtString.insert(strIdx, "NULL");
					}
				}
			}
			break;
		case 'q':
		case 'Q':											//double quote
			fmtString.erase(strIdx, 2);
			fmtString.insert(strIdx, "\"");
			break;
		case '{':											//omit portion of string based on flag param
			{
				fmtString.erase(strIdx, 2);
				double flag = 0;
				if (!args.Arg(args.kArgType_Float, &flag))
					return false;

				UInt32 omitEnd = fmtString.find("%}", strIdx);
				if (omitEnd == -1)
					omitEnd = fmtString.length();

				if (!flag)
				{
					OmitFormatStringArgs(fmtString.substr(strIdx, omitEnd - strIdx), args);
					fmtString.erase(strIdx, omitEnd - strIdx + 2);
				}
				else
					fmtString.erase(omitEnd, 2);
			}
			break;
		case '}':											//in case someone left a stray closing bracket
			fmtString.erase(strIdx, 2);
			break;
		case 'x':											//hex
		case 'X':
			{
				double data = 0;
				if (!args.Arg(args.kArgType_Float, &data))
					return false;

				UInt64* hexArg = (UInt64*)(&f[argIdx++]);
				*hexArg = data;
				fmtString.erase(strIdx, 2);
				char width = 0;
				if (strIdx < fmtString.length())
				{
					if (isdigit(fmtString[strIdx]))	//single-digit width specifier optionally follows %x
					{
						width = fmtString[strIdx];
						fmtString.erase(strIdx, 1);
					}
				}
				fmtString.insert(strIdx, "%0llX");
				if (width)
					fmtString.insert(strIdx + 2, 1, width);
				strIdx++;
			}
			break;
		default:											//float
			{
				double data = 0;
				if (!args.Arg(args.kArgType_Float, &data))
					return false;

				f[argIdx++] = data;
				strIdx++;
			}
			break;
		}
	}

	if (sprintf_s(buffer, kMaxMessageLength - 2, fmtString.c_str(), f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8], f[9], f[10], f[11], f[12], f[13], f[14], f[15], f[16], f[17], f[18], f[19]) > 0)
	{
		buffer[kMaxMessageLength-1] = '\0';
		return true;
	}
	else if (fmtString.length() == 0)
	{
		buffer[0] = '\0';
		return true;
	}
	else
		return false;
}

void RegisterStringVarInterface(NVSEStringVarInterface* intfc)
{
	s_StringVarInterface = intfc;
}

#if NVSE_CORE

//fmtStringPos is index of fmtString param in paramInfo, with first param = 0
bool ExtractFormatStringArgs(UInt32 fmtStringPos, char* buffer, ParamInfo * paramInfo, void * scriptDataIn, UInt32 * scriptDataOffset, Script * scriptObj, ScriptEventList * eventList, UInt32 maxParams, ...)
{
	va_list args;
	va_start(args, maxParams);

	UInt8	* scriptData = ((UInt8 *)scriptDataIn) + *scriptDataOffset;
	SInt16	numArgs = *((SInt16 *)scriptData);
	scriptData += 2;

	if (numArgs < 0) {
		UInt32 offsetPtr = 0;
		bool bResult = ExtractArgsOverride::ExtractFormattedString(paramInfo, scriptData, scriptObj, eventList, &offsetPtr, args,
			fmtStringPos, buffer, maxParams);
		if (!bResult) {
			DEBUG_PRINT("ExtractFormatStringArgs returns false");
		}
		return bResult;
	}

	numArgs -= fmtStringPos + 1;

	bool bExtracted = false;
	if (fmtStringPos > 0)
	{
		bExtracted = v_ExtractArgsEx(fmtStringPos, paramInfo, scriptData, scriptObj, eventList, args);
		if (!bExtracted)
			return false;
	}

	ScriptFormatStringArgs scriptArgs(numArgs, scriptData, scriptObj, eventList);
	bExtracted = ExtractFormattedString(scriptArgs, buffer);

	numArgs = scriptArgs.GetNumArgs();
	scriptData = scriptArgs.GetScriptData();
	//NOTE: if v_ExtractArgsEx was called above, passing args again in second call below = undefined behavior. Needs fixing.
	if (bExtracted && numArgs > 0)			//some optional normal params following format string params
	{
		if ((numArgs + fmtStringPos + 21) > maxParams)		//scripter included too many optional params - adjust to prevent crash
			numArgs = (maxParams - fmtStringPos - 21);

		bExtracted = v_ExtractArgsEx(numArgs, &(paramInfo[fmtStringPos + 21]), scriptData, scriptObj, eventList, args);
	}

	va_end(args);
	return bExtracted;
}

#endif

bool ExtractSetStatementVar(Script* script, ScriptEventList* eventList, void* scriptDataIn, double * outVarData, UInt8* outModIndex, bool shortPath)
{
	/*	DOES NOT WORK WITH FalloutNV, we are going to abuse the stack instead:
	//when script command called as righthand side of a set statement, the script data containing the variable
	//to assign to remains on the stack as arg to a previous function. We can get to it through scriptData in COMMAND_ARGS
	*/
	UInt8* dataStart = (UInt8*)scriptDataIn;	// should be 0x58 (or 0x72 if called with dot syntax)

	if (!((*dataStart == 0x58 || *dataStart == 0x72))) {
		return false;
	}

	// Calculate frame pointer for 4 calls above:
	/*void* callerFramePointer;
	_asm {
		mov callerFramePointer, ebp
	}
	for (int i = 0; i < 3; i++)
		callerFramePointer = (void*)(*(UInt32*)callerFramePointer);
	if (!shortPath) {
		callerFramePointer = (void*)(*(UInt32*)callerFramePointer);	// sv_Destruct calls us directly, others goes through AssignToStringVar
		callerFramePointer = (void*)(*(UInt32*)callerFramePointer);	// one more added for when multiple commands are grouped (like GetBipedModelPath)
	}

	UInt32 scriptDataPtrAddr = (UInt32)(callerFramePointer) + 0x08;
	UInt32* scriptDataAddr = (UInt32*)scriptDataPtrAddr;
	UInt8* scriptData = (UInt8*)(*scriptDataAddr);

	SInt32 scriptDataOffset = (UInt32)scriptData - (UInt32)(script->data);*/
	auto scriptData = reinterpret_cast<UInt8*>(g_lastScriptData);
	SInt32 scriptDataOffset = (UInt32)scriptData - (UInt32)(script->data);

	if (scriptDataOffset < 5)
		return false;

	bool bExtracted = false;
	scriptData -= 5;

	switch (*scriptData)			//work backwards from opcode to find lefthand var
	{
	case 'G':						//global
		{
			UInt16 refIdx = *(UInt16*)(scriptData + 1);
			Script::RefVariable* refVar = script->GetVariable(refIdx);
			if (!refVar)
				break;

			TESGlobal* globalVar = DYNAMIC_CAST(refVar->form, TESForm, TESGlobal);
			if (globalVar)
			{
				*outVarData = globalVar->data;
				if (outModIndex)
					*outModIndex = (globalVar->refID >> 24);
				bExtracted = true;
			}
		}
		break;
	case 'l':
	case 'f':
	case 's':
		{
			if (scriptDataOffset >= 8 && *(scriptData - 3) == 'r')	//external var
			{
				UInt16 refIdx = *(UInt16*)(scriptData - 2);
				Script::RefVariable* refVar = script->GetVariable(refIdx);
				if (!refVar)
					break;

				refVar->Resolve(eventList);
				TESForm* refForm = refVar->form;
				if (!refForm)
					break;

				if (refForm->typeID == kFormType_Reference)
				{
					TESObjectREFR* refr = DYNAMIC_CAST(refForm, TESForm, TESObjectREFR);
					TESScriptableForm* scriptable = DYNAMIC_CAST(refr->baseForm, TESForm, TESScriptableForm);
					if (scriptable)
					{
						script = scriptable->script;
						eventList = refr->GetEventList();
					}
					else
						break;
				}
				else if (refForm->typeID == kFormType_Quest)
				{
					TESScriptableForm* scriptable = DYNAMIC_CAST(refForm, TESForm, TESScriptableForm);
					if (scriptable)
					{
						script = scriptable->script;
						TESQuest* quest = DYNAMIC_CAST(scriptable, TESScriptableForm, TESQuest);
						eventList = quest->scriptEventList;
					}
					else
						break;
				}
				else
					break;
			}

			UInt16 varIdx = *(UInt16*)(scriptData + 1);
			ScriptEventList::Var* var = eventList->GetVariable(varIdx);
			if (var)
			{
				*outVarData = var->data;
				if (outModIndex)
					*outModIndex = (script->refID >> 24);
				bExtracted = true;
			}
		}
		break;
	default:
		SCRIPT_ASSERT(false, script, "Function must be used within a Set statement");
	}

	return bExtracted;
}

// g_baseActorValueNames is only filled in after oblivion's global initializers run
const char* GetActorValueString(UInt32 actorValue)
{
	char* name = 0;
	if (actorValue <= eActorVal_FalloutMax)
		name = GetActorValueName(actorValue);
	if (!name)
		name = "unknown";

	return name;
}

UInt32 GetActorValueForScript(const char* avStr) 
{
	for (UInt32 i = 0; i <= eActorVal_FalloutMax; i++) {
		char* name = GetActorValueName(i);
		if (_stricmp(avStr, name) == 0)
			return i;
	}

	return eActorVal_NoActorValue;
}

UInt32 GetActorValueForString(const char* strActorVal, bool bForScript)
{
	if (bForScript)
		return GetActorValueForScript(strActorVal);

	for (UInt32 n = 0; n <= eActorVal_FalloutMax; n++) {
		char* name = GetActorValueName(n);
		if (_stricmp(strActorVal, name) == 0)
			return n;
	}
	return eActorVal_NoActorValue;
}

ScriptFormatStringArgs::ScriptFormatStringArgs(UInt32 _numArgs, UInt8* _scriptData, Script* _scriptObj, ScriptEventList* _eventList)
: numArgs(_numArgs), scriptData(_scriptData), scriptObj(_scriptObj), eventList(_eventList)
{
	//extract format string
	UInt16 len = *((UInt16*)scriptData);
	char* szFmt = new char[len+1];
	scriptData += 2;
	memcpy(szFmt, scriptData, len);
	szFmt[len] = '\0';

	scriptData += len;
	fmtString = std::string(std::string(ResolveStringArgument(eventList, szFmt)));
	delete szFmt;
}

std::string ScriptFormatStringArgs::GetFormatString()
{
	return fmtString;
}

bool ScriptFormatStringArgs::HasMoreArgs()
{
	return (numArgs > 0);
}

UInt32 ScriptFormatStringArgs::GetNumArgs()
{
	return numArgs;
}

UInt8* ScriptFormatStringArgs::GetScriptData()
{
	return scriptData;
}

bool ScriptFormatStringArgs::SkipArgs(UInt32 numToSkip)
{
	while (numToSkip--)
	{
		switch (*scriptData)
		{
		case 'z':
			scriptData += sizeof(double) + 1;
			break;
		case 'r':
			scriptData += 6;
			break;
		default:
			scriptData += 3;
			break;
		}

		numArgs--;
	}

	return true;
}

//Log error if expression evaluates to false
bool SCRIPT_ASSERT(bool expr, Script* script, const char * errorMsg, ...)
{
	//	static bool bAlerted = false;			//only alert user on first error
	//	static std::set<UInt32> naughtyScripts;	//one error per script to avoid thrashing
	//
	//	if (!expr && naughtyScripts.find(script->refID) == naughtyScripts.end())
	//	{
	//		const ModEntry ** activeMods = (*g_dataHandler)->GetActiveModList();
	//		UInt8 modIndex = script->GetModIndex();
	//		const ModEntry * modEntry = activeMods[modIndex];
	//
	//		const char * modName;
	//		if (modIndex != 0xFF && modEntry && modEntry->data && modEntry->data->name)
	//			modName = modEntry->data->name;
	//		else
	//			modName = "Unknown";
	//
	////		sprintf_s(errorHeader, sizeof(errorHeader) - 1, "** Error: Script %08X in file \"%s\" **", script->refID, modName);
	////		_MESSAGE("%s", errorHeader);
	//		_MESSAGE("** Script Error: Script %08x in file \"%s\" **", script->refID, modName);
	//
	//		va_list args;
	//		va_start(args, errorMsg);
	//
	//		char errorBuf[512];
	//		vsprintf_s(errorBuf, sizeof(errorBuf) - 1, errorMsg, args);
	//		va_end(args);
	//
	//		gLog.Indent();
	//		_MESSAGE("%s", errorBuf);
	//		gLog.Outdent();
	//
	//		if (!bAlerted)
	//		{
	//			MessageBoxAlert("NVSE has detected a script error. \n\nPlease check nvse.log for details.");
	//			bAlerted = true;
	//		}
	//
	//		naughtyScripts.insert(script->refID);
	//	}
	return expr;
}

bool ScriptFormatStringArgs::Arg(FormatStringArgs::argType asType, void * outResult)
{
	if (!SCRIPT_ASSERT((numArgs > 0), scriptObj, "Too few args for format specifier"))
		return false;

	numArgs--;

	switch (asType)
	{
	case kArgType_Float:
		{
			double data = 0;
			if (ExtractFloat(data, scriptData, scriptObj, eventList))
			{
				*((double*)outResult) = data;
				return true;
			}
		}
		break;
	case kArgType_Form:
		{
			TESForm* form = ExtractFormFromFloat(scriptData, scriptObj, eventList);
			*((TESForm**)outResult) = form;
			return true;
		}
	}

	return false;
}

void ShowCompilerError(ScriptLineBuffer* lineBuf, const char * fmt, ...)
{

	char errorHeader[0x400];
	UInt32 offset = sprintf_s(errorHeader, 0x400, "Error on line %d\n\n", lineBuf->lineNumber);

	va_list	args;
	va_start(args, fmt);

	char	errorMsg[0x200];
	vsprintf_s(errorMsg, 0x200, fmt, args);

	strcat_s(errorHeader, 0x400, errorMsg);
	Console_Print(errorHeader);
	_MESSAGE(errorHeader);

	va_end(args);
}

UInt32 GetActorValueMax(UInt32 actorValueCode) {
	switch (actorValueCode ) {
		case eActorVal_Aggression:			return   3; break;
		case eActorVal_Confidence:			return   4; break;
		case eActorVal_Energy:				return 100; break;
		case eActorVal_Responsibility:		return 100; break;
		case eActorVal_Mood:				return   8; break;

		case eActorVal_Strength:			return  10; break;
		case eActorVal_Perception:			return  10; break;
		case eActorVal_Endurance:			return  10; break;
		case eActorVal_Charisma:			return  10; break;
		case eActorVal_Intelligence:		return  10; break;
		case eActorVal_Agility:				return  10; break;
		case eActorVal_Luck:				return  10; break;

		case eActorVal_ActionPoints:		return   1; break;
		case eActorVal_CarryWeight:			return   1; break;
		case eActorVal_CritChance:			return 100; break;
		case eActorVal_HealRate:			return   1; break;
		case eActorVal_Health:				return   1; break;
		case eActorVal_MeleeDamage:			return   1; break;
		case eActorVal_DamageResistance:	return   1; break;
		case eActorVal_PoisonResistance:	return   1; break;
		case eActorVal_RadResistance:		return   1; break;
		case eActorVal_SpeedMultiplier:		return   1; break;
		case eActorVal_Fatigue:				return   1; break;
		case eActorVal_Karma:				return   1; break;
		case eActorVal_XP:					return   1; break;

		case eActorVal_Head:				return 100; break;
		case eActorVal_Torso:				return 100; break;
		case eActorVal_LeftArm:				return 100; break;
		case eActorVal_RightArm:			return 100; break;
		case eActorVal_LeftLeg:				return 100; break;
		case eActorVal_RightLeg:			return 100; break;
		case eActorVal_Brain:				return 100; break;

		case eActorVal_Barter:				return 100; break;
		case eActorVal_BigGuns:				return 100; break;
		case eActorVal_EnergyWeapons:		return 100; break;
		case eActorVal_Explosives:			return 100; break;
		case eActorVal_Lockpick:			return 100; break;
		case eActorVal_Medicine:			return 100; break;
		case eActorVal_MeleeWeapons:		return 100; break;
		case eActorVal_Repair:				return 100; break;
		case eActorVal_Science:				return 100; break;
		case eActorVal_Guns:				return 100; break;
		case eActorVal_Sneak:				return 100; break;
		case eActorVal_Speech:				return 100; break;
		case eActorVal_Survival:			return 100; break;
		case eActorVal_Unarmed:				return 100; break;

		case eActorVal_InventoryWeight:		return   1; break;
		case eActorVal_Paralysis:			return   1; break;
		case eActorVal_Invisibility:		return   1; break;
		case eActorVal_Chameleon:			return   1; break;
		case eActorVal_NightEye:			return   1; break;
		case eActorVal_Turbo:				return   1; break;
		case eActorVal_FireResistance:		return   1; break;
		case eActorVal_WaterBreathing:		return   1; break;
		case eActorVal_RadLevel:			return   1; break;
		case eActorVal_BloodyMess:			return   1; break;
		case eActorVal_UnarmedDamage:		return   1; break;
		case eActorVal_Assistance:			return   2; break;

		case eActorVal_ElectricResistance:	return   1; break;

		case eActorVal_EnergyResistance:	return   1; break;
		case eActorVal_EMPResistance:		return   1; break;
		case eActorVal_Var1Medical:			return   1; break;
		case eActorVal_Var2:				return   1; break;
		case eActorVal_Var3:				return   1; break;
		case eActorVal_Var4:				return   1; break;
		case eActorVal_Var5:				return   1; break;
		case eActorVal_Var6:				return   1; break;
		case eActorVal_Var7:				return   1; break;
		case eActorVal_Var8:				return   1; break;
		case eActorVal_Var9:				return   1; break;
		case eActorVal_Var10:				return   1; break;

		case eActorVal_IgnoreCrippledLimbs:	return   1; break;
		case eActorVal_Dehydration:			return   1; break;
		case eActorVal_Hunger:				return   1; break;
		case eActorVal_Sleepdeprevation:	return   1; break;
		case eActorVal_Damagethreshold:		return   1; break;
		default: return 1;
	}
}

typedef FontManager* (* _FontManager_GetSingleton)(void);
const _FontManager_GetSingleton FontManager_GetSingleton = (_FontManager_GetSingleton)0x011F33F8;

FontManager* FontManager::GetSingleton()
{
	return FontManager_GetSingleton();
}

FontManager::FontInfo* FontManager::FontInfo::Load(const char* path, UInt32 ID)
{
	FontInfo* info = (FontInfo*)FormHeap_Allocate(sizeof(FontInfo));
#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
	return (FontManager::FontInfo*)ThisStdCall(0x00A12020, info, ID, path, 1);
#elif RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525ng
	return (FontManager::FontInfo*)ThisStdCall(0x00A11BE0, info, ID, path, 1);
#else
#error unsupported Runtime version
#endif
}

bool FontManager::FontInfo::GetName(char* out)
{
	UInt32 len = strlen(path);
	len -= 4;					// '.fnt'
	UInt32 start = len;
	while (path[start-1] != '\\') {
		start--;
	}

	len -= start;

	memcpy(out, path+start, len);
	out[len] = 0;

	return true;
}

void Debug_DumpFontNames(void)
{
	FontManager::FontInfo** fonts = FontManager::GetSingleton()->fontInfos;

	for(UInt32 i = 0; i < FontArraySize; i++)
		_MESSAGE("Font %d is named %s", i+1, fonts[i]->path);

}

#endif
