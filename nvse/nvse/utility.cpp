#include "nvse/utility.h"

const double
kDblOne = 1,
kDblPI = 3.141592653589793,
kDblPIx2 = 6.283185307179586,
kDblPIx3d2 = 4.71238898038469,
kDblPId2 = 1.5707963267948966,
kDblPId6 = 0.5235987755982989,
kDbl2dPI = 0.6366197723675814,
kDbl4dPI = 1.2732395447351628,
kDblTanPId6 = 0.5773502691896257,
kDblTanPId12 = 0.2679491924311227;

memcpy_t _memcpy = memcpy, _memmove = memmove;

double cos_p(double angle)
{
	angle *= angle;
	return 0.999999953464 + angle * (angle * (0.0416635846769 + angle * (0.00002315393167 * angle - 0.0013853704264)) - 0.499999053455);
}

double dCos(double angle)
{
	if (angle < 0) angle = -angle;
	while (angle > kDblPIx2)
		angle -= kDblPIx2;

	int quad = int(angle * kDbl2dPI);
	switch (quad)
	{
		case 0:
			return cos_p(angle);
		case 1:
			return -cos_p(kDblPI - angle);
		case 2:
			return -cos_p(angle - kDblPI);
		default:
			return cos_p(kDblPIx2 - angle);
	}
}

double dSin(double angle)
{
	return dCos(kDblPId2 - angle);
}


double tan_p(double angle)
{
	angle *= kDbl4dPI;
	double ang2 = angle * angle;
	return angle * (211.849369664121 - 12.5288887278448 * ang2) / (269.7350131214121 + ang2 * (ang2 - 71.4145309347748));
}

double dTan(double angle)
{
	while (angle > kDblPIx2)
		angle -= kDblPIx2;

	int octant = int(angle * kDbl4dPI);
	switch (octant)
	{
		case 0:
			return tan_p(angle);
		case 1:
			return 1.0 / tan_p(kDblPId2 - angle);
		case 2:
			return -1.0 / tan_p(angle - kDblPId2);
		case 3:
			return -tan_p(kDblPI - angle);
		case 4:
			return tan_p(angle - kDblPI);
		case 5:
			return 1.0 / tan_p(kDblPIx3d2 - angle);
		case 6:
			return -1.0 / tan_p(angle - kDblPIx3d2);
		default:
			return -tan_p(kDblPIx2 - angle);
	}
}

double dAtan(double value)
{
	bool sign = (value < 0);
	if (sign) value = -value;

	bool complement = (value > 1.0);
	if (complement) value = 1.0 / value;

	bool region = (value > kDblTanPId12);
	if (region)
		value = (value - kDblTanPId6) / (1.0 + kDblTanPId6 * value);

	double res = value;
	value *= value;
	res *= (1.6867629106 + value * 0.4378497304) / (1.6867633134 + value);

	if (region) res += kDblPId6;
	if (complement) res = kDblPId2 - res;

	return sign ? -res : res;
}

double dAsin(double value)
{
	__asm
	{
		movq	xmm5, value
		movq	xmm6, xmm5
		mulsd	xmm6, xmm6
		movq	xmm7, kDblOne
		subsd	xmm7, xmm6
		sqrtsd	xmm7, xmm7
		divsd	xmm5, xmm7
		movq	value, xmm5
	}
	return dAtan(value);
}

double dAcos(double value)
{
	return kDblPId2 - dAsin(value);
}

double dAtan2(double y, double x)
{
	if (x != 0)
	{
		double z = y / x;
		if (x > 0)
			return dAtan(z);
		else if (y < 0)
			return dAtan(z) - kDblPI;
		else
			return dAtan(z) + kDblPI;
	}
	else if (y > 0)
		return kDblPId2;
	else if (y < 0)
		return -kDblPId2;
	return 0;
}

__declspec(naked) UInt32 __fastcall StrLen(const char *str)
{
	__asm
	{
		mov		eax, ecx
		test	ecx, ecx
		jz		done
	iterHead:
		cmp		[eax], 0
		jz		done
		inc		eax
		jmp		iterHead
	done:
		sub		eax, ecx
		retn
	}
}

__declspec(naked) char* __fastcall StrEnd(const char *str)
{
	__asm
	{
		mov		eax, ecx
		test	ecx, ecx
		jz		done
	iterHead:
		cmp		[eax], 0
		jz		done
		inc		eax
		jmp		iterHead
	done:
		retn
	}
}

