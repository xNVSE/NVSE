#pragma once

class IFIFO
{
	public:
		IFIFO(UInt32 length = 0);
		virtual ~IFIFO();

		virtual bool	Push(UInt8 * buf, UInt32 length);
		virtual bool	Pop(UInt8 * buf, UInt32 length);
		virtual bool	Peek(UInt8 * buf, UInt32 length);
		virtual void	Clear(void);

		UInt32	GetBufferSize(void)		{ return fifoBufSize; }
		UInt32	GetBufferRemain(void)	{ return fifoBufSize - fifoDataLength; }
		UInt32	GetDataLength(void)		{ return fifoDataLength; }

	private:
		UInt32	ToRawOffset(UInt32 in)	{ return in % fifoBufSize; }
		UInt32	ToDataOffset(UInt32 in)	{ return ToRawOffset(fifoBase + in); }
		UInt32	GetWriteOffset(void)	{ return ToDataOffset(fifoDataLength); }

		UInt8	* fifoBuf;
		UInt32	fifoBufSize;		// size of the buffer (in bytes)
		UInt32	fifoBase;			// pointer to the beginning of the data block
		UInt32	fifoDataLength;		// size of the data block
};
