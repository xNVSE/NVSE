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



ArrayIterLoop::ArrayIterLoop(const ForEachContext* context, Script* script) : m_script(script)
{
	m_srcID = context->sourceID;
	m_iterID = context->iteratorID;
	m_valueIterVar = context->iterVar;

	if (m_iterID)
	{
		// clear the iterator var before initializing it
		g_ArrayMap.RemoveReference(&m_valueIterVar.GetScriptLocal()->data, m_script->GetModIndex());
		g_ArrayMap.AddReference(&m_valueIterVar.GetScriptLocal()->data, m_iterID, m_script->GetModIndex());
	}

	Init();
}

ArrayIterLoop::ArrayIterLoop(ArrayID sourceID, Script* script, Variable valueIterVar, std::optional<Variable> keyIterVar)
	: m_srcID(sourceID), m_script(script), m_valueIterVar(valueIterVar), m_keyIterVar(keyIterVar)
{
	Init();
}

void ArrayIterLoop::Init()
{
	if (m_srcID) [[likely]]
	{
		if (ArrayVar* arr = g_ArrayMap.Get(m_srcID)) [[likely]]
		{
			const ArrayKey* key;
			ArrayElement* elem;
			if (arr->GetFirstElement(&elem, &key))
			{
				m_curKey = *key;
				if (!UpdateIterator(elem)) {		// initialize iterator to first element in array
					m_srcID = 0; // our way of signalling something messed up
				}
			}
		}
		else {
			m_srcID = 0;
		}
	}
}

bool UpdateForEachAltIterator(const ArrayElement* elem, const ArrayKey& curKey,
	const Variable keyIterVar, const Variable valueIterVar, Script* script)
{
	// Assume the types for the keys don't change mid-iteration and that the key iter var was already typechecked at the start.
	if (keyIterVar.IsValid()) {
		if (curKey.KeyType() == kDataType_String) {
			auto id = g_StringMap.Add(script->GetModIndex(), curKey.key.GetStr(), true, nullptr);
			keyIterVar.GetScriptLocal()->data = id;
		}
		else {
			auto num = curKey.key.num;
			if (keyIterVar.GetType() == Script::eVarType_Integer) {
				num = std::floor(num);
			}
			keyIterVar.GetScriptLocal()->data = num;
		}
	}

	if (valueIterVar.IsValid())
	{
		// Try to assign the array's nth value to a variable.
		// If the variable is the wrong type, report error.
		switch (elem->DataType())
		{
		case DataType::kDataType_Array:
		{
			if (valueIterVar.GetType() != Script::eVarType_Array) [[unlikely]] {
				ShowRuntimeError(script, "ForEachAlt >> Received an array-type value, but the value iter variable has type %s.",
					VariableTypeToName(valueIterVar.GetType()));
				return false;
			}
			valueIterVar.GetScriptLocal()->data = elem->m_data.arrID;
			break;
		}
		case DataType::kDataType_String:
		{
			if (valueIterVar.GetType() != Script::eVarType_String) [[unlikely]] {
				ShowRuntimeError(script, "ForEachAlt >> Received a string-type value, but the value iter variable has type %s.",
					VariableTypeToName(valueIterVar.GetType()));
				return false;
			}
			auto id = g_StringMap.Add(script->GetModIndex(), elem->m_data.GetStr(), true, nullptr);
			valueIterVar.GetScriptLocal()->data = id;
			break;
		}
		case DataType::kDataType_Numeric:
		{
			if (valueIterVar.GetType() != Script::eVarType_Float
				&& valueIterVar.GetType() != Script::eVarType_Integer) [[unlikely]]
			{
				ShowRuntimeError(script, "ForEachAlt >> Received a numeric-type value, but the value iter variable has type %s.",
					VariableTypeToName(valueIterVar.GetType()));
				return false;
			}
			auto num = elem->m_data.num;
			if (valueIterVar.GetType() == Script::eVarType_Integer) {
				num = std::floor(num);
			}
			valueIterVar.GetScriptLocal()->data = num;
			break;
		}
		case DataType::kDataType_Form:
		{
			if (valueIterVar.GetType() != Script::eVarType_Ref) [[unlikely]] {
				ShowRuntimeError(script, "ForEachAlt >> Received a form-type value, but the value iter variable has type %s.",
					VariableTypeToName(valueIterVar.GetType()));
				return false;
			}
			*reinterpret_cast<UInt32*>(&valueIterVar.GetScriptLocal()->data) = elem->m_data.formID;
			break;
		}
		default:
			return false;
		}
	}

	return true;
}

