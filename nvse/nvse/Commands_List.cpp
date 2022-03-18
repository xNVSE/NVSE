#include "Commands_List.h"

#include "FunctionScripts.h"
#include "GameForms.h"
#include "GameRTTI.h"
#include "GameObjects.h"
#include "GameAPI.h"

#define REPORT_BAD_FORMLISTS 0

#if REPORT_BAD_FORMLISTS

void ReportBadFormlist(COMMAND_ARGS, BGSListForm * listForm)
{
	_ERROR("### BAD FORMLIST ###");
	_ERROR("script: %08X", scriptObj ? scriptObj->refID : 0);
	_ERROR("offset: %08X", *opcodeOffsetPtr);
	_ERROR("list: %08X", listForm ? listForm->refID : 0);
}

#endif

bool Cmd_ListGetCount_Execute(COMMAND_ARGS)
{
	*result = 0;
	BGSListForm* pListForm = NULL;

#if REPORT_BAD_FORMLISTS
	__try {
#endif

	if (ExtractArgs(EXTRACT_ARGS, &pListForm)) {
		if (pListForm) {
			*result = pListForm->Count();
#if _DEBUG
			if(IsConsoleMode())
				Console_Print("count: %d", pListForm->Count());
#endif
		}
	}

#if REPORT_BAD_FORMLISTS
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		ReportBadFormlist(PASS_COMMAND_ARGS, pListForm);
	}
#endif

	return true;
}

bool Cmd_ListGetNthForm_Execute(COMMAND_ARGS)
{
	*result = 0;
	BGSListForm* pListForm = NULL;
	SInt32 n = 0;

#if REPORT_BAD_FORMLISTS
	__try {
#endif

	if (ExtractArgs(EXTRACT_ARGS, &pListForm, &n)) {
		if (pListForm) {
			TESForm* pForm = pListForm->GetNthForm(n);
			if (pForm) {
				*((UInt32 *)result) = pForm->refID;
#if _DEBUG
				if(IsConsoleMode())
				{
					TESFullName* listName = DYNAMIC_CAST(pListForm, BGSListForm, TESFullName);
					TESFullName* formName = DYNAMIC_CAST(pForm, TESForm, TESFullName);

					Console_Print("%s item %d: %X %s", listName ? listName->name.m_data : "unknown list", n, pForm->refID, formName ? formName->name.m_data : "unknown item");
				}
#endif
			}
		}
	}

#if REPORT_BAD_FORMLISTS
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		ReportBadFormlist(PASS_COMMAND_ARGS, pListForm);
	}
#endif

	return true;
}

bool Cmd_ListAddForm_Execute(COMMAND_ARGS)
{
	*result = eListInvalid;
	BGSListForm* pListForm = NULL;
	TESForm* pForm = NULL;
	SInt32 n = eListEnd;
	UInt32 bCheckForDupes = false;

#if REPORT_BAD_FORMLISTS
	__try {
#endif

	ExtractArgsEx(EXTRACT_ARGS_EX, &pListForm, &pForm, &n, &bCheckForDupes);
	if (pListForm && pForm) {
		auto const index = pListForm->AddAt(pForm, n, bCheckForDupes);
		*result = index;
		if (IsConsoleMode()) {
			Console_Print("Index: %d", index);
		}
	}

#if REPORT_BAD_FORMLISTS
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		ReportBadFormlist(PASS_COMMAND_ARGS, pListForm);
	}
#endif

	return true;
}

bool Cmd_ListAddReference_Execute(COMMAND_ARGS)
{
	*result = eListInvalid;
	BGSListForm* pListForm = nullptr;
	SInt32 n = eListEnd;
	UInt32 bCheckForDupes = false;
	
#if REPORT_BAD_FORMLISTS
	__try {
#endif

	if (ExtractArgs(EXTRACT_ARGS, &pListForm, &n, &bCheckForDupes)) {
		if (!pListForm || !thisObj) return true;

		auto const index = pListForm->AddAt(thisObj, n, bCheckForDupes);
		*result = index;
		if (IsConsoleMode()) {
			Console_Print("Index: %d", index);
		}
	}

#if REPORT_BAD_FORMLISTS
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		ReportBadFormlist(PASS_COMMAND_ARGS, pListForm);
	}
#endif

	return true;
}

