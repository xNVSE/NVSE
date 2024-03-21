#pragma once

#define __HOOK __declspec(naked) void

void SafeWrite8(UInt32 addr, UInt32 data);
void SafeWrite16(UInt32 addr, UInt32 data);
void SafeWrite32(UInt32 addr, UInt32 data);
void SafeWriteBuf(UInt32 addr, void * data, UInt32 len);

// 5 bytes
void WriteRelJump(UInt32 jumpSrc, UInt32 jumpTgt);
void WriteRelCall(UInt32 jumpSrc, UInt32 jumpTgt);

// 6 bytes
void WriteRelJnz(UInt32 jumpSrc, UInt32 jumpTgt);
void WriteRelJle(UInt32 jumpSrc, UInt32 jumpTgt);

void PatchMemoryNop(ULONG_PTR Address, SIZE_T Size);

template <typename T>
void WriteRelCall(UInt32 jumpSrc, T jumpTgt)
{
	WriteRelCall(jumpSrc, (UInt32)jumpTgt);
}

template <typename T>
void WriteRelJump(UInt32 jumpSrc, T jumpTgt)
{
	WriteRelJump(jumpSrc, (UInt32)jumpTgt);
}

// Credits to lStewieAl
[[nodiscard]] UInt32 __stdcall DetourVtable(UInt32 addr, UInt32 dst);

// From lStewieAl
// Returns the address of the jump/called function, assuming there is one.
UInt32 GetRelJumpAddr(UInt32 jumpSrc);

// Stores the function-to-call before overwriting it, to allow calling the overwritten function after our hook is over.
class CallDetour
{
	UInt32 overwritten_addr = 0;
public:
	void WriteRelCall(UInt32 jumpSrc, UInt32 jumpTgt)
	{
		if (*reinterpret_cast<UInt8*>(jumpSrc) != 0xE8) {
			_ERROR("Cannot write detour; jumpSrc is not a function call.");
			return;
		}
		overwritten_addr = GetRelJumpAddr(jumpSrc);
		::WriteRelCall(jumpSrc, jumpTgt);
	}
	[[nodiscard]] UInt32 GetOverwrittenAddr() const { return overwritten_addr; }
};