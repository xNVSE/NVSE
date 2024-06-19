#include "Commands_MiscRef.h"

#include "GameForms.h"
#include "GameObjects.h"
#include "GameAPI.h"
#include "GameRTTI.h"
#include "GameBSExtraData.h"
#include "GameExtraData.h"
#include "GameScript.h"
#include "GameOSDepend.h"
#include "GameSettings.h"
#include "GameProcess.h"
#include "ArrayVar.h"
#include "InventoryReference.h"

bool Cmd_GetBaseObject_Execute(COMMAND_ARGS)
{
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;

	if (thisObj && thisObj->baseForm) {
		*refResult = thisObj->baseForm->refID;
		if (IsConsoleMode())
			Console_Print("GetBaseObject >> %08x (%s)", thisObj->baseForm->refID, GetFullName(thisObj->baseForm));
	}
	return true;
}

bool Cmd_GetBaseForm_Execute(COMMAND_ARGS)	// For LeveledForm, find real baseForm, not temporary one
{
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;
	TESForm* baseForm = GetPermanentBaseForm(thisObj);

	if (baseForm) {
		*refResult = baseForm->refID;
		if (IsConsoleMode())
			Console_Print("GetBaseForm >> %08x (%s)", baseForm->refID, GetFullName(baseForm));
	}
	return true;
}

bool Cmd_IsPersistent_Execute(COMMAND_ARGS)
{
	*result = 0;

	if (thisObj)
		*result = (thisObj->IsPersistent() ? 1 : 0);

	return true;
}

bool Cmd_GetLinkedDoor_Execute(COMMAND_ARGS)
{
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;

	if (!thisObj)
		return true;

	ExtraTeleport* xTele = GetByTypeCast(thisObj->extraDataList, Teleport);
	if (xTele)
		*refResult = xTele->data->linkedDoor->refID;

	return true;
}

bool Cmd_GetTeleportCell_Execute(COMMAND_ARGS)
{
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;

	if (!thisObj)
		return true;

	ExtraTeleport* xTele = GetByTypeCast(thisObj->extraDataList, Teleport);
	// parentCell will be null if linked door's cell is not currently loaded (e.g. most exterior cells)
	if (xTele && xTele->data && xTele->data->linkedDoor && xTele->data->linkedDoor->parentCell) {
		*refResult = xTele->data->linkedDoor->parentCell->refID;
	}

	return true;
}

bool Cmd_IsLoadDoor_Execute(COMMAND_ARGS)
{
	*result = 0;

	if (!thisObj)
		return true;

	if (GetByTypeCast(thisObj->extraDataList, Teleport) || GetByTypeCast(thisObj->extraDataList, RandomTeleportMarker))
		*result = 1;

	return true;
}

enum {
	kTeleport_X,
	kTeleport_Y,
	kTeleport_Z,
	kTeleport_Rot,
};

static const float kRadToDegree = 57.29577951f;
static const float kDegreeToRad = 0.01745329252f;

bool GetTeleportInfo(COMMAND_ARGS, UInt32 which)
{
	*result = 0;

	if (!thisObj || thisObj->baseForm->typeID != kFormType_TESObjectDOOR)
		return true;

	ExtraTeleport* tele = GetByTypeCast(thisObj->extraDataList, Teleport);
	if (tele && tele->data)
	{
		switch (which)
		{
		case kTeleport_X:
			*result = tele->data->x;
			break;
		case kTeleport_Y:
			*result = tele->data->y;
			break;
		case kTeleport_Z:
			*result = tele->data->z;
			break;
		case kTeleport_Rot:
			*result = tele->data->zRot * kRadToDegree;
			break;
		}
	}

	return true;
}

bool Cmd_GetDoorTeleportX_Execute(COMMAND_ARGS)
{
	return GetTeleportInfo(PASS_COMMAND_ARGS, kTeleport_X);
}

bool Cmd_GetDoorTeleportY_Execute(COMMAND_ARGS)
{
	return GetTeleportInfo(PASS_COMMAND_ARGS, kTeleport_Y);
}

bool Cmd_GetDoorTeleportZ_Execute(COMMAND_ARGS)
{
	return GetTeleportInfo(PASS_COMMAND_ARGS, kTeleport_Z);
}

bool Cmd_GetDoorTeleportRot_Execute(COMMAND_ARGS)
{
	return GetTeleportInfo(PASS_COMMAND_ARGS, kTeleport_Rot);
}

bool Cmd_SetDoorTeleport_Execute(COMMAND_ARGS)
{
	// linkedDoor x y z (rot). if omitted, coords/rot taken from linked ref

	*result = 0;
	if (!thisObj || thisObj->baseForm->typeID != kFormType_TESObjectDOOR)
		return true;

	TESObjectREFR* linkedDoor = NULL;
	float x = 999;
	float y = 999;
	float z = 999;
	float rot = 999;

	if (GetByTypeCast(thisObj->extraDataList, RandomTeleportMarker))
		return true;

	if (ExtractArgs(EXTRACT_ARGS, &linkedDoor, &x, &y, &z, &rot) && linkedDoor && linkedDoor->IsPersistent())	// ###TODO: necessary for linkedref to be door?
	{
		ExtraTeleport* tele = GetByTypeCast(thisObj->extraDataList, Teleport);
		if (!tele)
		{
			tele = ExtraTeleport::Create();
			thisObj->extraDataList.Add(tele);
		}

		tele->data->linkedDoor = linkedDoor;
		if (x == 999 && y == 999 && z == 999)
		{
			x = linkedDoor->posX;
			y = linkedDoor->posY;
			z = linkedDoor->posZ;
		}

		if (rot == 999)
			rot = linkedDoor->rotZ;
		else
			rot *= kDegreeToRad;

		tele->data->x = x;
		tele->data->y = y;
		tele->data->z = z;
		tele->data->zRot = rot;

		*result = 1;
	}

	return true;
}

bool Cmd_GetParentCell_Execute(COMMAND_ARGS)
{
	UInt32	* refResult = (UInt32 *)result;
	*refResult = 0;

	if(!thisObj) return true;

	if (thisObj->parentCell) {
		*refResult = thisObj->parentCell->refID;
	}

	return true;
}

bool Cmd_GetParentWorldspace_Execute(COMMAND_ARGS)
{
	UInt32	* refResult = (UInt32 *)result;
	*refResult = 0;

	if(!thisObj) return true;

	ExtraPersistentCell* xPersistentCell = (ExtraPersistentCell*)GetByTypeCast(thisObj->extraDataList, PersistentCell);
	if (xPersistentCell && xPersistentCell->persistentCell && xPersistentCell->persistentCell->worldSpace)
		*refResult = xPersistentCell->persistentCell->worldSpace->refID;
	else
		if (thisObj->parentCell && thisObj->parentCell->worldSpace) {
			*refResult = thisObj->parentCell->worldSpace->refID;
		}
	return true;
}

struct CellScanInfo
{
	TESObjectCELL::RefList::Iterator	prev;	//last ref returned to script
	const	TESObjectCELL * curCell;					//cell currently being scanned
	const	TESObjectCELL * cell;						//player's current cell
	const	TESWorldSpace * world;
	SInt8	curX;										//offsets of curCell from player's cell
	SInt8	curY;
	UInt8	formType;									//form type to scan for
	UInt8	cellDepth;									//depth of adjacent cells to scan
	bool	includeTakenRefs;