bool Cmd_ListRemoveNthForm_Execute(COMMAND_ARGS)
{
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;

	BGSListForm* pListForm = nullptr;
	SInt32 n = eListEnd;

#if REPORT_BAD_FORMLISTS
	__try {
#endif

	if (ExtractArgs(EXTRACT_ARGS, &pListForm, &n)) {
		if (pListForm) {
			TESForm* pRemoved = pListForm->RemoveNthForm(n);
			if (pRemoved) {
				*refResult = pRemoved->refID;
			}
		}
	}

#if REPORT_BAD_FORMLISTS
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		ReportBadFormlist(PASS_COMMAND_ARGS, pListForm);
	}
#endif

	return true;
}

bool Cmd_ListReplaceNthForm_Execute(COMMAND_ARGS)
{
	UInt32* refResult = (UInt32*)result;
	*refResult = 0;

	BGSListForm* pListForm = NULL;
	TESForm* pReplaceWith = NULL;
	SInt32 n = eListEnd;

#if REPORT_BAD_FORMLISTS
	__try {
#endif

	if (ExtractArgs(EXTRACT_ARGS, &pListForm, &pReplaceWith, &n)) {
		if (pListForm && pReplaceWith) {
			TESForm* pReplaced = pListForm->ReplaceNthForm(n, pReplaceWith);
			if (pReplaced) {
				*refResult = pReplaced->refID;
			}
		}
	}

#if REPORT_BAD_FORMLISTS
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		ReportBadFormlist(PASS_COMMAND_ARGS, pListForm);
	}
#endif

	return true;
}

bool Cmd_ListRemoveForm_Execute(COMMAND_ARGS)
{
	*result = 0;
	BGSListForm* pListForm = NULL;
	TESForm* pForm = NULL;

#if REPORT_BAD_FORMLISTS
	__try {
#endif

	ExtractArgsEx(EXTRACT_ARGS_EX, &pListForm, &pForm);
	if (pListForm && pForm) {
		SInt32 index = pListForm->RemoveForm(pForm);
		*result = index;
	}

#if REPORT_BAD_FORMLISTS
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		ReportBadFormlist(PASS_COMMAND_ARGS, pListForm);
	}
#endif

	return true;
}

bool Cmd_ListReplaceForm_Execute(COMMAND_ARGS)
{
	*result = 0;
	BGSListForm* pListForm = NULL;
	TESForm* pForm = NULL;
	TESForm* pReplaceWith = NULL;

#if REPORT_BAD_FORMLISTS
	__try {
#endif

	ExtractArgsEx(EXTRACT_ARGS_EX, &pListForm, &pReplaceWith, &pForm);
	if (pListForm && pForm && pReplaceWith) {
		SInt32 index = pListForm->ReplaceForm(pForm, pReplaceWith);
		*result = index;
	}
#if REPORT_BAD_FORMLISTS
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		ReportBadFormlist(PASS_COMMAND_ARGS, pListForm);
	}
#endif
	return true;
}


bool Cmd_ListClear_Execute(COMMAND_ARGS)
{
	*result = 0;
	BGSListForm* pListForm = NULL;

#if REPORT_BAD_FORMLISTS
	__try {
#endif
	if (ExtractArgs(EXTRACT_ARGS, &pListForm)) {
		pListForm->list.RemoveAll();
		pListForm->numAddedObjects = 0;
	}
#if REPORT_BAD_FORMLISTS
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		ReportBadFormlist(PASS_COMMAND_ARGS, pListForm);
	}
#endif

	return true;
}

