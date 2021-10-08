#include "Commands_Packages.h"

#include "GameForms.h"
#include "GameObjects.h"
#include "GameAPI.h"
#include "GameRTTI.h"
#include "GameBSExtraData.h"
#include "GameExtraData.h"
#include "GameProcess.h"

bool Cmd_GetDialogueTarget_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;
	BaseProcess* pProcess = NULL;
	TESPackage* pPackage = NULL;
	DialoguePackage* pDPackage = NULL;

	Actor* pActor = DYNAMIC_CAST(thisObj, TESForm, Actor);
	if (pActor)
		pProcess = pActor->baseProcess;
	if (pProcess) {
		pPackage = pProcess->GetCurrentPackage();
		//DumpClass(pPackage, 128);
	}
	if (pPackage) {
		pDPackage = DYNAMIC_CAST(pPackage, TESPackage, DialoguePackage);
		if (pDPackage) {
			if (pDPackage->target)
				*refResult = pDPackage->target->refID;
		}
		//DEBUG_MESSAGE("\t\tGDT E Actor:%x package:[%#10x] refResult:[%x]\n", pActor->refID, pPackage, *refResult);
	}

	if (IsConsoleMode())
		Console_Print("GetDialogueTarget >> %10X", *refResult);
	return true;
}

bool Cmd_GetDialogueTarget_Execute(COMMAND_ARGS)
{
	*result = 0;

	if(!thisObj)
		return true;

	//DEBUG_MESSAGE("\t\tGDT 0 Actor:%x target:[%x]\n", thisObj->refID, *result);
	bool rc = Cmd_GetDialogueTarget_Eval(thisObj, 0, 0, result);
	DEBUG_MESSAGE("\t\tGDT 1 Actor:%x target:[%x]\n", thisObj->refID, *result);
	return rc;
}

bool Cmd_GetDialogueSubject_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;
	BaseProcess* pProcess = NULL;
	TESPackage* pPackage = NULL;
	DialoguePackage* pDPackage = NULL;

	Actor* pActor = DYNAMIC_CAST(thisObj, TESForm, Actor);
	if (pActor)
		pProcess = pActor->baseProcess;
	if (pProcess) {
		pPackage = pProcess->GetCurrentPackage();
		//DumpClass(pPackage, 128);
	}
	if (pPackage) {
		pDPackage = DYNAMIC_CAST(pPackage, TESPackage, DialoguePackage);
		if (pDPackage) {
			if (pDPackage->subject)
				*refResult = pDPackage->subject->refID;
		}
		//DEBUG_MESSAGE("\t\tGDS E Actor:%x package:[%#10x] refResult:[%x]\n", pActor->refID, pPackage, *refResult);
	}

	if (IsConsoleMode())
		Console_Print("GetDialogueSubject >> %10X", *refResult);
	return true;
}

bool Cmd_GetDialogueSubject_Execute(COMMAND_ARGS)
{
	*result = 0;

	if(!thisObj)
		return true;

	//DEBUG_MESSAGE("\t\tGDS 0 Actor:%x subject:[%x]\n", thisObj->refID, *result);
	bool rc = Cmd_GetDialogueSubject_Eval(thisObj, 0, 0, result);
	DEBUG_MESSAGE("\t\tGDS 1 Actor:%x subject:[%x]\n", thisObj->refID, *result);
	return rc;
}

bool Cmd_GetDialogueSpeaker_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;
	BaseProcess* pProcess = NULL;
	TESPackage* pPackage = NULL;
	DialoguePackage* pDPackage = NULL;

	Actor* pActor = DYNAMIC_CAST(thisObj, TESForm, Actor);
	if (pActor)
		pProcess = pActor->baseProcess;
	if (pProcess) {
		pPackage = pProcess->GetCurrentPackage();
		//DumpClass(pPackage, 128);
	}
	if (pPackage) {
		pDPackage = DYNAMIC_CAST(pPackage, TESPackage, DialoguePackage);
		if (pDPackage) {
			if (pDPackage->speaker)
				*refResult = pDPackage->speaker->refID;
		}
		//DEBUG_MESSAGE("\t\tGDK E Actor:%x package:[%#10x] refResult:[%x]\n", pActor->refID, pPackage, *refResult);
	}

	if (IsConsoleMode())
		Console_Print("GetDialogueSpeaker >> %10X", *refResult);
	return true;
}

