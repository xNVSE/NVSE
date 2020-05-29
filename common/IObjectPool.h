#pragma once

#include "common/ITypes.h"
#include "common/IErrors.h"

#pragma warning(push)
#pragma warning(disable: 4804)

/**
 *	A memory pool of statically allocated objects
 */
template <class T, UInt32 numObjects>
class IObjectPool
{
	public:
				IObjectPool()	{ lastFreed = 0; ASSERT_STR(numObjects > 0, "IObjectPool: bad numObjects"); }
				~IObjectPool()	{ }
		
		//! Get an object from the pool
		T &		Alloc(void)
		{
			UInt32	traverse = lastFreed;
			
			for(UInt32 i = 0; i < numObjects; i++)
			{
				if(!pool[traverse].allocated)
					return pool[traverse].data;
				
				traverse++;
				if(traverse > numObjects)
					traverse = 0;
			}
			
			HALT("IObjectPool::Alloc: couldn't find free entry");
			
			return pool[0].data;
		}
		
		//! Release an object back to the pool
		void	Free(T & in)
		{
			for(UInt32 i = 0; i < numObjects; i++)
			{
				if(pool[i].allocated && (&in == &pool[i].data))
				{
					pool[i].allocated = 0;
					lastFreed = i;
					
					return;
				}
			}
			
			HALT("IObjectPool::Free: object not in list");
		}
	
	private:
		//! Object storage with an "allocated" flag
		struct Pair
		{
			T		data;		//!< the object
			UInt32	allocated;	//!< is this object allocated?
		};
		
		Pair	pool[numObjects];	//!< the object pool
		UInt32	lastFreed;			//!< the last freed object
};

#pragma warning(pop)
