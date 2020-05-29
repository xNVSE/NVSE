template <class DataType>
void IDatabase <DataType>::Save(IDataStream * stream)
{
	stream->Write32(theDataMap.size());
	stream->Write64(newKeyHint);
	
	for(DataMapType::iterator iter = theDataMap.begin(); iter != theDataMap.end(); iter++)
	{
		stream->Write64((*iter).first);
		stream->WriteBuf(&((*iter).second), sizeof(DataType));
	}
}

template <class DataType>
void IDatabase <DataType>::Load(IDataStream * stream)
{
	UInt32	numEntries = stream->Read32();
	newKeyHint = stream->Read64();
	
	theDataMap.clear();
	
	for(UInt32 i = 0; i < numEntries; i++)
	{
		UInt64	key = stream->Read64();
		stream->ReadBuf(&(theDataMap[key]), sizeof(DataType));
	}
}

template <class DataType>
bool IDatabase <DataType>::SaveToFile(char * name)
{
	IFileStream	stream;

	if(stream.Create(name))
	{
		Save(&stream);
		return true;
	}

	return false;
}

template <class DataType>
bool IDatabase <DataType>::LoadFromFile(char * name)
{
	IFileStream	stream;

	if(stream.Open(name))
	{
		Load(&stream);
		return true;
	}

	return false;
}
