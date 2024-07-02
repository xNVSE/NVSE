#include "nvse/utility.h"

memcpy_t _memcpy = memcpy, _memmove = memmove;

__declspec(naked) PrimitiveCS *PrimitiveCS::Enter()
{
	__asm
	{
		push	ebx
		mov		ebx, ecx
		call	GetCurrentThreadId
		cmp		[ebx], eax
		jnz		doSpin
	done:
		mov		eax, ebx
		pop		ebx
		retn
		NOP_0xA
	doSpin:
		mov		ecx, eax
		xor		eax, eax
		lock cmpxchg [ebx], ecx
		test	eax, eax
		jz		done
		push	esi
		push	edi
		mov		esi, ecx
		mov		edi, 0x2710
	spinHead:
		dec		edi
		mov		edx, edi
		shr		edx, 0x1F
		push	edx
		call	Sleep
		xor		eax, eax
		lock cmpxchg [ebx], esi
		test	eax, eax
		jnz		spinHead
		pop		edi
		pop		esi
		mov		eax, ebx
		pop		ebx
		retn
	}
}

__declspec(naked) TESForm* __stdcall LookupFormByRefID(UInt32 refID)
{
	__asm
	{
		mov		ecx, ds:[0x11C54C0]
		mov		eax, [esp+4]
		xor		edx, edx
		div		dword ptr [ecx+4]
		mov		eax, [ecx+8]
		mov		eax, [eax+edx*4]
		test	eax, eax
		jz		done
		mov		edx, [esp+4]
		ALIGN 16
	iterHead:
		cmp		[eax+4], edx
		jz		found
		mov		eax, [eax]
		test	eax, eax
		jnz		iterHead
		retn	4
	found:
		mov		eax, [eax+8]
	done:
		retn	4
	}
}

__declspec(naked) int __vectorcall ifloor(float value)
{
	__asm
	{
		movd	eax, xmm0
		test	eax, eax
		jns		isPos
		push	0x3FA0
		ldmxcsr	[esp]
		cvtss2si	eax, xmm0
		mov		dword ptr [esp], 0x1FA0
		ldmxcsr	[esp]
		pop		ecx
		retn
	isPos:
		cvttss2si	eax, xmm0
		retn
	}
}

__declspec(naked) int __vectorcall iceil(float value)
{
	__asm
	{
		movd	eax, xmm0
		test	eax, eax
		js		isNeg
		push	0x5FA0
		ldmxcsr	[esp]
		cvtss2si	eax, xmm0
		mov		dword ptr [esp], 0x1FA0
		ldmxcsr	[esp]
		pop		ecx
		retn
	isNeg:
		cvttss2si	eax, xmm0
		retn
	}
}

__declspec(naked) UInt32 __fastcall StrLen(const char *str)
{
	__asm
	{
		test	ecx, ecx
		jnz		proceed
		xor		eax, eax
		retn
	proceed:
		push	ecx
		test	ecx, 3
		jz		iter4
	iter1:
		mov		al, [ecx]
		inc		ecx
		test	al, al
		jz		done1
		test	ecx, 3
		jnz		iter1
		ALIGN 16
	iter4:
		mov		eax, [ecx]
		mov		edx, 0x7EFEFEFF
		add		edx, eax
		not		eax
		xor		eax, edx
		add		ecx, 4
		test	eax, 0x81010100
		jz		iter4
		mov		eax, [ecx-4]
		test	al, al
		jz		done4
		test	ah, ah
		jz		done3
		test	eax, 0xFF0000
		jz		done2
		test	eax, 0xFF000000
		jnz		iter4
	done1:
		lea		eax, [ecx-1]
		pop		ecx
		sub		eax, ecx
		retn
	done2:
		lea		eax, [ecx-2]
		pop		ecx
		sub		eax, ecx
		retn
	done3:
		lea		eax, [ecx-3]
		pop		ecx
		sub		eax, ecx
		retn
	done4:
		lea		eax, [ecx-4]
		pop		ecx
		sub		eax, ecx
		retn
	}
}

__declspec(naked) void __fastcall MemZero(void *dest, UInt32 bsize)
{
	__asm
	{
		push	edi
		test	ecx, ecx
		jz		done
		mov		edi, ecx
		xor		eax, eax
		mov		ecx, edx
		shr		ecx, 2
		jz		write1
		rep stosd
	write1:
		and		edx, 3
		jz		done
		mov		ecx, edx
		rep stosb
	done:
		pop		edi
		retn
	}
}

