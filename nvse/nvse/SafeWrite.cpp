#include "SafeWrite.h"

void SafeWrite8(UInt32 addr, const UInt32 data) {
	UInt32	oldProtect;

	VirtualProtect(reinterpret_cast<void *>(addr), 4, PAGE_EXECUTE_READWRITE, &oldProtect);
	*reinterpret_cast<UInt8 *>(addr) = static_cast<UInt8>(data);
	VirtualProtect(reinterpret_cast<void *>(addr), 4, oldProtect, &oldProtect);
}

void SafeWrite16(UInt32 addr, const UInt32 data) {
	UInt32	oldProtect;

	VirtualProtect(reinterpret_cast<void *>(addr), 4, PAGE_EXECUTE_READWRITE, &oldProtect);
	*reinterpret_cast<UInt16 *>(addr) = static_cast<UInt16>(data);
	VirtualProtect(reinterpret_cast<void *>(addr), 4, oldProtect, &oldProtect);
}

void SafeWrite32(UInt32 addr, const UInt32 data) {
	UInt32	oldProtect;

	VirtualProtect(reinterpret_cast<void *>(addr), 4, PAGE_EXECUTE_READWRITE, &oldProtect);
	*reinterpret_cast<UInt32 *>(addr) = static_cast<UInt32>(data);
	VirtualProtect(reinterpret_cast<void *>(addr), 4, oldProtect, &oldProtect);
}

void SafeWriteBuf(UInt32 addr, const void *data, const UInt32 len) {
	UInt32	oldProtect;

	VirtualProtect(reinterpret_cast<void *>(addr), len, PAGE_EXECUTE_READWRITE, &oldProtect);
	memcpy(reinterpret_cast<void *>(addr), data, len);
	VirtualProtect(reinterpret_cast<void *>(addr), len, oldProtect, &oldProtect);
}

void WriteRelJump(const UInt32 jumpSrc, const UInt32 jumpTgt) { // jmp rel32
	SafeWrite8(jumpSrc, 0xE9);
	SafeWrite32(jumpSrc + 1, jumpTgt - jumpSrc - 1 - 4);
}

void WriteRelCall(const UInt32 jumpSrc, const UInt32 jumpTgt) { // call rel32
	SafeWrite8(jumpSrc, 0xE8);
	SafeWrite32(jumpSrc + 1, jumpTgt - jumpSrc - 1 - 4);
}

void WriteRelJnz(const UInt32 jumpSrc, const UInt32 jumpTgt) { // jnz rel32
	
	SafeWrite16(jumpSrc, 0x850F);
	SafeWrite32(jumpSrc + 2, jumpTgt - jumpSrc - 2 - 4);
}

void WriteRelJle(const UInt32 jumpSrc, const UInt32 jumpTgt) { // jle rel32
	SafeWrite16(jumpSrc, 0x8E0F);
	SafeWrite32(jumpSrc + 2, jumpTgt - jumpSrc - 2 - 4);
}

void PatchMemoryNop(const ULONG_PTR Address, const SIZE_T Size) {
	DWORD d = 0;
	VirtualProtect(reinterpret_cast<LPVOID>(Address), Size, PAGE_EXECUTE_READWRITE, &d);

	for (SIZE_T i = 0; i < Size; i++)
		*reinterpret_cast<volatile BYTE*>(Address + i) = 0x90; //0x90 == opcode for NOP

	VirtualProtect(reinterpret_cast<LPVOID>(Address), Size, d, &d);
	FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<LPVOID>(Address), Size);
}