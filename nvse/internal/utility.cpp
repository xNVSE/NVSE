#include "internal/utility.h"
#include <ctime>

#include "nvse/GameAPI.h"

bool fCompare(float lval, float rval)
{
	return fabs(lval - rval) < FLT_EPSILON;
}

__declspec(naked) int __stdcall lfloor(float value)
{
	__asm
	{
		fld		dword ptr[esp + 4]
		fstcw[esp + 4]
		mov		dx, [esp + 4]
		or word ptr[esp + 4], 0x400
		fldcw[esp + 4]
		fistp	dword ptr[esp + 4]
		mov		eax, [esp + 4]
		mov[esp + 4], dx
		fldcw[esp + 4]
		retn	4
	}
}

__declspec(naked) int __stdcall lceil(float value)
{
	__asm
	{
		fld		dword ptr[esp + 4]
		fstcw[esp + 4]
		mov		dx, [esp + 4]
		or word ptr[esp + 4], 0x800
		fldcw[esp + 4]
		fistp	dword ptr[esp + 4]
		mov		eax, [esp + 4]
		mov[esp + 4], dx
		fldcw[esp + 4]
		retn	4
	}
}

__declspec(naked) float __stdcall fSqrt(float value)
{
	__asm
	{
		fld		dword ptr[esp + 4]
		fsqrt
		retn	4
	}
}

__declspec(naked) double __stdcall dSqrt(double value)
{
	__asm
	{
		fld		qword ptr[esp + 4]
		fsqrt
		retn	8
	}
}

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

UInt32 __fastcall GetNextPrime(UInt32 num)
{
	if (num <= 2) return 2;
	else if (num == 3) return 3;
	UInt32 a = num / 6, b = num - (6 * a), c = (b < 2) ? 1 : 5, d;
	num = (6 * a) + c;
	a = (3 + c) / 2;
	do {
		b = 4;
		c = 5;
		do {
			d = num / c;
			if (c > d) return num;
			if (num == (c * d)) break;
			c += b ^= 6;
		} while (true);
		num += a ^= 6;
	} while (true);
	return num;
}

__declspec(naked) UInt32 __fastcall RGBHexToDec(UInt32 rgb)
{
	__asm
	{
		movzx	eax, cl
		imul	eax, 0xF4240
		movzx	edx, ch
		imul	edx, 0x3E8
		add		eax, edx
		shr		ecx, 0x10
		add		eax, ecx
		retn
	}
}

__declspec(naked) UInt32 __fastcall RGBDecToHex(UInt32 rgb)
{
	__asm
	{
		push	ebx
		mov		eax, ecx
		mov		ecx, 0xF4240
		cdq
		idiv	ecx
		mov		ebx, eax
		mov		eax, edx
		mov		ecx, 0x3E8
		cdq
		idiv	ecx
		shl		eax, 8
		add		eax, ebx
		shl		edx, 0x10
		add		eax, edx
		pop		ebx
		retn
	}
}

__declspec(naked) void* __fastcall MemCopy(void* dest, const void* src, UInt32 length)
{
	__asm
	{
		push	ecx
		push	ebx
		test	ecx, ecx
		jz		done
		test	edx, edx
		jz		done
		mov		ebx, [esp + 0xC]
		test	ebx, ebx
		jz		done
		mov		eax, ecx
		sub		eax, edx
		js		copy10F
		jz		done
		cmp		eax, ebx
		jb		revCopy
		copy10F :
		cmp		ebx, 0x10
			jb		copy8F
			movdqu	xmm0, xmmword ptr[edx]
			movdqu	xmmword ptr[ecx], xmm0
			sub		ebx, 0x10
			jz		done
			add		ecx, 0x10
			add		edx, 0x10
			jmp		copy10F
			copy8F :
		test	bl, 8
			jz		copy4F
			movq	xmm0, qword ptr[edx]
			movq	qword ptr[ecx], xmm0
			and bl, 7
			jz		done
			add		ecx, 8
			add		edx, 8
			copy4F:
		test	bl, 4
			jz		copy2F
			mov		eax, [edx]
			mov[ecx], eax
			and bl, 3
			jz		done
			add		ecx, 4
			add		edx, 4
			copy2F:
		test	bl, 2
			jz		copy1F
			mov		ax, [edx]
			mov[ecx], ax
			and bl, 1
			jz		done
			add		ecx, 2
			add		edx, 2
			copy1F:
		mov		al, [edx]
			mov[ecx], al
			done :
		pop		ebx
			pop		eax
			retn	4
			revCopy :
			add		ecx, ebx
			add		edx, ebx
			copy10B :
		cmp		ebx, 0x10
			jb		copy8B
			sub		ecx, 0x10
			sub		edx, 0x10
			movdqu	xmm0, xmmword ptr[edx]
			movdqu	xmmword ptr[ecx], xmm0
			sub		ebx, 0x10
			jz		done
			jmp		copy10B
			copy8B :
		test	bl, 8
			jz		copy4B
			sub		ecx, 8
			sub		edx, 8
			movq	xmm0, qword ptr[edx]
			movq	qword ptr[ecx], xmm0
			and bl, 7
			jz		done
			copy4B :
		test	bl, 4
			jz		copy2B
			sub		ecx, 4
			sub		edx, 4
			mov		eax, [edx]
			mov[ecx], eax
			and bl, 3
			jz		done
			copy2B :
		test	bl, 2
			jz		copy1B
			sub		ecx, 2
			sub		edx, 2
			mov		ax, [edx]
			mov[ecx], ax
			and bl, 1
			jz		done
			copy1B :
		mov		al, [edx - 1]
			mov[ecx - 1], al
			jmp		done
	}
}