	CellScanInfo() {}
	CellScanInfo(UInt8 _cellDepth, UInt8 _formType, bool _includeTaken, TESObjectCELL* _cell) 
					:	curCell(NULL), cell(_cell), world(NULL), curX(0), curY(0), formType(_formType), cellDepth(_cellDepth), includeTakenRefs(_includeTaken)
	{
		world = cell->worldSpace;

		if (world && cellDepth)		//exterior, cell depth > 0
		{
			curX = cell->coords->x - cellDepth;
			curY = cell->coords->y - cellDepth;
			UInt32 key = (curX << 16) + ((curY << 16) >> 16);
			curCell = world->cellMap->Lookup(key);
		}
		else
		{
			cellDepth = 0;
			curCell = cell;
			curX = cell->coords->x;
			curY = cell->coords->y;
		}
	}

	bool NextCell()		//advance to next cell in area
	{
		if (!world || !cellDepth)
		{
			curCell = NULL;
			return false;
		}

		do
		{
			if (curX - cell->coords->x == cellDepth)
			{
				if (curY - cell->coords->y == cellDepth)
				{
					curCell = NULL;
					return false;
				}
				else
				{
					curY++;
					curX -= cellDepth * 2;
					UInt32 key = (curX << 16) + ((curY << 16) >> 16);
					curCell = world->cellMap->Lookup(key);
				}
			}
			else
			{
				curX++;
				UInt32 key = (curX << 16) + ((curY << 16) >> 16);
				curCell = world->cellMap->Lookup(key);
			}
		}while (!curCell);
		
		return true;
	}

	void FirstCell()	//init curCell to point to first valid cell
	{
		if (!curCell)
			NextCell();
	}

};

class RefMatcherARefr
{
	bool m_includeTaken;
	TESObjectREFR* m_refr;

public:
	RefMatcherARefr(bool includeTaken, TESObjectREFR* refr) : m_includeTaken(includeTaken), m_refr(refr)
		{ }

	bool Accept(const TESObjectREFR* refr)
	{
		if (!m_includeTaken && refr->IsInventoryObject() && refr->IsTaken())
			return false;
		else if (refr == m_refr)
			return true;
		else
			return false;
	}
};

struct BaseFormMatcher
{
	TESForm* m_baseForm = nullptr;

	BaseFormMatcher() = default;
	BaseFormMatcher(TESForm* baseForm) : m_baseForm(baseForm) { }

	bool MatchBaseForms(const TESObjectREFR* refr) const
	{
		if (!m_baseForm)
			return true;
		return refr->baseForm == m_baseForm;
	}
};

struct DistanceAngleMatcher
{
	TESObjectREFR* m_distanceRef = nullptr;	//if null, ignore distance check.
	float m_maxDistance = 0;	//if 0 or below, ignore.
	float m_maxHeadingAngle = -1;	//An absolute value. If below 0, ignore.
	
	DistanceAngleMatcher() = default;
	DistanceAngleMatcher(TESObjectREFR* distanceRef, float maxDistance, float maxHeadingAngle) :
		m_distanceRef(distanceRef), m_maxDistance(maxDistance), m_maxHeadingAngle(maxHeadingAngle)
	{}

	bool MatchDistanceAndAngle(const TESObjectREFR* refr) const
	{
		if (!m_distanceRef || refr == m_distanceRef)
			return true;

		if (m_maxDistance > 0)
		{
			if (GetDistance3D(m_distanceRef, refr) > m_maxDistance)
			{
				return false;
			}
		}

		if (m_maxHeadingAngle >= 0)
		{
#if _DEBUG
			// test if GetHeadingAngle has same result as Cmd_GetHeadingAngle
			// Cmd_GetHeadingAngle only works on actor calling refs, so only enable the test for that case.
			if (s_AreRuntimeTestsEnabled && m_distanceRef->Unk_3F())
			{
				double cmdResult;
				CdeclCall(0x5A0410, m_distanceRef, refr, 0, &cmdResult); // call Cmd_GetHeadingAngle
				const double result = m_distanceRef->GetHeadingAngle(refr);
				if (!FloatEqual(result, cmdResult))
				{
					Console_Print("ERROR in NVSE's GetHeadingAngle - Expected %f, got %f. Target ref: %s", 
						cmdResult, result, refr->GetStringRepresentation().c_str());
				}
			}
#endif

			if (abs(m_distanceRef->GetHeadingAngle(refr)) > m_maxHeadingAngle)
			{
				return false;
			}
		}

		return true;
	}
};

struct IncludeTakenMatcher
{
	bool m_includeTaken = true;	//if true, ignore.

	IncludeTakenMatcher() = default;
	IncludeTakenMatcher(bool includeTaken) :
		m_includeTaken(includeTaken)
	{}

	bool MatchTakenItems(const TESObjectREFR* refr) const
	{
		if (!m_includeTaken && refr->IsTaken())
			return false;
		
		return true;
	}
};

struct RefMatcherAnyForm: DistanceAngleMatcher, IncludeTakenMatcher, BaseFormMatcher
{
	RefMatcherAnyForm(bool includeTaken) :
		DistanceAngleMatcher(), IncludeTakenMatcher(includeTaken), BaseFormMatcher()
	{}

	RefMatcherAnyForm(bool includeTaken, TESObjectREFR* distanceRef, float maxDistance, float maxHeadingAngle, TESForm* baseForm):
		DistanceAngleMatcher(distanceRef, maxDistance, maxHeadingAngle), 
		IncludeTakenMatcher(includeTaken),
		BaseFormMatcher(baseForm)
	{}

	bool Accept(const TESObjectREFR* refr) const
	{
		if (!MatchTakenItems(refr) || !MatchBaseForms(refr) || !MatchDistanceAndAngle(refr))
		{
			return false;
		}
		return true;
	}
};

struct RefMatcherFormType: DistanceAngleMatcher, IncludeTakenMatcher, BaseFormMatcher
{
	UInt32 m_formType;

	RefMatcherFormType(UInt32 formType, bool includeTaken) :
	IncludeTakenMatcher(includeTaken), m_formType(formType)
	{}

	RefMatcherFormType(UInt32 formType, bool includeTaken, 
		TESObjectREFR* distanceRef, float maxDistance, float maxHeadingAngle,
		TESForm* baseForm) :
		m_formType(formType), 
		IncludeTakenMatcher(includeTaken),
		DistanceAngleMatcher(distanceRef, maxDistance, maxHeadingAngle), 
		BaseFormMatcher(baseForm)
	{}

	bool Accept(const TESObjectREFR* refr) const
	{
		if (!MatchTakenItems(refr))
			return false;

		if (!MatchBaseForms(refr))
			return false;
		
		if (refr->baseForm->typeID != m_formType || refr->baseForm->refID == 7)	//exclude player for kFormType_TESNPC
			return false;

		if (!MatchDistanceAndAngle(refr))
			return false;

		return true;
	}
};

struct RefMatcherActor: DistanceAngleMatcher, BaseFormMatcher
{
	RefMatcherActor() = default;
	RefMatcherActor(TESObjectREFR* distanceRef, float maxDistance, float maxHeadingAngle, TESForm* baseForm):
		DistanceAngleMatcher(distanceRef, maxDistance, maxHeadingAngle),
		BaseFormMatcher(baseForm)
	{}
	
	bool Accept(const TESObjectREFR* refr) const
	{
		if (!MatchBaseForms(refr))
			return false;

		if (refr->baseForm->typeID != kFormType_TESCreature
			&& (refr->baseForm->typeID != kFormType_TESNPC || refr->baseForm->refID == 7)) //exclude the player for kFormType_TESNPC
		{
			return false;
		}

		if (!MatchDistanceAndAngle(refr))
			return false;

		return true;
	}
};

struct RefMatcherItem: IncludeTakenMatcher, DistanceAngleMatcher, BaseFormMatcher
{
	RefMatcherItem(bool includeTaken) : IncludeTakenMatcher(includeTaken)
	{ }

