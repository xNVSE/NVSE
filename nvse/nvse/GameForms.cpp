#include "GameForms.h"

#include "GameAPI.h"
#include "GameRTTI.h"
#include "GameObjects.h"
#include "GameData.h"

#if RUNTIME
static const ActorValueInfo** ActorValueInfoPointerArray = (const ActorValueInfo**)0x0011D61C8;		// See GetActorValueInfo
static const _GetActorValueInfo GetActorValueInfo = (_GetActorValueInfo)0x00066E920;	// See GetActorValueName
BGSDefaultObjectManager ** g_defaultObjectManager = (BGSDefaultObjectManager**)0x011CA80C;
#endif

TESForm * TESForm::TryGetREFRParent(void)
{
	TESForm			* result = this;
	if(result) {
		TESObjectREFR	* refr = DYNAMIC_CAST(this, TESForm, TESObjectREFR);
		if(refr && refr->baseForm)
			result = refr->baseForm;
	}
	return result;
}

UInt8 TESForm::GetModIndex() const
{
	return (refID >> 24);
}

TESFullName* TESForm::GetFullName() const
{
	if (typeID == kFormType_TESObjectCELL)		// some exterior cells inherit name of parent worldspace
	{
		TESObjectCELL *cell = (TESObjectCELL*)this;
		TESFullName *fullName = &cell->fullName;
		if ((!fullName->name.m_data || !fullName->name.m_dataLen) && cell->worldSpace)
			return &cell->worldSpace->fullName;
		return fullName;
	}
	const TESForm *baseForm = GetIsReference() ? ((TESObjectREFR*)this)->baseForm : this;
	return DYNAMIC_CAST(baseForm, TESForm, TESFullName);
}

const char* TESForm::GetTheName()
{
	TESFullName* fullName = GetFullName();
	if (fullName)
		return fullName->name.CStr();
	return "";
}

void TESForm::DoAddForm(TESForm* newForm, bool persist, bool record) const
{
	CALL_MEMBER_FN(DataHandler::Get(), DoAddForm)(newForm);

	if(persist)
	{
		// Only some forms can be safely saved as SaveForm. ie TESPackage at the moment.
		bool canSave = false;
		TESPackage* package = DYNAMIC_CAST(newForm, TESForm, TESPackage);
		if (package)
			canSave = true;
		// ... more ?

		if (canSave)
			CALL_MEMBER_FN(TESSaveLoadGame::Get(), AddCreatedForm)(newForm);
	}
}

TESForm * TESForm::CloneForm(bool persist) const
{
	TESForm	* result = CreateFormInstance(typeID);
	if(result)
	{
		result->CopyFrom(this);
		// it looks like some fields are not copied, case in point: TESObjectCONT does not copy BoundObject information.
		TESBoundObject* boundObject = DYNAMIC_CAST(result, TESForm, TESBoundObject);
		if (boundObject)
		{
			TESBoundObject* boundSource = DYNAMIC_CAST(this, TESForm, TESBoundObject);
			if (boundSource)
			{
				for (UInt8 i=0; i<6; i++)
					boundObject->bounds[i] = boundSource->bounds[i];
			}
		}
		DoAddForm(result, persist);
	}

	return result;
}

bool TESForm::IsCloned() const
{
	return GetModIndex() == 0xff;
}

std::string TESForm::GetStringRepresentation() const
{
	return FormatString(R"([id: %X, edid: "%s", name: "%s"])", refID, GetName(), GetFullName() ? GetFullName()->name.CStr() : "<no name>");
}

// static
UInt32 TESBipedModelForm::MaskForSlot(UInt32 slot)
{
	switch(slot) {
		case ePart_Head:		return eSlot_Head;
		case ePart_Hair:		return eSlot_Hair;
		case ePart_UpperBody:	return eSlot_UpperBody;
		case ePart_LeftHand:	return eSlot_LeftHand;
		case ePart_RightHand:	return eSlot_RightHand;
		case ePart_Weapon:		return eSlot_Weapon;
		case ePart_PipBoy:		return eSlot_PipBoy;
		case ePart_Backpack:	return eSlot_Backpack;
		case ePart_Necklace:	return eSlot_Necklace;
		case ePart_Headband:	return eSlot_Headband;
		case ePart_Hat:			return eSlot_Hat;
		case ePart_Eyeglasses:	return eSlot_Eyeglasses;
		case ePart_Nosering:	return eSlot_Nosering;
		case ePart_Earrings:	return eSlot_Earrings;
		case ePart_Mask:		return eSlot_Mask;
		case ePart_Choker:		return eSlot_Choker;
		case ePart_MouthObject:	return eSlot_MouthObject;
		case ePart_BodyAddon1:	return eSlot_BodyAddon1;
		case ePart_BodyAddon2:	return eSlot_BodyAddon2;
		case ePart_BodyAddon3:	return eSlot_BodyAddon3;
		default:				return -1;
	}
}

