#pragma once

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
