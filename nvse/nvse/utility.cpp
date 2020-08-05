#include "utility.h"
#include <cmath>
#include <cstdarg>
#include <ctime>
//#include "internal/md5/md5.h"

UInt8 s_cpIteratorBuffer[0x1000];

bool fCompare(float lval, float rval)
{
	return fabs(lval - rval) < FLT_EPSILON;
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

const UInt8 kCaseConverter[] =
"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F\
\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2A\x2B\x2C\x2D\x2E\x2F\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3A\x3B\x3C\x3D\x3E\x3F\
\x40\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x5B\x5C\x5D\x5E\x5F\
\x60\x61\x62\x63\x64\x65\x66\x67\x68\x69\x6A\x6B\x6C\x6D\x6E\x6F\x70\x71\x72\x73\x74\x75\x76\x77\x78\x79\x7A\x7B\x7C\x7D\x7E\x7F\
\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F\
\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF\
\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF\
\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF";


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

extern char* s_strArgBuffer;

void __fastcall GetTimeStamp(char* buffer)
{
	time_t rawTime = time(NULL);
	tm timeInfo;
	localtime_s(&timeInfo, &rawTime);
	sprintf_s(buffer, 0x10, "%02d-%02d-%02d", timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
}

struct ControlName
{
	UInt32		unk00;
	const char* name;
	UInt32		unk0C;
};

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

__declspec(naked) UInt32 __fastcall GetHighBit(UInt32 num)
{
	__asm
	{
		mov		eax, 0x80000000
	iterHead:
		test	ecx, eax
		jnz		done
		shr		eax, 1
		jnz		iterHead
	done:
		retn
	}
}

__declspec(naked) void* __fastcall MemCopy(void *dest, const void *src, UInt32 length)
{
	__asm
	{
		push	ecx
		push	ebx
		test	ecx, ecx
		jz		done
		test	edx, edx
		jz		done
		mov		ebx, [esp+0xC]
		test	ebx, ebx
		jz		done
		mov		eax, ecx
		sub		eax, edx
		js		copy10F
		jz		done
		cmp		eax, ebx
		jb		revCopy
	copy10F:
		cmp		ebx, 0x10
		jb		copy8F
		movdqu	xmm0, xmmword ptr [edx]
		movdqu	xmmword ptr [ecx], xmm0
		sub		ebx, 0x10
		jz		done
		add		ecx, 0x10
		add		edx, 0x10
		jmp		copy10F
	copy8F:
		test	bl, 8
		jz		copy4F
		movq	xmm0, qword ptr [edx]
		movq	qword ptr [ecx], xmm0
		and		bl, 7
		jz		done
		add		ecx, 8
		add		edx, 8
	copy4F:
		test	bl, 4
		jz		copy2F
		mov		eax, [edx]
		mov		[ecx], eax
		and		bl, 3
		jz		done
		add		ecx, 4
		add		edx, 4
	copy2F:
		test	bl, 2
		jz		copy1F
		mov		ax, [edx]
		mov		[ecx], ax
		and		bl, 1
		jz		done
		add		ecx, 2
		add		edx, 2
	copy1F:
		mov		al, [edx]
		mov		[ecx], al
	done:
		pop		ebx
		pop		eax
		retn	4
	revCopy:
		add		ecx, ebx
		add		edx, ebx
	copy10B:
		cmp		ebx, 0x10
		jb		copy8B
		sub		ecx, 0x10
		sub		edx, 0x10
		movdqu	xmm0, xmmword ptr [edx]
		movdqu	xmmword ptr [ecx], xmm0
		sub		ebx, 0x10
		jz		done
		jmp		copy10B
	copy8B:
		test	bl, 8
		jz		copy4B
		sub		ecx, 8
		sub		edx, 8
		movq	xmm0, qword ptr [edx]
		movq	qword ptr [ecx], xmm0
		and		bl, 7
		jz		done
	copy4B:
		test	bl, 4
		jz		copy2B
		sub		ecx, 4
		sub		edx, 4
		mov		eax, [edx]
		mov		[ecx], eax
		and		bl, 3
		jz		done
	copy2B:
		test	bl, 2
		jz		copy1B
		sub		ecx, 2
		sub		edx, 2
		mov		ax, [edx]
		mov		[ecx], ax
		and		bl, 1
		jz		done
	copy1B:
		mov		al, [edx-1]
		mov		[ecx-1], al
		jmp		done
	}
}


__declspec(naked) UInt32 __fastcall StrHash(const char *inKey)
{
	__asm
	{
		xor		eax, eax
		test	ecx, ecx
		jz		done
		xor		edx, edx
	iterHead:
		mov		dl, [ecx]
		test	dl, dl
		jz		done
		mov		dl, kCaseConverter[edx]
		imul	eax, 0x65
		add		eax, edx
		inc		ecx
		jmp		iterHead
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
		call	malloc
		pop		ecx
		pop		edx
		mov		ecx, eax
		call	MemCopy
		retn
	}
}
