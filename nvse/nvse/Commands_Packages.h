#pragma once

#include "CommandTable.h"
#include "ParamInfos.h"

extern bool Cmd_GetDialogueTarget_Eval(COMMAND_ARGS_EVAL);
extern bool Cmd_GetDialogueTarget_Execute(COMMAND_ARGS);
extern bool Cmd_GetDialogueSubject_Eval(COMMAND_ARGS_EVAL);
extern bool Cmd_GetDialogueSubject_Execute(COMMAND_ARGS);

DEFINE_CMD_ALT_COND(GetDialogueTarget, GDT, get the NPC talked to, 1, kParams_OneOptionalActorRef);
DEFINE_CMD_ALT_COND(GetDialogueSubject, GDS, get the NPC who started the conversation, 1, kParams_OneOptionalActorRef);
DEFINE_CMD_ALT_COND(GetDialogueSpeaker, GDK, get the NPC who is currently speaking in the conversation, 1, kParams_OneOptionalActorRef);

DEFINE_COMMAND(GetCurrentPackage, gets the current package from an actor, 0, 1, kParams_OneOptionalObjectRef);
DEFINE_COMMAND(SetPackageLocationReference, sets package start location as a reference, 0, 2, kParams_OneForm_OneOptionalObjectRef);
DEFINE_CMD_ALT(GetPackageLocation, GetPackageLocationReference, gets package start location, 0, 1, kParams_OneForm);
DEFINE_COMMAND(SetPackageLocationRadius, sets package start location radius, 0, 2, kParams_OneForm_OneFloat);
DEFINE_COMMAND(GetPackageLocationRadius, gets package start location radius, 0, 1, kParams_OneForm);
DEFINE_COMMAND(SetPackageTargetReference, sets package target as a reference, 0, 2, kParams_OneForm_OneOptionalObjectRef);
DEFINE_CMD_ALT(SetPackageTargetCount, SetPackageTargetDistance, sets package target count or distance, 0, 2, kParams_OneForm_OneInt);
DEFINE_CMD_ALT(GetPackageTargetCount, GetPackageTargetDistance, gets package target count or distance, 0, 1, kParams_OneForm);

// NPC_ baseForm package list access and manipulation

DEFINE_CMD_COND(GetPackageCount, gets the count of packages from an actor base form, 0, kParams_OneOptionalObjectRef);
DEFINE_COMMAND(GetNthPackage, gets the Nth package from an actor base form, 0, 2, kParams_OneIndexOneOptionalObjectRef);
DEFINE_COMMAND(SetNthPackage, sets and returns the Nth package to an actor base form, 0, 3, kParams_OnePackageOneIndexOneOptionalObjectRef);
DEFINE_COMMAND(AddPackageAt, adds the Nth package to an actor base form : 0 at top -1 at end returns index, 0, 3, kParams_OnePackageOneIndexOneOptionalObjectRef);
DEFINE_COMMAND(RemovePackageAt, removes and returns the Nth package from an actor base form, 0, 2, kParams_OneIndexOneOptionalObjectRef);
DEFINE_COMMAND(RemoveAllPackages, removes all packages from an actor base form returns count removed, 0, 1, kParams_OneOptionalObjectRef);
