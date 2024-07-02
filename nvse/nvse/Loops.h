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
	Loop() {}
	virtual ~Loop() {}

	virtual bool Update(COMMAND_ARGS) = 0;
};

// continues until test expression evaluates false
class WhileLoop : public Loop
{
	UInt32 m_exprOffset; // offset of test expression in script data
public:
	WhileLoop(UInt32 exprOffset) : m_exprOffset(exprOffset) {}
	virtual ~WhileLoop() {}

	virtual bool Update(COMMAND_ARGS);

	void *operator new(size_t size);
	void operator delete(void *p);
};

// iterates over contents of some collection
class ForEachLoop : public Loop
{
public:
	virtual bool Update(COMMAND_ARGS) = 0;
	// Is executed at the start of the ForEach loop to check if the loop should be breaked early to save time.
	virtual bool IsEmpty() = 0;
};

// iterates over elements of an Array
class ArrayIterLoop : public ForEachLoop
{
	ArrayID m_srcID;
	ArrayID m_iterID{}; // potentially null if passing values directly to variables while iterating.
	ArrayKey m_curKey;
	Script* m_script;

	void Init();
	bool UpdateIterator(const ArrayElement *elem);

public:
	// Either one is optional, or both could be valid at the same time.
	// If m_iterID is not null, then m_keyIterVar is unused and m_valueIterVar will point to the iterator arrayID.
	// If m_iterID is null, then the variables will always be stack variables.
	Variable m_valueIterVar{};
	std::optional<Variable> m_keyIterVar{};

	ArrayIterLoop(const ForEachContext* context, Script* script);
	ArrayIterLoop(ArrayID sourceID, Script* script, Variable valueIterVar, std::optional<Variable> keyIterVar);
	~ArrayIterLoop() override;

	virtual bool Update(COMMAND_ARGS);
	bool IsEmpty()
	{
		if (!m_srcID) [[unlikely]] {
			return true;
		}
		ArrayVar *arr = g_ArrayMap.Get(m_srcID);
		return !arr || !arr->Size();
	}
};

// iterates over characters in a string
class StringIterLoop : public ForEachLoop
{
	std::string m_src;
	UInt32 m_curIndex;
	UInt32 m_iterID{};

public:
	StringIterLoop(const ForEachContext *context);
	virtual ~StringIterLoop() {}

	virtual bool Update(COMMAND_ARGS);
	bool IsEmpty() { return m_src.length() == 0; }
};

// iterates over contents of a container, creating temporary reference for each item in turn
class ContainerIterLoop : public ForEachLoop
{
	typedef InventoryReference::Data IRefData;

	InventoryReference *m_invRef;
	ScriptLocal* m_refVar;
	UInt32 m_iterIndex;
	Vector<ExtraContainerChanges::EntryData *> m_elements;

	bool SetIterator();
	bool UnsetIterator();

public:
	ContainerIterLoop(const ForEachContext *context);
	virtual ~ContainerIterLoop();

	virtual bool Update(COMMAND_ARGS);
	virtual bool IsEmpty() { return m_elements.Empty(); }
};

class FormListIterLoop : public ForEachLoop
{
	ListNode<TESForm>	*m_iter;
	ScriptLocal* m_refVar;

	bool GetNext();

public:
	FormListIterLoop(const ForEachContext *context);
	virtual ~FormListIterLoop() {}

	virtual bool Update(COMMAND_ARGS);
	inline bool IsEmpty() {return !m_iter || !m_iter->data;}
};

class LoopManager
{
	LoopManager() {}

	struct LoopInfo
	{
		Loop *loop;
		SavedIPInfo ipInfo; // stack depth, ip of loop start
		UInt32 endIP;		// ip of instruction following loop end
	};

	Stack<LoopInfo> m_loops;

	void RestoreStack(ScriptRunner *state, SavedIPInfo *info);

public:
	static LoopManager *GetSingleton();

	void Add(Loop *loop, ScriptRunner *state, UInt32 startOffset, UInt32 endOffset, COMMAND_ARGS);
	bool Break(ScriptRunner *state, COMMAND_ARGS);
	bool Continue(ScriptRunner *state, COMMAND_ARGS);
};