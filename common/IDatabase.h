#pragma once

#include <map>
#include "common/IDataStream.h"
#include "common/IFilestream.h"

template <class DataType>
class IDatabase
{
	public:
		typedef std::map <UInt64, DataType>		DataMapType;
		typedef typename DataMapType::iterator	DataMapIterator;

		static const UInt64	kGUIDMask = 0x0FFFFFFFFFFFFFFF;

		IDatabase()	{ newKeyHint = 1; }
		virtual ~IDatabase()	{ }

		DataType *	Get(UInt64 key)
		{
			key &= kGUIDMask;

			if(!key)
				return NULL;

			DataMapType::iterator	iter = theDataMap.find(key);

			return (iter == theDataMap.end()) ? NULL : &((*iter).second);
		}

		DataType *	Alloc(UInt64 key)
		{
			key &= kGUIDMask;

			if(!key)
				return NULL;

			DataMapType::iterator	iter = theDataMap.find(key);

			return (iter == theDataMap.end()) ? &theDataMap[key] : NULL;
		}

		DataType *	Alloc(UInt64 * key)
		{
			UInt64	newKey = newKeyHint;

			do
			{
				if(!newKey)
					newKey++;

				DataMapType::iterator	iter = theDataMap.find(newKey);

				// is 'newKey' unused?
				if(iter == theDataMap.end())
				{
					*key = newKey;
					newKeyHint = (newKey + 1) & kGUIDMask;
					return &theDataMap[newKey];
				}
				else
				{
					++iter;
					if(iter == theDataMap.end())
					{
						newKey = 1;
					}
					else
					{
						UInt64	nextKey = (newKey + 1) & kGUIDMask;
						if(iter->first != nextKey)
						{
							*key = nextKey;
							newKeyHint = (nextKey + 1) & kGUIDMask;
							return &theDataMap[nextKey];
						}
					}
				}
			}
			while(1);

			*key = 0;

			return NULL;
		}

		void		Delete(UInt64 key)
		{
			if(key)
			{
				key &= kGUIDMask;

				theDataMap.erase(key);

				newKeyHint = key;
			}
		}

		void		Save(IDataStream * stream);
		void		Load(IDataStream * stream);

		bool		SaveToFile(char * name);
		bool		LoadFromFile(char * name);

		DataMapType &	GetData(void)		{ return theDataMap; }

		DataMapIterator	Begin(void)		{ return theDataMap.begin(); }
		DataMapIterator	End(void)		{ return theDataMap.end(); }
		UInt32			Length(void)	{ return theDataMap.size(); }

	private:
		DataMapType	theDataMap;
		UInt64		newKeyHint;
};

#include "common/IDatabase.inc"