	RefMatcherItem(bool includeTaken, 
		TESObjectREFR* distanceRef, float maxDistance, float maxHeadingAngle, 
		TESForm* baseForm) :
		IncludeTakenMatcher(includeTaken), 
		DistanceAngleMatcher(distanceRef, maxDistance, maxHeadingAngle),
		BaseFormMatcher(baseForm)
	{ }

	bool Accept(const TESObjectREFR* refr) const
	{
		if (!MatchTakenItems(refr))
			return false;

		if (!MatchBaseForms(refr))
			return false;

		switch (refr->baseForm->typeID)
		{
			case kFormType_TESObjectARMO:
			case kFormType_TESObjectBOOK:
			case kFormType_TESObjectCLOT:
			case kFormType_IngredientItem:
			case kFormType_TESObjectMISC:
			case kFormType_TESObjectWEAP:
			case kFormType_TESAmmo:
			case kFormType_TESKey:
			case kFormType_AlchemyItem:
			case kFormType_BGSNote:
			case kFormType_TESObjectARMA:
			case kFormType_TESObjectIMOD:
			case kFormType_TESCasinoChips:
			case kFormType_TESCaravanCard:
			case kFormType_TESCaravanMoney:
				break;

			case kFormType_TESObjectLIGH:
				if (TESObjectLIGH* light = DYNAMIC_CAST(refr->baseForm, TESForm, TESObjectLIGH))
					if (light->lightFlags & 2)
						break;
			default:
				return false;
		}

		if (!MatchDistanceAndAngle(refr))
			return false;
		
		return true;
	}
};

enum CustomFormTypeFilters
{
	kFormTypeFilter_AnyType = 0,
	kFormTypeFilter_Actor = 200,
	kFormTypeFilter_InventoryItem,
	kFormTypeFilter_Projectile, // currently unused
	kFormTypeFilter_SpecificReference // used for GetInGrid, GetInGridInCell
};

static const TESObjectCELL::RefList::Iterator GetCellRefEntry(const TESObjectCELL::RefList& refList, UInt32 formType, TESObjectCELL::RefList::Iterator prev,
															  bool includeTaken /*, ProjectileFinder* projFinder = NULL*/, TESObjectREFR* refr = NULL)
{
	TESObjectCELL::RefList::Iterator entry;
	switch(formType)
	{
	case kFormTypeFilter_AnyType:
		entry = refList.Find(RefMatcherAnyForm(includeTaken), prev);
		break;
	case kFormTypeFilter_Actor:
		entry = refList.Find(RefMatcherActor(), prev);
		break;
	case kFormTypeFilter_InventoryItem:
		entry = refList.Find(RefMatcherItem(includeTaken), prev);
		break;
	//case kFormTypeFilter_Projectile:	//Owned Projectile
	//	if (projFinder)
	//		entry = visitor.Find(*projFinder, prev);
	//	break;
	case kFormTypeFilter_SpecificReference:
		if (refr)
			entry = refList.Find(RefMatcherARefr(includeTaken, refr), prev);
		break;
	default:
		entry = refList.Find(RefMatcherFormType(formType, includeTaken), prev);
	}

	return entry;
}

UnorderedMap<UInt32, CellScanInfo> s_scanScripts;

static TESObjectREFR* CellScan(Script* scriptObj, TESObjectCELL* cellToScan = NULL, UInt32 formType = 0, UInt32 cellDepth = 0, bool getFirst = false, bool includeTaken = false, TESObjectREFR* refr = NULL)
{
	CellScanInfo *info;
	auto refId = scriptObj->refID;
	if (scriptObj->GetModIndex() == 0xFF && IsConsoleMode())
		refId = 0xFFFEED; // console creates new script per command, use common magic refID for all
	if (s_scanScripts.Insert(refId, &info) || getFirst)
	{
		if (!cellToScan)
		{
			ShowRuntimeError(scriptObj, "GetFirstRef MUST be called before GetNextRef");
			return nullptr;
		}
		new (info) CellScanInfo(cellDepth, formType, includeTaken, cellToScan);
		info->FirstCell();
	}

	bool bContinue = true;
	while (bContinue)
	{
		info->prev = GetCellRefEntry(info->curCell->objectList, info->formType, info->prev, info->includeTakenRefs, refr);
		if (info->prev.End() || !*info->prev)				//no ref found
		{
			if (!info->NextCell())			//check next cell if possible
				bContinue = false;
		}
		else
			bContinue = false;			//found a ref
	}
	
	if (!info->prev.End() && *info->prev)
		return info->prev.Get();
	s_scanScripts.Erase(scriptObj->refID);
	return NULL;
}

static bool GetFirstRef_Execute(COMMAND_ARGS, bool bUsePlayerCell = true, bool bUseRefr = false)
{
	UInt32 formType = kFormTypeFilter_AnyType;
	SInt32 cellDepth = -127;
	UInt32 bIncludeTakenRefs = 0;
	UInt32* refResult = (UInt32*)result;
	TESObjectCELL* cell = NULL;
	TESObjectREFR* refr = NULL;
	double uGrid = 0;
	*refResult = 0;

	PlayerCharacter* pc = PlayerCharacter::GetSingleton();
	if (!pc)
		return true;						//avoid crash when these functions called in main menu before parentCell instantiated

	if (bUsePlayerCell)
	{
		if (bUseRefr)
		{
			formType = kFormTypeFilter_SpecificReference;
			if (ExtractArgs(EXTRACT_ARGS, &refr, &cellDepth, &bIncludeTakenRefs))
				cell = pc->parentCell;
			else
				return true;
		}
		else
			if (ExtractArgs(EXTRACT_ARGS, &formType, &cellDepth, &bIncludeTakenRefs))
				cell = pc->parentCell;
			else
				return true;
	}
	else
		if (bUseRefr)
		{
			formType = kFormTypeFilter_SpecificReference;
			if (!ExtractArgs(EXTRACT_ARGS, &cell, &refr, &cellDepth, &bIncludeTakenRefs))
				return true;
		}
		else
			if (!ExtractArgs(EXTRACT_ARGS, &cell, &formType, &cellDepth, &bIncludeTakenRefs))
				return true;

	if (!cell)
		return true;

	if (cellDepth == -127)
		cellDepth = 0;
	else if (cellDepth == -1)
		if (GetNumericIniSetting("uGridsToLoad:General", &uGrid))
			cellDepth = uGrid;
		else
			cellDepth = 0;

	TESObjectREFR* found = CellScan(scriptObj, cell, formType, cellDepth, true, bIncludeTakenRefs ? true : false, refr);
	if (found)
		*refResult = found->refID;

	if (IsConsoleMode())
		Console_Print("GetFirstRef >> %08x", *refResult);

	return true;
}

bool Cmd_GetFirstRef_Execute(COMMAND_ARGS)
{
	GetFirstRef_Execute(PASS_COMMAND_ARGS, true);
	return true;
}

bool Cmd_GetFirstRefInCell_Execute(COMMAND_ARGS)
{
	GetFirstRef_Execute(PASS_COMMAND_ARGS, false);
	return true;
}

bool Cmd_GetInGrid_Execute(COMMAND_ARGS)
{
	if (GetFirstRef_Execute(PASS_COMMAND_ARGS, true, true))
		*result = *(UInt32*)result ? 1.0 : 0.0;
	return true;
}

bool Cmd_GetInGridInCell_Execute(COMMAND_ARGS)
{
	if (GetFirstRef_Execute(PASS_COMMAND_ARGS, false, true))
		*result = *(UInt32*)result ? 1.0 : 0.0;
	return true;
}