bool ArrayIterLoop::UpdateIterator(const ArrayElement* elem)
{
	if (m_iterID)
	{
		ArrayVar* arr = g_ArrayMap.Get(m_iterID);

		// iter["key"] = element key
		ArrayElement* newElem = arr->Get("key", true);
		if (newElem)
		{
			if (m_curKey.KeyType() == kDataType_String)
				newElem->SetString(m_curKey.key.str);
			else newElem->SetNumber(m_curKey.key.num);
		}
		else [[unlikely]] {
			return false;
		}

		// iter["value"] = element data
		newElem = arr->Get("value", true);
		if (newElem) {
			newElem->Set(elem);
		}
		else [[unlikely]] {
			return false;
		}
		return true;
	}
	else { // handle ForEachAlt-style iteration over an array
		const bool isSourceArrayAMap = g_ArrayMap.Get(m_srcID)->GetContainerType() != kContainer_Array;
		Variable valueIterVar{};
		Variable keyIterVar{};
		if (!m_keyIterVar.has_value() && isSourceArrayAMap) {
			keyIterVar = m_valueIterVar;
		}
		else {
			valueIterVar = m_valueIterVar;
			if (m_keyIterVar.has_value()) {
				keyIterVar = *m_keyIterVar;
			}
		}
		return UpdateForEachAltIterator(elem, m_curKey, keyIterVar, valueIterVar, m_script);
	}
}

bool ArrayIterLoop::Update(COMMAND_ARGS)
{
	if (!m_srcID) [[unlikely]] {
		//ShowRuntimeError(scriptObj, "ForEach >> Invalid source array ID");
		return false;
	}
	ArrayVar* arr = g_ArrayMap.Get(m_srcID);
	if (!arr) {
		//ShowRuntimeError(scriptObj, "ForEach >> Invalid source array ID");
		return false;
	}

	ArrayElement *elem;
	const ArrayKey *key;
	if (arr->GetNextElement(&m_curKey, &elem, &key))
	{
		m_curKey = *key;
		return UpdateIterator(elem);	
	}
	return false;
}

ArrayIterLoop::~ArrayIterLoop()
{
	if (m_iterID)
	{
		//g_ArrayMap.RemoveReference(&m_iterID, 0xFF);
		g_ArrayMap.RemoveReference(&m_valueIterVar.GetScriptLocal()->data, m_script->GetModIndex());
	}
	else {
		if (m_valueIterVar.IsValid()) {
			m_valueIterVar.GetScriptLocal()->data = 0;
		}
		if (m_keyIterVar.has_value() && m_keyIterVar->IsValid()) {
			m_keyIterVar->GetScriptLocal()->data = 0;
		}
	}
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
	m_refVar = context->iterVar.GetScriptLocal();
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
			FormHeap_Free(iter->extendData);
		FormHeap_Free(*iter);
	}
	m_invRef->Release();
	m_refVar->data = 0;
}

bool FormListIterLoop::GetNext()
{
	// Go to the next valid value, skipping over what our m_iter was pointing to (should already have been used).
	if (m_iter)
		m_iter = m_iter->next;
	else
		return false;

	while (m_iter)
	{
		if (TESForm* element = m_iter->data)
		{
			*((UInt64*)&m_refVar->data) = element->refID;
			return true;
		}
		m_iter = m_iter->next;
	}
	return false;
}

FormListIterLoop::FormListIterLoop(const ForEachContext *context)
{
	m_iter = ((BGSListForm*)context->sourceID)->list.Head();
	m_refVar = context->iterVar.GetScriptLocal();

	// Move m_iter to first valid value, and save that as the first loop element.
	// Do NOT call GetNext here, since it will move m_iter, 
	// ...which will make the IsEmpty() check mistakenly fail when ForEach is first called on a formlist w/ only 1 element.
	while (m_iter)
	{
		if (TESForm* element = m_iter->data)
		{
			*((UInt64*)&m_refVar->data) = element->refID;
			break;
		}
		m_iter = m_iter->next;
	}
}

bool FormListIterLoop::Update(COMMAND_ARGS)
{
	return GetNext();
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
