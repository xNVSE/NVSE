#include "nvse/containers.h"
#include "utility.h"

#define MAX_CACHED_BLOCK_SIZE 0x400
#define MEMORY_POOL_SIZE 0x1000

alignas(16) void *s_availableCachedBlocks[(MAX_CACHED_BLOCK_SIZE >> 4) + 1] = {NULL};

#if !_DEBUG
__declspec(naked) void* __fastcall Pool_Alloc(UInt32 size)
{
	__asm
	{
		cmp		ecx, 0x10
		jbe		minSize
		test	cl, 0xF
		jz		isAligned
		and		cl, 0xF0
		add		ecx, 0x10
	isAligned:
		cmp		ecx, MAX_CACHED_BLOCK_SIZE
		jbe		doCache
		push	ecx
		call	_malloc_base
		pop		ecx
		retn
		NOP_0x6
	minSize:
		mov		ecx, 0x10
	doCache:
		mov		edx, offset s_availableCachedBlocks
	spinHead:
		xor		eax, eax
		lock cmpxchg [edx], edx
		test	eax, eax
		jnz		spinHead
		mov		eax, ecx
		shr		eax, 2
		add		edx, eax
		mov		eax, [edx]
		test	eax, eax
		jz		allocPool
		mov		ecx, [eax]
		mov		[edx], ecx
		xor		edx, edx
		mov		s_availableCachedBlocks, edx
		retn
		nop
	allocPool:
		push	esi
		mov		esi, ecx
		mov		ecx, MEMORY_POOL_SIZE
		push	edx
		xor		edx, edx
		mov		eax, ecx
		div		esi
		push	eax
		sub		ecx, edx
		add		ecx, 8
		push	ecx
		call	_malloc_base
		pop		ecx
		pop		ecx
		sub		ecx, 2
		pop		edx
		add		eax, 8
		and		al, 0xF0
		mov		[edx], eax
		lea		edx, [eax+esi]
	linkHead:
		mov		[eax], edx
		mov		eax, edx
		add		edx, esi
		dec		ecx
		jnz		linkHead
		mov		[eax], ecx
		mov		eax, edx
		mov		s_availableCachedBlocks, ecx
		pop		esi
		retn
	}
}

__declspec(naked) void __fastcall Pool_Free(void *pBlock, UInt32 size)
{
	__asm
	{
		test	ecx, ecx
		jz		nullPtr
		test	dl, 0xF
		jz		isAligned
		and		dl, 0xF0
		add		edx, 0x10
		ALIGN 16
	isAligned:
		cmp		edx, MAX_CACHED_BLOCK_SIZE
		jbe		doCache
		push	ecx
		call	_free_base
		pop		ecx
	nullPtr:
		retn
		NOP_0xA
	doCache:
		push	edx
		mov		edx, offset s_availableCachedBlocks
	spinHead:
		xor		eax, eax
		lock cmpxchg [edx], edx
		test	eax, eax
		jnz		spinHead
		pop		eax
		shr		eax, 2
		add		edx, eax
		mov		eax, [edx]
		mov		[ecx], eax
		mov		[edx], ecx
		mov		s_availableCachedBlocks, 0
		retn
	}
}

__declspec(naked) void* __fastcall Pool_Realloc(void *pBlock, UInt32 curSize, UInt32 reqSize)
{
	__asm
	{
		mov		eax, ecx
		mov		ecx, [esp+4]
		test	eax, eax
		jnz		notNull
		call	Pool_Alloc
		retn	4
	notNull:
		cmp		ecx, edx
		jbe		done
		cmp		edx, MAX_CACHED_BLOCK_SIZE
		jbe		doCache
		push	ecx
		push	eax
		call	_realloc_base
		add		esp, 8
	done:
		retn	4
		ALIGN 16
	doCache:
		push	edx
		push	eax
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
		mov		eax, ecx
		bsr		ecx, eax
		bsf		edx, eax
		cmp		cl, dl
		jz		done
		mov		eax, 2
		shl		eax, cl
	done:
		retn
	}
}