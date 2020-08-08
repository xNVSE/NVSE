#include "nvse/containers.h"

__declspec(naked) UInt32 __fastcall AlignBucketCount(UInt32 count)
{
	__asm
	{
		cmp		ecx, MAP_DEFAULT_BUCKET_COUNT
		ja		gtMin
		mov		eax, MAP_DEFAULT_BUCKET_COUNT
		retn
	gtMin:
		mov		eax, MAP_MAX_BUCKET_COUNT
		cmp		ecx, eax
		jb		iterHead
		retn
	iterHead:
		shr		eax, 1
		test	ecx, eax
		jz		iterHead
		cmp		eax, ecx
		jz		done
		shl		eax, 1
	done:
		retn
	}
}

void *s_scrappedMapEntries[0x20] = {NULL};

__declspec(naked) void* __fastcall AllocMapEntry(UInt32 size)
{
	__asm
	{
		lea		edx, s_scrappedMapEntries[ecx]
		mov		eax, [edx]
		test	eax, eax
		jz		doAlloc
		mov		ecx, [eax]
		mov		[edx], ecx
		retn
	doAlloc:
		push	ecx
		call	malloc
		pop		ecx
		retn
	}
}

__declspec(naked) void __fastcall ScrapMapEntry(void *entry, UInt32 size)
{
	__asm
	{
		lea		eax, s_scrappedMapEntries[edx]
		mov		edx, [eax]
		mov		[ecx], edx
		mov		[eax], ecx
		retn
	}
}