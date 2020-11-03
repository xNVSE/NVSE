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

	ArrayVar *arr = g_ArrayMap.Get(m_srcID);
	if (arr)
	{
		const ArrayKey *key;
		ArrayElement *elem;
		if (arr->GetFirstElement(&elem, &key))
		{
			m_curKey = *key;
			UpdateIterator(elem);		// initialize iterator to first element in array
		}
	}
}

void ArrayIterLoop::UpdateIterator(const ArrayElement* elem)
{
	ArrayVar *arr = g_ArrayMap.Get(m_iterID);
	if (!arr) return;

	// iter["key"] = element key
	ArrayElement *newElem = arr->Get("key", true);
	if (newElem)
	{
		if (m_curKey.KeyType() == kDataType_String)
			newElem->SetString(m_curKey.key.str);
		else newElem->SetNumber(m_curKey.key.num);
	}
	// iter["value"] = element data
	newElem = arr->Get("value", true);
	if (newElem) newElem->Set(elem);
}

bool ArrayIterLoop::Update(COMMAND_ARGS)
{
	ArrayVar *arr = g_ArrayMap.Get(m_srcID);
	if (arr)
	{
		ArrayElement *elem;
		const ArrayKey *key;
		if (arr->GetNextElement(&m_curKey, &elem, &key))
		{
			m_curKey = *key;
			UpdateIterator(elem);	
			return true;
		}
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
	TESObjectREFR* contRef = (TESObjectREFR*)context->sourceID;
	m_refVar = context->var;
	m_iterIndex = 0;
	m_invRef = CreateInventoryRef(contRef, IRefData(), false);

	InventoryItemsMap invItems(0x40);
	if (contRef->GetInventoryItems(invItems))
	{
		TESForm *item;
		SInt32 baseCount, xCount;
		ExtraContainerChanges::EntryData *entry;
		ExtraDataList *xData;

		for (auto dataIter = invItems.Begin(); !dataIter.End(); ++dataIter)
		{
			item = dataIter.Key();
			baseCount = dataIter.Get().count;
			entry = dataIter.Get().entry;
			if (entry && entry->extendData)
			{
				for (auto xdlIter = entry->extendData->Begin(); !xdlIter.End(); ++xdlIter)
				{
					xData = xdlIter.Get();
					xCount = GetCountForExtraDataList(xData);
					if (xCount < 1) continue;
					if (xCount > baseCount)
						xCount = baseCount;
					baseCount -= xCount;
					m_elements.Append(CreateTempEntry(item, xCount, xData));
					if (!baseCount) break;
				}
			}
			if (baseCount > 0)
				m_elements.Append(CreateTempEntry(item, baseCount, NULL));
		}
	}

	// initialize the iterator
	SetIterator();
}

bool ContainerIterLoop::UnsetIterator()
{
	return m_invRef->WriteRefDataToContainer();
}

bool ContainerIterLoop::SetIterator()
{
	TESObjectREFR* refr = m_invRef->GetRef();
	if (m_iterIndex < m_elements.Size() && refr)
	{
		ExtraContainerChanges::EntryData *entry = m_elements[m_iterIndex];
		ExtraDataList *xData = entry->extendData ? entry->extendData->GetFirstItem() : NULL;
		m_invRef->SetData(IRefData(entry->type, entry, xData));
		*((UInt64*)&m_refVar->data) = refr->refID;
		return true;
	}
	else
	{
		// loop ends, ref will shortly be invalid so zero out the var
		m_refVar->data = 0;
		m_invRef->SetData(IRefData());
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
	for (auto iter = m_elements.Begin(); !iter.End(); ++iter)
	{
		if (iter->extendData)
			GameHeapFree(iter->extendData);
		GameHeapFree(*iter);
	}
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
	savedInfo->stackDepth = state->ifStackDepth;
	memcpy(savedInfo->stack, state->ifStack, (savedInfo->stackDepth + 1) * sizeof(UInt32));

	m_loops.Push(loopInfo);
}

void LoopManager::RestoreStack(ScriptRunner* state, SavedIPInfo* info)
{
	state->ifStackDepth = info->stackDepth;
	memcpy(state->ifStack, info->stack, (info->stackDepth + 1) * sizeof(UInt32));
}

bool LoopManager::Break(ScriptRunner* state, COMMAND_ARGS)
{
	if (!m_loops.Size())
		return false;

	LoopInfo* loopInfo = &m_loops.Top();

	RestoreStack(state, &loopInfo->ipInfo);

	ScriptRunner	* scriptRunner = GetScriptRunner(opcodeOffsetPtr);
	SInt32			* calculatedOpLength = GetCalculatedOpLength(opcodeOffsetPtr);

	// restore ip
	*calculatedOpLength += loopInfo->endIP - *opcodeOffsetPtr;

	delete loopInfo->loop;

	m_loops.Pop();

	return true;
}

bool LoopManager::Continue(ScriptRunner* state, COMMAND_ARGS)
{
	if (!m_loops.Size())
		return false;

	LoopInfo* loopInfo = &m_loops.Top();

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
	{
		bResult = eval.Arg(0)->GetBool();
	}

	return bResult;
}


static SmallObjectsAllocator::LockBasedAllocator<WhileLoop, 8> g_whileLoopAllocator;

void* WhileLoop::operator new(size_t size)
{
	return g_whileLoopAllocator.Allocate();
}

void WhileLoop::operator delete(void* p)
{
	g_whileLoopAllocator.Free(p);
}