bool Cmd_GetDialogueSpeaker_Execute(COMMAND_ARGS)
{
	*result = 0;

	if(!thisObj)
		return true;

	//DEBUG_MESSAGE("\t\tGDK 0 Actor:%x subject:[%x]\n", thisObj->refID, *result);
	bool rc = Cmd_GetDialogueSpeaker_Eval(thisObj, 0, 0, result);
	DEBUG_MESSAGE("\t\tGDK 1 Actor:%x subject:[%x]\n", thisObj->refID, *result);
	return rc;
}

bool Cmd_GetCurrentPackage_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;

	//DEBUG_MESSAGE("\t\tGCP @\n");
	TESObjectREFR* pRefr = NULL;
	Actor * pActor = NULL;
	TESPackage* pPackage = NULL;
	ExtractArgs(EXTRACT_ARGS, &pRefr);
	if (!pRefr)
		if(!thisObj)
			return true;
		else
			pRefr = thisObj;
	//DEBUG_MESSAGE("\t\tGCP 0 Refr:%x\n", pRefr->refID);
	pActor = DYNAMIC_CAST(pRefr, TESObjectREFR, Actor);
	if (!pActor || !pActor->baseProcess)
			return true;
	//DEBUG_MESSAGE("\t\tGCP 1 Package:[%x] Refr:%x\n", pForm, pRefr->refID);
	pPackage = pActor->baseProcess->GetCurrentPackage();
	//DEBUG_MESSAGE("\t\tGCP 2 Package:[%x] Refr:%x\n", pPackage, pRefr->refID);
	if (pPackage) {
		*refResult = pPackage->refID;
		//DEBUG_MESSAGE("\t\tGCP 3 Package:%x  Refr:%x\n", *refResult, pRefr->refID);
	}
	if (IsConsoleMode())
		Console_Print("GetCurrentPackage >> [%08X] ", *result);
	return true;
}

bool Cmd_SetPackageLocationReference_Execute(COMMAND_ARGS)
{
	*result = 0;

	//DEBUG_MESSAGE("\t\tSPL @\n");
	TESObjectREFR* pRefr = NULL;
	TESForm * pForm = NULL;
	TESPackage* pPackage = NULL;
	ExtractArgs(EXTRACT_ARGS, &pForm, &pRefr);
	if (!pRefr)
		if(!thisObj)
			return true;
		else
			pRefr = thisObj;
	//DEBUG_MESSAGE("\t\tSPL 0 Refr:%x\n", pRefr->refID);
	if (!pForm)
		return true;
	//DEBUG_MESSAGE("\t\tSPL 1 Package:[%x] Refr:%x\n", pForm, pRefr->refID);
	pPackage = DYNAMIC_CAST(pForm, TESForm, TESPackage);
	//DEBUG_MESSAGE("\t\tSPL 2 Package:[%x] Refr:%x\n", pPackage, pRefr->refID);
	if (pPackage) {
		//DEBUG_MESSAGE("\t\tSPL 3 Package:%x Refr:%x\n", pPackage->refID, pRefr->refID);
		TESPackage::LocationData * pLocation = pPackage->GetLocationData();
		//DEBUG_MESSAGE("\t\tSPL 4 Package:%x Refr:%x Location:[%x]\n", pPackage->refID, pRefr->refID, pLocation);
		if (pLocation) {
			pLocation->locationType = TESPackage::LocationData::kPackLocation_NearReference;
			pLocation->object.form = pRefr;

			//DEBUG_MESSAGE("\t\tSPL 5 Package:%x Refr:%x Location:[%x]\n", pPackage->refID, pRefr->refID, pLocation);
		}
	}
	return true;
}