UInt32 TESBipedModelForm::GetSlotsMask() const {
	return partMask;
}

void TESBipedModelForm::SetSlotsMask(UInt32 mask)
{
	partMask = (mask & ePartBitMask_Full);
}

UInt32 TESBipedModelForm::GetBipedMask() const {
	return bipedFlags & 0xFF;
}

void TESBipedModelForm::SetBipedMask(UInt32 mask)
{
	bipedFlags = mask & 0xFF;
}

void  TESBipedModelForm::SetPath(const char* newPath, UInt32 whichPath, bool bFemalePath)
{
	String* toSet = NULL;

	switch (whichPath)
	{
	case ePath_Biped:
		toSet = &bipedModel[bFemalePath ? 1 : 0].nifPath;
		break;
	case ePath_Ground:
		toSet = &groundModel[bFemalePath ? 1 : 0].nifPath;
		break;
	case ePath_Icon:
		toSet = &icon[bFemalePath ? 1 : 0].ddsPath;
		break;
	}

	if (toSet)
		toSet->Set(newPath);
}

const char* TESBipedModelForm::GetPath(UInt32 whichPath, bool bFemalePath)
{
	String* pathStr = NULL;

	switch (whichPath)
	{
	case ePath_Biped:
		pathStr = &bipedModel[bFemalePath ? 1 : 0].nifPath;
		break;
	case ePath_Ground:
		pathStr = &groundModel[bFemalePath ? 1 : 0].nifPath;
		break;
	case ePath_Icon:
		pathStr = &icon[bFemalePath ? 1 : 0].ddsPath;
		break;
	}

	if (pathStr)
		return pathStr->m_data;
	else
		return "";
}

SInt8 TESActorBaseData::GetFactionRank(TESFaction* faction)
{
	for(tList<FactionListData>::Iterator iter = factionList.Begin(); !iter.End(); ++iter)
	{
		FactionListData	* data = iter.Get();
		if(data && (data->faction == faction))
			return data->rank;
	}

	return -1;
}

static const UInt8 kHandGripTable[] =
{
	TESObjectWEAP::eHandGrip_Default,
	TESObjectWEAP::eHandGrip_1,
	TESObjectWEAP::eHandGrip_2,
	TESObjectWEAP::eHandGrip_3,
	TESObjectWEAP::eHandGrip_4,
	TESObjectWEAP::eHandGrip_5,
	TESObjectWEAP::eHandGrip_6,
};

UInt8 TESObjectWEAP::HandGrip() const
{
	for(UInt32 i = 0; i < sizeof(kHandGripTable) / sizeof(kHandGripTable[0]); i++)
		if(handGrip == kHandGripTable[i])
			return i;

	return 0;
}

void TESObjectWEAP::SetHandGrip(UInt8 _handGrip)
{
	if(_handGrip < sizeof(kHandGripTable) / sizeof(kHandGripTable[0]))
		handGrip = kHandGripTable[_handGrip];
}

UInt8 TESObjectWEAP::AttackAnimation() const
{
	switch (attackAnim)
	{
		case eAttackAnim_Default:		return 0;
		case eAttackAnim_Attack3:		return 1;
		case eAttackAnim_Attack4:		return 2;
		case eAttackAnim_Attack5:		return 3;
		case eAttackAnim_Attack6:		return 4;
		case eAttackAnim_Attack7:		return 5;
		case eAttackAnim_Attack8:		return 6;
		case eAttackAnim_AttackLeft:	return 7;
		case eAttackAnim_AttackLoop:	return 8;
		case eAttackAnim_AttackRight:	return 9;
		case eAttackAnim_AttackSpin:	return 10;
		case eAttackAnim_AttackSpin2:	return 11;
		case eAttackAnim_AttackThrow:	return 12;
		case eAttackAnim_AttackThrow2:	return 13;
		case eAttackAnim_AttackThrow3:	return 14;
		case eAttackAnim_AttackThrow4:	return 15;
		case eAttackAnim_AttackThrow5:	return 16;
		case eAttackAnim_PlaceMine:		return 17;
		case eAttackAnim_PlaceMine2:	return 18;
		case eAttackAnim_Attack9:		return 19;
		case eAttackAnim_AttackThrow6:	return 20;
		case eAttackAnim_AttackThrow7:	return 21;
		case eAttackAnim_AttackThrow8:	return 22;
		default:						return 255;
	}
}

