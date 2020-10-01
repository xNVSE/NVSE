#include "common/ICriticalSection.h"
#include "nvse/containers.h"

#define MAX_CACHED_BLOCK_SIZE 0x200

alignas(16) void *s_availableCachedBlocks[(MAX_CACHED_BLOCK_SIZE >> 2) + 1] = {NULL};

ICriticalSection s_poolCritSection;

__declspec(naked) void* __fastcall Pool_Alloc(UInt32 size)
{
	__asm
	{
		push	ecx
		cmp		ecx, MAX_CACHED_BLOCK_SIZE
		ja		doAlloc
		push	offset s_poolCritSection
		call	EnterCriticalSection
		pop		ecx
		lea		edx, s_availableCachedBlocks[ecx]
		mov		eax, [edx]
		test	eax, eax
		jz		leaveCS
		mov		ecx, [eax]
		mov		[edx], ecx
		push	eax
		push	offset s_poolCritSection
		call	LeaveCriticalSection
		pop		eax
		retn
	leaveCS:
		push	ecx
		push	offset s_poolCritSection
		call	LeaveCriticalSection
	doAlloc:
		call	_malloc_base
		pop		ecx
		retn
	}
}

__declspec(naked) void* __fastcall Pool_Alloc_Al4(UInt32 size)
{
	__asm
	{
		test	cl, 3
		jz		isAligned
		and		cl, 0xFC
		add		ecx, 4
	isAligned:
		jmp		Pool_Alloc
	}
}

__declspec(naked) void __fastcall Pool_Free(void *pBlock, UInt32 size)
{
	__asm
	{
		push	ecx
		cmp		edx, MAX_CACHED_BLOCK_SIZE
		ja		doFree
		push	edx
		push	offset s_poolCritSection
		call	EnterCriticalSection
		pop		edx
		pop		ecx
		lea		eax, s_availableCachedBlocks[edx]
		mov		edx, [eax]
		mov		[ecx], edx
		mov		[eax], ecx
		push	offset s_poolCritSection
		call	LeaveCriticalSection
		retn
	doFree:
		call	_free_base
		pop		ecx
		retn
	}
}

__declspec(naked) void __fastcall Pool_Free_Al4(void *pBlock, UInt32 size)
{
	__asm
	{
		test	dl, 3
		jz		isAligned
		and		dl, 0xFC
		add		edx, 4
	isAligned:
		jmp		Pool_Free
	}
}

__declspec(naked) void* __fastcall Pool_Realloc(void *pBlock, UInt32 curSize, UInt32 reqSize)
{
	__asm
	{
		cmp		edx, MAX_CACHED_BLOCK_SIZE
		ja		doRealloc
		push	edx
		push	ecx
		mov		ecx, [esp+0xC]
		call	Pool_Alloc
		push	eax
		call	_memcpy
		pop		eax
		pop		ecx
		pop		edx
		push	eax
		call	Pool_Free
		pop		eax
		retn	4
	doRealloc:
		push	dword ptr [esp+4]
		push	ecx
		call	_realloc_base
		add		esp, 8
		retn	4
	}
}

__declspec(naked) void* __fastcall Pool_Realloc_Al4(void *pBlock, UInt32 curSize, UInt32 reqSize)
{
	__asm
	{
		test	dl, 3
		jz		isAligned
		and		dl, 0xFC
		add		edx, 4
	isAligned:
		cmp		edx, MAX_CACHED_BLOCK_SIZE
		ja		doRealloc
		push	edx
		push	ecx
		mov		ecx, [esp+0xC]
		call	Pool_Alloc_Al4
		push	eax
		call	_memcpy
		pop		eax
		pop		ecx
		pop		edx
		push	eax
		call	Pool_Free
		pop		eax
		retn	4
	doRealloc:
		push	dword ptr [esp+4]
		push	ecx
		call	_realloc_base
		add		esp, 8
		retn	4
	}
}

__declspec(naked) void* __fastcall Pool_Alloc_Buckets(UInt32 numBuckets)
{
	__asm
	{
		push	ecx
		shl		ecx, 2
		call	Pool_Alloc
		pop		ecx
		push	eax
		push	edi
		mov		edi, eax
		xor		eax, eax
		rep stosd
		pop		edi
		pop		eax
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
		cmp		ecx, MAP_MAX_BUCKET_COUNT
		jb		ltMax
		mov		eax, MAP_MAX_BUCKET_COUNT
		retn
	ltMax:
		xor		eax, eax
		bsr		edx, ecx
		bts		eax, edx
		cmp		eax, ecx
		jz		done
		shl		eax, 1
	done:
		retn
	}
}