bool Cmd_GetNextRef_Execute(COMMAND_ARGS)
{
	PlayerCharacter* pc = PlayerCharacter::GetSingleton();
	if (!pc || !(pc->parentCell))
		return true;						//avoid crash when these functions called in main menu before parentCell instantiated

	UInt32* refResult = (UInt32*)result;
	*refResult = 0;

	TESObjectREFR* refr = CellScan(scriptObj);
	if (refr)
		*refResult = refr->refID;

	if (IsConsoleMode())
		Console_Print("GetNextRef >> %08x", *refResult);

	return true;
}

static bool GetNumRefs_Execute(COMMAND_ARGS, bool bUsePlayerCell = true)
{
	*result = 0;
	UInt32 formType = kFormTypeFilter_AnyType;
	SInt32 cellDepth = -127;
	UInt32 includeTakenRefs = 0;
	double uGrid = 0;
	float maxDistance = 0;
	float maxHeadingAngle = -1; // compared against abs(GetHeadingAngle)
	TESForm* baseForm = nullptr;

	PlayerCharacter* pc = PlayerCharacter::GetSingleton();
	if (!pc || !(pc->parentCell))
		return true;						//avoid crash when these functions called in main menu before parentCell instantiated

	TESObjectCELL* cell = NULL;
	if (bUsePlayerCell)
		if (ExtractArgs(EXTRACT_ARGS, &formType, &cellDepth, &includeTakenRefs, &maxDistance, &maxHeadingAngle, &baseForm))
			cell = pc->parentCell;
		else
			return true;
	else
		if (!ExtractArgs(EXTRACT_ARGS, &cell, &formType, &cellDepth, &includeTakenRefs, &maxDistance, &maxHeadingAngle, &baseForm))
			return true;

	if (!cell)
		return true;

	bool const bIncludeTakenRefs = includeTakenRefs ? true : false;
	if (cellDepth == -127)
		cellDepth = 0;
	else if (cellDepth == -1)
		if (GetNumericIniSetting("uGridsToLoad:General", &uGrid))
			cellDepth = uGrid;
		else
			cellDepth = 0;

	CellScanInfo info(cellDepth, formType, bIncludeTakenRefs, cell);
	info.FirstCell();

	auto const anyFormMatcher = RefMatcherAnyForm(bIncludeTakenRefs, thisObj, maxDistance, maxHeadingAngle, baseForm);
	auto const actorMatcher = RefMatcherActor(thisObj, maxDistance, maxHeadingAngle, baseForm);
	auto const itemMatcher = RefMatcherItem(bIncludeTakenRefs, thisObj, maxDistance, maxHeadingAngle, baseForm);
	auto const formTypeMatcher = RefMatcherFormType(formType, bIncludeTakenRefs, thisObj, maxDistance, maxHeadingAngle, baseForm);

	while (info.curCell)
	{
		const TESObjectCELL::RefList& refList = info.curCell->objectList;
		
		switch (formType)
		{
		case kFormTypeFilter_AnyType:
			*result += refList.CountIf(anyFormMatcher);
			break;
		case kFormTypeFilter_Actor:
			*result += refList.CountIf(actorMatcher);
			break;
		case kFormTypeFilter_InventoryItem:
			*result += refList.CountIf(itemMatcher);
			break;
		default:
			*result += refList.CountIf(formTypeMatcher);
		}
		info.NextCell();
	}

	return true;
}

bool Cmd_GetNumRefs_Execute(COMMAND_ARGS)
{
	return GetNumRefs_Execute(PASS_COMMAND_ARGS, true);
}

bool Cmd_GetNumRefsInCell_Execute(COMMAND_ARGS)
{
	return GetNumRefs_Execute(PASS_COMMAND_ARGS, false);
}

bool GetRefs_Execute(COMMAND_ARGS, bool bUsePlayerCell = true)
{
	// returns an array of references formID in the specified cell(s)
	ArrayVar *arr = g_ArrayMap.Create(kDataType_Numeric, true, scriptObj->GetModIndex());
	*result = arr->ID();
	UInt32 formType = kFormTypeFilter_AnyType;
	SInt32 cellDepth = -127;
	UInt32 includeTakenRefs = 0;
	double uGrid = 0;
	double arrIndex = 0;
	float maxDistance = 0;
	float maxHeadingAngle = -1; // compared against abs(GetHeadingAngle)
	TESForm* baseForm = nullptr;

	PlayerCharacter* pc = PlayerCharacter::GetSingleton();
	if (!pc || !(pc->parentCell))
		return true;						//avoid crash when these functions called in main menu before parentCell instantiated

	TESObjectCELL* cell = NULL;
	if (bUsePlayerCell)
		if (ExtractArgs(EXTRACT_ARGS, &formType, &cellDepth, &includeTakenRefs, &maxDistance, &maxHeadingAngle, &baseForm))
			cell = pc->parentCell;
		else
			return true;
	else
		if (!ExtractArgs(EXTRACT_ARGS, &cell, &formType, &cellDepth, &includeTakenRefs, &maxDistance, &maxHeadingAngle, &baseForm))
			return true;

	if (!cell)
		return true;

	bool const bIncludeTakenRefs = includeTakenRefs ? true : false;
	if (cellDepth == -127)
		cellDepth = 0;
	else if (cellDepth == -1)
		if (GetNumericIniSetting("uGridsToLoad:General", &uGrid))
			cellDepth = uGrid;
		else
			cellDepth = 0;

	CellScanInfo info(cellDepth, formType, bIncludeTakenRefs, cell);
	info.FirstCell();

	auto const anyFormMatcher = RefMatcherAnyForm(bIncludeTakenRefs, thisObj, maxDistance, maxHeadingAngle, baseForm);
	auto const actorMatcher = RefMatcherActor(thisObj, maxDistance, maxHeadingAngle, baseForm);
	auto const itemMatcher = RefMatcherItem(bIncludeTakenRefs, thisObj, maxDistance, maxHeadingAngle, baseForm);
	auto const formTypeMatcher = RefMatcherFormType(formType, bIncludeTakenRefs, thisObj, maxDistance, maxHeadingAngle, baseForm);

	while (info.curCell)
	{
		const TESObjectCELL::RefList& refList = info.curCell->objectList;
		for (TESObjectCELL::RefList::Iterator iter = refList.Begin(); !iter.End(); ++iter)
		{
			if (TESObjectREFR* const pRefr = iter.Get())
			{
				switch (formType)
				{
				case kFormTypeFilter_AnyType:
					if (anyFormMatcher.Accept(pRefr))
					{
						arr->SetElementFormID(arrIndex, pRefr->refID);
						arrIndex += 1;
					}
					break;
				case kFormTypeFilter_Actor:
					if (actorMatcher.Accept(pRefr))
					{
						arr->SetElementFormID(arrIndex, pRefr->refID);
						arrIndex += 1;
					}
					break;
				case kFormTypeFilter_InventoryItem:
					if (itemMatcher.Accept(pRefr))
					{
						arr->SetElementFormID(arrIndex, pRefr->refID);
						arrIndex += 1;
					}
					break;
				default:
					if (formTypeMatcher.Accept(pRefr))
					{
						arr->SetElementFormID(arrIndex, pRefr->refID);
						arrIndex += 1;
					}
				}
			}
		}
		info.NextCell();
	}

	return true;
}

bool Cmd_GetRefs_Execute(COMMAND_ARGS)
{
	return GetRefs_Execute(PASS_COMMAND_ARGS, true);
}

bool Cmd_GetRefsInCell_Execute(COMMAND_ARGS)
{
	return GetRefs_Execute(PASS_COMMAND_ARGS, false);
}

