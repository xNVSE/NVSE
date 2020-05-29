#pragma once

#include "loader_common/IdentifyEXE.h"

bool DoInjectDLL(PROCESS_INFORMATION * info, const char * dllPath, ProcHookInfo * hookInfo);
