#include "IDataStream.h"

/**** IDataStream *************************************************************/

IDataStream::IDataStream()
:streamLength(0), streamOffset(0), swapBytes(false)
{

}

IDataStream::~IDataStream()
{

}

/**
 *	Reads and returns an 8-bit value from the stream
 */
UInt8 IDataStream::Read8(void)
{
	UInt8	out;

	ReadBuf(&out, sizeof(UInt8));

	return out;
}

/**
 *	Reads and returns a 16-bit value from the stream
 */
UInt16 IDataStream::Read16(void)
{
	UInt16	out;

	ReadBuf(&out, sizeof(UInt16));

	if(swapBytes)
		out = Swap16(out);

	return out;
}

/**
 *	Reads and returns a 32-bit value from the stream
 */
UInt32 IDataStream::Read32(void)
{
	UInt32	out;

	ReadBuf(&out, sizeof(UInt32));

	if(swapBytes)
		out = Swap32(out);

	return out;
}

/**
 *	Reads and returns a 64-bit value from the stream
 */
UInt64 IDataStream::Read64(void)
{
	UInt64	out;

	ReadBuf(&out, sizeof(UInt64));

	if(swapBytes)
		out = Swap64(out);

	return out;
}

/**
 *	Reads and returns a 32-bit floating point value from the stream
 */
float IDataStream::ReadFloat(void)
{
	UInt32	out = Read32();

	return *((float *)&out);
}

/**
 *	Reads a null-or-return-terminated string from the stream
 *	
 *	If the buffer is too small to hold the entire string, it is truncated and
 *	properly terminated.
 *	
 *	@param buf the output buffer
 *	@param bufLength the size of the output buffer
 *	@return the number of characters written to the buffer
 */
UInt32 IDataStream::ReadString(char * buf, UInt32 bufLength, char altTerminator, char altTerminator2)
{
	char	* traverse = buf;
	bool	breakOnReturns = false;

	if((altTerminator == '\n') || (altTerminator2 == '\n'))
		breakOnReturns = true;

	ASSERT_STR(bufLength > 0, "IDataStream::ReadString: zero-sized buffer");

	if(bufLength == 1)
	{
		buf[0] = 0;
		return 0;
	}

	bufLength--;

	for(UInt32 i = 0; i < bufLength; i++)
	{
		UInt8	data = Read8();

		if(breakOnReturns)
		{
			if(data == 0x0D)
			{
				if(Peek8() == 0x0A)
					Skip(1);

				break;
			}
		}

		if(!data || (data == altTerminator) || (data == altTerminator2))
		{
			break;
		}

		*traverse++ = data;
	}

	*traverse++ = 0;

	return traverse - buf - 1;
}

/**
 *	Reads and returns an 8-bit value from the stream without advancing the stream's position
 */
UInt8 IDataStream::Peek8(void)
{
	IDataStream_PositionSaver	saver(this);

	return Read8();
}

/**
 *	Reads and returns a 16-bit value from the stream without advancing the stream's position
 */
UInt16 IDataStream::Peek16(void)
{
	IDataStream_PositionSaver	saver(this);

	return Read16();
}

/**
 *	Reads and returns a 32-bit value from the stream without advancing the stream's position
 */
UInt32 IDataStream::Peek32(void)
{
	IDataStream_PositionSaver	saver(this);

	return Read32();
}

/**
 *	Reads and returns a 32-bit value from the stream without advancing the stream's position
 */
UInt64 IDataStream::Peek64(void)
{
	IDataStream_PositionSaver	saver(this);

	return Read64();
}

/**
 *	Reads and returns a 32-bit floating point value from the stream without advancing the stream's position
 */
float IDataStream::PeekFloat(void)
{
	IDataStream_PositionSaver	saver(this);

	return ReadFloat();
}

/**
 *	Reads raw data into a buffer without advancing the stream's position
 */
void IDataStream::PeekBuf(void * buf, UInt32 inLength)
{
	IDataStream_PositionSaver	saver(this);

	ReadBuf(buf, inLength);
}

/**
 *	Skips a specified number of bytes down the stream
 */
void IDataStream::Skip(SInt64 inBytes)
{
	SetOffset(GetOffset() + inBytes);
}

/**
 *	Writes an 8-bit value to the stream.
 */
void IDataStream::Write8(UInt8 inData)
{
	WriteBuf(&inData, sizeof(UInt8));
}

/**
 *	Writes a 16-bit value to the stream.
 */
void IDataStream::Write16(UInt16 inData)
{
	if(swapBytes)
		inData = Swap16(inData);

	WriteBuf(&inData, sizeof(UInt16));
}

/**
 *	Writes a 32-bit value to the stream.
 */
void IDataStream::Write32(UInt32 inData)
{
	if(swapBytes)
		inData = Swap32(inData);

	WriteBuf(&inData, sizeof(UInt32));
}

/**
 *	Writes a 64-bit value to the stream.
 */
void IDataStream::Write64(UInt64 inData)
{
	if(swapBytes)
		inData = Swap64(inData);

	WriteBuf(&inData, sizeof(UInt64));
}

