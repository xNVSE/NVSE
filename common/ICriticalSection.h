#pragma once

#if 0

class ICriticalSection
{
	public:
		ICriticalSection()	{ InitializeCriticalSection(&critSection); }
		~ICriticalSection()	{ DeleteCriticalSection(&critSection); }

		void	Enter(void)		{ EnterCriticalSection(&critSection); }
		void	Leave(void)		{ LeaveCriticalSection(&critSection); }
		bool	TryEnter(void)	{ return TryEnterCriticalSection(&critSection) != 0; }

	private:
		CRITICAL_SECTION	critSection;
};

class ScopedLock
{
public:
	ScopedLock(ICriticalSection& critSection) : m_critSection(critSection)
	{
		m_critSection.Enter();
	}

	~ScopedLock()
	{
		m_critSection.Leave();
	}

private:
	ICriticalSection& m_critSection;
};

#else

class ICriticalSection
{
	DWORD	owningThread;
	DWORD	enterCount;

public:
	ICriticalSection() : owningThread(0), enterCount(0) {}

	void Enter()
	{
		DWORD currThread = GetCurrentThreadId();
		if (owningThread == currThread)
		{
			enterCount++;
			return;
		}
		if (InterlockedCompareExchange(&owningThread, currThread, 0))
		{
			DWORD fastIdx = 10000;
			do {
				Sleep(--fastIdx >> 0x1F);
			} while (InterlockedCompareExchange(&owningThread, currThread, 0));
		}
		enterCount = 1;
	}

	__forceinline void Leave()
	{
		if (!--enterCount)
			owningThread = 0;
	}
};

class ScopedLock
{
	ICriticalSection		*m_cs;

public:
	ScopedLock(ICriticalSection &cs) : m_cs(&cs) {cs.Enter();}
	~ScopedLock() {m_cs->Leave();}
};

#endif