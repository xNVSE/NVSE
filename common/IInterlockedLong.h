#pragma once

struct IInterlockedLong
{
	public:
		long	Increment(void)	{ return InterlockedIncrement(&value); }
		long	Decrement(void)	{ return InterlockedDecrement(&value); }
		long	Get(void)		{ return value; }
		long	Set(long in)	{ return InterlockedExchange(&value, in); }
		long	TrySetIf(long newValue, long expectedOldValue)
								{ return InterlockedCompareExchange(&value, newValue, expectedOldValue); }

		// interlock variable semantics
		bool	Claim(void)		{ return TrySetIf(1, 0) == 0; }
		bool	Release(void)	{ return TrySetIf(0, 1) == 1; }

	private:
		volatile long	value;
};