__declspec(naked) UInt32 __fastcall StrLen(const char* str)
{
	__asm
	{
		mov		eax, ecx
		test	ecx, ecx
		jz		done
		iterHead :
		cmp[eax], 0
			jz		done
			inc		eax
			jmp		iterHead
			done :
		sub		eax, ecx
			retn
	}
}

__declspec(naked) char* __fastcall StrEnd(const char* str)
{
	__asm
	{
		mov		eax, ecx
		test	ecx, ecx
		jz		done
		iterHead :
		cmp[eax], 0
			jz		done
			inc		eax
			jmp		iterHead
			done :
		retn
	}
}

__declspec(naked) bool __fastcall MemCmp(const void* ptr1, const void* ptr2, UInt32 bsize)
{
	__asm
	{
		push	esi
		push	edi
		mov		esi, ecx
		mov		edi, edx
		mov		eax, [esp + 0xC]
		mov		ecx, eax
		shr		ecx, 2
		jz		comp1
		repe cmpsd
		jnz		done
		comp1 :
		and eax, 3
			jz		done
			mov		ecx, eax
			repe cmpsb
			done :
		setz	al
			pop		edi
			pop		esi
			retn	4
	}
}

__declspec(naked) void __fastcall MemZero(void* dest, UInt32 bsize)
{
	__asm
	{
		push	edi
		test	ecx, ecx
		jz		done
		mov		edi, ecx
		xor eax, eax
		mov		ecx, edx
		shr		ecx, 2
		jz		write1
		rep stosd
		write1 :
		and edx, 3
			jz		done
			mov		ecx, edx
			rep stosb
			done :
		pop		edi
			retn
	}
}

__declspec(naked) char* __fastcall StrCopy(char* dest, const char* src)
{
	__asm
	{
		test	ecx, ecx
		jz		done
		test	edx, edx
		jz		nullTerm
		mov		eax, edx
		getSize :
		cmp[eax], 0
			jz		doCopy
			inc		eax
			jmp		getSize
			doCopy :
		sub		eax, edx
			push	eax
			push	eax
			call	MemCopy
			pop		ecx
			add		ecx, eax
			nullTerm :
		mov[ecx], 0
			done :
			mov		eax, ecx
			retn
	}
}

__declspec(naked) char* __fastcall StrNCopy(char* dest, const char* src, UInt32 length)
{
	__asm
	{
		push	esi
		test	ecx, ecx
		jz		done
		test	edx, edx
		jz		nullTerm
		mov		esi, [esp + 8]
		test	esi, esi
		jz		nullTerm
		mov		eax, edx
		getSize :
		cmp[eax], 0
			jz		doCopy
			inc		eax
			dec		esi
			jnz		getSize
			doCopy :
		sub		eax, edx
			lea		esi, [ecx + eax]
			push	eax
			call	MemCopy
			mov		ecx, esi
			nullTerm :
		mov[ecx], 0
			done :
			mov		eax, ecx
			pop		esi
			retn	4
	}
}

__declspec(naked) char* __fastcall StrLenCopy(char* dest, const char* src, UInt32 length)
{
	__asm
	{
		test	ecx, ecx
		jz		done
		test	edx, edx
		jz		nullTerm
		mov		eax, [esp + 4]
		test	eax, eax
		jz		nullTerm
		push	eax
		call	MemCopy
		mov		ecx, eax
		add		ecx, [esp + 4]
		nullTerm:
		mov[ecx], 0
			done :
			mov		eax, ecx
			retn	4
	}
}

__declspec(naked) char* __fastcall StrCat(char* dest, const char* src)
{
	__asm
	{
		test	ecx, ecx
		jnz		iterHead
		mov		eax, ecx
		retn
		iterHead :
		cmp[ecx], 0
			jz		done
			inc		ecx
			jmp		iterHead
			done :
		jmp		StrCopy
	}
}

__declspec(naked) UInt32 __fastcall StrHash(const char* inKey)
{
	__asm
	{
		xor eax, eax
		test	ecx, ecx
		jz		done
		xor edx, edx
		iterHead :
		mov		dl, [ecx]
			cmp		dl, 'Z'
			jg		notCap
			test	dl, dl
			jz		done
			cmp		dl, 'A'
			jl		notCap
			or dl, 0x20
			notCap:
		imul	eax, 0x65
			add		eax, edx
			inc		ecx
			jmp		iterHead
			done :
		retn
	}
}

