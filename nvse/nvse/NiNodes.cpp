#include "NiNodes.h"

#include "GameScript.h"
#include "MemoizedMap.h"

std::span<TESAnimGroup::AnimGroupInfo> g_animGroups = { reinterpret_cast<TESAnimGroup::AnimGroupInfo*>(0x011977D8), TESAnimGroup::kAnimGroup_Max };

#if RUNTIME
void TextureFormat::InitFromD3DFMT(UInt32 fmt)
{
	typedef void (* _D3DFMTToTextureFormat)(UInt32 d3dfmt, TextureFormat * dst);
	_D3DFMTToTextureFormat D3DFMTToTextureFormat = (_D3DFMTToTextureFormat)0x00E7C1E0;
	
	D3DFMTToTextureFormat(fmt, this);
}

static const UInt32 kNiObjectNET_SetNameAddr = 0x00A5B690;

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
	return (groupCode < TESAnimGroup::kAnimGroup_Max) ? g_animGroups[groupCode].name : NULL;
}

MemoizedMap<const char*, UInt32> s_animGroupMap;

UInt32 TESAnimGroup::AnimGroupForString(const char* groupName)
{
	return s_animGroupMap.Get(groupName, [](const char* groupName)
	{
		for (UInt32 i = 0; i < kAnimGroup_Max; i++) 
		{
			if (!_stricmp(g_animGroups[i].name, groupName)) 
			{
				return i;
			}
		}
		return static_cast<UInt32>(kAnimGroup_Max);
	});
}

void DumpAnimGroups(void)
{
	for (UInt32 i = 0; i < TESAnimGroup::kAnimGroup_Max; i++) {
		_MESSAGE("%d,%s", i , g_animGroups[i].name);
		//if (!_stricmp(s_animGroupInfos[i].name, "JumpLandRight"))
		//	break;
	}
}
#endif