__declspec(naked) bool __fastcall MemCmp(const void *ptr1, const void *ptr2, UInt32 bsize)
{
	__asm
	{
		push	esi
		push	edi
		mov		esi, ecx
		mov		edi, edx
		mov		eax, [esp+0xC]
		mov		ecx, eax
		shr		ecx, 2
		jz		comp1
		repe cmpsd
		jnz		done
	comp1:
		and		eax, 3
		jz		done
		mov		ecx, eax
		repe cmpsb
	done:
		setz	al
		pop		edi
		pop		esi
		retn	4
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
		mov		ecx, edx
		test	edx, edx
		jnz		getSize
		mov		[eax], 0
	done:
		retn
	getSize:
		cmp		[ecx], 0
		jz		doCopy
		inc		ecx
		jmp		getSize
	doCopy:
		sub		ecx, edx
		push	ecx
		inc		ecx
		push	ecx
		push	edx
		push	eax
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
		push	esi
		mov		eax, ecx
		test	ecx, ecx
		jz		done
		test	edx, edx
		jz		nullTerm
		mov		esi, [esp+8]
		test	esi, esi
		jz		nullTerm
		mov		ecx, edx
	getSize:
		cmp		[ecx], 0
		jz		doCopy
		inc		ecx
		dec		esi
		jnz		getSize
	doCopy:
		sub		ecx, edx
		lea		esi, [eax+ecx]
		push	ecx
		push	edx
		push	eax
		call	_memmove
		add		esp, 0xC
		mov		eax, esi
	nullTerm:
		mov		[eax], 0
	done:
		pop		esi
		retn	4
	}
}

__declspec(naked) char* __fastcall StrCat(char *dest, const char *src)
{
	__asm
	{
		test	ecx, ecx
		jnz		iterHead
		mov		eax, ecx
		retn
	iterHead:
		cmp		[ecx], 0
		jz		done
		inc		ecx
		jmp		iterHead
	done:
		jmp		StrCopy
	}
}

__declspec(align(16)) const UInt8 kCaseConverter[] =
{
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
	0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
	0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
	0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
	0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
	0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
	0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
};

__declspec(naked) bool __fastcall StrEqualCS(const char *lstr, const char *rstr)
{
	__asm
	{
		push	esi
		push	edi
		test	ecx, ecx
		jz		retn0
		test	edx, edx
		jz		retn0
		mov		esi, ecx
		mov		edi, edx
		call	StrLen
		mov		edx, eax
		mov		ecx, edi
		call	StrLen
		cmp		eax, edx
		jnz		retn0
		mov		ecx, edx
		shr		ecx, 2
		jz		comp1
		repe cmpsd
		jnz		retn0
	comp1:
		and		edx, 3
		jz		retn1
		mov		ecx, edx
		repe cmpsb
		jnz		retn0
	retn1:
		mov		al, 1
		pop		edi
		pop		esi
		retn
	retn0:
		xor		al, al
		pop		edi
		pop		esi
		retn
	}
}

__declspec(naked) bool __fastcall StrEqualCI(const char *lstr, const char *rstr)
{
	__asm
	{
		push	esi
		push	edi
		test	ecx, ecx
		jz		retnFalse
		test	edx, edx
		jz		retnFalse
		mov		esi, ecx
		mov		edi, edx
		call	StrLen
		mov		edx, eax
		mov		ecx, edi
		call	StrLen
		cmp		eax, edx
		jnz		retnFalse
		xor		eax, eax
		mov		ecx, eax
	iterHead:
		mov		al, [esi]
		mov		cl, kCaseConverter[eax]
		mov		al, [edi]
		cmp		cl, kCaseConverter[eax]
		jnz		retnFalse
		inc		esi
		inc		edi
		dec		edx
		jnz		iterHead
		mov		al, 1
		pop		edi
		pop		esi
		retn
	retnFalse:
		xor		al, al
		pop		edi
		pop		esi
		retn
	}
}

