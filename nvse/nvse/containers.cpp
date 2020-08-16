#include "nvse/containers.h"

__declspec(naked) void __fastcall AllocDataArray(void *container, UInt32 dataSize)
{
	__asm
	{
		cmp		dword ptr [ecx], 0
		jnz		hasData
		push	ecx
		imul	edx, [ecx+8]
		push	edx
		call	malloc
		pop		ecx
		pop		ecx
		mov		[ecx], eax
		retn
	hasData:
		mov		eax, [ecx+8]
		cmp		eax, [ecx+4]
		ja		done
		shl		eax, 1
		mov		[ecx+8], eax
		push	ecx
		imul	edx, eax
		push	edx
		push	dword ptr [ecx]
		call	realloc
		add		esp, 8
		pop		ecx
		mov		[ecx], eax
	done:
		retn
	}
}

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
