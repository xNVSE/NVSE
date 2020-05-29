#include "Loops.h"
#include "ThreadLocal.h"
#include "ArrayVar.h"
#include "StringVar.h"
#include "ScriptUtils.h"
#include "GameAPI.h"
#include "GameObjects.h"
#include "GameRTTI.h"

LoopManager* LoopManager::GetSingleton()
{
	ThreadLocalData& localData = ThreadLocalData::Get();
	if (!localData.loopManager) {
		localData.loopManager = new LoopManager();
	}

	return localData.loopManager;
}

ArrayIterLoop::ArrayIterLoop(const ForEachContext* context, UInt8 modIndex)
{
	m_srcID = context->sourceID;
	m_iterID = context->iteratorID;
	m_iterVar = context->var;

	// clear the iterator var before initializing it
	g_ArrayMap.RemoveReference(&m_iterVar->data, modIndex);
	g_ArrayMap.AddReference(&m_iterVar->data, context->iteratorID, 0xFF);

	ArrayElement elem;
	ArrayKey key;

	if (g_ArrayMap.GetFirstElement(m_srcID, &elem, &key))
	{
		m_curKey = key;
		UpdateIterator(&elem);		// initialize iterator to first element in array
	}
}

void ArrayIterLoop::UpdateIterator(const ArrayElement* elem)
{
	std::string val("value");
	std::string key("key");

	// iter["value"] = element data
	switch (elem->DataType())
	{
	case kDataType_String:
		g_ArrayMap.SetElementString(m_iterID, val, elem->m_data.str);
		break;
	case kDataType_Numeric:
		g_ArrayMap.SetElementNumber(m_iterID, val, elem->m_data.num);
		break;
	case kDataType_Form:
		{
			g_ArrayMap.SetElementFormID(m_iterID, val, elem->m_data.formID);
			break;
		}
	case kDataType_Array:
		{
			ArrayID arrID = elem->m_data.num;
			g_ArrayMap.SetElementArray(m_iterID, val, arrID);
			break;
		}
	default:
		DEBUG_PRINT("ArrayIterLoop::UpdateIterator(): unknown datatype %d found for element value", elem->DataType());
	}

	// iter["key"] = element key
	switch (m_curKey.KeyType())
	{
	case kDataType_String:
		g_ArrayMap.SetElementString(m_iterID, key, m_curKey.Key().str);
		break;
	default:
		g_ArrayMap.SetElementNumber(m_iterID, key, m_curKey.Key().num);
	}
}

bool ArrayIterLoop::Update(COMMAND_ARGS)
{
	ArrayElement elem;
	ArrayKey key;

	if (g_ArrayMap.GetNextElement(m_srcID, &m_curKey, &elem, &key))
	{
		m_curKey = key;
		UpdateIterator(&elem);	
		return true;
	}

	return false;
}

ArrayIterLoop::~ArrayIterLoop()
{
	//g_ArrayMap.RemoveReference(&m_iterID, 0xFF);
	g_ArrayMap.RemoveReference(&m_iterVar->data, 0xFF);
}

StringIterLoop::StringIterLoop(const ForEachContext* context)
{
	StringVar* srcVar = g_StringMap.Get(context->sourceID);
	StringVar* iterVar = g_StringMap.Get(context->iteratorID);
	if (srcVar && iterVar)
	{
		m_src = srcVar->String();
		m_curIndex = 0;
		m_iterID = context->iteratorID;
		if (m_src.length())
			iterVar->Set(m_src.substr(0, 1).c_str());
	}
}

bool StringIterLoop::Update(COMMAND_ARGS)
{
	StringVar* iterVar = g_StringMap.Get(m_iterID);
	if (iterVar)
	{
		m_curIndex++;
		if (m_curIndex < m_src.length())
		{
			iterVar->Set(m_src.substr(m_curIndex, 1).c_str());
			return true;
		}
	}

	return false;
}

