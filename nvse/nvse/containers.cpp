#include "common/ICriticalSection.h"
#include "nvse/containers.h"

#define MAX_CACHED_BLOCK_SIZE 0x200

alignas(16) void *s_availableCachedBlocks[(MAX_CACHED_BLOCK_SIZE >> 2) + 1] = {NULL};

alignas(16) UInt32 s_poolSemaphore = 0;

#if !_DEBUG
__declspec(naked) void* __fastcall Pool_Alloc(UInt32 size)
{
	__asm
	{
		test	cl, 3
		jz		isAligned
		and		cl, 0xFC
		add		ecx, 4
		jmp		isAligned
		and		esp, 0xEFFFFFFF
	isAligned:
		cmp		ecx, MAX_CACHED_BLOCK_SIZE
		ja		doAlloc
		mov		edx, offset s_poolSemaphore
	spinHead:
		xor		eax, eax
		lock cmpxchg [edx], edx
		test	eax, eax
		jnz		spinHead
		lea		edx, s_availableCachedBlocks[ecx]
		mov		eax, [edx]
		test	eax, eax
		jz		noCached
		mov		ecx, [eax]
		mov		[edx], ecx
		mov		s_poolSemaphore, 0
		retn
	noCached:
		mov		s_poolSemaphore, 0
	doAlloc:
		push	ecx
		call	_malloc_base
		pop		ecx
		retn
	}
}

__declspec(naked) void __fastcall Pool_Free(void *pBlock, UInt32 size)
{
	__asm
	{
		test	ecx, ecx
		jz		nullPtr
		test	dl, 3
		jz		isAligned
		and		dl, 0xFC
		add		edx, 4
	isAligned:
		cmp		edx, MAX_CACHED_BLOCK_SIZE
		jbe		doCache
		push	ecx
		call	_free_base
		pop		ecx
	nullPtr:
		retn
		and		esp, 0xEFFFFFFF
		mov		eax, 0
	doCache:
		push	edx
		mov		edx, offset s_poolSemaphore
	spinHead:
		xor		eax, eax
		lock cmpxchg [edx], edx
		test	eax, eax
		jnz		spinHead
		pop		edx
		lea		eax, s_availableCachedBlocks[edx]
		mov		edx, [eax]
		mov		[ecx], edx
		mov		[eax], ecx
		mov		s_poolSemaphore, 0
		retn
	}
}

__declspec(naked) void* __fastcall Pool_Realloc(void *pBlock, UInt32 curSize, UInt32 reqSize)
{
	__asm
	{
		test	dl, 3
		jz		isAligned
		and		dl, 0xFC
		add		edx, 4
	isAligned:
		cmp		edx, MAX_CACHED_BLOCK_SIZE
		jbe		doCache
		push	dword ptr [esp+4]
		push	ecx
		call	_realloc_base
		add		esp, 8
		retn	4
	doCache:
		test	ecx, ecx
		jnz		doRealloc
		mov		ecx, [esp+4]
		call	Pool_Alloc
		retn	4
	doRealloc:
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

#else

// doing this to track memory related issues, debug mode new operator will memset the data

void* Pool_Alloc(UInt32 size)
{
	return ::operator new(size);
}

void Pool_Free(void *pBlock, UInt32 size)
{
	::operator delete(pBlock);
}

void* Pool_Realloc(void *pBlock, UInt32 curSize, UInt32 reqSize)
{
	void* data = Pool_Alloc(reqSize);
	std::memcpy(data, pBlock, curSize);
	Pool_Free(pBlock, curSize);
	return data;
}

void* Pool_Alloc_Buckets(UInt32 numBuckets)
{
	void* data = Pool_Alloc(numBuckets * 4);
	std::memset(data, 0, numBuckets * 4);
	return data;
}
#endif

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