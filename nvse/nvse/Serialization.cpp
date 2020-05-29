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

IFileStream	s_currentFile;

typedef std::vector <PluginCallbacks>	PluginCallbackList;
PluginCallbackList	s_pluginCallbacks;

PluginHandle	s_currentPlugin = 0;

Header			s_fileHeader = { 0 };

UInt64			s_pluginHeaderOffset = 0;
PluginHeader	s_pluginHeader = { 0 };

bool			s_chunkOpen = false;
bool			ignoreNextChunk = false;	// if true do not complain the plugin left data behind (it ws preloaded)
UInt64			s_chunkHeaderOffset = 0;
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

bool WriteRecord(UInt32 type, UInt32 version, const void * buf, UInt32 length)
{
	if(!OpenRecord(type, version)) return false;

	return WriteRecordData(buf, length);
}

// flush a chunk header to the file if one is currently open
static void FlushWriteChunk(void)
{
	if(!s_chunkOpen)
		return;

	UInt64	curOffset = s_currentFile.GetOffset();
	UInt64	chunkSize = curOffset - s_chunkHeaderOffset - sizeof(s_chunkHeader);

	ASSERT(chunkSize < 0x80000000);	// stupidity check

	s_chunkHeader.length = (UInt32)chunkSize;

	s_currentFile.SetOffset(s_chunkHeaderOffset);
	s_currentFile.WriteBuf(&s_chunkHeader, sizeof(s_chunkHeader));

	s_currentFile.SetOffset(curOffset);

	s_pluginHeader.length += chunkSize + sizeof(s_chunkHeader);

	s_chunkOpen = false;
}

bool OpenRecord(UInt32 type, UInt32 version)
{
	if(!s_pluginHeader.numChunks)
	{
		ASSERT(!s_chunkOpen);

		s_pluginHeaderOffset = s_currentFile.GetOffset();
		s_currentFile.Skip(sizeof(s_pluginHeader));
	}

	FlushWriteChunk();

	s_chunkHeaderOffset = s_currentFile.GetOffset();
	s_currentFile.Skip(sizeof(s_chunkHeader));

	s_pluginHeader.numChunks++;

	s_chunkHeader.type = type;
	s_chunkHeader.version = version;
	s_chunkHeader.length = 0;

	s_chunkOpen = true;

	return true;
}

bool WriteRecordData(const void * buf, UInt32 length)
{
	s_currentFile.WriteBuf(buf, length);

	return true;
}

