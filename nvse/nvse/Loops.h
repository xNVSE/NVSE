#pragma once

#include "InventoryReference.h"
#include "GameAPI.h"
#include "CommandTable.h"
#include "ArrayVar.h"

#include "Commands_Scripting.h"

#include <stack>
#include <vector>

#include "FastStack.h"

class ScriptRunner;
struct ForEachContext;

// abstract base for Loop classes
class Loop
{
public:
	Loop() { }
	virtual ~Loop() { }

	virtual bool Update(COMMAND_ARGS) = 0;
};

// continues until test expression evaluates false
class WhileLoop : public Loop
{
	UInt32		m_exprOffset;		// offset of test expression in script data
public:
	WhileLoop(UInt32 exprOffset) : m_exprOffset(exprOffset) { }
	virtual ~WhileLoop() { }

	virtual bool Update(COMMAND_ARGS);

	void* operator new(size_t size);
	void operator delete(void* p);
};

// iterates over contents of some collection
class ForEachLoop : public Loop
{
public:
	virtual bool Update(COMMAND_ARGS) = 0;
	virtual bool IsEmpty() = 0;
};

// iterates over elements of an Array
class ArrayIterLoop : public ForEachLoop
{
	ArrayID					m_srcID;
	ArrayID					m_iterID;
	ArrayKey				m_curKey;
	ScriptEventList::Var	*m_iterVar;

	void UpdateIterator(const ArrayElement* elem);
public:
	ArrayIterLoop(const ForEachContext* context, UInt8 modIndex);
	virtual ~ArrayIterLoop();

	virtual bool Update(COMMAND_ARGS);
	bool IsEmpty()
	{
		ArrayVar *arr = g_ArrayMap.Get(m_srcID);
		return !arr || !arr->Size();
	}
};

// iterates over characters in a string
class StringIterLoop : public ForEachLoop
{
	std::string		m_src;
	UInt32			m_curIndex;
	UInt32			m_iterID;

public:
	StringIterLoop(const ForEachContext* context);
	virtual ~StringIterLoop() { }

	virtual bool Update(COMMAND_ARGS);
	bool IsEmpty() { return m_src.length() == 0; }
};

// iterates over contents of a container, creating temporary reference for each item in turn
class ContainerIterLoop : public ForEachLoop
{
	typedef InventoryReference::Data	IRefData;

	InventoryReference							*m_invRef;
	ScriptEventList::Var						*m_refVar;
	UInt32										m_iterIndex;
	Vector<ExtraContainerChanges::EntryData*>	m_elements;

	bool SetIterator();
	bool UnsetIterator();
public:
	ContainerIterLoop(const ForEachContext* context);
	virtual ~ContainerIterLoop();

	virtual bool Update(COMMAND_ARGS);
	virtual bool IsEmpty() { return m_elements.Empty(); }
};

class LoopManager
{
	LoopManager() { }

	struct LoopInfo 
	{
		Loop*		loop;
		SavedIPInfo	ipInfo;		// stack depth, ip of loop start
		UInt32		endIP;		// ip of instruction following loop end
	};

	Stack<LoopInfo>	m_loops;
	
	void RestoreStack(ScriptRunner* state, SavedIPInfo* info);

public:
	static LoopManager* GetSingleton();

	void Add(Loop* loop, ScriptRunner* state, UInt32 startOffset, UInt32 endOffset, COMMAND_ARGS);
	bool Break(ScriptRunner* state, COMMAND_ARGS);
	bool Continue(ScriptRunner* state, COMMAND_ARGS);
};