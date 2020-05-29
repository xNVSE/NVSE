#pragma once

#include "common/IInterlockedLong.h"

class IEvent
{
	public:
		static const UInt32 kDefaultTimeout = 1000 * 10;

		IEvent();
		~IEvent();

		bool	Block(void);
		bool	UnBlock(void);
		bool	Wait(UInt32 timeout = kDefaultTimeout);

		bool	IsBlocked(void)	{ return blockCount.Get() > 0; }

	private:
		HANDLE				theEvent;
		IInterlockedLong	blockCount;
};

class IAutoEvent
{
	public:
		static const UInt32 kDefaultTimeout = 1000 * 10;

		IAutoEvent()	{ ASSERT(theEvent = CreateEvent(NULL, false, true, NULL)); }
		~IAutoEvent()	{ CloseHandle(theEvent); }

		void	Pulse(void)	{ PulseEvent(theEvent); }
		bool	Wait(UInt32 timeout = kDefaultTimeout);

	private:
		HANDLE	theEvent;
};
