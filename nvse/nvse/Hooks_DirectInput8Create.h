#pragma once

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <queue>

void Hook_DirectInput8Create_Init();

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

class DIHookControl : public ISingleton <DIHookControl>
{
public:
	enum
	{
		// data sources
		kFlag_GameState =				1 << 0,	// input passed to game post-filtering
		kFlag_RawState =				1 << 1,	// user input
		kFlag_InsertedState =			1 << 2,	// keydown was inserted by script

		// modifiers
		kFlag_IgnoreDisabled_User =		1 << 3,	// ignore user-disabled keys
		kFlag_IgnoreDisabled_Script =	1 << 4,	// ignore script-disabled keys

		kFlag_DefaultBackCompat =		kFlag_GameState,
	};

	enum
	{
		kDisable_User =		1 << 0,
		kDisable_Script =	1 << 1,

		kDisable_All =		kDisable_User | kDisable_Script,
	};

	DIHookControl();

	bool	IsKeyPressed(UInt32 keycode, UInt32 flags = 0);
	bool	IsKeyDisabled(UInt32 keycode);
	bool	IsKeyHeld(UInt32 keycode);
	bool	IsKeyTapped(UInt32 keycode);

	void	SetKeyDisableState(UInt32 keycode, bool bDisable, UInt32 mask);
	void	SetKeyHeldState(UInt32 keycode, bool bHold);
	void	TapKey(UInt32 keycode);

	void	BufferedKeyTap(UInt32 key);
	void	BufferedKeyPress(UInt32 key);
	void	BufferedKeyRelease(UInt32 key);

	// hook interface
	void	ProcessKeyboardData(UInt8 * data);
	void	ProcessMouseData(DIMOUSESTATE2 * data);
	HRESULT	ProcessBufferedData(IDirectInputDevice8 * device, DWORD dataSize, DIDEVICEOBJECTDATA * outData, DWORD * outDataLen, DWORD flags);

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

		bool	Process(bool keyDown, UInt32 idx);
	};

	KeyInfo	m_keys[kMaxMacros];

	typedef std::queue <DIDEVICEOBJECTDATA>	BufferedPressQueue;
	BufferedPressQueue	m_bufferedPresses;
};

class FramerateTracker
{
public:
	FramerateTracker();

	void	Update(void);

private:
	enum
	{
		kFrameTimeHistoryLength = 32
	};

	UInt32	m_lastTime;
	float	m_lastFrameLength;	// frametime in seconds

	float	m_frameTimeHistory[kFrameTimeHistoryLength];	// last kFrameTimeHistoryLength frametimes
	UInt32	m_frameTimeHistoryIdx;		// slot that will be filled on the next update call
	bool	m_frameTimeHistoryPrimed;	// true after history buffer is full

	float	m_averageFrameTime;	// average of m_frameTimeHistory
};

extern FramerateTracker	g_framerateTracker;
