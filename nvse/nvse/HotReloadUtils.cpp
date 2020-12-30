#include "SocketUtils.h"
#include "GameAPI.h"
#include "GameRTTI.h"
#include "SafeWrite.h"
#include <thread>

#include "GameData.h"

class ScriptTransferObject
{
public:
	UInt32 scriptRefID;
	UInt32 nameLength;
	UInt32 dataLength;
};

const auto g_nvsePort = 12059;

#if RUNTIME

SocketServer g_hotReloadServer(g_nvsePort);
std::thread g_hotReloadThread;

bool g_clearTokenCaches = false;

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
			Error("Failed to listen for clients (%d)", g_hotReloadServer.m_errno);
			return;
		}
		std::vector<char> buf;
		if (!g_hotReloadServer.ReadData(buf, sizeof(ScriptTransferObject)))
		{
			Error("Failed to receive transfer data (%d)", g_hotReloadServer.m_errno);
			return;
		}
		auto* obj = reinterpret_cast<ScriptTransferObject*>(buf.data());
		std::vector<char> modNameBuf;
		if (!g_hotReloadServer.ReadData(modNameBuf, obj->nameLength))
		{
			Error("Failed to receive mod name (%d)", g_hotReloadServer.m_errno);
			return;
		}
		std::string modName(modNameBuf.begin(), modNameBuf.end());
		std::vector<char> scriptData;
		if (!g_hotReloadServer.ReadData(scriptData, obj->dataLength))
		{
			Error("Failed to receive script data (%d)", g_hotReloadServer.m_errno);
			return;
		}
		g_hotReloadServer.CloseConnection();
		const auto* modInfo = DataHandler::Get()->LookupModByName(modName.c_str());
		if (!modInfo)
		{
			Error("Mod name %s is not loaded in-game");
			continue;
		}
		const auto refId = obj->scriptRefID + (modInfo->modIndex << 24);
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
		auto* newData = GameHeapAlloc(obj->dataLength);
		std::memcpy(newData, scriptData.data(), obj->dataLength);
		script->data = newData;
		GameHeapFree(oldDataPtr);
		std::string reloadMsg = "(xNVSE) Reloaded script " + std::string(script->GetName()) + " in " + modName;
		Console_Print(reloadMsg.c_str());
		QueueUIMessage(reloadMsg.c_str(), 0, nullptr, nullptr, 2.5F, false);
		g_clearTokenCaches = true;
	}
}
#else

void __fastcall SendHotReloadData(Script* script)
{
	ModInfo* activeFile = DataHandler::Get()->activeFile;
	SocketClient client("127.0.0.1", g_nvsePort);
	if (client.m_errno)
		return;
	ScriptTransferObject scriptTransferObject;
	scriptTransferObject.scriptRefID = script->refID;
	scriptTransferObject.dataLength = script->info.dataLength;
	scriptTransferObject.nameLength = strlen(activeFile->name);
	client.SendData(reinterpret_cast<UInt8*>(&scriptTransferObject), sizeof(ScriptTransferObject));
	client.SendData(reinterpret_cast<UInt8*>(activeFile->name), scriptTransferObject.nameLength);
	client.SendData(static_cast<UInt8*>(script->data), script->info.dataLength);
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
		call SendHotReloadData
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
#else
	WriteRelJump(0x5C97DB, UInt32(Hook_HotReload));
#endif
}