bool Cmd_GetRefCount_Execute(COMMAND_ARGS)
{
	InventoryReference *invRefr = s_invRefMap.GetPtr(thisObj->refID);
	if (invRefr)
		*result = invRefr->m_data.entry->countDelta;
	else
	{
		ExtraCount *xCount = GetByTypeCast(thisObj->extraDataList, Count);
		*result = xCount ? xCount->count : 1;
	}
	return true;
}

bool Cmd_SetRefCount_Execute(COMMAND_ARGS)
{
	UInt32 newCount;
	if (ExtractArgs(EXTRACT_ARGS, &newCount) && newCount && (newCount <= 0x7FFF))
	{
		InventoryReference *invRefr = s_invRefMap.GetPtr(thisObj->refID);
		if (invRefr)
		{
			if (invRefr->m_data.xData)
			{
				ExtraCount *xCount = (ExtraCount*)invRefr->m_data.xData->GetByType(kExtraData_Count);
				if (xCount) xCount->count = newCount;
			}
			invRefr->m_data.entry->countDelta = newCount;
		}
		else
		{
			ExtraCount *xCount = (ExtraCount*)thisObj->extraDataList.GetByType(kExtraData_Count);
			if (xCount)
				xCount->count = newCount;
			else
				thisObj->extraDataList.Add(ExtraCount::Create(newCount));
			thisObj->MarkAsModified(0x400);
		}
	}
	return true;
}

bool Cmd_GetOpenKey_Execute(COMMAND_ARGS)
{
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;

	if (!thisObj)	return true;
	
	ExtraLock* xLock = GetByTypeCast(thisObj->extraDataList, Lock);
	if (xLock && xLock->data && xLock->data->key)
		*refResult = xLock->data->key->refID;

#if _DEBUG
	if (IsConsoleMode())
		Console_Print("GetOpenKey >> %X", *result);
#endif
	return true;
}

bool Cmd_SetOpenKey_Execute(COMMAND_ARGS)
{
	TESForm* form;
	*result = 0;
	
	if (!thisObj)	
		return true;

	ExtractArgsEx(EXTRACT_ARGS_EX, &form);
	if (!form)	
		return true;

	TESKey* key = DYNAMIC_CAST(form, TESForm, TESKey);
	if (!key)	
		return true;

	ExtraLock* xLock = GetByTypeCast(thisObj->extraDataList, Lock);
	if (!xLock) {
		xLock = ExtraLock::Create();
		thisObj->extraDataList.Add(xLock);
	}

	if (xLock)
	{
		xLock->data->key = key;
		*result = 1;
	}

	return true;
}

bool Cmd_ClearOpenKey_Execute(COMMAND_ARGS)
{
	*result = 0;
	
	if (!thisObj)	
		return true;

	ExtraLock* xLock = GetByTypeCast(thisObj->extraDataList, Lock);
	if (xLock)
	{
		thisObj->extraDataList.Remove(xLock, true);
		*result = 1;
	}

	return true;
}

static TESForm* GetOwner(BaseExtraList& xDataList)
{
	TESForm* owner = NULL;
	ExtraOwnership* xOwner = GetByTypeCast(xDataList, Ownership);
	if (xOwner)
		owner = xOwner->owner;

	return owner;
}

static UInt32 GetOwningFactionRequiredRank(BaseExtraList& xDataList)
{
	ExtraRank * xRank = GetByTypeCast(xDataList, Rank);
	if(xRank)
		return xRank->rank;

	return 0;
}

bool Cmd_GetOwner_Execute(COMMAND_ARGS)
{
	UInt32* refResult = (UInt32 *)result;
	*refResult = 0;

	if(!thisObj)
		return true;

	TESForm * owner = GetOwner(thisObj->extraDataList);
	if(owner)
		*refResult = owner->refID;

	return true;
}

bool Cmd_GetParentCellOwner_Execute(COMMAND_ARGS)
{
	UInt32* refResult = (UInt32 *)result;
	*refResult = 0;

	if(!thisObj)
		return true;

	TESForm * owner = GetOwner(thisObj->parentCell->extraDataList);
	if(owner)
		*refResult = owner->refID;

	return true;
}

bool Cmd_GetOwningFactionRequiredRank_Execute(COMMAND_ARGS)
{
	*result = 0;

	if(!thisObj)
		return true;

	*result = GetOwningFactionRequiredRank(thisObj->extraDataList);

	return true;
}

bool Cmd_GetParentCellOwningFactionRequiredRank_Execute(COMMAND_ARGS)
{
	*result = 0;

	if(!thisObj)
		return true;

	*result = GetOwningFactionRequiredRank(thisObj->parentCell->extraDataList);

	return true;
}

bool Cmd_GetActorBaseFlagsLow_Execute(COMMAND_ARGS)
{
	TESActorBase * obj = NULL;

	*result = 0;

	if(!ExtractArgs(EXTRACT_ARGS, &obj)) return true;

	if(!obj && thisObj && thisObj->baseForm)
		obj = DYNAMIC_CAST(thisObj->baseForm, TESForm, TESActorBase);

	if(obj)
		*result = obj->baseData.flags & 0xFFFF;

	return true;
}

bool Cmd_SetActorBaseFlagsLow_Execute(COMMAND_ARGS)
{
	TESActorBase	* obj = NULL;
	UInt32			data;

	*result = 0;

	if(!ExtractArgs(EXTRACT_ARGS, &data, &obj)) return true;

	if(!obj && thisObj && thisObj->baseForm)
		obj = DYNAMIC_CAST(thisObj->baseForm, TESForm, TESActorBase);

	if(obj)
		obj->baseData.flags = (data & 0x0000FFFF) | (obj->baseData.flags & 0xFFFF0000);

	return true;
}

bool Cmd_GetActorBaseFlagsHigh_Execute(COMMAND_ARGS)
{
	TESActorBase * obj = NULL;

	*result = 0;

	if(!ExtractArgs(EXTRACT_ARGS, &obj)) return true;

	if(!obj && thisObj && thisObj->baseForm)
		obj = DYNAMIC_CAST(thisObj->baseForm, TESForm, TESActorBase);

	if(obj)
		*result = (obj->baseData.flags >> 16) & 0xFFFF;

	return true;
}

bool Cmd_SetActorBaseFlagsHigh_Execute(COMMAND_ARGS)
{
	TESActorBase	* obj = NULL;
	UInt32			data;

	*result = 0;

	if(!ExtractArgs(EXTRACT_ARGS, &data, &obj)) return true;

	if(!obj && thisObj && thisObj->baseForm)
		obj = DYNAMIC_CAST(thisObj->baseForm, TESForm, TESActorBase);

	if(obj)
		obj->baseData.flags = ((data << 16) & 0xFFFF0000) | (obj->baseData.flags & 0x0000FFFF);

	return true;
}

bool Cmd_GetFlagsLow_Execute(COMMAND_ARGS)
{
	TESForm	* obj = NULL;

	*result = 0;

	if(!ExtractArgs(EXTRACT_ARGS, &obj)) return true;

	if(!obj && thisObj)
		obj = thisObj;

	if(obj)
		*result = obj->flags & 0xFFFF;

	return true;
}

bool Cmd_SetFlagsLow_Execute(COMMAND_ARGS)
{
	TESForm	* obj = NULL;
	UInt32			data;

	*result = 0;

	if(!ExtractArgs(EXTRACT_ARGS, &data, &obj)) return true;

	if(!obj && thisObj)
		obj = thisObj;

	if(obj)
		obj->flags = (data & 0x0000FFFF) | (obj->flags & 0xFFFF0000);

	return true;
}

