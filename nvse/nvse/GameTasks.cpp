#include "GameTasks.h"
#include "GameAPI.h"
#include "GameForms.h"
#include "GameObjects.h"
#include "Hooks_Gameplay.h"

// IOManager** g_ioManager = (IOManager**)0x00B33A10;
ModelLoader** g_modelLoader = (ModelLoader**)0x011C3B3C;
UInt32 kModelLoader_QueueReference = 0x00444850;
UInt32 * kBSTaskCounter = (UInt32*) 0x0011C3B38;

#if 0
bool IOManager::IsInQueue(TESObjectREFR *refr) 
{
	for (LockFreeQueue< NiPointer<IOTask> >::Node *node = taskQueue->head->next; node; node = node->next) 
	{
		QueuedReference *qr = OBLIVION_CAST(node->data, IOTask, QueuedReference);
		if (!qr)
			continue;

		if (qr->refr == refr)
			return true;
	}

	return false;
}

void IOManager::DumpQueuedTasks()
{
#if 0
	_MESSAGE("Dumping queued tasks:");
	for (LockFreeQueue< NiPointer<IOTask> >::Node *node = taskQueue->head->next; node; node = node->next)
	{
		QueuedReference* qr = OBLIVION_CAST(node->data, IOTask, QueuedReference);
		if (!qr)
			continue;
		else if (qr->refr)
		{
			Console_Print("\t%s (%08x)", GetFullName(qr->refr), qr->refr->refID);
			_MESSAGE("\t%s (%08x)", GetFullName(qr->refr), qr->refr->refID);
		}
		else
			_MESSAGE("NULL reference");
	}
#endif

}

IOManager* IOManager::GetSingleton()
{
	return *g_ioManager;
}
#endif

ModelLoader* ModelLoader::GetSingleton()
{
	return *g_modelLoader;
}

void ModelLoader::QueueReference(TESObjectREFR* refr, UInt32 arg1, bool ifInMainThread)
{
	ThisStdCall(kModelLoader_QueueReference, this, refr, arg1, (UInt32)ifInMainThread);	// arg1 is encoded based on the parent cell and arg2 is most likely a boolean
}

UInt32* BSTask::GetCounterSingleton()
{
	return kBSTaskCounter;
}