__declspec(naked) char __fastcall StrCompare(const char *lstr, const char *rstr)
{
	__asm
	{
		push	ebx
		test	ecx, ecx
		jnz		proceed
		test	edx, edx
		jz		retnEQ
		jmp		retnLT
	proceed:
		test	edx, edx
		jz		retnGT
		xor		eax, eax
		mov		ebx, eax
	iterHead:
		mov		al, [ecx]
		test	al, al
		jz		iterEnd
		mov		bl, kCaseConverter[eax]
		mov		al, [edx]
		cmp		bl, kCaseConverter[eax]
		jz		iterNext
		jl		retnLT
		jmp		retnGT
	iterNext:
		inc		ecx
		inc		edx
		jmp		iterHead
	iterEnd:
		cmp		[edx], 0
		jz		retnEQ
	retnLT:
		mov		al, -1
		pop		ebx
		retn
	retnGT:
		mov		al, 1
		pop		ebx
		retn
	retnEQ:
		xor		al, al
		pop		ebx
		retn
	}
}

__declspec(naked) char __fastcall StrBeginsCS(const char *lstr, const char *rstr)
{
	__asm
	{
		push	esi
		push	edi
		test	ecx, ecx
		jz		retn0
		test	edx, edx
		jz		retn0
		mov		esi, ecx
		mov		edi, edx
		call	StrLen
		mov		edx, eax
		mov		ecx, edi
		call	StrLen
		cmp		eax, edx
		jg		retn0
		mov		ecx, eax
		shr		ecx, 2
		jz		comp1
		repe cmpsd
		jnz		retn0
	comp1:
		and		eax, 3
		jz		retn1
		mov		ecx, eax
		repe cmpsb
		jnz		retn0
	retn1:
		cmp		[esi], 0
		setz	al
		inc		al
		pop		edi
		pop		esi
		retn
	retn0:
		xor		al, al
		pop		edi
		pop		esi
		retn
	}
}

__declspec(naked) char __fastcall StrBeginsCI(const char *lstr, const char *rstr)
{
	__asm
	{
		push	esi
		push	edi
		test	ecx, ecx
		jz		retn0
		test	edx, edx
		jz		retn0
		mov		esi, ecx
		mov		edi, edx
		call	StrLen
		mov		edx, eax
		mov		ecx, edi
		call	StrLen
		cmp		eax, edx
		jg		retn0
		xor		ecx, ecx
		mov		edx, ecx
	iterHead:
		mov		cl, [edi]
		test	cl, cl
		jz		iterEnd
		mov		dl, kCaseConverter[ecx]
		mov		cl, [esi]
		cmp		dl, kCaseConverter[ecx]
		jnz		retn0
		inc		esi
		inc		edi
		dec		eax
		jnz		iterHead
	iterEnd:
		cmp		[esi], 0
		setz	al
		inc		al
		pop		edi
		pop		esi
		retn
	retn0:
		xor		al, al
		pop		edi
		pop		esi
		retn
	}
}

__declspec(naked) void __fastcall StrToLower(char *str)
{
	__asm
	{
		test	ecx, ecx
		jz		done
		xor		eax, eax
	iterHead:
		mov		al, [ecx]
		test	al, al
		jz		done
		mov		dl, kCaseConverter[eax]
		mov		[ecx], dl
		inc		ecx
		jmp		iterHead
	done:
		retn
	}
}

__declspec(naked) void __fastcall ReplaceChr(char *str, char from, char to)
{
	__asm
	{
		test	ecx, ecx
		jz		done
		mov		al, [esp+4]
	iterHead:
		cmp		[ecx], 0
		jz		done
		cmp		[ecx], dl
		jnz		iterNext
		mov		[ecx], al
	iterNext:
		inc		ecx
		jmp		iterHead
	done:
		retn	4
	}
}

__declspec(naked) char* __fastcall FindChr(const char *str, char chr)
{
	__asm
	{
		mov		eax, ecx
		test	ecx, ecx
		jz		done
	iterHead:
		cmp		[eax], 0
		jz		retnNULL
		cmp		[eax], dl
		jz		done
		inc		eax
		jmp		iterHead
	retnNULL:
		xor		eax, eax
	done:
		retn
	}
}

__declspec(naked) char* __fastcall FindChrR(const char *str, UInt32 length, char chr)
{
	__asm
	{
		test	ecx, ecx
		jz		retnNULL
		lea		eax, [ecx+edx]
		mov		dl, [esp+4]
	iterHead:
		cmp		eax, ecx
		jz		retnNULL
		dec		eax
		cmp		[eax], dl
		jz		done
		jmp		iterHead
	retnNULL:
		xor		eax, eax
	done:
		retn	4
	}
}