__declspec(naked) bool __fastcall CmprLetters(const char* lstr, const char* rstr)
{
	__asm
	{
		mov		al, [ecx]
		cmp[edx], al
		jz		retnTrue
		cmp		al, 'A'
		jl		retnFalse
		cmp		al, 'z'
		jg		retnFalse
		cmp		al, 'Z'
		jle		isCap
		cmp		al, 'a'
		jl		retnFalse
		isCap :
		xor al, 0x20
			cmp[edx], al
			jz		retnTrue
			retnFalse :
		xor al, al
			retn
			retnTrue :
		mov		al, 1
			retn
	}
}

__declspec(naked) bool __fastcall StrEqualCS(const char* lstr, const char* rstr)
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
		comp1 :
		and edx, 3
			jz		retn1
			mov		ecx, edx
			repe cmpsb
			jnz		retn0
			retn1 :
		mov		al, 1
			pop		edi
			pop		esi
			retn
			retn0 :
		xor al, al
			pop		edi
			pop		esi
			retn
	}
}

__declspec(naked) bool __fastcall StrEqualCI(const char* lstr, const char* rstr)
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
		call	StrLen
		mov		edi, eax
		mov		ecx, edx
		call	StrLen
		cmp		eax, edi
		jnz		retnFalse
		mov		edx, esi
		iterHead :
		cmp[ecx], 0
			jz		retnTrue
			call	CmprLetters
			test	al, al
			jz		retnFalse
			inc		ecx
			inc		edx
			jmp		iterHead
			retnTrue :
		mov		al, 1
			pop		edi
			pop		esi
			retn
			retnFalse :
		xor al, al
			pop		edi
			pop		esi
			retn
	}
}

__declspec(naked) char __fastcall StrCompare(const char* lstr, const char* rstr)
{
	__asm
	{
		push	ebx
		test	ecx, ecx
		jnz		proceed
		test	edx, edx
		jz		retnEQ
		jmp		retnLT
		proceed :
		test	edx, edx
			jz		retnGT
			iterHead :
		mov		al, [ecx]
			mov		bl, [edx]
			test	al, al
			jz		iterEnd
			cmp		al, bl
			jz		iterNext
			cmp		al, 'Z'
			jg		lNotCap
			cmp		al, 'A'
			jl		lNotCap
			or al, 0x20
			lNotCap:
		cmp		bl, 'Z'
			jg		rNotCap
			cmp		bl, 'A'
			jl		rNotCap
			or bl, 0x20
			rNotCap :
			cmp		al, bl
			jl		retnLT
			jg		retnGT
			iterNext :
		inc		ecx
			inc		edx
			jmp		iterHead
			iterEnd :
		test	bl, bl
			jz		retnEQ
			retnLT :
		mov		al, -1
			pop		ebx
			retn
			retnGT :
		mov		al, 1
			pop		ebx
			retn
			retnEQ :
		xor al, al
			pop		ebx
			retn
	}
}

__declspec(naked) char __fastcall StrBeginsCS(const char* lstr, const char* rstr)
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
		comp1 :
		and eax, 3
			jz		retn1
			mov		ecx, eax
			repe cmpsb
			jnz		retn0
			retn1 :
		cmp[esi], 0
			setz	al
			inc		al
			pop		edi
			pop		esi
			retn
			retn0 :
		xor al, al
			pop		edi
			pop		esi
			retn
	}
}

__declspec(naked) char __fastcall StrBeginsCI(const char* lstr, const char* rstr)
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
		call	StrLen
		mov		edi, eax
		mov		ecx, edx
		call	StrLen
		cmp		eax, edi
		jg		retn0
		mov		edx, esi
		iterHead :
		cmp[ecx], 0
			jz		iterEnd
			call	CmprLetters
			test	al, al
			jz		retn0
			inc		ecx
			inc		edx
			jmp		iterHead
			iterEnd :
		cmp[edx], 0
			setz	al
			inc		al
			pop		edi
			pop		esi
			retn
			retn0 :
		xor al, al
			pop		edi
			pop		esi
			retn
	}
}

__declspec(naked) void __fastcall FixPath(char* str)
{
	__asm
	{
		test	ecx, ecx
		jz		done
		iterHead :
		mov		al, [ecx]
			test	al, al
			jz		done
			cmp		al, 'Z'
			jg		checkSlash
			cmp		al, 'A'
			jl		iterNext
			or byte ptr[ecx], 0x20
			jmp		iterNext
			checkSlash :
		cmp		al, '\\'
			jnz		iterNext
			mov		byte ptr[ecx], '/'
			iterNext :
			inc		ecx
			jmp		iterHead
			done :
		retn
	}
}

