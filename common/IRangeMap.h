#pragma once

#include <map>

// t_key must be a numeric type
// ### you can't create a range taking up the entire range of t_key
// ### (could be done by switching from start/length -> start/end)
template <typename t_key, typename t_data>
class IRangeMap
{
public:
	struct Entry
	{
		bool	Contains(t_key addr, t_key base)
		{
			return (addr >= base) && (addr <= (base + length - 1));
		}

		t_key	length;
		t_data	data;
	};

	typedef std::map <t_key, Entry>			EntryMapType;
	typedef typename EntryMapType::iterator	Iterator;

	IRangeMap()
	{
		//
	}

	virtual ~IRangeMap()
	{
		//
	}

	void	Clear(void)
	{
		m_entries.clear();
	}

	t_data *	Add(t_key start, t_key length)
	{
		t_data	* result = NULL;
		Entry	* entry = NULL;

		t_key	end = start + length - 1;

		if(end >= start)	// check for overflow ### should also check for overflow on length - 1, but that's pedantic
		{
			// special-case empty lists
			if(m_entries.empty())
			{
				entry = &m_entries[start];
			}
			else
			{
				// collision check

				EntryMapType::iterator	iter = m_entries.lower_bound(start);
				// iter contains the first entry at or after start (or null)

				if(iter == m_entries.begin())
				{
					// there can't be anything before this entry
					// so we only need to check if it's colliding with us

					if(iter->first > end)
					{
						// can't provide a hint because we're inserting at the top
						entry = &m_entries[start];
					}
				}
				else
				{
					// see if this entry doesn't collide
					// can be null (null entries don't collide)
					if((iter == m_entries.end()) || (iter->first > end))
					{
						// we didn't get the first entry in the map
						// and there is at least one entry in the map
						// therefore there's an entry before iter
						EntryMapType::iterator	preIter = iter;
						preIter--;

						// check if this collides
						// guaranteed to be the first entry before start
						t_key	preEnd = preIter->first + preIter->second.length - 1;

						if(preEnd < start)
						{
							// cool, everything's fine, allocate it
							EntryMapType::iterator	newEntry = m_entries.insert(preIter, EntryMapType::value_type(start, Entry()));
							entry = &newEntry->second;
						}
					}
				}
			}
		}

		// set up the entry
		if(entry)
		{
			entry->length = length;

			result = &entry->data;
		}

		return result;
	}

	t_data *	Lookup(t_key addr, t_key * base = NULL, t_key * length = NULL)
	{
		t_data	* result = NULL;

		EntryMapType::iterator	iter = LookupIter(addr);
		if(iter != m_entries.end())
		{
			if(base) *base = iter->first;
			if(length) *length = iter->second.length;

			result = &iter->second.data;
		}

		return result;
	}

	bool	Erase(t_key addr, t_key * base = NULL, t_key * length = NULL)
	{
		bool result = false;

		EntryMapType::iterator	iter = LookupIter(addr);
		if(iter != m_entries.end())
		{
			if(base) *base = iter->first;
			if(length) *length = iter->second.length;

			m_entries.erase(iter);

			result = true;
		}

		return result;
	}

	t_key	GetDataRangeLength(t_data * data)
	{
		Entry	* entry = reinterpret_cast <Entry *>(reinterpret_cast <UInt8 *>(data) - offsetof(Entry, data));

		return entry->length;
	}

	typename EntryMapType::iterator	LookupIter(t_key addr)
	{
		EntryMapType::iterator	result = m_entries.end();

		if(!m_entries.empty())
		{
			// we need to find the last entry less than or equal to addr

			// find the first entry not less than addr
			EntryMapType::iterator	iter = m_entries.lower_bound(addr);

			// iter is either equal to addr, greater than addr, or the end
			if(iter == m_entries.end())
			{
				// iter is the end
				// can only be in the entry before this
				// which does exist because map isn't empty
				--iter;

				if(iter->second.Contains(addr, iter->first))
				{
					result = iter;
				}
			}
			// at this point iter must be valid
			else if(iter->first > addr)
			{
				// iter is greater than addr
				// can only be in the entry before this
				// but there may not be an entry before this

				if(iter != m_entries.begin())
				{
					--iter;

					if(iter->second.Contains(addr, iter->first))
					{
						result = iter;
					}
				}
			}
			else
			{
				// iter is equal to addr and matches
				result = iter;
			}
		}

		return result;
	}

	typename EntryMapType::iterator	Begin(void)
	{
		return m_entries.begin();
	}

	typename EntryMapType::iterator	End(void)
	{
		return m_entries.end();
	}

private:
	EntryMapType	m_entries;
};
