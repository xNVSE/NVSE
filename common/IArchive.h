#pragma once

#include "common/IDataStream.h"
#include "common/IDynamicCreate.h"

#if ENABLE_IDYNAMICCREATE

/**
 *	An object archive
 *	
 *	This class implements reading and instantiating objects from an object archive.
 */
class IArchive
{
	public:
		class iterator;
		friend	iterator;

		IArchive();
		IArchive(IDataStream * inStream);
		~IArchive();

		void	AttachStream(IDataStream * inStream);
		void	Dispose(void);

		iterator	begin(void)	{ return iterator(0, this); }
		iterator	end(void)	{ return iterator(header.numEntries, this); }

		static const UInt32	kFileID = CHAR_CODE(0x00, 'A', 'R', 0x01);
		static const UInt32	kCurrentVersion = VERSION_CODE(1, 0, 0);

	private:
		struct FileHeader
		{
			UInt32	fileID;		// IArchive::kFileID
			UInt32	version;	// IArchive::kCurrentVersion
			UInt32	numEntries;
			UInt32	nameTableOffset;
			UInt32	nameTableLength;
		};

		struct HeaderEntry
		{
			UInt32	typeID;
			UInt32	subID;
			UInt32	dataOffset;
			UInt32	dataLength;
			UInt32	nameOffset;
		};

		void	ReadHeader(void);

		IDataStream	* theStream;

		FileHeader	header;
		HeaderEntry	* entries;

		char		* nameTable;

	public:
		class iterator
		{
			public:
				iterator()										{ idx = 0; owner = NULL; }
				iterator(UInt32 inIdx, IArchive * inArchive)	{ idx = inIdx; owner = inArchive; }
				~iterator()										{ }

				IDynamic *	Instantiate(void);

				UInt32		GetTypeID(void)			{ return GetData()->typeID; }
				UInt32		GetSubID(void)			{ return GetData()->subID; }
				UInt32		GetDataLength(void)		{ return GetData()->dataLength; }
				char *		GetName(void)			{ return &owner->nameTable[GetData()->nameOffset]; }
				void *		GetBuffer(UInt32 * outLength);

				iterator &	operator++() { Next(); return *this; }
				iterator &	operator--() { Prev(); return *this; }

				void		NextOfType(UInt32 typeID);
				void		Next(void)	{ idx++; }

				void		PrevOfType(UInt32 typeID);
				void		Prev(void)	{ idx--; }

			private:
				HeaderEntry *	GetData(void)	{ return &owner->entries[idx]; }

				UInt32		GetDataOffset(void)	{ return GetData()->dataOffset; }

				UInt32		idx;
				IArchive	* owner;
		};
};

#endif