ContainerIterLoop::ContainerIterLoop(const ForEachContext* context)
{
	TESObjectREFR* contRef = DYNAMIC_CAST((TESForm*)context->sourceID, TESForm, TESObjectREFR);
	m_refVar = context->var;
	m_iterIndex = 0;
	m_invRef = InventoryReference::Create(contRef, IRefData(NULL, NULL, NULL), false);	

	// first: figure out what items exist by default
	std::map<TESForm*, SInt32> baseObjectCounts;
	TESContainer* cont = DYNAMIC_CAST(contRef->baseForm, TESForm, TESContainer);
	if (cont) {
		for (TESContainer::FormCountList::Iterator cur = cont->formCountList.Begin(); !cur.End(); ++cur) {
			if (cur.Get() && cur.Get()->form && cur.Get()->form->typeID != kFormType_LeveledItem) {
				DEBUG_PRINT("Base container has %d %s", cur.Get()->count, GetFullName(cur.Get()->form));
				baseObjectCounts[cur.Get()->form] = cur.Get()->count;
			}
		}
	
		// now populate the vec
		ExtraContainerChanges* xChanges = (ExtraContainerChanges*)contRef->extraDataList.GetByType(kExtraData_ContainerChanges);
		if (xChanges && xChanges->data) {
			for (ExtraContainerChanges::EntryDataList::Iterator entry = xChanges->data->objList->Begin(); !entry.End(); ++entry) {
				if (entry.Get()) {
					TESForm* baseObj = entry.Get()->type;

					SInt32 countDelta = entry.Get()->countDelta;
					SInt32 actualCount = countDelta;
					bool isInBaseContainer = baseObjectCounts.find(baseObj) != baseObjectCounts.end();
					if (isInBaseContainer) {
						baseObjectCounts[baseObj] += countDelta;
						actualCount = baseObjectCounts[baseObj];
					}

					if (entry.Get()->extendData) {
						UInt32 total = 0;
						for (ExtraContainerChanges::ExtendDataList::Iterator extend = entry.Get()->extendData->Begin(); !extend.End(); ++extend) {
							if (total >= actualCount) {
								break;
							}

							total += GetCountForExtraDataList(extend.Get());
							m_elements.push_back(IRefData(baseObj, entry.Get(), extend.Get()));
						}

						SInt32 remainder = isInBaseContainer ? baseObjectCounts[baseObj] : countDelta;
						remainder -= total;
						if (remainder > 0) {
							InventoryReference::Data::CreateForUnextendedEntry(entry.Get(), remainder, m_elements);
						}
					}
					else {
						SInt32 actualCount = countDelta;
						if (isInBaseContainer) {
							actualCount += baseObjectCounts[baseObj];
						}
						if (actualCount > 0) {
							InventoryReference::Data::CreateForUnextendedEntry(entry.Get(), actualCount, m_elements);
						}
					}

					if (isInBaseContainer) {
						baseObjectCounts.erase(baseObj);
					}
				}
				else {
					// wtf??
					DEBUG_PRINT("Warning: encountered NULL ExtraContainerChanges::Entry::Data pointer in ContainerIterLoop constructor.");
				}
			}
		}
		else if (baseObjectCounts.size()) {
			if (!xChanges) {
				xChanges = ExtraContainerChanges::Create();
			}

			xChanges->data = ExtraContainerChanges::Data::Create(contRef);
			xChanges->data->objList = (ExtraContainerChanges::EntryDataList*)FormHeap_Allocate(sizeof(ExtraContainerChanges::EntryDataList));

			std::map<TESForm*, SInt32>::iterator first = baseObjectCounts.begin();
			xChanges->data->objList->AddAt(ExtraContainerChanges::EntryData::Create(first->first, first->second), eListEnd);
			baseObjectCounts.erase(first);
		}

		// now add entries for objects in base but without associated ExtraContainerChanges
		// these extra entries will be removed when we're done with the loop
		if (baseObjectCounts.size()) {
			for (std::map<TESForm*, SInt32>::iterator iter = baseObjectCounts.begin(); iter != baseObjectCounts.end(); ++iter) {
				if (iter->second > 0) {
					ExtraContainerChanges::EntryData* ed = ExtraContainerChanges::EntryData::Create(iter->first, iter->second);
					ExtraDataList* xData = NULL;
					if (iter->second > 1) {
						ed->extendData = (ExtraContainerChanges::ExtendDataList*)FormHeap_Allocate(sizeof(ExtraContainerChanges::ExtendDataList));
						xData = ExtraDataList::Create(ExtraCount::Create(iter->second));
						ed->extendData->AddAt(xData, eListEnd);
					}

					m_elements.push_back(IRefData(iter->first, ed, xData));
				}
			}
		}
	}

	// initialize the iterator
	SetIterator();
}

