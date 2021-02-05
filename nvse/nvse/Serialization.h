#pragma once

#include "PluginAPI.h"

extern NVSESerializationInterface	g_NVSESerializationInterface;

namespace Serialization
{

struct SerializationTask
{
	UInt8		*bufferPtr;
	UInt32		length;

	void Reset();

	SerializationTask() : bufferPtr(NULL), length(0) {}

	bool Save();
	bool Load();

	UInt32 GetOffset() const;
	void SetOffset(UInt32 offset);

	void Skip(UInt32 size);

	void Write8(UInt8 inData);
	void Write16(UInt16 inData);
	void Write32(UInt32 inData);
	void Write64(const void *inData);
	void WriteBuf(const void *inData, UInt32 size);

	UInt8 Read8();
	UInt16 Read16();
	UInt32 Read32();
	void Read64(void *outData);
	void ReadBuf(void *outData, UInt32 size);

	void PeekBuf(void *outData, UInt32 size);

	UInt32 GetRemain() const {return length - GetOffset();}
};

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

void	WriteRecord8(UInt8 inData);
void	WriteRecord16(UInt16 inData);
void	WriteRecord32(UInt32 inData);
void	WriteRecord64(const void *inData);

bool	GetNextRecordInfo(UInt32 * type, UInt32 * version, UInt32 * length);
UInt32	ReadRecordData(void * buf, UInt32 length);

UInt8	ReadRecord8();
UInt16	ReadRecord16();
UInt32	ReadRecord32();
void	ReadRecord64(void *outData);

void	SkipNBytes(UInt32 byteNum);

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
