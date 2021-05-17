#pragma once

NVSEMessagingInterface* g_messagingInterface;
NVSEInterface* g_nvseInterface;
NVSECommandTableInterface* g_cmdTable;
const CommandInfo* g_TFC;

#if RUNTIME  //if non-GECK version (in-game)
NVSEScriptInterface* g_script;
#endif

bool (*ExtractArgsEx)(COMMAND_ARGS_EX, ...);