bool Cmd_GetPackageLocation_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;

	//DEBUG_MESSAGE("\t\tSPL @\n");
	TESForm * pForm = NULL;
	TESPackage* pPackage = NULL;
	ExtractArgs(EXTRACT_ARGS, &pForm);
	if (!pForm)
		return true;
	//DEBUG_MESSAGE("\t\tGPL 1 Package:[%x]\n", pForm);
	pPackage = DYNAMIC_CAST(pForm, TESForm, TESPackage);
	//DEBUG_MESSAGE("\t\tGPL 2 Package:[%x]\n", pPackage);
	if (pPackage && pPackage->location) {
		//DEBUG_MESSAGE("\t\tGPL 3 Package:%x\n", pPackage->refID);
		TESPackage::LocationData * pLocation = pPackage->GetLocationData();
		//DEBUG_MESSAGE("\t\GSPL 4 Package:%x Location:[%x]\n", pPackage->refID, pLocation);
		if (pLocation && pLocation->object.form)
			switch (pLocation->locationType) {
				case TESPackage::LocationData::kPackLocation_NearReference:
				case TESPackage::LocationData::kPackLocation_InCell:
				case TESPackage::LocationData::kPackLocation_ObjectID:
					*refResult = pLocation->object.form->refID;
					break;
				case TESPackage::LocationData::kPackLocation_ObjectType:
					*refResult = pLocation->object.objectCode;
					break;
		}

			//DEBUG_MESSAGE("\t\tSPL 5 Package:%x Location:[%x]\n", pPackage->refID, *refResult);
	}
	return true;
}

bool Cmd_SetPackageLocationRadius_Execute(COMMAND_ARGS)
{
	*result = 0;

	//DEBUG_MESSAGE("\t\tSPLR @\n");
	float aRadius = 0.0;
	TESForm * pForm = NULL;
	TESPackage* pPackage = NULL;
	ExtractArgs(EXTRACT_ARGS, &pForm, &aRadius);
	//DEBUG_MESSAGE("\t\tSPLR 0 aRadius:%f\n", aRadius);
	if (!pForm)
		return true;
	//DEBUG_MESSAGE("\t\tSPLR 1 Form:0x%08x aRadius:%f\n", pForm, aRadius);
	pPackage = DYNAMIC_CAST(pForm, TESForm, TESPackage);
	//DEBUG_MESSAGE("\t\tSPLR 2 Package:0x%08x aRadius:%f\n", pPackage, aRadius);
	if (pPackage && pPackage->location) {
		//DEBUG_MESSAGE("\t\tSPLR 3 Package:[%08X] aRadius:%f\n", pPackage->refID, aRadius);
		TESPackage::LocationData * pLocation = pPackage->GetLocationData();
		//DEBUG_MESSAGE("\t\tSPLR 4 Package:%x Refr:%x Location:[%x]\n", pPackage->refID, pForm->refID, pLocation);
		if (pLocation) {
			pLocation->radius = aRadius;

			//DEBUG_MESSAGE("\t\tSPLR 5 Package:%x Refr:%x Location:[%x] radius=%d\n", pPackage->refID, pForm->refID, pLocation, pLocation->radius);
		}
	}
	return true;
}

bool Cmd_GetPackageLocationRadius_Execute(COMMAND_ARGS)
{
	*result = 0;

	//DEBUG_MESSAGE("\t\tSPLR @\n");
	TESForm * pForm = NULL;
	TESPackage* pPackage = NULL;
	ExtractArgs(EXTRACT_ARGS, &pForm);
	if (!pForm)
		return true;
	//DEBUG_MESSAGE("\t\tGPLR 1 Package:0x%08x\n", pForm);
	pPackage = DYNAMIC_CAST(pForm, TESForm, TESPackage);
	//DEBUG_MESSAGE("\t\tGPLR 2 Package:0x%08x\n", pPackage);
	if (pPackage && pPackage->location) {
		//DEBUG_MESSAGE("\t\tGPLR 3 Package:[%08X]\n", pPackage->refID);
		TESPackage::LocationData * pLocation = pPackage->GetLocationData();
		//DEBUG_MESSAGE("\t\GSPLR 4 Package:[%08X] Location:0x%08x\n", pPackage->refID, pLocation);
		if (pLocation)
			*result = pLocation->radius;

		//DEBUG_MESSAGE("\t\GSPLR 5 Package:[%08X] Radius:%f\n", pPackage->refID, *result);
	}
	return true;
}

