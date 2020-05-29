#pragma once

class IMutex
{
	public:
		static const UInt32	kDefaultTimeout = 1000 * 10;

		IMutex();
		~IMutex();

		bool	Wait(UInt32 timeout = kDefaultTimeout);
		void	Release(void);

	private:
		HANDLE	theMutex;
};
