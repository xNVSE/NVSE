#pragma once

#include "Serialization.h"

// simple template class used to support NVSE custom data types (strings, arrays, etc)

struct _VarIDs : Set<UInt32>
{
	UInt32 PopFirst()
	{
		UInt32 *keys = Keys(), first = *keys;
		if (--numKeys)
			memmove(keys, keys + 1, numKeys * 4);
		return first;
	}

	UInt32 LastKey() {return Keys()[numKeys - 1];}
};

template <class Var>
class VarMap
{
protected:
#if _DEBUG
	typedef Map<UInt32, Var> _VarMap;
#else
	typedef UnorderedMap<UInt32, Var> _VarMap;
#endif
	class VarCache
	{
		// if desired this can be replaced with an impl that caches more than one var without changing client code
		UInt32		varID;
		Var			* var;

	public:
		VarCache() : varID(0), var(NULL) {}

		~VarCache()
		{ 
			Reset(); 
		}

		void Insert(UInt32 id, Var* v)
		{
			varID = id;
			var = v;
		}

		// clear all cached vars (only one in current impl)
		void Reset()
		{
			varID = 0;
			var = NULL;
		}

		void Remove(UInt32 id)
		{
			if (id == varID)
			{
				Reset();
			}
		}

		Var* Get(UInt32 id)
		{
			return (varID == id) ? var : NULL;
		}
	};

	_VarMap				vars;
	_VarIDs				usedIDs;
	_VarIDs				tempIDs;		// set of IDs of unreferenced vars, makes for easy cleanup
	_VarIDs				availableIDs;	// IDs < greatest used ID available as IDs for new vars
	VarCache			cache;
	CRITICAL_SECTION	cs;				// trying to avoid what looks like concurrency issues


	void SetIDAvailable(UInt32 id)
	{
		if (id) availableIDs.Insert(id);
	}

	UInt32 GetUnusedID()
	{
		UInt32 id = 1;
		::EnterCriticalSection(&cs);

		if (!availableIDs.Empty())
			id = availableIDs.PopFirst();
		else if (!usedIDs.Empty())
			id = usedIDs.LastKey() + 1;
		::LeaveCriticalSection(&cs);
		return id;
	}

public:
	VarMap()
	{
		::InitializeCriticalSection(&cs);
	}

	~VarMap()
	{
		Reset();
		::DeleteCriticalSection(&cs);
	}

	Var* Get(UInt32 varID)
	{
		if (!varID) return NULL;
		Var* var = cache.Get(varID);
		if (!var)
		{
			var = vars.GetPtr(varID);
			if (var)
				cache.Insert(varID, var);
		}
		return var;
	}

	bool VarExists(UInt32 varID)
	{
		return Get(varID) ? true : false;
	}

	template <typename ...Args>
	Var* Insert(UInt32 varID, Args&& ...args)
	{
		::EnterCriticalSection(&cs);
		usedIDs.Insert(varID);
		Var* var = vars.Emplace(varID, std::forward<Args>(args)...);
		::LeaveCriticalSection(&cs);
		return var;
	}

	void Delete(UInt32 varID)
	{
		::EnterCriticalSection(&cs);
		cache.Remove(varID);
		vars.Erase(varID);
		usedIDs.Erase(varID);
		tempIDs.Erase(varID);
		SetIDAvailable(varID);
		::LeaveCriticalSection(&cs);
	}

	static void DeleteBySelf(VarMap* self, UInt32 varID)
	{
		if (self)
			self->Delete(varID);
	}

	void Reset()
	{
		cache.Reset();

		typename _VarMap::Iterator iter;
		while (true)
		{
			iter.Init(vars);
			if (iter.End()) break;
			iter.Remove();
		}

		usedIDs.Clear();
		tempIDs.Clear();
		availableIDs.Clear();
	}


	void MarkTemporary(UInt32 varID, bool bTemporary)
	{
		if (bTemporary)
		{
			tempIDs.Insert(varID);
		}
		else
		{
			tempIDs.Erase(varID);
		}
	}

	bool IsTemporary(UInt32 varID)
	{
		return tempIDs.HasKey(varID);
	}
};