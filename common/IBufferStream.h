#pragma once

#include "common/IDataStream.h"

class IBufferStream : public IDataStream
{
	public:
		IBufferStream();
		IBufferStream(const IBufferStream & rhs);
		IBufferStream(void * buf, UInt64 inLength);
		virtual ~IBufferStream();

		IBufferStream &	operator=(IBufferStream & rhs);

		void	SetBuffer(void * buf, UInt64 inLength);
		void *	GetBuffer(void)	{ return streamBuf; }

		void	OwnBuffer(void)		{ flags |= kFlag_OwnedBuf; }
		void	DisownBuffer(void)	{ flags &= ~kFlag_OwnedBuf; }

		// read
		virtual void	ReadBuf(void * buf, UInt32 inLength);

		// write
		virtual void	WriteBuf(const void * buf, UInt32 inLength);

	protected:
		UInt8	* streamBuf;
		UInt32	flags;

		enum
		{
			kFlag_OwnedBuf = 1 << 0
		};
};