bool Cmd_SetPackageTargetReference_Execute(COMMAND_ARGS)
{
	*result = 0;

	//DEBUG_MESSAGE("\t\tSPT @\n");
	TESObjectREFR* pRefr = NULL;
	TESForm * pForm = NULL;
	TESPackage* pPackage = NULL;
	ExtractArgs(EXTRACT_ARGS, &pForm, &pRefr);
	if (!pRefr)
		if(!thisObj)
			return true;
		else
			pRefr = thisObj;
	//DEBUG_MESSAGE("\t\tSPT 0 Refr:[%08X]\n", pRefr->refID);
	if (!pForm)
			return true;
	//DEBUG_MESSAGE("\t\tSPT 1 Form:0x%x Refr:[%08X]\n", pForm, pRefr->refID);
	pPackage = DYNAMIC_CAST(pForm, TESForm, TESPackage);
	//DEBUG_MESSAGE("\t\tSPT 2 Package:0x%x Refr:[%08X]\n", pPackage, pRefr->refID);
	if (pPackage) {
		//if (pPackage->target)
		//	DEBUG_MESSAGE("target is %s", pPackage->target->StringForTargetCodeAndData());
		//DEBUG_MESSAGE("\t\tSPT 3 Package:[%08X] Refr:[%08X] Target:0x%x\n", pPackage->refID, pRefr->refID, pPackage->target);
		pPackage->SetTarget(pRefr);
		//DEBUG_MESSAGE("\t\tSPT 4 Package:[%08X] Refr:[%08X] Target:0x%x\n", pPackage->refID, pRefr->refID, pPackage->target);
		//if (pPackage->target)
		//	DEBUG_MESSAGE("target is %s", pPackage->target->StringForTargetCodeAndData());
	}
	return true;
}

bool Cmd_SetPackageTargetCount_Execute(COMMAND_ARGS)
{
	*result = 0;

	//DEBUG_MESSAGE("\t\tSPC @\n");
	UInt32 aCount = 0;
	TESForm * pForm = NULL;
	TESPackage* pPackage = NULL;
	ExtractArgs(EXTRACT_ARGS, &pForm, &aCount);
	//DEBUG_MESSAGE("\t\tSPC 0 Count:%u\n", aCount);
	if (!pForm)
			return true;
	//DEBUG_MESSAGE("\t\tSPC 1 Form:0x%x Count:%u\n", pForm, aCount);
	pPackage = DYNAMIC_CAST(pForm, TESForm, TESPackage);
	//DEBUG_MESSAGE("\t\tSPC 2 Package:0x%x Count:%u\n", pPackage, aCount);
	if (pPackage && pPackage->target) {
		//DEBUG_MESSAGE("\t\tSPC 3 Package:[%08X] Count:%u\n", pPackage->refID, aCount);
		pPackage->SetCount(aCount);
		//DEBUG_MESSAGE("target is %s", pPackage->GetTargetData()->StringForTargetCodeAndData());
		//DEBUG_MESSAGE("\t\tSPC 4 Package:[%08X] Count:%u Target:0x%08x\n", pPackage->refID, aCount, pPackage->target);
	}
	return true;
}

bool Cmd_GetPackageTargetCount_Execute(COMMAND_ARGS)
{
	*result = 0;

	//DEBUG_MESSAGE("\t\tGPTC @\n");
	TESForm * pForm = NULL;
	TESPackage* pPackage = NULL;
	ExtractArgs(EXTRACT_ARGS, &pForm);
	if (!pForm)
			return true;
	//DEBUG_MESSAGE("\t\tGPTC 0 Form:0x%x\n", pForm);
	pPackage = DYNAMIC_CAST(pForm, TESForm, TESPackage);
	//DEBUG_MESSAGE("\t\tGPTC 1 Package:0x%x\n", pPackage);
	if (pPackage && pPackage->target) {
		//DEBUG_MESSAGE("\t\tSPC 3 Package:[%08X] Target:0x%08x\n", pPackage->refID, pPackage->target);
		TESPackage::TargetData* pTarget = pPackage->GetTargetData();
		//DEBUG_MESSAGE("\t\tSPC 4 Package:[%08X] Target:0x%08x\n", pPackage->refID, pTarget);
		if (pTarget) {
			*result = pTarget->count;
			//DEBUG_MESSAGE("target is %s", pTarget->StringForTargetCodeAndData());
		}
	}
	return true;
}

