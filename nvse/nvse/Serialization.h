#pragma once

#include "PluginAPI.h"

extern NVSESerializationInterface	g_NVSESerializationInterface;

namespace Serialization
{

struct PluginCallbacks
{
	PluginCallbacks()
		:save(NULL), load(NULL), newGame(NULL), preLoad(NULL) { }

	NVSESerializationInterface::EventCallback	save;
	NVSESerializationInterface::EventCallback	load;
	NVSESerializationInterface::EventCallback	newGame;
	NVSESerializationInterface::EventCallback	preLoad;
	
	bool	hadData;
};

// plugin API
void	SetSaveCallback(PluginHandle plugin, NVSESerializationInterface::EventCallback callback);
void	SetLoadCallback(PluginHandle plugin, NVSESerializationInterface::EventCallback callback);
void	SetNewGameCallback(PluginHandle plugin, NVSESerializationInterface::EventCallback callback);
void	SetPreLoadCallback(PluginHandle plugin, NVSESerializationInterface::EventCallback callback);

bool	WriteRecord(UInt32 type, UInt32 version, const void * buf, UInt32 length);
bool	OpenRecord(UInt32 type, UInt32 version);
bool	WriteRecordData(const void * buf, UInt32 length);

bool	GetNextRecordInfo(UInt32 * type, UInt32 * version, UInt32 * length);
UInt32	ReadRecordData(void * buf, UInt32 length);

bool	ResolveRefID(UInt32 refID, UInt32 * outRefID);

// internal event handlers
void	HandleSaveGame(const char * path);
void	HandleLoadGame(const char * path, NVSESerializationInterface::EventCallback PluginCallbacks::* callback = &PluginCallbacks::load);
void	HandleDeleteGame(const char * path);
void	HandleRenameGame(const char * oldPath, const char * newPath);
void	HandleNewGame(void);
void	HandlePreLoadGame(const char* path);
void	HandlePostLoadGame(bool bLoadSucceeded);

void	InternalSetSaveCallback(PluginHandle plugin, NVSESerializationInterface::EventCallback callback);
void	InternalSetLoadCallback(PluginHandle plugin, NVSESerializationInterface::EventCallback callback);
void	InternalSetNewGameCallback(PluginHandle plugin, NVSESerializationInterface::EventCallback callback);
void	InternalSetPreLoadCallback(PluginHandle plugin, NVSESerializationInterface::EventCallback callback);

const char * GetSavePath(void);
extern bool ignoreNextChunk;

}
