#include "GameProcess.h"

#if RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525
	ActorProcessManager * g_actorProcessManager = (ActorProcessManager*)0x011E0E80;
#elif RUNTIME_VERSION == RUNTIME_VERSION_1_4_0_525ng
	ActorProcessManager * g_actorProcessManager = (ActorProcessManager*)0x011E0E80;
#else
#error
#endif