__declspec(naked) char* __fastcall StrCopy(char *dest, const char *src)
{
	__asm
	{
		mov		eax, ecx
		test	ecx, ecx
		jz		done
		test	edx, edx
		jnz		proceed
		mov		[eax], 0
	done:
		retn
	proceed:
		push	ecx
		mov		ecx, edx
		call	StrLen
		pop		edx
		push	eax
		inc		eax
		push	eax
		push	ecx
		push	edx
		call	_memmove
		add		esp, 0xC
		pop		ecx
		add		eax, ecx
		retn
	}
}

__declspec(naked) char* __fastcall StrNCopy(char *dest, const char *src, UInt32 length)
{
	__asm
	{
		mov		eax, ecx
		test	ecx, ecx
		jz		done
		test	edx, edx
		jz		nullTerm
		cmp		dword ptr [esp+4], 0
		jz		nullTerm
		push	esi
		mov		esi, ecx
		mov		ecx, edx
		call	StrLen
		mov		edx, [esp+8]
		cmp		edx, eax
		cmova	edx, eax
		push	edx
		push	ecx
		push	esi
		add		esi, edx
		call	_memmove
		add		esp, 0xC
		mov		eax, esi
		pop		esi
	nullTerm:
		mov		[eax], 0
	done:
		retn	4
	}
}

__declspec(naked) char* __fastcall StrCat(char *dest, const char *src)
{
	__asm
	{
		test	ecx, ecx
		jnz		proceed
		mov		eax, ecx
		retn
	proceed:
		push	edx
		call	StrLen
		pop		edx
		add		ecx, eax
		jmp		StrCopy
	}
}

alignas(16) const char
kLwrCaseConverter[] =
{
	'\x00', '\x01', '\x02', '\x03', '\x04', '\x05', '\x06', '\x07', '\x08', '\x09', '\x0A', '\x0B', '\x0C', '\x0D', '\x0E', '\x0F',
	'\x10', '\x11', '\x12', '\x13', '\x14', '\x15', '\x16', '\x17', '\x18', '\x19', '\x1A', '\x1B', '\x1C', '\x1D', '\x1E', '\x1F',
	'\x20', '\x21', '\x22', '\x23', '\x24', '\x25', '\x26', '\x27', '\x28', '\x29', '\x2A', '\x2B', '\x2C', '\x2D', '\x2E', '\x2F',
	'\x30', '\x31', '\x32', '\x33', '\x34', '\x35', '\x36', '\x37', '\x38', '\x39', '\x3A', '\x3B', '\x3C', '\x3D', '\x3E', '\x3F',
	'\x40', '\x61', '\x62', '\x63', '\x64', '\x65', '\x66', '\x67', '\x68', '\x69', '\x6A', '\x6B', '\x6C', '\x6D', '\x6E', '\x6F',
	'\x70', '\x71', '\x72', '\x73', '\x74', '\x75', '\x76', '\x77', '\x78', '\x79', '\x7A', '\x5B', '\x5C', '\x5D', '\x5E', '\x5F',
	'\x60', '\x61', '\x62', '\x63', '\x64', '\x65', '\x66', '\x67', '\x68', '\x69', '\x6A', '\x6B', '\x6C', '\x6D', '\x6E', '\x6F',
	'\x70', '\x71', '\x72', '\x73', '\x74', '\x75', '\x76', '\x77', '\x78', '\x79', '\x7A', '\x7B', '\x7C', '\x7D', '\x7E', '\x7F',
	'\x80', '\x81', '\x82', '\x83', '\x84', '\x85', '\x86', '\x87', '\x88', '\x89', '\x8A', '\x8B', '\x8C', '\x8D', '\x8E', '\x8F',
	'\x90', '\x91', '\x92', '\x93', '\x94', '\x95', '\x96', '\x97', '\x98', '\x99', '\x9A', '\x9B', '\x9C', '\x9D', '\x9E', '\x9F',
	'\xA0', '\xA1', '\xA2', '\xA3', '\xA4', '\xA5', '\xA6', '\xA7', '\xA8', '\xA9', '\xAA', '\xAB', '\xAC', '\xAD', '\xAE', '\xAF',
	'\xB0', '\xB1', '\xB2', '\xB3', '\xB4', '\xB5', '\xB6', '\xB7', '\xB8', '\xB9', '\xBA', '\xBB', '\xBC', '\xBD', '\xBE', '\xBF',
	'\xC0', '\xC1', '\xC2', '\xC3', '\xC4', '\xC5', '\xC6', '\xC7', '\xC8', '\xC9', '\xCA', '\xCB', '\xCC', '\xCD', '\xCE', '\xCF',
	'\xD0', '\xD1', '\xD2', '\xD3', '\xD4', '\xD5', '\xD6', '\xD7', '\xD8', '\xD9', '\xDA', '\xDB', '\xDC', '\xDD', '\xDE', '\xDF',
	'\xE0', '\xE1', '\xE2', '\xE3', '\xE4', '\xE5', '\xE6', '\xE7', '\xE8', '\xE9', '\xEA', '\xEB', '\xEC', '\xED', '\xEE', '\xEF',
	'\xF0', '\xF1', '\xF2', '\xF3', '\xF4', '\xF5', '\xF6', '\xF7', '\xF8', '\xF9', '\xFA', '\xFB', '\xFC', '\xFD', '\xFE', '\xFF'
};

