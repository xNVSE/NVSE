#include "IMutex.h"

IMutex::IMutex()
{
	theMutex = CreateMutex(NULL, true, NULL);
}

IMutex::~IMutex()
{
	CloseHandle(theMutex);
}

bool IMutex::Wait(UInt32 timeout)
{
	switch(WaitForSingleObject(theMutex, timeout))
	{
		case WAIT_ABANDONED:
			HALT("IMutex::Wait: got abandoned mutex");
			return false;

		case WAIT_OBJECT_0:
			return true;

		default:
		case WAIT_TIMEOUT:
			gLog.FormattedMessage("IMutex::Wait: timeout");
			return false;
	}
}

void IMutex::Release(void)
{
	ASSERT_STR(ReleaseMutex(theMutex), "IMutex::Release: failed to release mutex");
}