// package list access and manipulation

bool Cmd_GetPackageCount_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;
	TESAIForm* pAI = NULL;
	Actor* pActor = DYNAMIC_CAST(thisObj, TESForm, Actor);
	if (pActor)
		pAI = DYNAMIC_CAST(pActor->baseForm, TESForm, TESAIForm);
	if (pAI) {
		*result = pAI->GetPackageCount();
		//DEBUG_MESSAGE("\t\tGPC E Actor:%x AI:[%#10x] intResult:[%0.f]\n", pActor->refID, pAI, *result);
	}
	if (IsConsoleMode())
	{
		// Can't directly use "%u" with a double.
		UInt32 temp = *result;
		Console_Print("GetPackageCount >> %u", temp);
	}
	return true;
}

bool Cmd_GetPackageCount_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESObjectREFR* pRefr = NULL;

	ExtractArgs(EXTRACT_ARGS, &pRefr);
	if (!pRefr)
		if(!thisObj)
			return true;
		else
			pRefr = thisObj;

	//DEBUG_MESSAGE("\t\tGPC 0 Actor:%x count:[%0.f]\n", pRefr->refID, *result);
	bool rc = Cmd_GetPackageCount_Eval(pRefr, 0, 0, result);
	//DEBUG_MESSAGE("\t\tGPC 1 Actor:%x count:[%0.f]\n", pRefr->refID, *result);
	return rc;
}

bool Cmd_GetNthPackage_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32* refResult = (UInt32*)result;
	TESObjectREFR* pRefr = NULL;
	TESAIForm* pAI = NULL;
	TESPackage* pPackage = NULL;
	SInt32 anIndex = 0;

	ExtractArgs(EXTRACT_ARGS, &anIndex, &pRefr);
	if (!pRefr)
		if(!thisObj)
			return true;
		else
			pRefr = thisObj;

	//DEBUG_MESSAGE("\t\tGNP 0 Actor:%x index:[%d] package:[%010x]\n", pRefr->refID, anIndex, *result);
	Actor* pActor = DYNAMIC_CAST(pRefr, TESForm, Actor);
	if (pActor)
		pAI = DYNAMIC_CAST(pActor->baseForm, TESForm, TESAIForm);
	if (pAI) {
		pPackage = pAI->GetNthPackage(anIndex);
		if (pPackage)
			*refResult = pPackage->refID;
	}
	if (IsConsoleMode())
		Console_Print("GetNthPackage >> %08x (%s)", pPackage->refID, pPackage->GetName());
	//DEBUG_MESSAGE("\t\tGNP 1 Actor:%x index:[%d] package:[%010x]\n", pRefr->refID, anIndex, *result);
	return true;
}

bool Cmd_SetNthPackage_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32* refResult = (UInt32*)result;
	TESForm* pForm;
	TESObjectREFR* pRefr = NULL;
	TESAIForm* pAI = NULL;
	TESPackage* pPackage = NULL;
	SInt32 anIndex = 0;

	ExtractArgs(EXTRACT_ARGS, &pForm, &anIndex, &pRefr);
	if (!pRefr)
		if(!thisObj)
			return true;
		else
			pRefr = thisObj;

	//DEBUG_MESSAGE("\t\tSNP 0 Actor:%x index:[%d] package:[%010x]\n", pRefr->refID, anIndex, *result);
	Actor* pActor = DYNAMIC_CAST(pRefr, TESForm, Actor);
	if (pActor)
		pAI = DYNAMIC_CAST(pActor->baseForm, TESForm, TESAIForm);
	if (pAI)
		pPackage = DYNAMIC_CAST(pForm, TESForm, TESPackage);
	if (pPackage) {
		pPackage = pAI->SetNthPackage(pPackage, anIndex);
		if (pPackage)
			*refResult = pPackage->refID;
	}
	//DEBUG_MESSAGE("\t\tSNP 1 Actor:%x index:[%d] package:[%010x]\n", pRefr->refID, anIndex, *result);
	return true;
}