__declspec(naked) void __fastcall StrToLower(char* str)
{
	__asm
	{
		test	ecx, ecx
		jz		done
		iterHead :
		mov		al, [ecx]
			cmp		al, 'Z'
			jg		iterNext
			test	al, al
			jz		done
			cmp		al, 'A'
			jl		iterNext
			or byte ptr[ecx], 0x20
			iterNext:
		inc		ecx
			jmp		iterHead
			done :
		retn
	}
}

__declspec(naked) void __fastcall ReplaceChr(char* str, char from, char to)
{
	__asm
	{
		test	ecx, ecx
		jz		done
		mov		al, [esp + 4]
		iterHead:
		cmp[ecx], 0
			jz		done
			cmp[ecx], dl
			jnz		iterNext
			mov[ecx], al
			iterNext :
		inc		ecx
			jmp		iterHead
			done :
		retn	4
	}
}

__declspec(naked) char* __fastcall FindChr(const char* str, char chr)
{
	__asm
	{
		mov		eax, ecx
		test	ecx, ecx
		jz		done
		iterHead :
		cmp[eax], 0
			jz		retnNULL
			cmp[eax], dl
			jz		done
			inc		eax
			jmp		iterHead
			retnNULL :
		xor eax, eax
			done :
		retn
	}
}

__declspec(naked) char* __fastcall FindChrR(const char* str, UInt32 length, char chr)
{
	__asm
	{
		test	ecx, ecx
		jz		retnNULL
		lea		eax, [ecx + edx]
		mov		dl, [esp + 4]
		iterHead:
		cmp		eax, ecx
			jz		retnNULL
			dec		eax
			cmp[eax], dl
			jz		done
			jmp		iterHead
			retnNULL :
		xor eax, eax
			done :
		retn	4
	}
}

__declspec(naked) char* __fastcall SubStr(const char* srcStr, const char* subStr)
{
	__asm
	{
		push	ebx
		push	esi
		push	edi
		mov		esi, ecx
		mov		ecx, edx
		call	StrLen
		test	eax, eax
		jz		retnNULL
		mov		edi, edx
		mov		ebx, eax
		mov		ecx, esi
		call	StrLen
		sub		eax, ebx
		mov		ebx, eax
		mainHead :
		cmp		ebx, 0
			jl		retnNULL
			subHead :
		cmp[edx], 0
			jnz		proceed
			mov		eax, esi
			jmp		done
			proceed :
		call	CmprLetters
			test	al, al
			jz		mainNext
			inc		ecx
			inc		edx
			jmp		subHead
			mainNext :
		dec		ebx
			inc		esi
			mov		ecx, esi
			mov		edx, edi
			jmp		mainHead
			retnNULL :
		xor eax, eax
			done :
		pop		edi
			pop		esi
			pop		ebx
			retn
	}
}

__declspec(naked) char* __fastcall SlashPos(const char* str)
{
	__asm
	{
		mov		eax, ecx
		test	ecx, ecx
		jz		done
		iterHead :
		mov		cl, [eax]
			test	cl, cl
			jz		retnNULL
			cmp		cl, '/'
			jz		done
			cmp		cl, '\\'
			jz		done
			inc		eax
			jmp		iterHead
			retnNULL :
		xor eax, eax
			done :
		retn
	}
}

__declspec(naked) char* __fastcall SlashPosR(const char* str)
{
	__asm
	{
		call	StrEnd
		test	eax, eax
		jz		done
		iterHead :
		cmp		eax, ecx
			jz		retnNULL
			dec		eax
			mov		dl, [eax]
			cmp		dl, '/'
			jz		done
			cmp		dl, '\\'
			jz		done
			jmp		iterHead
			retnNULL :
		xor eax, eax
			done :
		retn
	}
}

__declspec(naked) char* __fastcall GetNextToken(char* str, char delim)
{
	__asm
	{
		push	ebx
		mov		eax, ecx
		xor bl, bl
		iterHead :
		mov		cl, [eax]
			test	cl, cl
			jz		done
			cmp		cl, dl
			jz		chrEQ
			test	bl, bl
			jnz		done
			jmp		iterNext
			chrEQ :
		test	bl, bl
			jnz		iterNext
			mov		bl, 1
			mov[eax], 0
			iterNext :
			inc		eax
			jmp		iterHead
			done :
		pop		ebx
			retn
	}
}

__declspec(naked) char* __fastcall GetNextToken(char* str, const char* delims)
{
	__asm
	{
		push	ebx
		push	esi
		mov		eax, ecx
		mov		esi, edx
		xor bl, bl
		mainHead :
		mov		cl, [eax]
			test	cl, cl
			jz		done
			subHead :
		cmp[edx], 0
			jz		wasFound
			cmp		cl, [edx]
			jz		chrEQ
			inc		edx
			jmp		subHead
			chrEQ :
		test	bl, bl
			jnz		mainNext
			mov		bl, 1
			mov[eax], 0
			mainNext :
			inc		eax
			mov		edx, esi
			jmp		mainHead
			wasFound :
		test	bl, bl
			jz		mainNext
			done :
		pop		esi
			pop		ebx
			retn
	}
}