// Returns 0 if both strings are equal.
char __fastcall StrCompare(const char *lstr, const char *rstr)
{
	if (!lstr) return rstr ? -1 : 0;
	if (!rstr) return 1;
	UInt8 lchr, rchr;
	while (*lstr)
	{
		lchr = game_toupper(*(UInt8*)lstr);
		rchr = game_toupper(*(UInt8*)rstr);
		if (lchr == rchr)
		{
			lstr++;
			rstr++;
			continue;
		}
		return (lchr < rchr) ? -1 : 1;
	}
	return *rstr ? -1 : 0;
}

bool __fastcall StrEqual(const char* lstr, const char* rstr)
{
	return StrCompare(lstr, rstr) == 0;
}

void __fastcall StrToLower(char *str)
{
	if (!str) return;
	UInt8 curr;
	while (curr = *str)
	{
		*str = game_tolower(curr);
		str++;
	}
}

char* __fastcall SubStrCI(const char *srcStr, const char *subStr)
{
	int srcLen = StrLen(srcStr);
	if (!srcLen) return NULL;
	int subLen = StrLen(subStr);
	if (!subLen) return NULL;
	srcLen -= subLen;
	if (srcLen < 0) return NULL;
	int index;
	do
	{
		index = 0;
		while (true)
		{
			if (game_tolower(*(UInt8*)(srcStr + index)) != game_tolower(*(UInt8*)(subStr + index)))
				break;
			if (++index == subLen)
				return const_cast<char*>(srcStr);
		}
		srcStr++;
	}
	while (--srcLen >= 0);
	return NULL;
}

char* __fastcall SlashPos(const char *str)
{
	if (!str) return NULL;
	char curr;
	while (curr = *str)
	{
		if ((curr == '/') || (curr == '\\'))
			return const_cast<char*>(str);
		str++;
	}
	return NULL;
}

__declspec(naked) char* __fastcall CopyString(const char *key)
{
	__asm
	{
		call	StrLen
		inc		eax
		push	eax
		push	ecx
		push	eax
#if !_DEBUG
		call    _malloc_base
#else
		call	malloc
#endif
		pop		ecx
		push	eax
		call	_memcpy
		add		esp, 0xC
		retn
	}
}

__declspec(naked) char* __fastcall CopyString(const char* key, UInt32 length)
{
	__asm
	{
		mov		eax, edx // length
		inc		eax // length++ to account for null terminator
		push	eax
		push	ecx
		push	eax
#if !_DEBUG
		call    _malloc_base
#else
		call	malloc
#endif
		pop		ecx // equivalent to "add esp, 4" here
		// stack is now:
		// [esp+4]: length + 1
		// [esp]: "key" arg passed to CopyString

		// length -= 1, to avoid copying null terminator or past-the-end part of string (if it had no null terminator)
		dec		[esp+4]

		push	eax // dest = malloc's new ptr
		call	_memcpy
		// eax is now memcpy's new string
		add		esp, 8
		pop		edx // get pushed eax back (length)
		// add null terminator to copied string in case string arg didn't have one
		add		edx, eax  // advance to last char of string (where null terminator should be)
		mov		byte ptr[edx], 0
		retn
	}
}

__declspec(naked) char* __fastcall IntToStr(char *str, int num)
{
	__asm
	{
		push	esi
		push	edi
		test	edx, edx
		jns		skipNeg
		neg		edx
		mov		[ecx], '-'
		inc		ecx
	skipNeg:
		mov		esi, ecx
		mov		edi, ecx
		mov		eax, edx
		mov		ecx, 0xA
	workIter:
		xor		edx, edx
		div		ecx
		add		dl, '0'
		mov		[esi], dl
		inc		esi
		test	eax, eax
		jnz		workIter
		mov		[esi], 0
		mov		eax, esi
	swapIter:
		dec		esi
		cmp		esi, edi
		jbe		done
		mov		dl, [esi]
		mov		cl, [edi]
		mov		[esi], cl
		mov		[edi], dl
		inc		edi
		jmp		swapIter
	done:
		pop		edi
		pop		esi
		retn
	}
}