bool ContainerIterLoop::UnsetIterator()
{
	return m_invRef->WriteRefDataToContainer();

	/*
	// copy extra data back to container entry
	ExtraContainerChanges::EntryExtendData* data = m_elements[m_iterIndex].data;
	if (!data->data && m_tempRef->baseExtraList.m_data) {
		data->data = ExtraDataList::Create();
	}

	if (data->data) {
		data->data->m_data = m_tempRef->baseExtraList.m_data;
		memcpy(&data->data->m_presenceBitfield, &m_tempRef->baseExtraList.m_presenceBitfield, sizeof(data->data->m_presenceBitfield));
	}

	m_tempRef->baseExtraList.m_data = NULL;
	memset(&m_tempRef->baseExtraList.m_presenceBitfield, 0, 0xC);

	return true;
	*/
}

bool ContainerIterLoop::SetIterator()
{
	TESObjectREFR* refr = m_invRef->GetRef();
	if (m_iterIndex < m_elements.size() && refr) {
		m_invRef->SetData(m_elements[m_iterIndex]);
		*((UInt64*)&m_refVar->data) = refr->refID;
		return true;
	}
	else {
		// loop ends, ref will shortly be invalid so zero out the var
		m_refVar->data = 0;
		m_invRef->SetData(IRefData(NULL, NULL, NULL));
		return false;
	}
}

bool ContainerIterLoop::Update(COMMAND_ARGS)
{
	UnsetIterator();
	m_iterIndex++;
	return SetIterator();
}

ContainerIterLoop::~ContainerIterLoop()
{
	m_invRef->Release();
	m_refVar->data = 0;
}

void LoopManager::Add(Loop* loop, ScriptRunner* state, UInt32 startOffset, UInt32 endOffset, COMMAND_ARGS)
{
	// save the instruction offsets
	LoopInfo loopInfo;
	loopInfo.loop = loop;
	loopInfo.endIP = endOffset;

	// save the stack
	SavedIPInfo* savedInfo = &loopInfo.ipInfo;
	savedInfo->ip = startOffset;
	savedInfo->stackDepth = state->stackDepth;
	memcpy(savedInfo->stack, state->stack, (savedInfo->stackDepth + 1) * sizeof(UInt32));

	m_loops.push(loopInfo);
}

void LoopManager::RestoreStack(ScriptRunner* state, SavedIPInfo* info)
{
	state->stackDepth = info->stackDepth;
	memcpy(state->stack, info->stack, (info->stackDepth + 1) * sizeof(UInt32));
}

bool LoopManager::Break(ScriptRunner* state, COMMAND_ARGS)
{
	if (!m_loops.size())
		return false;

	LoopInfo* loopInfo = &m_loops.top();

	RestoreStack(state, &loopInfo->ipInfo);

	ScriptRunner	* scriptRunner = GetScriptRunner(opcodeOffsetPtr);
	SInt32			* calculatedOpLength = GetCalculatedOpLength(opcodeOffsetPtr);

	// restore ip
	*calculatedOpLength += loopInfo->endIP - *opcodeOffsetPtr;

	delete loopInfo->loop;

	m_loops.pop();

	return true;
}

bool LoopManager::Continue(ScriptRunner* state, COMMAND_ARGS)
{
	if (!m_loops.size())
		return false;

	LoopInfo* loopInfo = &m_loops.top();

	if (!loopInfo->loop->Update(PASS_COMMAND_ARGS))
	{
		Break(state, PASS_COMMAND_ARGS);
		return true;
	}

	RestoreStack(state, &loopInfo->ipInfo);

	ScriptRunner	* scriptRunner = GetScriptRunner(opcodeOffsetPtr);
	SInt32			* calculatedOpLength = GetCalculatedOpLength(opcodeOffsetPtr);

	// restore ip
	*calculatedOpLength += loopInfo->ipInfo.ip - *opcodeOffsetPtr;

	return true;
}


bool WhileLoop::Update(COMMAND_ARGS)
{
	// save *opcodeOffsetPtr so we can calc IP to branch to after evaluating loop condition
	UInt32 originalOffset = *opcodeOffsetPtr;

	// update offset to point to loop condition, evaluate
	*opcodeOffsetPtr = m_exprOffset;
	ExpressionEvaluator eval(PASS_COMMAND_ARGS);
	bool bResult = eval.ExtractArgs();

	*opcodeOffsetPtr = originalOffset;

	if (bResult && eval.Arg(0))
			bResult = eval.Arg(0)->GetBool();

	return bResult;
}