__declspec(naked) char* __fastcall CopyString(const char* key)
{
	__asm
	{
		call	StrLen
		inc		eax
		push	eax
		push	ecx
		push	eax
		call	malloc
		pop		ecx
		pop		edx
		mov		ecx, eax
		call	MemCopy
		retn
	}
}

char* __fastcall CopyCString(const char* src)
{
	UInt32 length = StrLen(src);
	if (!length) return NULL;
	length++;
	char* result = (char*)GameHeapAlloc(length);
	MemCopy(result, src, length);
	return result;
}

__declspec(naked) char* __fastcall IntToStr(char* str, int num)
{
	__asm
	{
		push	esi
		push	edi
		test	edx, edx
		jns		skipNeg
		neg		edx
		mov[ecx], '-'
		inc		ecx
		skipNeg :
		mov		esi, ecx
			mov		edi, ecx
			mov		eax, edx
			mov		ecx, 0xA
			workIter :
			cdq
			div		ecx
			add		dl, '0'
			mov[esi], dl
			inc		esi
			test	eax, eax
			jnz		workIter
			mov[esi], 0
			mov		eax, esi
			swapIter :
		dec		esi
			cmp		esi, edi
			jbe		done
			mov		dl, [esi]
			mov		cl, [edi]
			mov[esi], cl
			mov[edi], dl
			inc		edi
			jmp		swapIter
			done :
		pop		edi
			pop		esi
			retn
	}
}

int FltToStr(char* str, double num)
{
	int printed = sprintf_s(str, 0x20, "%.6f", num);
	char* endPtr = str + printed;
	while (*--endPtr == '0')
		printed--;
	if (*endPtr == '.')
		printed--;
	str[printed] = 0;
	return printed;
}

__declspec(naked) int __fastcall StrToInt(const char* str)
{
	__asm
	{
		xor eax, eax
		test	ecx, ecx
		jnz		proceed
		retn
		proceed :
		push	esi
			mov		esi, ecx
			mov		ecx, eax
			cmp[esi], '-'
			setz	dl
			jnz		charIter
			inc		esi
			charIter :
		mov		cl, [esi]
			sub		cl, '0'
			cmp		cl, 9
			ja		iterEnd
			imul	eax, 0xA
			add		eax, ecx
			inc		esi
			jmp		charIter
			iterEnd :
		test	dl, dl
			jz		done
			neg		eax
			done :
		pop		esi
			retn
	}
}

__declspec(naked) float __fastcall StrToFlt(const char* str)
{
	static const UInt32 kFactor10Div[] = { 0x41200000, 0x42C80000, 0x447A0000, 0x461C4000, 0x47C35000, 0x49742400, 0x4B189680 };
	__asm
	{
		test	ecx, ecx
		jnz		proceed
		fldz
		retn
		proceed :
		push	ebx
			xor eax, eax
			xor ebx, ebx
			xorps	xmm0, xmm0
			cmp[ecx], '-'
			setz	dl
			jnz		intIter
			inc		ecx
			intIter :
		mov		bl, [ecx]
			sub		bl, '0'
			cmp		bl, 9
			ja		intEnd
			imul	eax, 0xA
			add		eax, ebx
			inc		ecx
			jmp		intIter
			intEnd :
		test	eax, eax
			jz		noInt
			cvtsi2ss	xmm0, eax
			xor eax, eax
			noInt :
		cmp		bl, 0xFE
			jnz		done
			mov		dh, 0xFF
			fracIter :
			inc		ecx
			mov		bl, [ecx]
			sub		bl, '0'
			cmp		bl, 9
			ja		fracEnd
			imul	eax, 0xA
			add		eax, ebx
			inc		dh
			cmp		dh, 6
			jb		fracIter
			fracEnd :
		test	eax, eax
			jz		done
			cvtsi2ss	xmm1, eax
			mov		bl, dh
			divss	xmm1, kFactor10Div[ebx * 4]
			addss	xmm0, xmm1
			done :
		movd	eax, xmm0
			push	eax
			fld		dword ptr[esp]
			pop		eax
			test	dl, dl
			jz		noSign
			test	eax, eax
			jz		noSign
			fchs
			noSign :
		pop		ebx
			retn
	}
}

__declspec(naked) char* __fastcall UIntToHex(char* str, UInt32 num)
{
	static const char kCharAtlas[] = "0123456789ABCDEF";
	__asm
	{
		push	esi
		push	edi
		mov		esi, ecx
		mov		edi, ecx
		xor eax, eax
		workIter :
		mov		al, dl
			and al, 0xF
			mov		cl, kCharAtlas[eax]
			mov[esi], cl
			inc		esi
			shr		edx, 4
			jnz		workIter
			mov[esi], 0
			mov		eax, esi
			swapIter :
		dec		esi
			cmp		esi, edi
			jbe		done
			mov		dl, [esi]
			mov		cl, [edi]
			mov[esi], cl
			mov[edi], dl
			inc		edi
			jmp		swapIter
			done :
		pop		edi
			pop		esi
			retn
	}
}

