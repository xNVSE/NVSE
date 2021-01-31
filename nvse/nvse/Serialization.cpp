#include "Serialization.h"
#include "Core_Serialization.h"
#include "common/IFileStream.h"
#include "PluginManager.h"
#include "GameAPI.h"
#include <vector>
//#include "EventManager.h"

// ### TODO: only create save file when something has registered a handler

namespace Serialization
{

static UInt32	kNvseOpcodeBase = 0x1400;
static std::string	g_savePath;

// file format internals

//	general format:
//	Header			header
//		PluginHeader	plugin[header.numPlugins]
//			ChunkHeader		chunk[plugin.numChunks]
//				UInt8			data[chunk.length]

struct Header
{
	enum
	{
		kSignature =		MACRO_SWAP32('NVSE'),	// endian-swapping so the order matches
		kVersion =			1,

		kVersion_Invalid =	0
	};

	UInt32	signature;
	UInt32	formatVersion;
	UInt16	nvseVersion;
	UInt16	nvseMinorVersion;
	UInt32	falloutVersion;
	UInt32	numPlugins;
};

struct PluginHeader
{
	UInt32	opcodeBase;
	UInt32	numChunks;
	UInt32	length;		// length of following data including ChunkHeader
};

struct ChunkHeader
{
	UInt32	type;
	UInt32	version;
	UInt32	length;
};

// locals

SerializationTask s_serializationTask;

typedef std::vector <PluginCallbacks>	PluginCallbackList;
PluginCallbackList	s_pluginCallbacks;

PluginHandle	s_currentPlugin = 0;

Header			s_fileHeader = { 0 };

UInt32			s_pluginHeaderOffset = 0;
PluginHeader	s_pluginHeader = { 0 };

bool			s_chunkOpen = false;
bool			ignoreNextChunk = false;	// if true do not complain the plugin left data behind (it ws preloaded)
UInt32			s_chunkHeaderOffset = 0;
ChunkHeader		s_chunkHeader = { 0 };

bool			s_preloading = false;		// if true, we are reading co-save *before* savegame begins to load

// utilities

// change *.fos -> *.nvse
static std::string ConvertSaveFileName(std::string name)
{
	// name not passed by const reference so we can modify it temporarily

	std::string	result;

	// save off all of the ".bak" suffixes
	std::string	bakSuffix;

	int	firstDotBak = name.find(".bak");
	if(firstDotBak != -1)
	{
		bakSuffix = name.substr(firstDotBak);
		name = name.substr(0, firstDotBak);
	}

	// change extension to .nvse
	int	lastPeriod = name.rfind('.');

	if(lastPeriod == -1)
		result = name;
	else
		result = name.substr(0, lastPeriod);

	result += ".nvse";

	// readd the ".bak" suffixes
	result += bakSuffix;

	// if we don't have a path add the save game path (Delete and rename provides the full path);
	if (std::string::npos == result.rfind('\\'))
		result = GetSavegamePath() + result;

	return result;
}

// plugin API
void SetSaveCallback(PluginHandle plugin, NVSESerializationInterface::EventCallback callback)
{
	ASSERT(plugin);
	ASSERT(plugin <= g_pluginManager.GetNumPlugins());

	InternalSetSaveCallback(plugin, callback);
}

void SetLoadCallback(PluginHandle plugin, NVSESerializationInterface::EventCallback callback)
{
	ASSERT(plugin);
	ASSERT(plugin <= g_pluginManager.GetNumPlugins());

	InternalSetLoadCallback(plugin, callback);
}

void SetNewGameCallback(PluginHandle plugin, NVSESerializationInterface::EventCallback callback)
{
	ASSERT(plugin);
	ASSERT(plugin <= g_pluginManager.GetNumPlugins());

	InternalSetNewGameCallback(plugin, callback);
}

void SetPreLoadCallback(PluginHandle plugin, NVSESerializationInterface::EventCallback callback)
{
	ASSERT(plugin);
	ASSERT(plugin <= g_pluginManager.GetNumPlugins());

	InternalSetPreLoadCallback(plugin, callback);
}

void InternalSetSaveCallback(PluginHandle plugin, NVSESerializationInterface::EventCallback callback)
{
	if(plugin >= s_pluginCallbacks.size())
		s_pluginCallbacks.resize(plugin + 1);

	s_pluginCallbacks[plugin].save = callback;
}

void InternalSetLoadCallback(PluginHandle plugin, NVSESerializationInterface::EventCallback callback)
{
	if(plugin >= s_pluginCallbacks.size())
		s_pluginCallbacks.resize(plugin + 1);

	s_pluginCallbacks[plugin].load = callback;
}

void InternalSetNewGameCallback(PluginHandle plugin, NVSESerializationInterface::EventCallback callback)
{
	if(plugin >= s_pluginCallbacks.size())
		s_pluginCallbacks.resize(plugin + 1);

	s_pluginCallbacks[plugin].newGame = callback;
}

void InternalSetPreLoadCallback(PluginHandle plugin, NVSESerializationInterface::EventCallback callback)
{
	if(plugin >= s_pluginCallbacks.size())
		s_pluginCallbacks.resize(plugin + 1);

	s_pluginCallbacks[plugin].preLoad = callback;
}

//==========================================================================

#define SERIALIZATION_BUFFER_SIZE 0x400000

alignas(16) static UInt8 s_serializationBuffer[SERIALIZATION_BUFFER_SIZE];

void SerializationTask::Reset()
{
	bufferPtr = s_serializationBuffer;
	length = 0;
}

bool SerializationTask::Save()
{
	if (!length) return false;

	HANDLE saveFile = CreateFile(g_savePath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (saveFile == INVALID_HANDLE_VALUE)
	{
		_ERROR("HandleSaveGame: couldn't create save file (%s)", g_savePath.c_str());
		return false;
	}

	WriteFile(saveFile, s_serializationBuffer, length, &length, NULL);
	CloseHandle(saveFile);

	return true;
}

bool SerializationTask::Load()
{
	Reset();

	HANDLE saveFile = CreateFile(g_savePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (saveFile == INVALID_HANDLE_VALUE)
		return false;

	ReadFile(saveFile, s_serializationBuffer, SERIALIZATION_BUFFER_SIZE, &length, NULL);
	CloseHandle(saveFile);

	if (length == SERIALIZATION_BUFFER_SIZE)
	{
		_ERROR("HandleLoadGame: co-save file exceeds 4MB!");
		ShowErrorMessageBox("NVSE cosave failed to load as it exceeded 4MB. Please ping the devs on the xNVSE Discord server about this matter.");
		return false;
	}

	return length > 0;
}

UInt32 SerializationTask::GetOffset() const
{
	return (UInt32)(bufferPtr - s_serializationBuffer);
}

void SerializationTask::SetOffset(UInt32 offset)
{
	bufferPtr = s_serializationBuffer + offset;
}

void SerializationTask::Skip(UInt32 size)
{
	bufferPtr += size;
}

void SerializationTask::Write8(UInt8 inData)
{
	*bufferPtr++ = inData;
	length++;
}

void SerializationTask::Write16(UInt16 inData)
{
	*(UInt16*)bufferPtr = inData;
	bufferPtr += 2;
	length += 2;
}

void SerializationTask::Write32(UInt32 inData)
{
	*(UInt32*)bufferPtr = inData;
	bufferPtr += 4;
	length += 4;
}

void SerializationTask::Write64(const void *inData)
{
	*(double*)bufferPtr = *(double*)inData;
	bufferPtr += 8;
	length += 8;
}

void SerializationTask::WriteBuf(const void *inData, UInt32 size)
{
	switch (size)
	{
		case 0:
			return;
		case 1:
			*bufferPtr = *(UInt8*)inData;
			break;
		case 2:
			*(UInt16*)bufferPtr = *(UInt16*)inData;
			break;
		case 3:
		case 4:
			*(UInt32*)bufferPtr = *(UInt32*)inData;
			break;
		case 8:
			*(double*)bufferPtr = *(double*)inData;
			break;
		default:
			memcpy(bufferPtr, inData, size);
			break;
	}
	bufferPtr += size;
	length += size;
}

UInt8 SerializationTask::Read8()
{
	UInt8 result = *bufferPtr;
	bufferPtr++;
	return result;
}

UInt16 SerializationTask::Read16()
{
	UInt16 result = *(UInt16*)bufferPtr;
	bufferPtr += 2;
	return result;
}

UInt32 SerializationTask::Read32()
{
	UInt32 result = *(UInt32*)bufferPtr;
	bufferPtr += 4;
	return result;
}

void SerializationTask::Read64(void *outData)
{
	*(double*)outData = *(double*)bufferPtr;
	bufferPtr += 8;
}

void SerializationTask::ReadBuf(void *outData, UInt32 size)
{
	switch (size)
	{
		case 0:
			return;
		case 1:
			*(UInt8*)outData = *bufferPtr;
			break;
		case 2:
			*(UInt16*)outData = *(UInt16*)bufferPtr;
			break;
		case 3:
		case 4:
			*(UInt32*)outData = *(UInt32*)bufferPtr;
			break;
		case 8:
			*(double*)outData = *(double*)bufferPtr;
			break;
		default:
			memcpy(outData, bufferPtr, size);
			break;
	}
	bufferPtr += size;
}

void SerializationTask::PeekBuf(void *outData, UInt32 size)
{
	switch (size)
	{
		case 0:
			return;
		case 1:
			*(UInt8*)outData = *bufferPtr;
			break;
		case 2:
			*(UInt16*)outData = *(UInt16*)bufferPtr;
			break;
		case 3:
		case 4:
			*(UInt32*)outData = *(UInt32*)bufferPtr;
			break;
		default:
			memcpy(outData, bufferPtr, size);
			break;
	}
}

//==========================================================================

bool WriteRecord(UInt32 type, UInt32 version, const void * buf, UInt32 length)
{
	if (!OpenRecord(type, version)) return false;

	return WriteRecordData(buf, length);
}

// flush a chunk header to the file if one is currently open
static void FlushWriteChunk(void)
{
	if (!s_chunkOpen)
		return;

	UInt32 curOffset = s_serializationTask.GetOffset();
	UInt32 chunkSize = curOffset - s_chunkHeaderOffset - sizeof(s_chunkHeader);

	ASSERT(chunkSize < 0x80000000);	// stupidity check

	s_chunkHeader.length = chunkSize;

	s_serializationTask.SetOffset(s_chunkHeaderOffset);
	s_serializationTask.WriteBuf(&s_chunkHeader, sizeof(s_chunkHeader));

	s_serializationTask.SetOffset(curOffset);

	s_pluginHeader.length += chunkSize + sizeof(s_chunkHeader);

	s_chunkOpen = false;
}

bool OpenRecord(UInt32 type, UInt32 version)
{
	if(!s_pluginHeader.numChunks)
	{
		ASSERT(!s_chunkOpen);

		s_pluginHeaderOffset = s_serializationTask.GetOffset();
		s_serializationTask.Skip(sizeof(s_pluginHeader));
	}

	FlushWriteChunk();

	s_chunkHeaderOffset = s_serializationTask.GetOffset();
	s_serializationTask.Skip(sizeof(s_chunkHeader));

	s_pluginHeader.numChunks++;

	s_chunkHeader.type = type;
	s_chunkHeader.version = version;
	s_chunkHeader.length = 0;

	s_chunkOpen = true;

	return true;
}

bool WriteRecordData(const void * buf, UInt32 length)
{
	s_serializationTask.WriteBuf(buf, length);

	return true;
}

void WriteRecord8(UInt8 inData)
{
	s_serializationTask.Write8(inData);
}

void WriteRecord16(UInt16 inData)
{
	s_serializationTask.Write16(inData);
}

void WriteRecord32(UInt32 inData)
{
	s_serializationTask.Write32(inData);
}

void WriteRecord64(const void *inData)
{
	s_serializationTask.Write64(inData);
}

static void FlushReadRecord(void)
{
	if(s_chunkOpen)
	{
		if(s_chunkHeader.length)
		{
			if (!ignoreNextChunk)
				_WARNING("plugin didn't finish reading chunk");
			s_serializationTask.Skip(s_chunkHeader.length);
		}

		s_chunkOpen = false;
	}
}

bool GetNextRecordInfo(UInt32 * type, UInt32 * version, UInt32 * length)
{
	FlushReadRecord();

	if(!s_pluginHeader.numChunks)
		return false;

	s_pluginHeader.numChunks--;

	s_serializationTask.ReadBuf(&s_chunkHeader, sizeof(s_chunkHeader));

	*type =		s_chunkHeader.type;
	*version =	s_chunkHeader.version;
	*length =	s_chunkHeader.length;

	s_chunkOpen = true;

	return true;
}

UInt32 ReadRecordData(void * buf, UInt32 length)
{
	ASSERT(s_chunkOpen);

	if(length > s_chunkHeader.length)
		length = s_chunkHeader.length;

	s_serializationTask.ReadBuf(buf, length);

	s_chunkHeader.length -= length;

	return length;
}

UInt8 ReadRecord8()
{
	ASSERT(s_chunkOpen);

	if (s_chunkHeader.length == 0)
		return 0;

	s_chunkHeader.length--;

	return s_serializationTask.Read8();
}

UInt16 ReadRecord16()
{
	ASSERT(s_chunkOpen);

	if (s_chunkHeader.length < 2)
		return 0;

	s_chunkHeader.length -= 2;

	return s_serializationTask.Read16();
}

UInt32 ReadRecord32()
{
	ASSERT(s_chunkOpen);

	if (s_chunkHeader.length < 4)
		return 0;

	s_chunkHeader.length -= 4;

	return s_serializationTask.Read32();
}

void ReadRecord64(void *outData)
{
	ASSERT(s_chunkOpen);

	if (s_chunkHeader.length < 8)
		return;

	s_chunkHeader.length -= 8;

	s_serializationTask.Read64(outData);
}

void SkipNBytes(UInt32 byteNum)
{
	ASSERT(s_chunkOpen);

	if (!byteNum) return;

	if (byteNum > s_chunkHeader.length)
		byteNum = s_chunkHeader.length;

	s_serializationTask.Skip(byteNum);

	s_chunkHeader.length -= byteNum;
}

UInt32 PeekRecordData(void * buf, UInt32 length)
{
	ASSERT(s_chunkOpen);

	if(length > s_chunkHeader.length)
		length = s_chunkHeader.length;

	s_serializationTask.PeekBuf(buf, length);

	return length;
}

bool ResolveRefID(UInt32 refID, UInt32 * outRefID)
{
	UInt8 modID = refID >> 24;

	// pass dynamic ids straight through
	if (modID == 0xFF)
	{
		*outRefID = refID;
		return true;
	}

	UInt8 loadedModID = 0xFF;
	if (modID >= s_numPreloadMods)
		return false;

	loadedModID = ResolveModIndexForPreload(modID);
	if (loadedModID == 0xFF) 
		return false;	// unloaded

	// fixup ID, success
	*outRefID = (loadedModID << 24) | (refID & 0x00FFFFFF);

	return true;
}

const char * GetSavePath()
{
	return g_savePath.c_str();
}

// internal event handlers
void HandleSaveGame(const char * path)
{
	// pass file path to plugins registered as listeners
	PluginManager::Dispatch_Message(0, NVSEMessagingInterface::kMessage_SaveGame, (void*)path, strlen(path), NULL);
	// handled by Dispatch_Message EventManager::HandleNVSEMessage(NVSEMessagingInterface::kMessage_SaveGame, (void*)path);
	g_savePath = ConvertSaveFileName(path);

	_MESSAGE("saving to %s", g_savePath.c_str());

	s_serializationTask.Reset();

	try
	{
		// init header
		s_fileHeader.signature =		Header::kSignature;
		s_fileHeader.formatVersion =	Header::kVersion;
		s_fileHeader.nvseVersion =		NVSE_VERSION_INTEGER;
		s_fileHeader.nvseMinorVersion =	NVSE_VERSION_INTEGER_MINOR;
		s_fileHeader.falloutVersion =	RUNTIME_VERSION;
		s_fileHeader.numPlugins =		0;

		s_serializationTask.Skip(sizeof(s_fileHeader));

		// iterate through plugins
		_MESSAGE("saving %d plugins to %s", s_pluginCallbacks.size(), g_savePath.c_str());
		for (UInt32 i = 0; i < s_pluginCallbacks.size(); i++)
		{
			if(s_pluginCallbacks[i].save)
			{
				// set up header info
				s_currentPlugin = i;

				s_pluginHeader.opcodeBase = i ? g_pluginManager.GetBaseOpcode(i - 1) : kNvseOpcodeBase;
				s_pluginHeader.numChunks = 0;
				s_pluginHeader.length = 0;

				if(!s_pluginHeader.opcodeBase)
				{
					_ERROR("HandleSaveGame: plugin with default opcode base registered for serialization");
					continue;
				}

				s_chunkOpen = false;

				// call the plugin
				s_pluginCallbacks[i].save(NULL);

				// flush the remaining chunk data
				FlushWriteChunk();

				if(s_pluginHeader.numChunks)
				{
					UInt32 curOffset = s_serializationTask.GetOffset();

					s_serializationTask.SetOffset(s_pluginHeaderOffset);
					s_serializationTask.WriteBuf(&s_pluginHeader, sizeof(s_pluginHeader));

					s_serializationTask.SetOffset(curOffset);

					s_fileHeader.numPlugins++;
				}
			}
		}

		// write header
		s_serializationTask.SetOffset(0);
		s_serializationTask.WriteBuf(&s_fileHeader, sizeof(s_fileHeader));

		s_serializationTask.Save();
	}
	catch(...)
	{
		_ERROR("HandleSaveGame: exception during save");
	}
}

void HandlePreLoadGame(const char * path)
{
	PluginManager::Dispatch_Message(0, NVSEMessagingInterface::kMessage_PreLoadGame, (void*)path, strlen(path), NULL);

	s_preloading = true;
	HandleLoadGame(path, &PluginCallbacks::preLoad);
	s_preloading = false;
}

void HandlePostLoadGame(bool bLoadSucceeded)
{
	// plugins don't register a callback for this event, but internally we need to do some work based on
	// whether or not the game successfully loaded

	// inform plugins
	PluginManager::Dispatch_Message(0, NVSEMessagingInterface::kMessage_PostLoadGame, (void*)bLoadSucceeded, 1, NULL);
}

void HandleLoadGame(const char * path, NVSESerializationInterface::EventCallback PluginCallbacks::* callback)
{
	// pass file path to plugins registered as listeners
	if (!s_preloading) {
		PluginManager::Dispatch_Message(0, NVSEMessagingInterface::kMessage_LoadGame, (void*)path, strlen(path), NULL);
	}

	g_savePath = ConvertSaveFileName(path);

#if _DEBUG
	_MESSAGE("loading from %s", g_savePath.c_str());
#endif

	if (!s_serializationTask.Load())
	{
#if _DEBUG
		// ### this message is disabled because stupid users keep thinking it's an error
		_MESSAGE("HandleLoadGame: couldn't open file (%s), probably doesn't exist", g_savePath.c_str());
#endif
		if (!s_preloading) {
			HandleNewGame();	// treat this as a new game
		}

		return;
	}
	else
	{
		try
		{
			Header header;

			s_serializationTask.ReadBuf(&header, sizeof(header));

			if (header.signature != Header::kSignature)
			{
				_ERROR("HandleLoadGame: invalid file signature (found %08X expected %08X)", header.signature, Header::kSignature);
				return;
			}

			if (header.formatVersion <= Header::kVersion_Invalid)
			{
				_ERROR("HandleLoadGame: version invalid (%08X)", header.formatVersion);
				return;
			}

			if (header.formatVersion > Header::kVersion)
			{
				_ERROR("HandleLoadGame: version too new (found %08X current %08X)", header.formatVersion, Header::kVersion);
				return;
			}
			
			// no older versions to handle

			// reset flags
			for (PluginCallbackList::iterator iter = s_pluginCallbacks.begin(); iter != s_pluginCallbacks.end(); ++iter)
				iter->hadData = false;
			
			NVSESerializationInterface::EventCallback curCallback = NULL;
			// iterate through plugin data chunks
			while (s_serializationTask.GetRemain() >= sizeof(PluginHeader))
			{
				s_serializationTask.ReadBuf(&s_pluginHeader, sizeof(s_pluginHeader));

				UInt32 pluginChunkStart = s_serializationTask.GetOffset();

				// find the corresponding plugin
				UInt32 pluginIdx = (s_pluginHeader.opcodeBase == kNvseOpcodeBase) ? 0 : g_pluginManager.LookupHandleFromBaseOpcode(s_pluginHeader.opcodeBase);
				if (pluginIdx != kPluginHandle_Invalid)
				{
					s_pluginCallbacks[pluginIdx].hadData = true;

					if (s_pluginCallbacks[pluginIdx].*callback)
					{
						s_chunkOpen = false;
						curCallback = s_pluginCallbacks[pluginIdx].*callback;
						curCallback((void*)path);
					}
					else
					{
						// ### wtf?
						_WARNING("plugin has data in save file but no handler");

						s_serializationTask.Skip(s_pluginHeader.length);
					}
				}
				else
				{
					// ### TODO: save the data temporarily?
					_WARNING("data in save file for plugin, but plugin isn't loaded");

					s_serializationTask.Skip(s_pluginHeader.length);
				}

				UInt32 expectedOffset = pluginChunkStart + s_pluginHeader.length;
				if (s_serializationTask.GetOffset() != expectedOffset)
				{
					_WARNING("plugin did not read all of its data (at %016I64X expected %016I64X)", s_serializationTask.GetOffset(), expectedOffset);
					s_serializationTask.SetOffset(expectedOffset);
				}
			}

			// call load callback for plugins that didn't have data in the file
			for (PluginCallbackList::iterator iter = s_pluginCallbacks.begin(); iter != s_pluginCallbacks.end(); ++iter)
			{
				if (!iter->hadData && (*iter).*callback)
				{
					s_pluginHeader.numChunks = 0;
					s_chunkOpen = false;
					curCallback = (*iter).*callback;
					curCallback(NULL);
				}
			}
		}
		catch(...)
		{
			_ERROR("HandleLoadGame: exception during load");

			// ### this could be handled better, individually catch around each plugin so one plugin can't mess things up for everyone else
			if (!s_preloading) {
				HandleNewGame();
			}
		}
	}
}

void GetSaveName(std::string *saveName, const char * path)
{
  char fname[_MAX_FNAME];
  char ext[_MAX_EXT];

  _splitpath_s(path, NULL, 0, NULL, 0, fname, _MAX_FNAME, ext, _MAX_EXT);
  saveName->assign(fname);
  saveName->append(ext); // ext already has the period/dot.
};

void HandleDeleteGame(const char * path)
{
	std::string	savePath = ConvertSaveFileName(path);
	std::string saveName;
	GetSaveName(&saveName, path);

	_MESSAGE("deleting %s", savePath.c_str());
	PluginManager::Dispatch_Message(0, NVSEMessagingInterface::kMessage_DeleteGame, (void*)savePath.c_str(), strlen(savePath.c_str()), NULL);
	PluginManager::Dispatch_Message(0, NVSEMessagingInterface::kMessage_DeleteGameName, (void*)saveName.c_str(), strlen(saveName.c_str()), NULL);

	DeleteFile(savePath.c_str());
}

void HandleRenameGame(const char * oldPath, const char * newPath)
{
	std::string	oldSavePath = ConvertSaveFileName(oldPath);
	std::string	newSavePath = ConvertSaveFileName(newPath);
	std::string oldSaveName;
	std::string newSaveName;

	GetSaveName(&oldSaveName, oldPath);
	GetSaveName(&newSaveName, newPath);

	_MESSAGE("renaming %s -> %s", oldSavePath.c_str(), newSavePath.c_str());
	PluginManager::Dispatch_Message(0, NVSEMessagingInterface::kMessage_RenameGame, (void*)oldSavePath.c_str(), strlen(oldSavePath.c_str()), NULL);
	PluginManager::Dispatch_Message(0, NVSEMessagingInterface::kMessage_RenameGameName, (void*)oldSavePath.c_str(), strlen(oldSavePath.c_str()), NULL);
	PluginManager::Dispatch_Message(0, NVSEMessagingInterface::kMessage_RenameNewGame, (void*)newSavePath.c_str(), strlen(newSavePath.c_str()), NULL);
	PluginManager::Dispatch_Message(0, NVSEMessagingInterface::kMessage_RenameNewGameName, (void*)newSavePath.c_str(), strlen(newSavePath.c_str()), NULL);

	DeleteFile(newSavePath.c_str());
	rename(oldSavePath.c_str(), newSavePath.c_str());
}

void HandleNewGame(void)
{
	PluginManager::Dispatch_Message(0, NVSEMessagingInterface::kMessage_NewGame, NULL, 0, NULL);
	// iterate through plugins
	for(UInt32 i = 0; i < s_pluginCallbacks.size(); i++)
	{
		if(s_pluginCallbacks[i].newGame)
		{
			s_pluginCallbacks[i].newGame(NULL);
		}
	}
}

}

NVSESerializationInterface	g_NVSESerializationInterface =
{
	NVSESerializationInterface::kVersion,

	Serialization::SetSaveCallback,
	Serialization::SetLoadCallback,
	Serialization::SetNewGameCallback,

	Serialization::WriteRecord,
	Serialization::OpenRecord,
	Serialization::WriteRecordData,

	Serialization::GetNextRecordInfo,
	Serialization::ReadRecordData,

	Serialization::ResolveRefID,

	Serialization::SetPreLoadCallback,

	Serialization::GetSavePath,

	Serialization::PeekRecordData,

	Serialization::WriteRecord8,
	Serialization::WriteRecord16,
	Serialization::WriteRecord32,
	Serialization::WriteRecord64,

	Serialization::ReadRecord8,
	Serialization::ReadRecord16,
	Serialization::ReadRecord32,
	Serialization::ReadRecord64,

	Serialization::SkipNBytes,
};
