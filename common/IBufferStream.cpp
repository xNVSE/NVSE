#include "IBufferStream.h"

IBufferStream::IBufferStream()
:streamBuf(NULL), flags(0)
{

}

IBufferStream::IBufferStream(const IBufferStream & rhs)
{
	// explicitly not supporting copy constructor for self-owned buffers
	ASSERT((flags & kFlag_OwnedBuf) == 0);
}

IBufferStream::IBufferStream(void * buf, UInt64 inLength)
:streamBuf(NULL), flags(0)
{
	SetBuffer(buf, inLength);
}

IBufferStream::~IBufferStream()
{
	if(flags & kFlag_OwnedBuf)
	{
		delete [] streamBuf;
	}
}

IBufferStream & IBufferStream::operator=(IBufferStream & rhs)
{
	// explicitly not supporting copying for self-owned buffers
	ASSERT((flags & kFlag_OwnedBuf) == 0);

	streamBuf = rhs.streamBuf;
	flags = rhs.flags;

	return *this;
}

void IBufferStream::SetBuffer(void * buf, UInt64 inLength)
{
	streamBuf = (UInt8 *)buf;
	streamLength = inLength;

	Rewind();
}

void IBufferStream::ReadBuf(void * buf, UInt32 inLength)
{
	memcpy(buf, &streamBuf[streamOffset], inLength);
	streamOffset += inLength;
}

void IBufferStream::WriteBuf(const void * buf, UInt32 inLength)
{
	memcpy(&streamBuf[streamOffset], buf, inLength);
	streamOffset += inLength;
}