bool Cmd_ListGetFormIndex_Eval(COMMAND_ARGS_EVAL)
{
	*result = -1;
	BGSListForm* pListForm = NULL;
	TESForm* pForm = NULL;

#if REPORT_BAD_FORMLISTS
	__try {
#endif
	if (arg1 && arg2) {
		pListForm = (BGSListForm*)arg1;
		pForm = (TESForm*)arg2;
		SInt32 index = pListForm->GetIndexOf(pForm); 
		*result = index;
		if (IsConsoleMode()) {
			Console_Print("Index: %d", index);
		}
	}
#if REPORT_BAD_FORMLISTS
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		ReportBadFormlist(PASS_COMMAND_ARGS, pListForm);
	}
#endif

	return true;
}

bool Cmd_ListGetFormIndex_Execute(COMMAND_ARGS)
{
	*result = -1;
	BGSListForm* pListForm = NULL;
	TESForm* pForm = NULL;

#if REPORT_BAD_FORMLISTS
	__try {
#endif
	if (ExtractArgs(EXTRACT_ARGS, &pListForm, &pForm))
		return Cmd_ListGetFormIndex_Eval(thisObj, pListForm, pForm, result);
#if REPORT_BAD_FORMLISTS
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		ReportBadFormlist(PASS_COMMAND_ARGS, pListForm);
	}
#endif

	return true;
}

bool Cmd_IsRefInList_Eval(COMMAND_ARGS_EVAL)
{
	*result = -1;
	BGSListForm* pListForm = NULL;
	TESForm* pForm = NULL;

#if REPORT_BAD_FORMLISTS
	__try {
#endif
	if (arg1 && arg2) {
		pListForm = (BGSListForm*)arg1;
		pForm = (TESForm*)arg2;
		TESObjectREFR* pObj = DYNAMIC_CAST(pForm, TESForm, TESObjectREFR);
		SInt32 index = pListForm->GetIndexOf(pForm); 
		if ((index<0) && pObj)
			index = pListForm->GetIndexOf(GetPermanentBaseForm(pObj));
		if ((index<0) && pObj)
			index = pListForm->GetIndexOf(pObj->baseForm); 
		*result = index;
		if (IsConsoleMode()) {
			Console_Print("Index: %d", index);
		}
	}
#if REPORT_BAD_FORMLISTS
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		ReportBadFormlist(PASS_COMMAND_ARGS, pListForm);
	}
#endif

	return true;
}

bool Cmd_IsRefInList_Execute(COMMAND_ARGS)
{
	*result = -1;
	BGSListForm* pListForm = NULL;
	TESForm* pForm = NULL;

#if REPORT_BAD_FORMLISTS
	__try {
#endif
	if (ExtractArgs(EXTRACT_ARGS, &pListForm, &pForm))
		return Cmd_IsRefInList_Eval(thisObj, pListForm, pForm, result);
#if REPORT_BAD_FORMLISTS
	} __except(EXCEPTION_EXECUTE_HANDLER)
	{
		ReportBadFormlist(PASS_COMMAND_ARGS, pListForm);
	}
#endif

	return true;
}


struct ListFunctionContext
{
	BGSListForm* list = nullptr;
	Script* func = nullptr;
};

bool ExtractListFunctionContext(ListFunctionContext& ctx, COMMAND_ARGS)
{
	if (!ExtractArgs(EXTRACT_ARGS, &ctx.list, &ctx.func))
		return false;

	if (!ctx.list || !ctx.func)
		return false;

	if (!IS_ID(ctx.list, BGSListForm) || !IS_ID(ctx.func, Script))
		return false;
	
	return true;
}

bool Cmd_ForEachInList_Execute(COMMAND_ARGS)
{
	*result = false;	// result = bSuccess
	ListFunctionContext ctx;
	if (!ExtractListFunctionContext(ctx, PASS_COMMAND_ARGS))
		return true;
	auto& [list, functionScript] = ctx;
	for (auto iter = list->list.Begin(); !iter.End(); ++iter)
	{
		InternalFunctionCaller caller(functionScript, thisObj, containingObj);
		caller.SetArgs(1, iter.Get());
		auto tokenResult = UserFunctionManager::Call(std::move(caller));
	}
	*result = true;
	return true;
}