const UInt8 kAttackAnims[] = {255, 38, 44, 50, 56, 62, 68, 26, 74, 32, 80, 86, 114, 120, 126, 132, 138, 102, 108, 144, 150, 156, 162};

void TESObjectWEAP::SetAttackAnimation(UInt8 _attackAnim)
{
	attackAnim = kAttackAnims[_attackAnim];
}

TESObjectIMOD* TESObjectWEAP::GetItemMod(UInt8 which)
{
	TESObjectIMOD* pMod = NULL;
	switch(which) {
		case 1: pMod = itemMod1; break;
		case 2: pMod = itemMod2; break;
		case 3: pMod = itemMod3; break;
	}
	return pMod;
}


class FindByForm {
	TESForm* m_pForm;
public:
	FindByForm(TESForm* pForm) : m_pForm(pForm) {}
	bool Accept(TESForm* pForm) const {
		return pForm && (pForm->refID == m_pForm->refID) ? true : false;
	}
};

SInt32 BGSListForm::GetIndexOf(TESForm* pForm)
{
	return list.GetIndexOf(FindByForm(pForm));
}

SInt32 BGSListForm::RemoveForm(TESForm* pForm)
{
	SInt32 index = GetIndexOf(pForm);
	if (index >= 0) {
		RemoveNthForm(index);
	}
	return index;
}

SInt32 BGSListForm::ReplaceForm(TESForm* pForm, TESForm* pReplaceWith)
{
	SInt32 index = GetIndexOf(pForm);
	if (index >= 0) {
		list.ReplaceNth(index, pReplaceWith);
	}
	return index;
}

bool TESForm::IsInventoryObject() const
{
	typedef bool (* _IsInventoryObjectType)(UInt32 formType);
#if RUNTIME
	static _IsInventoryObjectType IsInventoryObjectType = (_IsInventoryObjectType)0x00481F30;	// first call from first case of main switch in _ExtractArg
#elif EDITOR
	static _IsInventoryObjectType IsInventoryObjectType = (_IsInventoryObjectType)0x004F4100;	// first call from first case of main switch in Cmd_DefaultParse
#endif
	return IsInventoryObjectType(typeID);
}

const char* TESPackage::TargetData::StringForTargetCode(UInt8 targetCode)
{
	switch (targetCode) {
		case TESPackage::kTargetType_Refr:
			return "Reference";
		case TESPackage::kTargetType_BaseObject:
			return "Object";
		case TESPackage::kTargetType_TypeCode:
			return "ObjectType";
		default:
			return NULL;
	}
}

UInt8 TESPackage::TargetData::TargetCodeForString(const char* targetStr)
{
	if (!_stricmp(targetStr, "REFERENCE"))
		return TESPackage::kTargetType_Refr;
	else if (!_stricmp(targetStr, "OBJECT"))
		return TESPackage::kTargetType_BaseObject;
	else if (!_stricmp(targetStr, "OBJECTTYPE"))
		return TESPackage::kTargetType_TypeCode;
	else
		return 0xFF;
}

TESPackage::TargetData* TESPackage::TargetData::Create()
{
	TargetData* data = (TargetData*)FormHeap_Allocate(sizeof(TargetData));

	// fill out with same defaults as editor uses
	data->count = 0;
	data->target.objectCode = TESPackage::kObjectType_Activators;
	data->targetType = TESPackage::kTargetType_TypeCode;

	return data;
}

TESPackage::TargetData* TESPackage::GetTargetData()
{
	if (!target)
		target = TargetData::Create();

	return target;
}

void TESPackage::SetTarget(TESObjectREFR* refr)
{
	TargetData* tdata = GetTargetData();
	tdata->targetType = kTargetType_Refr;
	tdata->target.refr = refr;
	tdata->count = 150;	//DefaultDistance
}

void TESPackage::SetCount(UInt32 aCount)
{
	if (target) {
		TargetData* tdata = GetTargetData();
		tdata->count = aCount;
	}
}

