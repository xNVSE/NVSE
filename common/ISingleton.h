#pragma once

#include "common/IErrors.h"

#pragma warning(push)
#pragma warning(disable: 4311 4312)

/**
 *	A singleton base class
 *	
 *	Singletons are useful when you have a class that will be instantiated once,
 *	like a global manager.
 */
template <typename T>
class ISingleton
{
	static T * ms_Singleton;

	public:
		ISingleton()
		{
			ASSERT(!ms_Singleton);
			int offset = (int)(T *)1 - (int)(ISingleton <T> *)(T *)1;
			ms_Singleton = (T *)((int)this + offset);
		}

		virtual ~ISingleton()
		{
			ASSERT(ms_Singleton);
			ms_Singleton = 0;
		}

		/**
		 *	Returns the single instance of the derived class
		 */
		static T& GetSingleton(void)
		{
			ASSERT(ms_Singleton);
			return *ms_Singleton;
		}

		/**
		 *	Returns a pointer to the single instance of the derived class
		 */
		static T * GetSingletonPtr(void)
		{
			return ms_Singleton;
		}
};

template <typename T> T * ISingleton <T>::ms_Singleton = 0;

#pragma warning(pop)