bool Cmd_GetFlagsHigh_Execute(COMMAND_ARGS)
{
	TESForm	* obj = NULL;

	*result = 0;

	if(!ExtractArgs(EXTRACT_ARGS, &obj)) return true;

	if(!obj && thisObj)
		obj = thisObj;

	if(obj)
		*result = (obj->flags >> 16) & 0xFFFF;

	return true;
}

bool Cmd_SetFlagsHigh_Execute(COMMAND_ARGS)
{
	TESActorBase	* obj = NULL;
	UInt32			data;

	*result = 0;

	if(!ExtractArgs(EXTRACT_ARGS, &data, &obj)) return true;

	if(!obj && thisObj && thisObj->baseForm)
		obj = DYNAMIC_CAST(thisObj->baseForm, TESForm, TESActorBase);

	if(obj)
		obj->flags = ((data << 16) & 0xFFFF0000) | (obj->flags & 0x0000FFFF);

	return true;
}

static UInt32 SetOwningFactionRequiredRank(BaseExtraList& xDataList, UInt32 rank)
{
	if (rank) {
		ExtraRank * xRank = ExtraRank::Create();
		if(xRank) {
			xRank->rank = rank;
			xDataList.Add(xRank);
		}
	}
	return rank;
}

bool Cmd_SetOwningFactionRequiredRank_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 rank = 0;

	if(!thisObj)
		return true;

	if (ExtractArgs(EXTRACT_ARGS, &rank))
		*result = SetOwningFactionRequiredRank(thisObj->extraDataList, rank);

	return true;
}

SInt8 GetFactionRank(TESObjectREFR * thisObj, TESFaction * faction)
{
	SInt8 foundRank = -1;
	if (thisObj && faction)
	{
		bool bFoundRank = false;
		foundRank = GetExtraFactionRank(thisObj->extraDataList, faction);
		bFoundRank = ( -1 != foundRank );
		if (!bFoundRank) {
			TESActorBaseData* actorBase = DYNAMIC_CAST(thisObj->baseForm, TESForm, TESActorBaseData);
			if (actorBase)
			{
				foundRank = actorBase->GetFactionRank(faction);
				bFoundRank = true;
			}
		}
	}

	return foundRank;
}

bool Cmd_HasOwnership_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0.0;
	TESObjectREFR * aRef = (TESObjectREFR*)arg1;

	if(!thisObj)
		return true;

	if(!aRef)
		return true;

	SInt8 rank = 0;
	TESForm * owner = GetOwner(aRef->extraDataList);
	DEBUG_MESSAGE("\t\t\tHO thisObj:%x aNPC:[%x] Owner:[%x]\n", thisObj->refID, aRef->refID, owner);
	if(!owner) {
		owner = GetOwner(aRef->parentCell->extraDataList);
		DEBUG_MESSAGE("\t\t\t\tHO thisObj:%x aNPC:[%x] Owner:[%x]\n", thisObj->refID, aRef->refID, owner);
		if (owner)
			rank = GetOwningFactionRequiredRank(aRef->parentCell->extraDataList);
	}
	else
		rank = GetOwningFactionRequiredRank(aRef->extraDataList);
	DEBUG_MESSAGE("\t\t\tHO thisObj:%x aNPC:[%x] Owner:[%x] Rank:%.2f\n", thisObj->refID, aRef->refID, owner, rank);

	if (owner)
		if (owner->refID==thisObj->baseForm->refID)
			*result = 1.0;
		else {
			TESFaction * faction = DYNAMIC_CAST(owner, TESForm, TESFaction);
			if (faction)
				*result = (GetFactionRank(thisObj, faction) >= rank) ? 1.0 : 0.0;
		}
	if (IsConsoleMode())
		Console_Print("HasOwnership >> %.2f", *result);

	return true;
}

bool Cmd_HasOwnership_Execute(COMMAND_ARGS)
{
	*result = 0.0;
	TESObjectREFR * aRef = NULL;

	if (!thisObj)
		return true;

	DEBUG_MESSAGE("\n\tHO HasOwnership called");
	if (ExtractArgs(EXTRACT_ARGS, &aRef)) {
		DEBUG_MESSAGE("\t\tHO thisObj:%x aRef:[%x]", thisObj->refID, aRef->refID);
		bool rc = Cmd_HasOwnership_Eval(thisObj, (void*)aRef, 0, result);
		if (rc && *result)
			DEBUG_MESSAGE("\t\tHO thisObj:%x aRef:[%x] hasOwnership:[%.2f]\n", thisObj->refID, aRef->refID, *result);
		return rc;
	}
	return true;

}
bool Cmd_IsOwned_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0.0;
	Actor* anNPC = (Actor*)arg1;

	if(!thisObj)
		return true;

	if(!anNPC)
		return true;

	SInt8 rank = 0;
	TESForm * owner = GetOwner(thisObj->extraDataList);
	DEBUG_MESSAGE("\t\t\tIO thisObj:%x anNPC:[%x] Owner:[%x]\n", thisObj->refID, anNPC->refID, owner);
	if(!owner) {
		owner = GetOwner(thisObj->parentCell->extraDataList);
		DEBUG_MESSAGE("\t\t\t\tIO thisObj:%x anNPC:[%x] Owner:[%x]\n", thisObj->refID, anNPC->refID, owner);
		if (owner)
			rank = GetOwningFactionRequiredRank(thisObj->parentCell->extraDataList);
	}
	else
		rank = GetOwningFactionRequiredRank(thisObj->extraDataList);
	DEBUG_MESSAGE("\t\t\tIO thisObj:%x anNPC:[%x] Owner:[%x] Rank:%.2f\n", thisObj->refID, anNPC->refID, owner, rank);

	if (owner)
		if (owner->refID==anNPC->baseForm->refID)
			*result = 1.0;
		else {
			TESFaction * faction = DYNAMIC_CAST(owner, TESForm, TESFaction);
			if (faction)
				*result = (GetFactionRank(anNPC, faction) >= rank) ? 1.0 : 0.0;
		}
	if (IsConsoleMode())
		Console_Print("IsOwned >> %.2f", *result);

	return true;
}

bool Cmd_IsOwned_Execute(COMMAND_ARGS)
{
	*result = 0.0;
	Actor * anNPC = NULL;

	if(!thisObj)
		return true;

	DEBUG_MESSAGE("\n\tIO IsOwned called");
	if (ExtractArgs(EXTRACT_ARGS, &anNPC)) {
		DEBUG_MESSAGE("\t\tIO thisObj:%x anNPC:[%x]", thisObj->refID, anNPC);
		bool rc = Cmd_IsOwned_Eval(thisObj, (void*)anNPC, 0, result);
		if (rc && *result)
			DEBUG_MESSAGE("\t\tHO thisObj:%x anNPC:[%x] isOwned:[%.2f]\n", thisObj->refID, anNPC, *result);
		return rc;
	}
	return true;

}

bool Cmd_SetEyes_Execute(COMMAND_ARGS)
{
	TESForm	* part = NULL;
	TESForm	* target = NULL;

	*result = 0;

	if(!ExtractArgsEx(EXTRACT_ARGS_EX, &part, &target))
		return true;

	TESEyes	* eyes = DYNAMIC_CAST(part, TESForm, TESEyes);
	if(!eyes)
		return true;

	if(!target)
	{
		if(!thisObj)
			return true;
		target = thisObj->baseForm;
	}

	if(!target)
		return true;

	TESNPC	* npc = DYNAMIC_CAST(target, TESForm, TESNPC);
	if(!npc)
		return true;

	npc->eyes = eyes;

	*result = 1;

	return true;
}