bool Cmd_AddPackageAt_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* pForm;
	TESObjectREFR* pRefr = NULL;
	TESAIForm* pAI = NULL;
	TESPackage* pPackage = NULL;
	SInt32 anIndex = 0;

	ExtractArgsEx(EXTRACT_ARGS_EX, &pForm, &anIndex, &pRefr);
	if (!pRefr)
		if(!thisObj)
			return true;
		else
			pRefr = thisObj;

	//DEBUG_MESSAGE("\t\tAPA 0 Actor:%x index:[%d] result:[%d]\n", pRefr->refID, anIndex, *result);
	Actor* pActor = DYNAMIC_CAST(pRefr, TESForm, Actor);
	if (pActor)
		pAI = DYNAMIC_CAST(pActor->baseForm, TESForm, TESAIForm);
	//DEBUG_MESSAGE("\t\tAPA 1 Actor:%x index:[%d] AI:[%x]\n", pRefr->refID, anIndex, pAI);
	if (pAI)
		pPackage = DYNAMIC_CAST(pForm, TESForm, TESPackage);
	//DEBUG_MESSAGE("\t\tAPA 2 Actor:%x index:[%d] Package:[%x]\n", pRefr->refID, anIndex, pPackage);
	if (pPackage) 
		*result = pAI->AddPackageAt(pPackage, anIndex);
	//DEBUG_MESSAGE("\t\tAPA 3 Actor:%x index:[%d] result:[%d]\n", pRefr->refID, anIndex, *result);
	return true;
}

bool Cmd_RemovePackageAt_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32* refResult = (UInt32*)result;
	TESObjectREFR* pRefr = NULL;
	TESAIForm* pAI = NULL;
	TESPackage* pPackage = NULL;
	SInt32 anIndex = 0;

	ExtractArgs(EXTRACT_ARGS, &anIndex, &pRefr);
	if (!pRefr)
		if(!thisObj)
			return true;
		else
			pRefr = thisObj;

	//DEBUG_MESSAGE("\t\tRPA 0 Actor:%x index:[%d] package:[%010x]\n", pRefr->refID, anIndex, *result);
	Actor* pActor = DYNAMIC_CAST(pRefr, TESForm, Actor);
	if (pActor)
		pAI = DYNAMIC_CAST(pActor->baseForm, TESForm, TESAIForm);
	if (pAI)
		pPackage = pAI->RemovePackageAt(anIndex);
	if (pPackage)
		*refResult = pPackage->refID;
	//DEBUG_MESSAGE("\t\tRPA 1 Actor:%x index:[%d] package:[%010x]\n", pRefr->refID, anIndex, *result);
	return true;
}

bool Cmd_RemoveAllPackages_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESObjectREFR* pRefr = NULL;
	TESAIForm* pAI = NULL;
	TESPackage* pPackage = NULL;
	SInt32 anIndex = 0;

	ExtractArgs(EXTRACT_ARGS, &pRefr);
	if (!pRefr)
		if(!thisObj)
			return true;
		else
			pRefr = thisObj;

	//DEBUG_MESSAGE("\t\tRAP 0 Actor:%x count:[%0.f]\n", pRefr->refID, *result);
	Actor* pActor = DYNAMIC_CAST(pRefr, TESForm, Actor);
	if (pActor)
		pAI = DYNAMIC_CAST(pActor->baseForm, TESForm, TESAIForm);
	if (pAI)
		*result = pAI->RemoveAllPackages();
	//DEBUG_MESSAGE("\t\tRAP 1 Actor:%x count:[%0.f]\n", pRefr->refID, *result);
	return true;
}
