#include "IMemPool.h"

void Test_IMemPool(void)
{
	IMemPool <UInt32, 3>	pool;

	_DMESSAGE("main: pool test");
	gLog.Indent();

	_DMESSAGE("start");
	pool.Dump();

	UInt32	* data0, * data1, * data2;

	data0 = pool.Allocate();
	_DMESSAGE("alloc0 = %08X", data0);
	pool.Dump();

	data1 = pool.Allocate();
	_DMESSAGE("alloc1 = %08X", data1);
	pool.Dump();

	data2 = pool.Allocate();
	_DMESSAGE("alloc2 = %08X", data2);
	pool.Dump();

	_DMESSAGE("free0 %08X", data0);
	pool.Free(data0);
	pool.Dump();

	data0 = pool.Allocate();
	_DMESSAGE("alloc0 = %08X", data0);
	pool.Dump();

	_DMESSAGE("free2 %08X", data2);
	pool.Free(data2);
	pool.Dump();

	_DMESSAGE("done");
	pool.Dump();

	gLog.Outdent();
}
