#pragma once
#include "common/ITypes.h"

enum
{
	kDeviceType_Keyboard = 1,
	kDeviceType_Mouse
};

enum
{
	// first 256 for keyboard, then 8 mouse buttons, then mouse wheel up, wheel down
	kMacro_MouseButtonOffset = 256,
	kMacro_MouseWheelOffset = kMacro_MouseButtonOffset + 8,

	kMaxMacros = kMacro_MouseWheelOffset + 2,
};

class DIHookControl
{
public:
	enum
	{
		// data sources
		kFlag_GameState = 1 << 0,	// input passed to game post-filtering
		kFlag_RawState = 1 << 1,	// user input
		kFlag_InsertedState = 1 << 2,	// keydown was inserted by script
		kFlag_Pressed = kFlag_GameState | kFlag_RawState | kFlag_InsertedState,

		// modifiers
		kFlag_IgnoreDisabled_User = 1 << 3,	// ignore user-disabled keys
		kFlag_IgnoreDisabled_Script = 1 << 4,	// ignore script-disabled keys
		kFlag_IgnoreDisabled = kFlag_IgnoreDisabled_User | kFlag_IgnoreDisabled_Script,

		kFlag_DefaultBackCompat = kFlag_GameState,
	};

	enum
	{
		kDisable_User = 1 << 0,
		kDisable_Script = 1 << 1,

		kDisable_All = kDisable_User | kDisable_Script,
	};

	bool IsKeyPressed(UInt32 keycode, UInt8 flags = 1);
	bool IsKeyPressedRaw(UInt32 keycode);
	bool IsLMBPressed();
	bool IsKeyDisabled(UInt32 keycode);
	bool IsKeyHeld(UInt32 keycode);
	bool IsKeyTapped(UInt32 keycode);

	void SetKeyDisableState(UInt32 keycode, bool bDisable, UInt8 mask);
	void SetLMBDisabled(bool bDisable);
	void SetKeyHeldState(UInt32 keycode, bool bHold);
	void TapKey(UInt32 keycode);

private:
	struct KeyInfo
	{
		bool	rawState;		// state from dinput last update
		bool	gameState;		// state sent to the game last update
		bool	insertedState;	// true if a script pushed/held this key down last update

		bool	hold;			// key is held down
		bool	tap;			// key is being tapped
		bool	userDisable;	// key cannot be pressed by user
		bool	scriptDisable;	// key cannot be pressed by script
	};

	void** vtbl;
	KeyInfo		m_keys[kMaxMacros];
};
STATIC_ASSERT(sizeof(DIHookControl) == 0x74C);

__declspec(naked) bool DIHookControl::IsKeyPressed(UInt32 keycode, UInt8 flags)
{
	__asm
	{
		mov		eax, [esp + 4]
		cmp		eax, kMaxMacros
		jb		proceed
		xor al, al
		retn	8
		proceed:
		mov		dl, [esp + 8]
			test	dl, dl
			jnz		doneFlag
			mov		dl, 1
			doneFlag :
			lea		ecx, [ecx + eax * 8 + 4]
			sub		ecx, eax
			mov		al, [ecx + 2]
			shl		al, 1
			or al, [ecx]
			shl		al, 1
			or al, [ecx + 1]
			test	al, dl
			setnz	al
			retn	8
	}
}

bool DIHookControl::IsKeyPressedRaw(UInt32 keycode)
{
	return (keycode < kMaxMacros) && m_keys[keycode].rawState;
}

bool DIHookControl::IsLMBPressed()
{
	return m_keys[0x100].rawState;
}

bool DIHookControl::IsKeyDisabled(UInt32 keycode)
{
	return (keycode < kMaxMacros) && (m_keys[keycode].userDisable || m_keys[keycode].scriptDisable);
}

bool DIHookControl::IsKeyHeld(UInt32 keycode)
{
	return (keycode < kMaxMacros) && m_keys[keycode].hold;
}

bool DIHookControl::IsKeyTapped(UInt32 keycode)
{
	return (keycode < kMaxMacros) && m_keys[keycode].tap;
}

void DIHookControl::SetKeyDisableState(UInt32 keycode, bool bDisable, UInt8 mask)
{
	if (!mask) mask = kDisable_All;
	if (keycode < kMaxMacros)
	{
		KeyInfo* info = &m_keys[keycode];
		if (mask & kDisable_User)
			info->userDisable = bDisable;
		if (mask & kDisable_Script)
			info->scriptDisable = bDisable;
	}
}

void DIHookControl::SetLMBDisabled(bool bDisable)
{
	m_keys[0x100].userDisable = bDisable;
}

void DIHookControl::SetKeyHeldState(UInt32 keycode, bool bHold)
{
	if (keycode < kMaxMacros)
		m_keys[keycode].hold = bHold;
}

void DIHookControl::TapKey(UInt32 keycode)
{
	if (keycode < kMaxMacros)
		m_keys[keycode].tap = true;
}