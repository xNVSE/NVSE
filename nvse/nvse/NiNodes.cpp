#include "NiNodes.h"

#if RUNTIME
void TextureFormat::InitFromD3DFMT(UInt32 fmt)
{
	typedef void (* _D3DFMTToTextureFormat)(UInt32 d3dfmt, TextureFormat * dst);
	_D3DFMTToTextureFormat D3DFMTToTextureFormat = (_D3DFMTToTextureFormat)0x00E7C1E0;
	
	D3DFMTToTextureFormat(fmt, this);
}

static const UInt32 kNiObjectNET_SetNameAddr = 0x00A5B690;

// an array of structs describing each of the game's anim groups
static const TESAnimGroup::AnimGroupInfo* s_animGroupInfos = (const TESAnimGroup::AnimGroupInfo*)0x011977D8;

void NiObjectNET::SetName(const char* newName)
{
	// uses cdecl, not stdcall
	_asm
	{
		push newName
		// mov ecx, this (already)
		call kNiObjectNET_SetNameAddr
		add esp, 4
	}
	// OBSE : ThisStdCall(kNiObjectNET_SetNameAddr, this, newName);
}

const char* TESAnimGroup::StringForAnimGroupCode(UInt32 groupCode)
{
	return (groupCode < TESAnimGroup::kAnimGroup_Max) ? s_animGroupInfos[groupCode].name : NULL;
}

UInt32 TESAnimGroup::AnimGroupForString(const char* groupName)
{
	for (UInt32 i = 0; i < TESAnimGroup::kAnimGroup_Max; i++) {
		if (!_stricmp(s_animGroupInfos[i].name, groupName)) {
			return i;
		}
	}

	return TESAnimGroup::kAnimGroup_Max;
}

void DumpAnimGroups(void)
{
	for (UInt32 i = 0; i < TESAnimGroup::kAnimGroup_Max; i++) {
		_MESSAGE("%d,%s", i , s_animGroupInfos[i].name);
		//if (!_stricmp(s_animGroupInfos[i].name, "JumpLandRight"))
		//	break;
	}
}
#endif