/**
 *	Writes a 32-bit floating point value to the stream.
 */
void IDataStream::WriteFloat(float inData)
{
	if(swapBytes)
	{
		UInt32	temp = *((UInt32 *)&inData);

		temp = Swap32(temp);

		WriteBuf(&temp, sizeof(UInt32));
	}
	else
	{
		WriteBuf(&inData, sizeof(float));
	}
}

/**
 *	Writes a null-terminated string to the stream.
 */
void IDataStream::WriteString(const char * buf)
{
	WriteBuf(buf, std::strlen(buf) + 1);
}

/**
 *	Returns the length of the stream
 */
SInt64 IDataStream::GetLength(void)
{
	return streamLength;
}

/**
 *	Returns the number of bytes remaining in the stream
 */
SInt64 IDataStream::GetRemain(void)
{
	return streamLength - streamOffset;
}

/**
 *	Returns the current offset into the stream
 */
SInt64 IDataStream::GetOffset(void)
{
	return streamOffset;
}

/**
 *	Returns whether we have reached the end of the stream or not
 */
bool IDataStream::HitEOF(void)
{
	return streamOffset >= streamLength;
}

/**
 *	Moves the current offset into the stream
 */
void IDataStream::SetOffset(SInt64 inOffset)
{
	streamOffset = inOffset;
}

/**
 *	Enables or disables byte swapping for basic data transfers
 */
void IDataStream::SwapBytes(bool inSwapBytes)
{
	swapBytes = inSwapBytes;
}

IDataStream * IDataStream::GetRootParent(void)
{
	IDataStream	* parent = GetParent();

	if(parent)
		return parent->GetRootParent();
	else
		return this;
}

void IDataStream::CopyStreams(IDataStream * out, IDataStream * in, UInt64 bufferSize, UInt8 * buf)
{
	in->Rewind();

	bool	ourBuffer = false;

	if(!buf)
	{
		buf = new UInt8[bufferSize];
		ourBuffer = true;
	}

	UInt64	remain = in->GetLength();

	while(remain > 0)
	{
		UInt64	transferSize = remain;

		if(transferSize > bufferSize)
			transferSize = bufferSize;

		in->ReadBuf(buf, transferSize);
		out->WriteBuf(buf, transferSize);

		remain -= transferSize;
	}

	if(ourBuffer)
		delete [] buf;
}

void IDataStream::CopySubStreams(IDataStream * out, IDataStream * in, UInt64 remain, UInt64 bufferSize, UInt8 * buf)
{
	bool	ourBuffer = false;

	if(!buf)
	{
		buf = new UInt8[bufferSize];
		ourBuffer = true;
	}

	while(remain > 0)
	{
		UInt64	transferSize = remain;

		if(transferSize > bufferSize)
			transferSize = bufferSize;

		in->ReadBuf(buf, transferSize);
		out->WriteBuf(buf, transferSize);

		remain -= transferSize;
	}

	if(ourBuffer)
		delete [] buf;
}

/**** IDataStream_PositionSaver ***********************************************/

/**
 *	The constructor; save the stream's position
 */
IDataStream_PositionSaver::IDataStream_PositionSaver(IDataStream * tgt)
{
	stream = tgt;
	offset = tgt->GetOffset();
}

/**
 *	The destructor; restore the stream's saved position
 */
IDataStream_PositionSaver::~IDataStream_PositionSaver()
{
	stream->SetOffset(offset);
}

/**** IDataSubStream **********************************************************/

IDataSubStream::IDataSubStream()
:stream(NULL), subBase(0)
{
	//
}

IDataSubStream::IDataSubStream(IDataStream * inStream, SInt64 inOffset, SInt64 inLength)
{
	stream = inStream;
	subBase = inOffset;
	streamLength = inLength;

	stream->SetOffset(inOffset);
}

IDataSubStream::~IDataSubStream()
{

}

void IDataSubStream::Attach(IDataStream * inStream, SInt64 inOffset, SInt64 inLength)
{
	stream = inStream;
	subBase = inOffset;
	streamLength = inLength;

	stream->SetOffset(inOffset);
}

void IDataSubStream::ReadBuf(void * buf, UInt32 inLength)
{
	ASSERT_STR(inLength <= GetRemain(), "IDataSubStream::ReadBuf: hit eof");

	if(stream->GetOffset() != subBase + streamOffset)
		stream->SetOffset(subBase + streamOffset);

	stream->ReadBuf(buf, inLength);

	streamOffset += inLength;
}

void IDataSubStream::WriteBuf(const void * buf, UInt32 inLength)
{
	if(stream->GetOffset() != subBase + streamOffset)
		stream->SetOffset(subBase + streamOffset);

	stream->WriteBuf(buf, inLength);

	streamOffset += inLength;

	if(streamLength < streamOffset)
		streamLength = streamOffset;
}

void IDataSubStream::SetOffset(SInt64 inOffset)
{
	stream->SetOffset(subBase + inOffset);
	streamOffset = inOffset;
}
