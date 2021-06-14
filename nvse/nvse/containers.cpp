#include "nvse/containers.h"
#include "utility.h"

#define MAX_BLOCK_SIZE		0x400
#define MEMORY_POOL_SIZE	0x1000

struct MemoryPool
{
	struct BlockNode
	{
		BlockNode	*m_next;
		//	Data
	};

	PrimitiveCS		m_cs;
	BlockNode		*m_pools[MAX_BLOCK_SIZE >> 4] = {nullptr};
};

alignas(16) MemoryPool s_memoryPool;

#if !_DEBUG
__declspec(naked) void* __fastcall Pool_Alloc(UInt32 size)
{
	__asm
	{
		cmp		ecx, 0x10
		jbe		minSize
		test	cl, 0xF
		jz		isAligned
		and		ecx, 0xFFFFFFF0
		add		ecx, 0x10
	isAligned:
		cmp		ecx, MAX_BLOCK_SIZE
		jbe		doCache
		push	ecx
		call	_malloc_base
		pop		ecx
		retn
	minSize:
		mov		ecx, 0x10
	doCache:
		push	ecx
		mov		ecx, offset s_memoryPool.m_cs
		call	PrimitiveCS::Enter
		pop		ecx
		mov		edx, ecx
		shr		edx, 2
		add		edx, eax
		mov		eax, [edx]
		test	eax, eax
		jz		allocPool
		mov		ecx, [eax]
		mov		[edx], ecx
		xor		edx, edx
		mov		s_memoryPool.m_cs.m_owningThread, edx
		retn
		ALIGN 16
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
		ALIGN 16
	linkHead:
		mov		[eax], edx
		mov		eax, edx
		add		edx, esi
		dec		ecx
		jnz		linkHead
		mov		[eax], ecx
		mov		eax, edx
		mov		s_memoryPool.m_cs.m_owningThread, ecx
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
		and		edx, 0xFFFFFFF0
		add		edx, 0x10
		ALIGN 16
	isAligned:
		cmp		edx, MAX_BLOCK_SIZE
		ja		doFree
		push	edx
		push	ecx
		mov		ecx, offset s_memoryPool.m_cs
		call	PrimitiveCS::Enter
		pop		ecx
		pop		edx
		shr		edx, 2
		add		edx, eax
		mov		eax, [edx]
		mov		[ecx], eax
		mov		[edx], ecx
		mov		s_memoryPool.m_cs.m_owningThread, 0
	nullPtr:
		retn
		ALIGN 16
	doFree:
		push	ecx
		call	_free_base
		pop		ecx
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
		jz		nullPtr
		cmp		ecx, edx
		jbe		done
		cmp		edx, MAX_BLOCK_SIZE
		ja		doRealloc
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
		ALIGN 16
	nullPtr:
		call	Pool_Alloc
	done:
		retn	4
		ALIGN 16
	doRealloc:
		push	ecx
		push	eax
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