__declspec(naked) char* __fastcall SubStrCS(const char *srcStr, const char *subStr)
{
	__asm
	{
		push	ebx
		push	esi
		push	edi
		call	StrLen
		test	eax, eax
		jz		done
		mov		esi, ecx
		mov		edi, edx
		mov		ebx, eax
		mov		ecx, edx
		call	StrLen
		sub		ebx, eax
		js		retnNULL
	mainHead:
		mov		ecx, esi
		mov		edx, edi
	subHead:
		mov		al, [edx]
		test	al, al
		jnz		proceed
		mov		eax, esi
		jmp		done
	proceed:
		cmp		al, [ecx]
		jnz		mainNext
		inc		ecx
		inc		edx
		jmp		subHead
	mainNext:
		inc		esi
		dec		ebx
		jns		mainHead
	retnNULL:
		xor		eax, eax
	done:
		pop		edi
		pop		esi
		pop		ebx
		retn
	}
}

__declspec(naked) char* __fastcall SubStrCI(const char *srcStr, const char *subStr)
{
	__asm
	{
		push	ebx
		push	esi
		push	edi
		call	StrLen
		test	eax, eax
		jz		done
		mov		esi, ecx
		mov		edi, edx
		mov		ebx, eax
		mov		ecx, edx
		call	StrLen
		sub		ebx, eax
		js		retnNULL
		push	ebx
		xor		eax, eax
		mov		ebx, eax
	mainHead:
		mov		ecx, esi
		mov		edx, edi
	subHead:
		mov		al, [edx]
		test	al, al
		jnz		proceed
		mov		eax, esi
		pop		ecx
		jmp		done
	proceed:
		mov		bl, kCaseConverter[eax]
		mov		al, [ecx]
		cmp		bl, kCaseConverter[eax]
		jnz		mainNext
		inc		ecx
		inc		edx
		jmp		subHead
	mainNext:
		inc		esi
		dec		dword ptr [esp]
		jns		mainHead
		pop		ecx
	retnNULL:
		xor		eax, eax
	done:
		pop		edi
		pop		esi
		pop		ebx
		retn
	}
}

__declspec(naked) char* __fastcall SlashPos(const char *str)
{
	__asm
	{
		mov		eax, ecx
		test	ecx, ecx
		jz		done
	iterHead:
		mov		cl, [eax]
		test	cl, cl
		jz		retnNULL
		cmp		cl, '/'
		jz		done
		cmp		cl, '\\'
		jz		done
		inc		eax
		jmp		iterHead
	retnNULL:
		xor		eax, eax
	done:
		retn
	}
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
		call	_malloc_base
		pop		ecx
		push	eax
		call	_memcpy
		add		esp, 0xC
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

__declspec(naked) UInt32 __fastcall StrHashCS(const char *inKey)
{
	__asm
	{
		xor		eax, eax
		test	ecx, ecx
		jnz		proceed
		retn
	iterHead:
		mov		edx, eax
		shl		edx, 5
		add		eax, edx
	proceed:
		movzx	edx, byte ptr [ecx]
		add		eax, edx
		inc		ecx
		test	edx, edx
		jnz		iterHead
		retn
	}
}

__declspec(naked) UInt32 __fastcall StrHashCI(const char *inKey)
{
	__asm
	{
		push	esi
		xor		eax, eax
		test	ecx, ecx
		jz		done
		mov		esi, ecx
		mov		ecx, eax
	iterHead:
		mov		cl, [esi]
		test	cl, cl
		jz		done
		mov		edx, eax
		shl		edx, 5
		add		eax, edx
		movzx	edx, kCaseConverter[ecx]
		add		eax, edx
		inc		esi
		jmp		iterHead
	done:
		pop		esi
		retn
	}
}

AuxBuffer s_auxBuffers[3];

__declspec(naked) UInt8* __fastcall GetAuxBuffer(AuxBuffer &buffer, UInt32 reqSize)
{
	__asm
	{
		mov		eax, [ecx]
		cmp		[ecx+4], edx
		jnb		sizeOK
		mov		[ecx+4], edx
		push	ecx
		push	edx
		test	eax, eax
		jz		doAlloc
		push	eax
		call	free
		pop		ecx
		jmp		doAlloc
	sizeOK:
		test	eax, eax
		jnz		done
		push	ecx
		push	dword ptr [ecx+4]
	doAlloc:
		call	malloc
		pop		ecx
		pop		ecx
		mov		[ecx], eax
	done:
		retn
	}
}
