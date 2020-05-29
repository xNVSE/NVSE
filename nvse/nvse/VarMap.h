#pragma once

#include <map>
#include <set>
#include "Serialization.h"

// simple template class used to support NVSE custom data types (strings, arrays, etc)

template <class Var>
class VarMap
{
protected:
	typedef std::map<UInt32, Var*>	_VarMap;
	typedef std::set<UInt32>		_VarIDs;

	class VarCache {
		// if desired this can be replaced with an impl that caches more than one var without changing client code
		UInt32		varID;
		Var			* var;

	public:
		VarCache() : varID(0), var(NULL) { }

		~VarCache() { 
			Reset(); 
		}

		void Insert(UInt32 id, Var* v) {
			varID = id;
			var = v;
		}

		// clear all cached vars (only one in current impl)
		void Reset() {
			varID = 0;
			var = NULL;
		}

		void Remove(UInt32 id) {
			if (id == varID) {
				Reset();
			}
		}

		Var* Get(UInt32 id) {
			return (varID == id) ? var : NULL;
		}
	};

	struct	State {
		_VarMap				vars;
		_VarIDs				tempVars;		// set of IDs of unreferenced vars, makes for easy cleanup
		_VarIDs				availableVars;	// IDs < greatest used ID available as IDs for new vars
		VarCache			cache;
		CRITICAL_SECTION	cs;				// trying to avoid what looks like concurrency issues

		State() {
			Init();
		}

		~State() {
			Reset();
		}

		UInt32	GetUnusedID()
		{
			UInt32 id = 1;

			::EnterCriticalSection(&cs);

			try
			{
				if (availableVars.size())
				{
					id = *availableVars.begin();
					availableVars.erase(id);
				}
				else if (vars.size())
				{
					_VarMap::iterator iter = vars.end();
					--iter;
					id = iter->first + 1;
				}
			} catch(...) {}

			::LeaveCriticalSection(&cs);

			return id;
		}

		Var*	Get(UInt32 varID)
		{
			if (varID != 0) {
				Var* var = cache.Get(varID);
				if (var) {
					return var;
				}

				_VarMap::iterator it = vars.find(varID);
				if (it != vars.end())  {
					cache.Insert(varID, it->second);
					return it->second;
				}
			}
			
			return NULL;
		}

		bool	VarExists(UInt32 varID)
		{
			return Get(varID) ? true : false;
		}

		void Insert(UInt32 varID, Var* var)
		{
			::EnterCriticalSection(&cs);

			try
			{
				vars[varID] = var;
			} catch(...) {}

			::LeaveCriticalSection(&cs);

		}

		void	Delete(UInt32 varID)
		{
			::EnterCriticalSection(&cs);

			try
			{
				Var* var = Get(varID);
				if (var)
				{
					cache.Remove(varID);

					delete var;
					vars.erase(varID);
				}
				tempVars.erase(varID);
				SetIDAvailable(varID);
			} catch(...) {}

			::LeaveCriticalSection(&cs);

		}

		void Init()
		{
			::InitializeCriticalSection(&cs);
		}

		void Reset()
		{
			try
			{
				cache.Reset();
				_VarMap::iterator itEnd = vars.end();
				_VarMap::iterator iter = vars.begin();
				_VarMap::iterator toErase = iter;
				while (iter != itEnd)
				{
					delete iter->second;
					toErase = iter;
					++iter;
					vars.erase(toErase);
				}

				vars.clear();
				tempVars.clear();
				availableVars.clear();
			} catch (...) {}

			::DeleteCriticalSection(&cs);
		}

		void	MarkTemporary(UInt32 varID, bool bTemporary)
		{
			if (bTemporary)
				tempVars.insert(varID);
			else
				tempVars.erase(varID);
		}

		bool IsTemporary(UInt32 varID)
		{
			return (tempVars.find(varID) != tempVars.end()) ? true : false;
		}

		void SetIDAvailable(UInt32 id) {
			if (id) {
				availableVars.insert(id);
			}
		}
	};

	State	* m_state;				// currently loaded vars
	State	* m_backupState;		// previously loaded vars, used as restore point in the event a saved game fails to load

	UInt32	GetUnusedID()
	{
		return m_state->GetUnusedID();
	}

	void SetIDAvailable(UInt32 id)
	{
		m_state->SetIDAvailable(id);
	}

public:
	VarMap()
	{
		m_state = new State();
		m_backupState = NULL;
	}

	~VarMap()
	{
		delete m_state;
		delete m_backupState;
	}

	Var*	Get(UInt32 varID)
	{
		return m_state->Get(varID);
	}

	bool VarExists(UInt32 varID)
	{
		return m_state->VarExists(varID);
	}

	void Insert(UInt32 varID, Var* var)
	{
		m_state->Insert(varID, var);
	}

	void Delete(UInt32 varID)
	{
		m_state->Delete(varID);
	}

	static void DeleteBySelf(VarMap* self, UInt32 varID)
	{
		if (self)
			self->Delete(varID);
	}

	void Reset(NVSESerializationInterface* intfc)
	{
		m_state->Reset();
		m_state->Init();
	}

	void Preload()
	{
		m_backupState = m_state;
		m_state = new State();
	}

	void PostLoad(bool bLoadSucceeded) 
	{
		// there is a possibility loading a saved game will fail. If so, restore vars to previous state.
		if (bLoadSucceeded) {
			if (m_backupState) {
				State* cur = m_state;
				m_state = m_backupState;
				m_backupState = NULL;
				delete m_state;
				m_state = cur;
			}
		}
		else {
			delete m_state;
			m_state = m_backupState;
			m_backupState = NULL;
			// if the loading operation failed right after game init (at the main menu), make sure the map is operable
			if (m_state == NULL)
				Preload();
		}
	}

	void	MarkTemporary(UInt32 varID, bool bTemporary)
	{
		m_state->MarkTemporary(varID, bTemporary);
	}

	bool IsTemporary(UInt32 varID)
	{
		return m_state->IsTemporary(varID);
	}
};