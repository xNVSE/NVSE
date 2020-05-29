#pragma once

#include "common/IErrors.h"

/**
 *	An arbitrary data stream
 */
class IDataStream
{
	public:
		IDataStream();
		virtual ~IDataStream();

		// read
		virtual UInt8	Read8(void);
		virtual UInt16	Read16(void);
		virtual UInt32	Read32(void);
		virtual UInt64	Read64(void);
		virtual float	ReadFloat(void);
		virtual UInt32	ReadString(char * buf, UInt32 bufLength, char altTerminator = 0, char altTerminator2 = 0);
		virtual void	ReadBuf(void * buf, UInt32 inLength) = 0;

		// peek
		virtual UInt8	Peek8(void);
		virtual UInt16	Peek16(void);
		virtual UInt32	Peek32(void);
		virtual UInt64	Peek64(void);
		virtual float	PeekFloat(void);
		virtual void	PeekBuf(void * buf, UInt32 inLength);

		virtual void	Skip(SInt64 inBytes);

		// write
		virtual void	Write8(UInt8 inData);
		virtual void	Write16(UInt16 inData);
		virtual void	Write32(UInt32 inData);
		virtual void	Write64(UInt64 inData);
		virtual void	WriteFloat(float inData);
		virtual void	WriteString(const char * buf);
		virtual void	WriteBuf(const void * buf, UInt32 inLength) = 0;
		
		SInt64			GetLength(void);
		SInt64			GetRemain(void);
		SInt64			GetOffset(void);
		bool			HitEOF(void);

		virtual void	SetOffset(SInt64 inOffset);
		void			Rewind(void)	{ SetOffset(0); }

		void			SwapBytes(bool inSwapBytes);

		virtual SInt64			GetParentOffset(void)	{ return GetOffset(); }
		virtual IDataStream *	GetParent(void)			{ return NULL; }

		IDataStream *			GetRootParent(void);

		static void		CopyStreams(IDataStream * out, IDataStream * in, UInt64 bufferSize = 1024 * 1024, UInt8 * buf = NULL);
		static void		CopySubStreams(IDataStream * out, IDataStream * in, UInt64 remain, UInt64 bufferSize = 1024 * 1024, UInt8 * buf = NULL);

	protected:
		SInt64			streamLength;
		SInt64			streamOffset;
		bool			swapBytes;
};

/**
 *	A utility class to automatically save and restore the current position of an IDataStream
 */
class IDataStream_PositionSaver
{
	public:
		IDataStream_PositionSaver(IDataStream * tgt);
		~IDataStream_PositionSaver();

	private:
		IDataStream	* stream;
		SInt64		offset;
};

class IDataSubStream : public IDataStream
{
	public:
		IDataSubStream();
		IDataSubStream(IDataStream * inStream, SInt64 inOffset, SInt64 inLength);
		~IDataSubStream();

		void	Attach(IDataStream * inStream, SInt64 inOffset, SInt64 inLength);

		void	ReadBuf(void * buf, UInt32 inLength);
		void	WriteBuf(const void * buf, UInt32 inLength);
		void	SetOffset(SInt64 inOffset);

		virtual SInt64			GetParentOffset(void)	{ return stream->GetOffset(); }
		virtual IDataStream *	GetParent(void)			{ return stream; }

		SInt64	GetSubBase(void)	{ return subBase; }

	private:
		IDataStream	* stream;

		SInt64	subBase;
};
