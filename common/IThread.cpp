#include "IThread.h"

IThread::IThread()
{
	mainProc = NULL;
	mainProcParam = NULL;
	stopRequested = false;
	isRunning = false;
	theThread = NULL;
	threadID = 0;
}

IThread::~IThread()
{
	ForceStop();

	if(theThread)
	{
		CloseHandle(theThread);
	}
}

void IThread::Start(MainProcPtr proc, void * procParam)
{
	if(!isRunning)
	{
		isRunning = true;
		stopRequested = false;

		mainProc = proc;
		mainProcParam = procParam;

		theThread = CreateThread(NULL, 0, _ThreadProc, static_cast<IThread *>(this), 0, &threadID);
	}
}

void IThread::Stop(void)
{
	if(isRunning)
	{
		stopRequested = true;
	}
}

void IThread::ForceStop(void)
{
	if(isRunning)
	{
		TerminateThread(theThread, 0);

		isRunning = false;
	}
}

UInt32 IThread::_ThreadProc(void * param)
{
	IThread	* _this = (IThread *)param;

	if(_this->mainProc)
		_this->mainProc(_this->mainProcParam);

	_this->isRunning = false;

	return 0;
}
