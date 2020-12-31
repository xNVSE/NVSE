#include "SocketUtils.h"
#include "GameAPI.h"
#include "GameRTTI.h"
#include "SafeWrite.h"
#include <thread>

#include "GameData.h"
#include "Hooks_ExpressionEvalOptimized.h"
#include "ScriptTokenCache.h"

class ScriptTransferObject
{
public:
	UInt32 scriptRefID;
	UInt32 nameLength;
	UInt32 dataLength;
	UInt32 varInfoLength;
};

class VarInfoTransferObject
{
public:
	UInt32 idx{};
	UInt32 type{};
	UInt32 nameLength{};
#if RUNTIME
	VarInfoTransferObject() = default;
#endif

	VarInfoTransferObject(UInt32 idx, UInt32 type, UInt32 nameLength) : idx(idx), type(type), nameLength(nameLength) {}
};

const auto g_nvsePort = 12059;

#if RUNTIME

class VarInfoObject : public VarInfoTransferObject
{
public:
	std::string name;
};

SocketServer g_hotReloadServer(g_nvsePort);
std::thread g_hotReloadThread;


void Error(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	char	errorMsg[0x400];
	vsprintf_s(errorMsg, 0x400, fmt, args);
	
	_MESSAGE("Hot reload error: %s", errorMsg);
	Console_Print("(xNVSE) Hot reload error: %s", errorMsg);
	QueueUIMessage("Hot reload error (see console print)", 0, reinterpret_cast<const char*>(0x1049638), nullptr, 2.5F, false);
	g_hotReloadServer.CloseConnection();
}

void InitHotReloadServer(int i)
{
	while (true)
	{
		if (g_hotReloadServer.m_errno)
		{
			Error("Failed to initialize hot reload server (%d)", g_hotReloadServer.m_errno);
			return;
		}
		if (!g_hotReloadServer.WaitForConnection())
		{
			Error("Failed to listen for client (%d)", g_hotReloadServer.m_errno);
			return;
		}
		ScriptTransferObject obj{};
		if (!g_hotReloadServer.ReadData(obj))
		{
			Error("Failed to receive transfer data (%d)", g_hotReloadServer.m_errno);
			return;
		}
		std::string modName;
		if (!g_hotReloadServer.ReadData(modName, obj.nameLength))
		{
			Error("Failed to receive mod name (%d)", g_hotReloadServer.m_errno);
			return;
		}
		std::vector<char> scriptData(obj.dataLength, 0);
		if (!g_hotReloadServer.ReadData(scriptData.data(), obj.dataLength))
		{
			Error("Failed to receive script data (%d)", g_hotReloadServer.m_errno);
			return;
		}
		std::vector<VarInfoObject> varInfos;
		for (auto i = 0U; i < obj.varInfoLength; ++i)
		{
			VarInfoTransferObject varInfo;
			if (!g_hotReloadServer.ReadData(varInfo))
			{
				Error("Failed to receive var info (index %d, errno %d)", i, g_hotReloadServer.m_errno);
				return;
			}
			
		}
		g_hotReloadServer.CloseConnection();
		const auto* modInfo = DataHandler::Get()->LookupModByName(modName.c_str());
		if (!modInfo)
		{
			Error("Mod name %s is not loaded in-game");
			continue;
		}
		const auto refId = obj.scriptRefID + (modInfo->modIndex << 24);
		auto* form = LookupFormByID(refId);
		if (!form)
		{
			Error("Reloading new scripts is not supported.");
			continue;
		}
		auto* script = DYNAMIC_CAST(form, TESForm, Script);
		if (!script)
		{
			Error("Tried to hot reload non-script (how did we get here?)");
			continue;
		}
		auto* oldDataPtr = script->data;
		auto* newData = GameHeapAlloc(obj.dataLength);
		std::memcpy(newData, scriptData.data(), obj.dataLength);
		script->data = newData;
		GameHeapFree(oldDataPtr);
		std::string reloadMsg = "(xNVSE) Reloaded script " + std::string(script->GetName()) + " in " + modName;
		Console_Print(reloadMsg.c_str());
		QueueUIMessage(reloadMsg.c_str(), 0, nullptr, nullptr, 2.5F, false);

		// clear any cached script data
		TokenCache::MarkForClear();
		kEvaluator::TokenListMap::MarkForClear();
	}
}
#else

void SendHotReloadData(Script* script)
{
	ModInfo* activeFile = DataHandler::Get()->activeFile;
	SocketClient client("127.0.0.1", g_nvsePort);
	if (client.m_errno)
		return;
	ScriptTransferObject scriptTransferObject;
	scriptTransferObject.scriptRefID = script->refID;
	scriptTransferObject.dataLength = script->info.dataLength;
	scriptTransferObject.nameLength = strlen(activeFile->name);
	client.SendData(scriptTransferObject);
	client.SendData(activeFile->name, scriptTransferObject.nameLength);
	client.SendData(static_cast<char*>(script->data), script->info.dataLength);
	auto* node = &script->varList;
	while (node)
	{
		if (node->data)
		{
			VarInfoTransferObject obj(node->data->idx, node->data->type, node->data->name.m_dataLen);
			client.SendData(obj);
			client.SendData(node->data->name.CStr(), node->data->name.m_dataLen);
		}
		node = node->Next();
	}
}

std::thread g_hotReloadClientThread;

void __fastcall SendHotReloadDataHook(Script* script)
{
	g_hotReloadClientThread = std::thread(SendHotReloadData, script);
	g_hotReloadClientThread.detach();
}

__declspec(naked) void Hook_HotReload()
{
	static UInt32 Script__CopyFromScriptBuffer = 0x5C5100;
	static UInt32 returnLocation = 0x5C97E0;
	static Script* script = nullptr;
	__asm
	{
		mov [script], ecx
		call Script__CopyFromScriptBuffer
		mov ecx, script
		call SendHotReloadDataHook
		jmp returnLocation
	}
}



#endif

void InitializeHotReload()
{
	WSADATA wsaData;
	const auto result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0)
		return;
	
#if RUNTIME
	g_hotReloadThread = std::thread(InitHotReloadServer, 0);
	g_hotReloadThread.detach();
#else
	WriteRelJump(0x5C97DB, UInt32(Hook_HotReload));
#endif
}