static void FlushReadRecord(void)
{
	if(s_chunkOpen)
	{
		if(s_chunkHeader.length)
		{
			if (!ignoreNextChunk)
				_WARNING("plugin didn't finish reading chunk");
			s_currentFile.Skip(s_chunkHeader.length);
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

	s_currentFile.ReadBuf(&s_chunkHeader, sizeof(s_chunkHeader));

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

	s_currentFile.ReadBuf(buf, length);

	s_chunkHeader.length -= length;

	return length;
}

UInt32 PeekRecordData(void * buf, UInt32 length)
{
	ASSERT(s_chunkOpen);

	if(length > s_chunkHeader.length)
		length = s_chunkHeader.length;

	s_currentFile.PeekBuf(buf, length);

	return length;
}

bool ResolveRefID(UInt32 refID, UInt32 * outRefID)
{
	UInt8	modID = refID >> 24;

	// pass dynamic ids straight through
	if(modID == 0xFF)
	{
		*outRefID = refID;
		return true;
	}

	UInt8	loadedModID = 0xFF;
	if(modID >= s_numPreloadMods)
	{
		_MESSAGE("ResolveRefID: requested ID for a mod idx higher than referenced in savegame (%02X, max %02X)",
			modID, s_numPreloadMods);
		return false;
	}

	loadedModID = ResolveModIndexForPreload(modID);
	if(loadedModID == 0xFF) 
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

	// disabled for testing purposes
#if 0
	if(s_pluginCallbacks.empty())
	{
		// no callbacks = nothing to write, delete the file if it exists
		DeleteFile(g_savePath.c_str());
	}
	else
#endif
	{
		if(!s_currentFile.Create(g_savePath.c_str()))
		{
			_ERROR("HandleSaveGame: couldn't create save file (%s)", g_savePath.c_str());
			return;
		}

		try
		{
			// init header
			s_fileHeader.signature =		Header::kSignature;
			s_fileHeader.formatVersion =	Header::kVersion;
			s_fileHeader.nvseVersion =		NVSE_VERSION_INTEGER;
			s_fileHeader.nvseMinorVersion =	NVSE_VERSION_INTEGER_MINOR;
			s_fileHeader.falloutVersion =	RUNTIME_VERSION;
			s_fileHeader.numPlugins =		0;

			s_currentFile.Skip(sizeof(s_fileHeader));

			// iterate through plugins
			_MESSAGE("saving %d plugins to %s", s_pluginCallbacks.size(), g_savePath.c_str());
			for(UInt32 i = 0; i < s_pluginCallbacks.size(); i++)
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
						UInt64	curOffset = s_currentFile.GetOffset();

						s_currentFile.SetOffset(s_pluginHeaderOffset);
						s_currentFile.WriteBuf(&s_pluginHeader, sizeof(s_pluginHeader));

						s_currentFile.SetOffset(curOffset);

						s_fileHeader.numPlugins++;
					}
				}
			}

			// write header
			s_currentFile.SetOffset(0);
			s_currentFile.WriteBuf(&s_fileHeader, sizeof(s_fileHeader));
		}
		catch(...)
		{
			_ERROR("HandleSaveGame: exception during save");
		}

		s_currentFile.Close();
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

	// handle internal stuff
	Core_PostLoadCallback(bLoadSucceeded);
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

	if(!s_currentFile.Open(g_savePath.c_str()))
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
			Header	header;

			s_currentFile.ReadBuf(&header, sizeof(header));

			if(header.signature != Header::kSignature)
			{
				_ERROR("HandleLoadGame: invalid file signature (found %08X expected %08X)", header.signature, Header::kSignature);
				goto done;
			}

			if(header.formatVersion <= Header::kVersion_Invalid)
			{
				_ERROR("HandleLoadGame: version invalid (%08X)", header.formatVersion);
				goto done;
			}

			if(header.formatVersion > Header::kVersion)
			{
				_ERROR("HandleLoadGame: version too new (found %08X current %08X)", header.formatVersion, Header::kVersion);
				goto done;
			}
			
			// no older versions to handle

			// reset flags
			for(PluginCallbackList::iterator iter = s_pluginCallbacks.begin(); iter != s_pluginCallbacks.end(); ++iter)
				iter->hadData = false;
			
			NVSESerializationInterface::EventCallback curCallback = NULL;
			// iterate through plugin data chunks
			while(s_currentFile.GetRemain() >= sizeof(PluginHeader))
			{
				s_currentFile.ReadBuf(&s_pluginHeader, sizeof(s_pluginHeader));

				UInt64	pluginChunkStart = s_currentFile.GetOffset();

				// find the corresponding plugin
				UInt32	pluginIdx = (s_pluginHeader.opcodeBase == kNvseOpcodeBase) ? 0 : g_pluginManager.LookupHandleFromBaseOpcode(s_pluginHeader.opcodeBase);
				if(pluginIdx != kPluginHandle_Invalid)
				{
					s_pluginCallbacks[pluginIdx].hadData = true;

					if(s_pluginCallbacks[pluginIdx].*callback)
					{
						s_chunkOpen = false;
						curCallback = s_pluginCallbacks[pluginIdx].*callback;
						curCallback((void*)path);
					}
					else
					{
						// ### wtf?
						_WARNING("plugin has data in save file but no handler");

						s_currentFile.Skip(s_pluginHeader.length);
					}
				}
				else
				{
					// ### TODO: save the data temporarily?
					_WARNING("data in save file for plugin, but plugin isn't loaded");

					s_currentFile.Skip(s_pluginHeader.length);
				}

				UInt64	expectedOffset = pluginChunkStart + s_pluginHeader.length;
				if(s_currentFile.GetOffset() != expectedOffset)
				{
					_WARNING("plugin did not read all of its data (at %016I64X expected %016I64X)", s_currentFile.GetOffset(), expectedOffset);
					s_currentFile.SetOffset(expectedOffset);
				}
			}

			// call load callback for plugins that didn't have data in the file
			for(PluginCallbackList::iterator iter = s_pluginCallbacks.begin(); iter != s_pluginCallbacks.end(); ++iter)
			{
				if(!iter->hadData && (*iter).*callback)
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

done:
	s_currentFile.Close();
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
};