bool Cmd_SetHair_Execute(COMMAND_ARGS)
{
	TESForm	* part = NULL;
	TESForm	* target = NULL;

	*result = 0;

	if(!ExtractArgsEx(EXTRACT_ARGS_EX, &part, &target))
		return true;

	TESHair	* hair = DYNAMIC_CAST(part, TESForm, TESHair);
	if(!hair)
		return true;

	if(!target)
	{
		if(!thisObj)
			return true;
		target = thisObj->baseForm;
	}

	if(!target)
		return true;

	TESNPC	* npc = DYNAMIC_CAST(target, TESForm, TESNPC);
	if(!npc)
		return true;

	npc->hair = hair;

	*result = 1;

	return true;
}

bool Cmd_SetHairLength_Execute(COMMAND_ARGS)
{
	float	length = 0;
	TESForm	* target = NULL;

	*result = 0;

	if(!ExtractArgs(EXTRACT_ARGS, &length, &target))
		return true;

	if(!target)
	{
		if(!thisObj)
			return true;
		target = thisObj->baseForm;
	}

	if(!target)
		return true;

	TESNPC	* npc = DYNAMIC_CAST(target, TESForm, TESNPC);
	if(!npc)
		return true;

	npc->hairLength = length;

	*result = 1;

	return true;

}

bool Cmd_SetHairColor_Execute(COMMAND_ARGS)
{
	UInt32	color = 0;
	TESForm	* target = NULL;

	*result = 0;

	if(!ExtractArgs(EXTRACT_ARGS, &color, &target))
		return true;

	if(!target)
	{
		if(!thisObj)
			return true;
		target = thisObj->baseForm;
	}

	if(!target)
		return true;

	TESNPC	* npc = DYNAMIC_CAST(target, TESForm, TESNPC);
	if(!npc)
		return true;

	npc->hairColor = color & 0x00FFFFFF;

	*result = 1;

	return true;

}

bool Cmd_GetEyes_Execute(COMMAND_ARGS)
{
	TESNPC	* npc = 0;
	UInt32	* refResult = (UInt32 *)result;

	*refResult = 0;

	if(!ExtractArgs(EXTRACT_ARGS, &npc))
		return true;

	if(!npc && thisObj)
		npc = DYNAMIC_CAST(thisObj->baseForm, TESForm, TESNPC);

	if(npc && npc->eyes)
	{
		*refResult = npc->eyes->refID;
	}

	if(IsConsoleMode())
		Console_Print("GetEyes: %08X", *refResult);

	return true;
}

bool Cmd_GetEyesFlags_Execute(COMMAND_ARGS)
{
	TESEyes*	eyes = NULL;
	TESForm*	form = NULL;
	UInt32		flagMask = 0x0FFFFFFFF;
	UInt32		iResult = 0;
	*result = 0;

	if (ExtractArgs(EXTRACT_ARGS, &form, &flagMask) && form) {
		eyes = DYNAMIC_CAST(form, TESForm, TESEyes);
		if (eyes)
		{
			iResult = eyes->eyeFlags & flagMask;
			*result = iResult;
		}
	}

	if(IsConsoleMode())
		Console_Print("GetEyesFlags: %08X", iResult);

	return true;
}

bool Cmd_SetEyesFlags_Execute(COMMAND_ARGS)
{
	TESEyes*	eyes = NULL;
	TESForm*	form = NULL;
	UInt32		flagMask = 0x0FFFFFFFF;
	*result = 0;

	if (ExtractArgs(EXTRACT_ARGS, &form, &flagMask) && form && (flagMask < 0x0FF) ) {
		eyes = DYNAMIC_CAST(form, TESForm, TESEyes);
		if (eyes)
			eyes->eyeFlags = flagMask;
	}

	return true;
}

bool Cmd_GetHair_Execute(COMMAND_ARGS)
{
	TESNPC	* npc = 0;
	UInt32	* refResult = (UInt32 *)result;

	*refResult = 0;

	if(!ExtractArgs(EXTRACT_ARGS, &npc))
		return true;

	if(!npc && thisObj)
		npc = DYNAMIC_CAST(thisObj->baseForm, TESForm, TESNPC);

	if(npc && npc->hair)
	{
		*refResult = npc->hair->refID;
	}

	if(IsConsoleMode())
		Console_Print("GetHair: %08X", *refResult);

	return true;
}

bool Cmd_GetHairFlags_Execute(COMMAND_ARGS)
{
	TESHair*	hair = NULL;
	TESForm*	form = NULL;
	UInt32		flagMask = 0x0FFFFFFFF;
	UInt32		iResult = 0;
	*result = 0;

	if (ExtractArgs(EXTRACT_ARGS, &form, &flagMask) && form) {
		hair = DYNAMIC_CAST(form, TESForm, TESHair);
		if (hair)
		{
			iResult = hair->hairFlags & flagMask;
			*result = iResult;
		}
	}

	if(IsConsoleMode())
		Console_Print("GetHairFlags: %08X", iResult);
	return true;
}

bool Cmd_SetHairFlags_Execute(COMMAND_ARGS)
{
	TESHair*	hair = NULL;
	TESForm*	form = NULL;
	UInt32		flagMask = 0x0FFFFFFFF;
	*result = 0;

	if (ExtractArgs(EXTRACT_ARGS, &form, &flagMask) && form && (flagMask < 0x0FF) ) {
		hair = DYNAMIC_CAST(form, TESForm, TESHair);
		if (hair)
			hair->hairFlags = flagMask;
	}

	return true;
}

bool Cmd_GetHairLength_Execute(COMMAND_ARGS)
{
	TESNPC	* npc = 0;

	*result = 0;

	if(!ExtractArgs(EXTRACT_ARGS, &npc))
		return true;

	if(!npc && thisObj)
		npc = DYNAMIC_CAST(thisObj->baseForm, TESForm, TESNPC);

	if(npc)
	{
		*result = npc->hairLength;
	}

	if(IsConsoleMode())
		Console_Print("GetHairLength: %f", *result);

	return true;
}

bool Cmd_GetHairColor_Execute(COMMAND_ARGS)
{
	*result = 0;

	TESNPC	* npc = 0;
	UInt32	code = 0;
	UInt32	color = 0;

	if(!ExtractArgs(EXTRACT_ARGS, &code, &npc))
		return true;

	if(!npc && thisObj)
		npc = DYNAMIC_CAST(thisObj->baseForm, TESForm, TESNPC);

	if(npc)
	{
		switch (code) {
			case 3:
				color = (npc->hairColor >> 16) & 0xFF;
				break;
			case 2:
				color = (npc->hairColor >>  8) & 0xFF;
				break;
			case 1:
				color = (npc->hairColor      ) & 0xFF;
				break;
			default:
				color = npc->hairColor;
				break;
		}
		*result = 1.0 * color;
		if (IsConsoleMode())
			Console_Print("GetHairColor: (%08X) Code=%d Color=%02x [%f]", npc->hairColor, code, color, *result);
	}

	return true;
}

bool Cmd_SetNPCWeight_Execute(COMMAND_ARGS)
{
	float	weight = 0;
	TESForm	* target = NULL;

	*result = 0;

	if(!ExtractArgs(EXTRACT_ARGS, &weight, &target))
		return true;

	if(!target)
	{
		if(!thisObj)
			return true;
		target = thisObj->baseForm;
	}

	if(!target)
		return true;

	TESNPC	* npc = DYNAMIC_CAST(target, TESForm, TESNPC);
	if(!npc)
		return true;

	npc->weight = weight;

	*result = 1;

	return true;

}

bool Cmd_GetNPCWeight_Execute(COMMAND_ARGS)
{
	TESNPC	* npc = 0;

	*result = 0;

	if(!ExtractArgs(EXTRACT_ARGS, &npc))
		return true;

	if(!npc && thisObj)
		npc = DYNAMIC_CAST(thisObj->baseForm, TESForm, TESNPC);

	if(npc)
	{
		*result = npc->weight;
	}

	if(IsConsoleMode())
		Console_Print("GetNPCweight: %f", *result);

	return true;
}

