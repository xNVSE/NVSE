#pragma once

#include <string>
#include <vector>

#include "nvse/PluginAPI.h"

class PluginManager
{
	static CommandInfo		RegisterTypedCommand_Setup(CommandInfo* _info, CommandReturnType &retnType);

public:
	PluginManager() = default;
	~PluginManager();

	bool	Init(void);
	void	DeInit(void);

	PluginInfo *	GetInfoByName(const char * name);
	PluginInfo *	GetInfoFromHandle(PluginHandle handle);
	PluginInfo *	GetInfoFromBase(UInt32 baseOpcode);
	const char *	GetPluginNameFromHandle(PluginHandle handle);

	UInt32			GetNumPlugins(void);
	UInt32			GetBaseOpcode(UInt32 idx);
	PluginHandle	LookupHandleFromBaseOpcode(UInt32 baseOpcode);
	PluginHandle	LookupHandleFromName(const char* pluginName);
	PluginHandle	LookupHandleFromPath(const char* pluginPath);

	static bool			RegisterCommand(CommandInfo * _info);
	static bool			RegisterTypedCommand(CommandInfo * _info, CommandReturnType retnType);
	static bool			RegisterTypedCommandVersion(CommandInfo* _info, CommandReturnType retnType, UInt32 requiredPluginVersion);
	static void			SetOpcodeBase(UInt32 opcode);
	static void *		QueryInterface(UInt32 id);
	static PluginHandle	GetPluginHandle(void);
	static const char *	GetFalloutDir();

	static bool Dispatch_Message(PluginHandle sender, UInt32 messageType, void * data, UInt32 dataLen, const char* receiver);
	static bool	RegisterListener(PluginHandle listener, const char* sender, NVSEMessagingInterface::EventCallback handler);

	static void * GetSingleton(UInt32 singletonID);
	static void * GetFunc(UInt32 funcID);
	static void * GetData(UInt32 dataID);
	static void ClearScriptDataCache();

	static std::vector<std::string> GetLoadErrors();

	static void InitExpressionEvaluatorUtils(ExpressionEvaluatorUtils *utils);

private:
	struct LoadedPlugin
	{
		HMODULE		handle;
		PluginInfo	info;
		UInt32		baseOpcode;

		_NVSEPlugin_Query	query;
		_NVSEPlugin_Load	load;

		char path[MAX_PATH]{};			// Added version 4.5 Beta 7
	};

	struct PluginLoadState
	{
		LoadedPlugin plugin{};
		std::string loadStatus;
		bool querySuccess = false;
		bool loadSuccess = false;

		~PluginLoadState();
	};

	bool	FindPluginDirectory(void);
	bool	InstallPlugins(const std::vector<std::string>& pluginPaths);
	void	InstallPlugins(void);

	static const char* SafeCallPluginExport(LoadedPlugin* plugin, const NVSEInterface* nvse, bool query, char* errorOut);
	static std::string SafeCallQueryPlugin(LoadedPlugin* plugin, const NVSEInterface* nvse);
	static std::string	SafeCallLoadPlugin(LoadedPlugin * plugin, const NVSEInterface * nvse);

	const char *	CheckPluginCompatibility(LoadedPlugin * plugin);
	static void RegisterLoadError(std::string message);

	typedef std::vector <LoadedPlugin>	LoadedPluginList;

	std::string			m_pluginDirectory;
	LoadedPluginList	m_plugins;

	static LoadedPlugin		 * s_currentLoadingPlugin;
	static PluginHandle		   s_currentPluginHandle;
};

extern PluginManager	g_pluginManager;

extern CommandInfo kCommandInfo_IsPluginInstalled;
extern CommandInfo kCommandInfo_GetPluginVersion;

typedef UInt32 (__stdcall *_GetLNEventMask)(const char *eventName);
extern _GetLNEventMask GetLNEventMask;
typedef bool (__stdcall *_ProcessLNEventHandler)(UInt32 eventMask, Script *udfScript, bool addEvt, TESForm *formFilter, UInt32 numFilter);
extern _ProcessLNEventHandler ProcessLNEventHandler;