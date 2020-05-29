#include "IFIFO.h"

IFIFO::IFIFO(UInt32 length)
{
	fifoBuf = new UInt8[length];
	fifoBufSize = length;
	fifoBase = 0;
	fifoDataLength = 0;
}

IFIFO::~IFIFO()
{
	delete fifoBuf;
}

bool IFIFO::Push(UInt8 * buf, UInt32 length)
{
	// would that overflow the buffer?
	if(length > GetBufferRemain())
		return false;

	UInt32	writeOffset = GetWriteOffset();

	// will this cross the end of the buffer?
	if(writeOffset + length > fifoBufSize)
	{
		UInt32	segmentLength = fifoBufSize - writeOffset;

		std::memcpy(&fifoBuf[writeOffset], buf, segmentLength);
		std::memcpy(fifoBuf, &buf[segmentLength], length - segmentLength);
	}
	else
	{
		std::memcpy(&fifoBuf[writeOffset], buf, length);
	}

	// update pointers
	fifoDataLength += length;

	return true;
}

bool IFIFO::Pop(UInt8 * buf, UInt32 length)
{
	bool	result = Peek(buf, length);

	// update pointers if we were successful
	if(result)
	{
		fifoDataLength -= length;
		fifoBase = ToRawOffset(fifoBase + length);
	}

	return result;
}

bool IFIFO::Peek(UInt8 * buf, UInt32 length)
{
	// would that underflow the buffer?
	if(length > fifoDataLength)
		return false;

	// will this cross the end of the buffer?
	if(fifoBase + length > fifoBufSize)
	{
		UInt32	segmentLength = fifoBufSize - fifoBase;

		std::memcpy(buf, &fifoBuf[fifoBase], segmentLength);
		std::memcpy(&buf[segmentLength], fifoBuf, length - segmentLength);
	}
	else
	{
		std::memcpy(buf, &fifoBuf[fifoBase], length);
	}

	return true;
}

void IFIFO::Clear(void)
{
	fifoDataLength = 0;

	// this isn't needed, but staying away from the buffer end is always good
	fifoBase = 0;
}