void TESPackage::SetTarget(TESForm* baseForm, UInt32 count)
{
	TargetData* tdata = GetTargetData();
	tdata->targetType = kTargetType_BaseObject;
	tdata->count = count;
	tdata->target.form = baseForm;
}

void TESPackage::SetTarget(UInt8 typeCode, UInt32 count)
{
	if (typeCode > 0 && typeCode < kObjectType_Max)
	{
		TargetData* tdata = GetTargetData();
		tdata->targetType = kTargetType_TypeCode;
		tdata->target.objectCode = typeCode;
		tdata->count= count;
	}
}

TESPackage::LocationData* TESPackage::LocationData::Create()
{
	LocationData* data = (LocationData*)FormHeap_Allocate(sizeof(LocationData));

	data->locationType = kPackLocation_CurrentLocation;
	data->object.form = NULL;
	data->radius = 0;

	return data;
}

TESPackage::LocationData* TESPackage::GetLocationData()
{
	if (!location)
		location = LocationData::Create();

	return location;
}

bool TESPackage::IsFlagSet(UInt32 flag)
{
	return (packageFlags & flag) == flag;
}

void TESPackage::SetFlag(UInt32 flag, bool bSet)
{
	if (bSet)
		packageFlags |= flag;
	else
		packageFlags &= ~flag;

	// handle either-or flags
	switch (flag)
	{
	case kPackageFlag_LockDoorsAtStart:
		if (IsFlagSet(kPackageFlag_UnlockDoorsAtStart) == bSet)
			SetFlag(kPackageFlag_UnlockDoorsAtStart, !bSet);
		break;
	case kPackageFlag_UnlockDoorsAtStart:
		if (IsFlagSet(kPackageFlag_LockDoorsAtStart) == bSet)
			SetFlag(kPackageFlag_LockDoorsAtStart, !bSet);
		break;
	case kPackageFlag_LockDoorsAtEnd:
		if (IsFlagSet(kPackageFlag_UnlockDoorsAtEnd) == bSet)
			SetFlag(kPackageFlag_UnlockDoorsAtEnd, !bSet);
		break;
	case kPackageFlag_UnlockDoorsAtEnd:
		if (IsFlagSet(kPackageFlag_LockDoorsAtEnd) == bSet)
			SetFlag(kPackageFlag_LockDoorsAtEnd, !bSet);
		break;
	case kPackageFlag_LockDoorsAtLocation:
		if (IsFlagSet(kPackageFlag_UnlockDoorsAtLocation) == bSet)
			SetFlag(kPackageFlag_UnlockDoorsAtLocation, !bSet);
		break;
	case kPackageFlag_UnlockDoorsAtLocation:
		if (IsFlagSet(kPackageFlag_LockDoorsAtLocation) == bSet)
			SetFlag(kPackageFlag_LockDoorsAtLocation, !bSet);
		break;
	}
}

static const char* TESPackage_ObjectTypeStrings[TESPackage::kObjectType_Max] =
{
	"NONE", "Activators", "Armors", "Books", "Clothing", "Containers", "Doors", "Ingredients", "Lights", "Miscellaneous", "Flora", "Furniture",
	"Weapons: Any", "Ammo", "NPCs", "Creatures", "Keys", "Alchemy", "Food", "All: Combat Wearable", "All: Wearable", "Weapons: Ranged", "Weapons: Melee",
	"Weapons: NONE", "Actor Effects: Any", "Actor Effects: Range Target", "Actor Effects: Range Touch", "Actor Effects: Range Self"
};

// add 1 to code before indexing
static const char* TESPackage_DayStrings[] = {
	"Any", "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Weekdays", "Weekends", "MWF", "TT"
};

// add 1
static const char* TESPackage_MonthString[] = {
	"Any", "January", "February", "March", "April", "May", "June", "July", "August", "September",
	"October", "November", "December", "Spring", "Summer", "Autumn", "Winter"
};

static const char* TESPackage_LocationStrings[] = {
	"Reference", "Cell", "Current", "Editor", "Object", "ObjectType"
};

static const char* TESPackage_TypeStrings[] = {
	"Find", "Follow", "Escort", "Eat", "Sleep", "Wander", "Travel", "Accompany", "UseItemAt", "Ambush",
	"FleeNotCombat", "Sandbox", "Patrol", "Guard", "Dialogue", "UseWeapon"
};