// By JazzIsParis
__declspec(naked) UInt32 __fastcall StrHashCS(const char *inKey)
{
	__asm
	{
		mov		eax, 0x6B49D20B
		test	ecx, ecx
		jz		done
		ALIGN 16
	iterHead:
		movzx	edx, byte ptr[ecx]
		test	dl, dl
		jz		done
		shl		edx, 4
		sub		eax, edx
		mov		edx, eax
		shl		eax, 5
		sub		eax, edx
		movzx	edx, byte ptr[ecx + 1]
		test	dl, dl
		jz		done
		shl		edx, 0xC
		sub		eax, edx
		mov		edx, eax
		shl		eax, 5
		sub		eax, edx
		movzx	edx, byte ptr[ecx + 2]
		test	dl, dl
		jz		done
		shl		edx, 0x14
		sub		eax, edx
		mov		edx, eax
		shl		eax, 5
		sub		eax, edx
		movzx	edx, byte ptr[ecx + 3]
		test	dl, dl
		jz		done
		sub		eax, edx
		mov		edx, eax
		shl		eax, 5
		sub		eax, edx
		add		ecx, 4
		jmp		iterHead
	done :
		ret
	}
}

// By JazzIsParis
// "only < 0.008% collisions"
__declspec(naked) UInt32 __fastcall StrHashCI(const char* inKey)
{
	__asm
	{
		push	esi
		mov		eax, 0x6B49D20B
		test	ecx, ecx
		jz		done
		mov		esi, ecx
		xor ecx, ecx
		ALIGN 16
	iterHead:
		mov		cl, [esi]
		test	cl, cl
		jz		done
		movzx	edx, kLwrCaseConverter[ecx]
		shl		edx, 4
		sub		eax, edx
		mov		edx, eax
		shl		eax, 5
		sub		eax, edx
		mov		cl, [esi + 1]
		test	cl, cl
		jz		done
		movzx	edx, kLwrCaseConverter[ecx]
		shl		edx, 0xC
		sub		eax, edx
		mov		edx, eax
		shl		eax, 5
		sub		eax, edx
		mov		cl, [esi + 2]
		test	cl, cl
		jz		done
		movzx	edx, kLwrCaseConverter[ecx]
		shl		edx, 0x14
		sub		eax, edx
		mov		edx, eax
		shl		eax, 5
		sub		eax, edx
		mov		cl, [esi + 3]
		test	cl, cl
		jz		done
		movzx	edx, kLwrCaseConverter[ecx]
		sub		eax, edx
		mov		edx, eax
		shl		eax, 5
		sub		eax, edx
		add		esi, 4
		jmp		iterHead
	done :
		pop		esi
		retn
	}
}

void SpinLock::Enter()
{
	UInt32 threadID = GetCurrentThreadId();
	if (owningThread == threadID)
	{
		enterCount++;
		return;
	}
	while (InterlockedCompareExchange(&owningThread, threadID, 0));
	enterCount = 1;
}

#define FAST_SLEEP_COUNT 10000UL

void SpinLock::EnterSleep()
{
	UInt32 threadID = GetCurrentThreadId();
	if (owningThread == threadID)
	{
		enterCount++;
		return;
	}
	UInt32 fastIdx = FAST_SLEEP_COUNT;
	while (InterlockedCompareExchange(&owningThread, threadID, 0))
	{
		if (fastIdx)
		{
			fastIdx--;
			Sleep(0);
		}
		else Sleep(1);
	}
	enterCount = 1;
}

void SpinLock::Leave()
{
	if (owningThread && !--enterCount)
		owningThread = 0;
}

// From JIP
alignas(16) const UInt32 kPackedValues[] =
{
	PS_DUP_4(0x7FFFFFFF),
	PS_DUP_1(0x7FFFFFFF),
	PS_DUP_4(0x80000000),
	PS_DUP_1(0x80000000),
	PS_DUP_3(0xFFFFFFFF),
	0xFFFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF, 0x7FFFFFFF,
	0, 0x80000000, 0, 0x80000000,
	PS_DUP_4(HEX(FLT_EPSILON)),
	PS_DUP_3(HEX(FltPId180)),
	PS_DUP_3(HEX(Flt180dPI)),
	PS_DUP_3(HEX(FltPId2)),
	PS_DUP_3(HEX(FltPI)),
	PS_DUP_3(HEX(FltPIx2)),
	PS_DUP_3(HEX(0.5F)),
	PS_DUP_3(HEX(1.0F)),
	PS_DUP_3(0x40DFF8D6),
	HEX(0.001F), HEX(0.01F), HEX(0.1F), HEX(0.25F),
	HEX(3.0F), HEX(10.0F), HEX(100.0F), 0
};