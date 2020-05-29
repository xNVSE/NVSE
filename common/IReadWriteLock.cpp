#include "IReadWriteLock.h"

IReadWriteLock::IReadWriteLock()
{
	readCount.Set(0);
	readBlocker.UnBlock();
	writeBlocker.UnBlock();
}

IReadWriteLock::~IReadWriteLock()
{
	//
}

void IReadWriteLock::StartRead(void)
{
	enterBlocker.Enter();
	readBlocker.Wait();
	if(readCount.Increment() == 1)
		writeBlocker.Block();
	enterBlocker.Leave();
}

void IReadWriteLock::EndRead(void)
{
	if(!readCount.Decrement())
		writeBlocker.UnBlock();
}

void IReadWriteLock::StartWrite(void)
{
	writeMutex.Enter();
	enterBlocker.Enter();
	readBlocker.Block();
	writeBlocker.Wait();
	enterBlocker.Leave();
}

void IReadWriteLock::EndWrite(void)
{
	readBlocker.UnBlock();
	writeMutex.Leave();
}