static const char* TESPackage_ProcedureStrings[] = {
	"TRAVEL", "ACTIVATE", "ACQUIRE", "WAIT", "DIALOGUE", "GREET", "GREET DEAD", "WANDER", "SLEEP", 
	"OBSERVE COMBAT", "EAT", "FOLLOW", "ESCORT", "COMBAT", "ALARM", "PURSUE", "FLEE", "DONE", "YELD", 
	"TRAVEL TARGET", "CREATE FOLLOW", "GET UP", "MOUNT HORSE", "DISMOUNT HORSE", "DO NOTHING", "UNKNOWN 019", "UNKNOWN 01B",
	"ACCOMPANY", "USE ITEM AT", "AIM", "NOTIFY", "SANDMAN", "WAIT AMBUSH", "SURFACE", "WAIT FOR SPELL", "CHOOSE CAST", 
	"FLEE NON COMBAT", "REMOVE WORN ITEMS", "SEARCH", "CLEAR MOUNT POSITION", "SUMMON CREATURE DEFEND", "AVOID AREA", 
	"UNEQUIP ARMOR", "PATROL", "USE WEAPON", "DIALOGUE ACTIVATE", "GUARD", "SANDBOX", "USE IDLE MARKER", "TAKE BACK ITEM", 
	"SITTING", "MOVEMENT BLOCKED", "CANIBAL FEED", 
};

const char* TESPackage::StringForPackageType(UInt32 pkgType)
{
	if (pkgType < kPackType_MAX) {
		return TESPackage_TypeStrings[pkgType];
	}
	else {
		return "";
	}
}

const char* TESPackage::StringForObjectCode(UInt8 objCode)
{
	if (objCode < kObjectType_Max)
		return TESPackage_ObjectTypeStrings[objCode];

	return "";
}

UInt8 TESPackage::ObjectCodeForString(const char* objString)
{
	for (UInt32 i = 0; i < kObjectType_Max; i++) {
		if (!_stricmp(objString, TESPackage_ObjectTypeStrings[i]))
			return i;
	}

	return kObjectType_Max;
}

#if RUNTIME
	static const char** s_procNames = (const char**)0x011A3CC0;
#endif

const char* TESPackage::StringForProcedureCode(eProcedure proc)
{
	if (proc < kProcedure_MAX)
		return TESPackage_ProcedureStrings[proc];

	return "";
}

//const char* TESPackage::StringForProcedureCode(eProcedure proc, bool bRemovePrefix)
//{
//	static size_t prefixLen = strlen("PROCEDURE_");
//
//	const char* name = NULL;
//	// special-case "AQUIRE" (sic) to fix typo in game executable
//	if (proc == TESPackage::kProcedure_ACQUIRE) {
//		name = "PROCEDURE_ACQUIRE";
//	}
//	else {
//		name = (proc <= TESPackage::kProcedure_MAX) ? s_procNames[proc] : NULL;
//	}
//
//	if (name && bRemovePrefix) {
//		name += prefixLen;
//	}
//
//	return name;
//}

const char* TESPackage::PackageTime::DayForCode(UInt8 dayCode)
{
	dayCode += 1;
	if (dayCode >= sizeof(TESPackage_DayStrings))
		return "";
	return TESPackage_DayStrings[dayCode];
}

const char* TESPackage::PackageTime::MonthForCode(UInt8 monthCode)
{
	monthCode += 1;
	if (monthCode >= sizeof(TESPackage_MonthString))
		return "";
	return TESPackage_MonthString[monthCode];
}

UInt8 TESPackage::PackageTime::CodeForDay(const char* dayStr)
{
	for (UInt8 i = 0; i < sizeof(TESPackage_DayStrings); i++) {
		if (!_stricmp(dayStr, TESPackage_DayStrings[i])) {
			return i-1;
		}
	}

	return kWeekday_Any;
}

UInt8 TESPackage::PackageTime::CodeForMonth(const char* monthStr)
{
	for (UInt8 i = 0; i < sizeof(TESPackage_MonthString); i++) {
		if (!_stricmp(monthStr, TESPackage_MonthString[i])) {
			return i-1;
		}
	}

	return kMonth_Any;
}

const char* TESPackage::LocationData::StringForLocationCode(UInt8 locCode)
{
	if (locCode < kPackLocation_Max)
		return TESPackage_LocationStrings[locCode];
	return "";
}