bool Cmd_SetNPCHeight_Execute(COMMAND_ARGS)
{
	float	height = 0;
	TESForm	* target = NULL;

	*result = 0;

	if(!ExtractArgs(EXTRACT_ARGS, &height, &target))
		return true;

	if(!target)
	{
		if(!thisObj)
			return true;
		target = thisObj->baseForm;
	}

	if(!target)
		return true;

	TESNPC	* npc = DYNAMIC_CAST(target, TESForm, TESNPC);
	if(!npc)
		return true;

	npc->height = height;

	*result = 1;

	return true;

}

bool Cmd_GetNPCHeight_Execute(COMMAND_ARGS)
{
	TESNPC	* npc = 0;

	*result = 0;

	if(!ExtractArgs(EXTRACT_ARGS, &npc))
		return true;

	if(!npc && thisObj)
		npc = DYNAMIC_CAST(thisObj->baseForm, TESForm, TESNPC);

	if(npc)
	{
		*result = npc->height;
	}

	if(IsConsoleMode())
		Console_Print("GetNPCheight: %f", *result);

	return true;
}

bool Cmd_Update3D_Execute(COMMAND_ARGS)
{
	*result = 0.0;
	if (thisObj)
		if (alternateUpdate3D)
		{
			if (thisObj->Update3D_v1c())
				*result = 1.0;
		}
		else
			if (thisObj->Update3D()) 
				*result = 1.0;

	return true;
}

bool Cmd_IsPlayerSwimming_Eval(COMMAND_ARGS_EVAL)
{
	*result = PlayerCharacter::GetSingleton()->IsPlayerSwimming();
	if(IsConsoleMode())
		Console_Print("Player is swimming? %f", *result);
	return true;
}

bool Cmd_IsPlayerSwimming_Execute(COMMAND_ARGS)
{
	return Cmd_IsPlayerSwimming_Eval(thisObj, NULL, NULL, result);
}

bool Cmd_GetTFC_Eval(COMMAND_ARGS_EVAL)
{
	*result = (*g_osGlobals)->isFlycam;
	if(IsConsoleMode())
		Console_Print("GetTFC: %f", *result);
	return true;
}

bool Cmd_GetTFC_Execute(COMMAND_ARGS)
{
	return Cmd_GetTFC_Eval(thisObj, NULL, NULL, result);
}

bool Cmd_PlaceAtMeAndKeep_Execute(COMMAND_ARGS)
{
	TESForm* form = NULL;
	double distance = 0.0;
	double direction = 0.0;
	CommandInfo* ciPlaceAtMe = g_scriptCommands.GetByName("PlaceAtMe");
	UInt32	* refResult = (UInt32 *)result;
	*refResult = 0;

	if (!ciPlaceAtMe || !ciPlaceAtMe->execute(PASS_COMMAND_ARGS))
		return false;
	if (*result)
		form = LookupFormByID(*refResult);
	if (form)
		CALL_MEMBER_FN(TESSaveLoadGame::Get(), AddCreatedForm)((TESForm*)(form));

	return true;
}

bool Cmd_GetActorFIKstatus_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	if (!thisObj)
		return false;

	Actor* actor = DYNAMIC_CAST(thisObj, TESObjectREFR, Actor);
	if (actor && actor->ragDollController)
		*result = /*actor->ragDollController->bool021F &&*/ actor->ragDollController->fikStatus;

	return true;
}

bool Cmd_GetActorFIKstatus_Execute(COMMAND_ARGS)
{
	return Cmd_GetActorFIKstatus_Eval(thisObj, NULL, NULL, result);
};

bool Cmd_SetActorFIKstatus_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32	doSet = 0;
	if(!ExtractArgs(EXTRACT_ARGS, &doSet))
		return true;

	if (!thisObj)
		return false;

	Actor* actor = DYNAMIC_CAST(thisObj, TESObjectREFR, Actor);
	if (actor && actor->ragDollController && actor->ragDollController->bool021F)
		actor->ragDollController->fikStatus = doSet ? true : false;

	return true;
};

// Port from OBSE

class EffectShaderFinder
{
	TESEffectShader		* m_shader;
	TESObjectREFR		* m_refr;
public:
	EffectShaderFinder(TESObjectREFR* refr, TESEffectShader* shader) : m_refr(refr), m_shader(shader) { }

	bool Accept(BSTempEffect* effect)
	{
		MagicShaderHitEffect* mgsh = DYNAMIC_CAST(effect, BSTempEffect, MagicShaderHitEffect);
		if (mgsh && mgsh->target == m_refr && mgsh->effectShader == m_shader) {
			return true;
		}
		else {
			return false;
		}
	}
};

bool Cmd_HasEffectShader_Execute(COMMAND_ARGS)
{
	TESEffectShader * shader = NULL;
	*result = 0.0;

	if (thisObj && ExtractArgs(EXTRACT_ARGS, &shader) && shader) {
		EffectShaderFinder finder(thisObj, shader);
		*result = g_actorProcessManager->tempEffects.CountIf(finder);
	}

	return true;
}

bool Cmd_SetEditorID_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* form{};
	char editorId[0x200];
	if (!ExtractArgs(EXTRACT_ARGS, &form, &editorId))
		return true;
	form->SetEditorID_AtRuntime(editorId);
	*result = 1;
	return true;
}

bool Cmd_CreateFormList_Execute(COMMAND_ARGS)
{
	*result = 0;
	if (ExpressionEvaluator eval(PASS_COMMAND_ARGS);
		eval.ExtractArgs())
	{
		BGSListForm* const formList = BGSListForm::MakeUnique().release();
		if (!formList)
			return true;
		UInt32* refResult = (UInt32*)result;
		*refResult = formList->refID;

		auto const numArgs = eval.NumArgs();
		if (numArgs >= 1)
		{
			if (auto const edID = eval.Arg(0)->GetString())
				formList->SetEditorID_AtRuntime(edID);

			if (numArgs >= 2)
			{
				//Fill the list with contents of array.
				if (auto const array = eval.Arg(1)->GetArrayVar())
				{
					for (auto const elem : *array)
					{
						UInt32 formId;
						if (elem->GetAsFormID(&formId))
						{
							formList->AddAt(LookupFormByID(formId), eListEnd);
						}
					}
				}
			}
		}
	}
	return true;
}

// Copied code from here: https://forums.nexusmods.com/index.php?/topic/1715964-getheadingangle-equivalent-for-x-axis/
// (Basic pythagorian theorem being applied)
// TODO: If passing player as thisObj, use the camera pos instead for more accurate results (?)
bool Cmd_GetHeadingAngleX_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESObjectREFR* targetRef;
	if (!ExtractArgs(EXTRACT_ARGS, &targetRef) || !thisObj)
		return true;
	
	float opposite = thisObj->posZ - targetRef->posZ;
	float hypotenuse = thisObj->GetDistance(targetRef);
	float fraction = opposite / hypotenuse;
	*result = (asin(fraction) - thisObj->rotX) * Flt180dPI;

	return true;
}

bool Cmd_EvaluateInventory_Execute(COMMAND_ARGS) {
	if (!DYNAMIC_CAST(thisObj, TESObjectREFR, Actor)) {
		return true;
	}

	if (!thisObj->IsCharacter()) {
		return true;
	}

	TESForm* baseForm = thisObj->baseForm;
	ValidBip01Names* bipNames = thisObj->GetValidBip01Names();

	ThisStdCall<ValidBip01Names*>(0x606540, baseForm, thisObj, bipNames, 1);

	return true;
}
