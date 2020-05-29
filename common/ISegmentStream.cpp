#include "common/ISegmentStream.h"

ISegmentStream::ISegmentStream()
{
	streamLength = 0;
}

ISegmentStream::~ISegmentStream()
{

}

void ISegmentStream::AttachStream(IDataStream * inStream)
{
	parent = inStream;
	streamLength = 0;
	streamOffset = 0;
}

void ISegmentStream::AddSegment(UInt64 offset, UInt64 length, UInt64 parentOffset)
{
	segmentInfo.push_back(SegmentInfo(offset, length, parentOffset));
	
	if(streamLength < (parentOffset + length))
		streamLength = parentOffset + length;
}

void ISegmentStream::ReadBuf(void * buf, UInt32 inLength)
{
	UInt32	remain = inLength;
	UInt8	* out = (UInt8 *)buf;

	while(remain > 0)
	{
		SegmentInfo	* info = LookupInfo(streamOffset);
		ASSERT(info);

		UInt64		segmentOffset = streamOffset - info->offset;
		UInt64		transferLength = info->length - segmentOffset;

		if(transferLength > remain)
			transferLength = remain;

		parent->SetOffset(info->parentOffset + segmentOffset);
		parent->ReadBuf(out, transferLength);

		streamOffset += transferLength;
		remain -= transferLength;
	}
}

void ISegmentStream::WriteBuf(const void * buf, UInt32 inLength)
{
	HALT("ISegmentStream::WriteBuf: writing unsupported");
}

void ISegmentStream::SetOffset(SInt64 inOffset)
{
	SegmentInfo	* info = LookupInfo(inOffset);
	ASSERT(info);

	UInt64		segmentOffset = inOffset - info->offset;

	parent->SetOffset(info->parentOffset + segmentOffset);

	streamOffset = inOffset;
}

ISegmentStream::SegmentInfo * ISegmentStream::LookupInfo(UInt64 offset)
{
	for(SegmentInfoListType::iterator iter = segmentInfo.begin(); iter != segmentInfo.end(); iter++)
		if((offset >= (*iter).offset) && (offset < (*iter).offset + (*iter).length))
			return &(*iter);

	return NULL;
}