const char* TESPackage::LocationData::StringForLocationCodeAndData(void)
{
#define resultSize 256
	static char result[resultSize];
	if (locationType < kPackLocation_Max) {
		switch (locationType) { 
			case kPackLocation_NearReference:
			case kPackLocation_InCell:
			case kPackLocation_ObjectID:
				if (object.form)
					sprintf_s(result, resultSize, "%s \"%s\" [%08X] with a radius of %u", TESPackage_LocationStrings[locationType], object.form->GetTheName(), 
						object.form->refID, radius);
				else
					sprintf_s(result, resultSize, "%s \"\" [%08X] with a radius of %u", TESPackage_LocationStrings[locationType], 0, radius);
				break;
			case kPackLocation_ObjectType:
				sprintf_s(result, resultSize, "%s \"%s\" [%04X] with a radius of %u", TESPackage_LocationStrings[locationType], StringForObjectCode(object.objectCode),
					object.objectCode, radius);
				break;
			default:
				sprintf_s(result, resultSize, "%s with a radius of %u", TESPackage_LocationStrings[locationType], radius);
				break;
		}
		return result;
	}
	return "";
}

const char* TESPackage::TargetData::StringForTargetCodeAndData(void)
{
#define resultSize 256
	static char result[resultSize];
	if (targetType < kTargetType_Max) {
		switch (targetType) { 
			case kTargetType_Refr:
				if (target.refr)
					sprintf_s(result, resultSize, "%s \"%s\" [%08X] with a distance of %u", StringForTargetCode(targetType), target.refr->GetTheName(),
						target.refr->refID, count);
				else
					sprintf_s(result, resultSize, "%s [%08X] with a distance of %u", StringForTargetCode(targetType), 0, count);
				break;
			case kTargetType_BaseObject:
				if (target.form)
					sprintf_s(result, resultSize, "%s \"%s\" [%08X] with a count of %u", StringForTargetCode(targetType), target.form->GetTheName(), target.form->refID, count);
				else
					sprintf_s(result, resultSize, "%s [%08X] with a count of %u", StringForTargetCode(targetType), 0, count);
				break;
			case kTargetType_TypeCode:
				sprintf_s(result, resultSize, "%s \"%s\" [%04X] with a radius of %u", StringForTargetCode(targetType), StringForObjectCode(target.objectCode),
						target.objectCode, count);
				break;
			default:
				sprintf_s(result, resultSize, "%s with a radius of %u", StringForTargetCode(targetType), count);
				break;
		}
		return result;
	}
	return "";
}

UInt8 TESPackage::LocationData::LocationCodeForString(const char* locStr)
{
	for (UInt32 i = 0; i < kPackLocation_Max; i++)
		if (!_stricmp(locStr, TESPackage_LocationStrings[i]))
			return i;
	return kPackLocation_Max;
}

const char* TESFaction::GetNthRankName(UInt32 whichRank, bool bFemale)
{
	TESFaction::Rank* rank = ranks.GetNthItem(whichRank);
	if (!rank)
		return NULL;
	else
		return bFemale ? rank->femaleName.CStr() : rank->name.CStr();
}

void TESFaction::SetNthRankName(const char* newName, UInt32 whichRank, bool bFemale)
{
	TESFaction::Rank* rank = ranks.GetNthItem(whichRank);
	if (rank)
	{
		if (bFemale)
			rank->femaleName.Set(newName);
		else
			rank->name.Set(newName);
	}
}

UInt32 EffectItemList::CountItems() const
{
	return list.Count();
}

EffectItem* EffectItemList::ItemAt(UInt32 whichItem)
{
	return list.GetNthItem(whichItem);
}

const char* EffectItemList::GetNthEIName(UInt32 whichEffect) const
{
	EffectItem* effItem = list.GetNthItem(whichEffect);
	if (effItem->setting)
		return GetFullName(effItem->setting);
	else
		return "<no name>";
}

BGSDefaultObjectManager* BGSDefaultObjectManager::GetSingleton()
{
	return *g_defaultObjectManager;
}

Script* EffectSetting::	SetScript(Script* newScript)
{
	Script* oldScript = NULL;
	if (1 == archtype )
	{
		oldScript = (Script*)associatedItem;
		associatedItem = (TESForm*)newScript;
	};
	return oldScript;
};

Script* EffectSetting::	RemoveScript()
{
	return SetScript(NULL);
};

SInt32 TESContainer::GetCountForForm(TESForm *form)
{
	SInt32 result = 0;
	for (auto iter = formCountList.Begin(); !iter.End(); ++iter)
		if (iter->form == form) result += iter->count;
	return result;
}