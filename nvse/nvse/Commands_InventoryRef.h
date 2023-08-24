#pragma once

#include "CommandTable.h"
#include "ParamInfos.h"

static ParamInfo kParams_OneOptionalContainerRef[1] =
{
	{ "containerRef",	kParamType_Container,	1	},
};

static ParamInfo kParams_OneContainerRef[1] =
{
	{	"containerRef",	kParamType_Container,	1	},
};

DEFINE_COMMAND(RemoveMeIR, "removes an inventory reference from its container, optionally transferring it to a different container, in much the same way as the vanilla RemoveMe command. The inventory reference becomes invalid once this command is called and should no longer be used.",
			   1, 1, kParams_OneOptionalContainerRef);
DEFINE_COMMAND(CopyIR, "copies an inventory reference to the specified container. The calling object needn't be in a container and remains valid after the command is called. If the calling object is equipped, the copy will not be equipped.",
			   1, 1, kParams_OneContainerRef);
DEFINE_COMMAND(CopyIRAlt, "copies an inventory reference to the specified container. The calling object needn't be in a container and remains valid after the command is called. If the calling object is equipped, the copy will not be equipped.",
	1, 1, kParams_OneContainerRef);
DEFINE_COMMAND(CreateTempRef, "creates a temporary reference to the specified form. This reference does not exist in the gameworld or in a container, and remains valid for only one frame. It is mostly useful for creating a stack of one or more items to be added to a container with CopyIR",
			   0, 1, kParams_OneObjectID);
DEFINE_COMMAND(GetFirstRefForItem, returns the first entry in an array of temp refs to objects of the specified type in the calling container,
			   1, 1, kParams_OneObjectID);
DEFINE_COMMAND(GetNextRefForItem, returns the next entry in the array of temp refs to objects of the specified type in the calling container,
			   1, 1, kParams_OneObjectID);

DEFINE_COMMAND(GetInvRefsForItem, returns an array of temp refs to objects of the specified type in the calling container,
			   1, 1, kParams_OneObjectID);

DEFINE_COMMAND(IsInventoryRef, "", true, 0, nullptr);