__declspec(naked) UInt32 __fastcall HexToUInt(const char* str)
{
	__asm
	{
		push	esi
		call	StrLen
		test	eax, eax
		jz		done
		lea		esi, [ecx + eax]
		mov		ch, al
		xor eax, eax
		xor cl, cl
		hexToInt :
		dec		esi
			movsx	edx, byte ptr[esi]
			sub		dl, '0'
			js		done
			cmp		dl, 9
			jle		doAdd
			or dl, 0x20
			cmp		dl, '1'
			jl		done
			cmp		dl, '6'
			jg		done
			sub		dl, 0x27
			doAdd:
		shl		edx, cl
			add		eax, edx
			add		cl, 4
			dec		ch
			jnz		hexToInt
			done :
		pop		esi
			retn
	}
}

__declspec(naked) int __stdcall GetRandomIntInRange(int from, int to)
{
	__asm
	{
		mov		eax, [esp + 8]
		sub		eax, [esp + 4]
		push	eax
		mov		eax, 0x476C00
		call	eax
		mov		ecx, eax
		mov		eax, 0xAA5230
		call	eax
		add		eax, [esp + 4]
		retn	8
	}
}

bool __fastcall FileExists(const char* path)
{
	UInt32 attr = GetFileAttributes(path);
	return (attr != INVALID_FILE_ATTRIBUTES) && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

bool FileStream::Open(const char* filePath)
{
	if (theFile) fclose(theFile);
	theFile = _fsopen(filePath, "rb", 0x20);
	return theFile ? true : false;
}

bool FileStream::OpenAt(const char* filePath, UInt32 inOffset)
{
	if (theFile) fclose(theFile);
	theFile = _fsopen(filePath, "rb", 0x20);
	if (!theFile) return false;
	fseek(theFile, 0, SEEK_END);
	if (ftell(theFile) < inOffset)
	{
		Close();
		return false;
	}
	fseek(theFile, inOffset, SEEK_SET);
	return true;
}

bool FileStream::OpenWrite(char* filePath, bool append)
{
	if (theFile) fclose(theFile);
	if (FileExists(filePath))
	{
		if (append)
		{
			theFile = _fsopen(filePath, "ab", 0x20);
			if (!theFile) return false;
			fputc('\n', theFile);
			fflush(theFile);
			return true;
		}
	}
	else MakeAllDirs(filePath);
	theFile = _fsopen(filePath, "wb", 0x20);
	return theFile ? true : false;
}

bool FileStream::Create(const char* filePath)
{
	if (theFile) fclose(theFile);
	theFile = _fsopen(filePath, "wb", 0x20);
	return theFile ? true : false;
}

UInt32 FileStream::GetLength()
{
	fseek(theFile, 0, SEEK_END);
	UInt32 result = ftell(theFile);
	rewind(theFile);
	return result;
}

void FileStream::SetOffset(UInt32 inOffset)
{
	fseek(theFile, 0, SEEK_END);
	if (ftell(theFile) > inOffset)
		fseek(theFile, inOffset, SEEK_SET);
}

char FileStream::ReadChar()
{
	return (char)fgetc(theFile);
}

void FileStream::ReadBuf(void* outData, UInt32 inLength)
{
	fread(outData, inLength, 1, theFile);
}

void FileStream::WriteChar(char chr)
{
	fputc(chr, theFile);
	fflush(theFile);
}

void FileStream::WriteStr(const char* inStr)
{
	fputs(inStr, theFile);
	fflush(theFile);
}

void FileStream::WriteBuf(const void* inData, UInt32 inLength)
{
	fwrite(inData, inLength, 1, theFile);
	fflush(theFile);
}

void FileStream::MakeAllDirs(char* fullPath)
{
	char* traverse = fullPath, curr;
	while (curr = *traverse)
	{
		if ((curr == '\\') || (curr == '/'))
		{
			*traverse = 0;
			CreateDirectory(fullPath, NULL);
			*traverse = '\\';
		}
		traverse++;
	}
}

bool DebugLog::Create(const char* filePath)
{
	theFile = _fsopen(filePath, "wb", 0x20);
	return theFile ? true : false;
}

void DebugLog::Message(const char* msgStr)
{
	if (!theFile) return;
	if (indent < 40)
		fputs(kIndentLevelStr + indent, theFile);
	fputs(msgStr, theFile);
	fputc('\n', theFile);
	fflush(theFile);
}

void DebugLog::FmtMessage(const char* fmt, va_list args)
{
	if (!theFile) return;
	if (indent < 40)
		fputs(kIndentLevelStr + indent, theFile);
	vfprintf(theFile, fmt, args);
	fputc('\n', theFile);
	fflush(theFile);
}

void PrintLog(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	s_log.FmtMessage(fmt, args);
	va_end(args);
}

void PrintDebug(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	s_debug.FmtMessage(fmt, args);
	va_end(args);
}

LineIterator::LineIterator(const char* filePath, char* buffer)
{
	dataPtr = buffer;
	FileStream sourceFile;
	if (!sourceFile.Open(filePath))
	{
		*buffer = 3;
		return;
	}
	UInt32 length = sourceFile.GetLength();
	sourceFile.ReadBuf(buffer, length);
	*(UInt16*)(buffer + length) = 0x300;
	while (length)
	{
		length--;
		if ((*buffer == '\n') || (*buffer == '\r'))
			*buffer = 0;
		buffer++;
	}
	while (!*dataPtr)
		dataPtr++;
}

void LineIterator::Next()
{
	while (*dataPtr)
		dataPtr++;
	while (!*dataPtr)
		dataPtr++;
}

bool __fastcall FileToBuffer(const char* filePath, char* buffer)
{
	FileStream srcFile;
	if (!srcFile.Open(filePath)) return false;
	UInt32 length = srcFile.GetLength();
	if (!length) return false;
	if (length > kMaxMessageLength)
		length = kMaxMessageLength;
	srcFile.ReadBuf(buffer, length);
	buffer[length] = 0;
	return true;
}

extern char* s_strArgBuffer;

void ClearFolder(char* pathEndPtr)
{
	DirectoryIterator dirIter(s_strArgBuffer);
	while (!dirIter.End())
	{
		if (dirIter.IsFolder())
			ClearFolder(StrCopy(StrCopy(pathEndPtr - 1, dirIter.Get()), "\\*"));
		else
		{
			StrCopy(pathEndPtr - 1, dirIter.Get());
			remove(s_strArgBuffer);
		}
		dirIter.Next();
	}
	dirIter.Close();
	*(pathEndPtr - 1) = 0;
	RemoveDirectory(s_strArgBuffer);
}

__declspec(naked) void __stdcall SafeWrite8(UInt32 addr, UInt32 data)
{
	__asm
	{
		push	ecx
		push	esp
		push	PAGE_EXECUTE_READWRITE
		push	4
		push	dword ptr[esp + 0x14]
		call	VirtualProtect
		mov		eax, [esp + 8]
		mov		edx, [esp + 0xC]
		mov[eax], dl
		mov		edx, [esp]
		push	esp
		push	edx
		push	4
		push	eax
		call	VirtualProtect
		pop		ecx
		retn	8
	}
}

__declspec(naked) void __stdcall SafeWrite16(UInt32 addr, UInt32 data)
{
	__asm
	{
		push	ecx
		push	esp
		push	PAGE_EXECUTE_READWRITE
		push	4
		push	dword ptr[esp + 0x14]
		call	VirtualProtect
		mov		eax, [esp + 8]
		mov		edx, [esp + 0xC]
		mov[eax], dx
		mov		edx, [esp]
		push	esp
		push	edx
		push	4
		push	eax
		call	VirtualProtect
		pop		ecx
		retn	8
	}
}

__declspec(naked) void __stdcall SafeWrite32(UInt32 addr, UInt32 data)
{
	__asm
	{
		push	ecx
		push	esp
		push	PAGE_EXECUTE_READWRITE
		push	4
		push	dword ptr[esp + 0x14]
		call	VirtualProtect
		mov		eax, [esp + 8]
		mov		edx, [esp + 0xC]
		mov[eax], edx
		mov		edx, [esp]
		push	esp
		push	edx
		push	4
		push	eax
		call	VirtualProtect
		pop		ecx
		retn	8
	}
}

__declspec(naked) void __stdcall SafeWriteBuf(UInt32 addr, void* data, UInt32 len)
{
	__asm
	{
		push	ecx
		push	esp
		push	PAGE_EXECUTE_READWRITE
		push	dword ptr[esp + 0x18]
		push	dword ptr[esp + 0x14]
		call	VirtualProtect
		push	dword ptr[esp + 0x10]
		mov		edx, [esp + 0x10]
		mov		ecx, [esp + 0xC]
		call	MemCopy
		mov		edx, [esp]
		push	esp
		push	edx
		push	dword ptr[esp + 0x18]
		push	eax
		call	VirtualProtect
		pop		ecx
		retn	0xC
	}
}

__declspec(naked) void __stdcall WriteRelJump(UInt32 jumpSrc, UInt32 jumpTgt)
{
	__asm
	{
		push	ecx
		push	esp
		push	PAGE_EXECUTE_READWRITE
		push	5
		push	dword ptr[esp + 0x14]
		call	VirtualProtect
		mov		eax, [esp + 8]
		mov		edx, [esp + 0xC]
		sub		edx, eax
		sub		edx, 5
		mov		byte ptr[eax], 0xE9
		mov[eax + 1], edx
		mov		edx, [esp]
		push	esp
		push	edx
		push	5
		push	eax
		call	VirtualProtect
		pop		ecx
		retn	8
	}
}

__declspec(naked) void __stdcall WriteRelCall(UInt32 jumpSrc, UInt32 jumpTgt)
{
	__asm
	{
		push	ecx
		push	esp
		push	PAGE_EXECUTE_READWRITE
		push	5
		push	dword ptr[esp + 0x14]
		call	VirtualProtect
		mov		eax, [esp + 8]
		mov		edx, [esp + 0xC]
		sub		edx, eax
		sub		edx, 5
		mov		byte ptr[eax], 0xE8
		mov[eax + 1], edx
		mov		edx, [esp]
		push	esp
		push	edx
		push	5
		push	eax
		call	VirtualProtect
		pop		ecx
		retn	8
	}
}

__declspec(naked) void __stdcall WritePushRetRelJump(UInt32 baseAddr, UInt32 retAddr, UInt32 jumpTgt)
{
	__asm
	{
		push	ecx
		push	esp
		push	PAGE_EXECUTE_READWRITE
		push	0xA
		push	dword ptr[esp + 0x14]
		call	VirtualProtect
		mov		eax, [esp + 8]
		mov		edx, [esp + 0xC]
		mov		byte ptr[eax], 0x68
		mov[eax + 1], edx
		mov		edx, [esp + 0x10]
		sub		edx, eax
		sub		edx, 0xA
		mov		byte ptr[eax + 5], 0xE9
		mov[eax + 6], edx
		mov		edx, [esp]
		push	esp
		push	edx
		push	0xA
		push	eax
		call	VirtualProtect
		pop		ecx
		retn	0xC
	}
}

void __fastcall GetTimeStamp(char* buffer)
{
	time_t rawTime = time(NULL);
	tm timeInfo;
	localtime_s(&timeInfo, &rawTime);
	sprintf_s(buffer, 0x10, "%02d:%02d:%02d", timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
}

struct ControlName
{
	UInt32		unk00;
	const char* name;
	UInt32		unk0C;
};

ControlName** g_keyNames = (ControlName**)0x11D52F0;
ControlName** g_mouseButtonNames = (ControlName**)0x11D5240;
ControlName** g_joystickNames = (ControlName**)0x11D51B0;

const char* __fastcall GetDXDescription(UInt32 keyID)
{
	const char* keyName = "<no key>";

	if (keyID <= 220)
	{
		if (g_keyNames[keyID])
			keyName = g_keyNames[keyID]->name;
	}
	else if (keyID < 255);
	else if (keyID == 255)
	{
		if (g_mouseButtonNames[0])
			keyName = g_mouseButtonNames[0]->name;
	}
	else if (keyID <= 263)
	{
		if (g_mouseButtonNames[keyID - 256])
			keyName = g_mouseButtonNames[keyID - 256]->name;
	}
	else if (keyID == 264)
		keyName = "WheelUp";
	else if (keyID == 265)
		keyName = "WheelDown";

	return keyName;
}

__declspec(naked) UInt32 __fastcall ByteSwap(UInt32 dword)
{
	__asm
	{
		mov		eax, ecx
		bswap	eax
		retn
	}
}

void DumpMemImg(void* data, UInt32 size, UInt8 extra)
{
	UInt32* ptr = (UInt32*)data;
	//Console_Print("Output");
	PrintDebug("\nDumping  %08X\n", ptr);
	for (UInt32 iter = 0; iter < size; iter += 4, ptr++)
	{
		if (!extra) PrintDebug("%03X\t\t%08X\t", iter, *ptr);
		else if (extra == 1) PrintDebug("%03X\t\t%08X\t[%08X]\t", iter, *ptr, ByteSwap(*ptr));
		else if (extra == 2) PrintDebug("%03X\t\t%08X\t%f", iter, *ptr, *(float*)ptr);
		else if (extra == 3) PrintDebug("%03X\t\t%08X\t[%08X]\t%f", iter, *ptr, ByteSwap(*ptr), *(float*)ptr);
		/*else
		{
			UInt32 addr = *ptr;
			if (!(addr & 3) && (addr > 0x08000000) && (addr < 0x34000000))
				PrintDebug("%03X\t\t%08X\t%08X\t", iter, *ptr, *(UInt32*)addr);
			else PrintDebug("%03X\t\t%08X\t", iter, *ptr);
		}*/
	}
}

/*void GetMD5File(const char *filePath, char *outHash)
{
	FileStream sourceFile;
	if (!sourceFile.Open(filePath)) return;

	MD5 md5;

	HANDLE handle = sourceFile.GetHandle();

	UInt8 buffer[0x400], digest[0x10];
	UInt32 offset = 0, length;

	while (!sourceFile.HitEOF())
	{
		ReadFile(handle, buffer, 0x400, &length, NULL);
		offset += length;
		sourceFile.SetOffset(offset);
		md5.MD5Update(buffer, length);
	}
	md5.MD5Final(digest);

	for (UInt8 idx = 0; idx < 0x10; idx++, outHash += 2)
		sprintf_s(outHash, 3, "%02X", digest[idx]);
}*/