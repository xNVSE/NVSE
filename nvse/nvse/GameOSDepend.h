#pragma once

#include <dinput.h>

// keeping this in a separate file so we don't need to include dinput/dsound everywhere

#define DIRECTSOUND_VERSION 0x0800

class NiCamera;

static constexpr auto kMaxControlBinds = 0x1C;

enum XboxControlCode
{
	kXboxCtrl_DPAD_UP = 1,
	kXboxCtrl_DPAD_DOWN,
	kXboxCtrl_DPAD_RIGHT = 4,
	kXboxCtrl_DPAD_LEFT,
	kXboxCtrl_START,
	kXboxCtrl_BACK,
	kXboxCtrl_LS_BUTTON,
	kXboxCtrl_RS_BUTTON,
	kXboxCtrl_BUTTON_A,
	kXboxCtrl_BUTTON_B,
	kXboxCtrl_BUTTON_X,
	kXboxCtrl_BUTTON_Y,
	kXboxCtrl_RB,
	kXboxCtrl_LB,
	kXboxCtrl_LT,
	kXboxCtrl_RT,
	kXboxCtrl_LS_UP = 0x13,
	kXboxCtrl_LS_DOWN,
	kXboxCtrl_LS_RIGHT = 0x16,
	kXboxCtrl_LS_LEFT,
};

enum KeyState;
enum ControlCode;

// 1C04
class OSInputGlobals
{
public:
	enum
	{
		kFlag_HasJoysticks =	1 << 0,
		kFlag_HasMouse =		1 << 1,
		kFlag_HasKeyboard =		1 << 2,
		kFlag_BackgroundMouse =	1 << 3,
	};

	// Have not verified nothing has changed here so commenting out (no controllers to test with currently)
#if 0
	enum
	{
		kMaxDevices = 8,
	};

	OSInputGlobals();
	~OSInputGlobals();

	// 244
	class Joystick
	{
	public:
		Joystick();
		~Joystick();

		UInt32	unk000[0x244 >> 2];
	};

	struct JoystickObjectsInfo
	{
		enum
		{
			kHasXAxis =		1 << 0,
			kHasYAxis =		1 << 1,
			kHasZAxis =		1 << 2,
			kHasXRotAxis =	1 << 3,
			kHasYRotAxis =	1 << 4,
			kHasZRotAxis =	1 << 5
		};

		UInt32	axis;
		UInt32	buttons;
	};

	// 2C
	struct Unk1AF4
	{
		UInt32	bufLen;
		UInt8	unk04[0x2C - 4];
	};

	// 28
	struct Unk1B20
	{
		UInt32	unk00;
		UInt32	unk04;
		UInt32	unk08;
		UInt32	unk0C;
		UInt32	unk10;
		UInt32	unk14;
		UInt32	unk18;
		UInt32	unk1C;
		UInt32	unk20;
		UInt32	unk24;
	};
#endif

	enum Device
	{
		kKeyboard = 0x0,
		kMouse = 0x1,
		kJoystick = 0x2,
		kController = 0x3,
	};

	UInt8			isControllerDisabled;	// 0000
	UInt8			byte0001;				// 0001
	UInt8			byte0002;				// 0002
	UInt8			byte0003;				// 0003
	UInt32			flags;					// 0004
	IDirectInput8	*directInput;			// 0008
	UInt32			unk000C;				// 000C 0xA230F3
	UInt32			unk0010;				// 0010
	UInt32			unk0014;				// 0014
	UInt32			unk0018;				// 0018
	UInt32			unk001C;				// 001C
	UInt32			unk0020;				// 0020
	UInt32			unk0024;				// 0024
	UInt32			unk0028;				// 0028
	void			*unk002C;				// 002C 0xA237BC, 0xA2315F
	void			*unk0030;				// 0030 0xA23201, 0xA23C0C
	
	// joystick
	UInt32			unk0034[320];			// 0034 0xA234D9 0xA23612
	UInt32			unk534[1264];			// 0534 0xA236C0
	UInt32			unk18F4;				// 18F4 0xA24896

	UInt8			currKeyStates[256];		// 18F8
	UInt8			lastKeyStates[256];		// 19F8
	UInt32			unk1AF8[11];			// 1AF8 0xA23A3A
	int				xDelta;					// 1B24
	int				yDelta;					// 1B28
	int				mouseWheelScroll;		// 1B2C
	UInt8			currButtonStates[8];	// 1B30
	int				lastxDelta;				// 1B38
	int				lastyDelta;				// 1B3C
	int				lastMouseWheelScroll;	// 1B40
	UInt8			lastButtonStates[8];	// 1B44
	UInt32			ltrtButtonState;		// 1B4C
	UInt8			mouseSensitivity;		// 1B50 0x711D6A 0xA22821
	UInt8			byte1B51;				// 1B51 
	UInt8			byte1B52;				// 1B52 
	UInt8			byte1B53;				// 1B53 
	UInt32			doubleClickTime;		// 1B54
	UInt8			buttonStates1B58[8];	// 1B58
	UInt32			unk1B60[8];				// 1B60
	UInt32			*controllerVibration;	// 1B80
	UInt32			unk1B84;				// 1B84
	UInt8			isControllerEnabled;	// 1B88 0xA257D7
	UInt8			byte1B89;				// 1B89
	UInt8			byte1B8A;				// 1B8A
	UInt8			byte1B8B;				// 1B8B
	UInt32			unk1B8C;				// 1B8C
	UInt8			byte1B90;				// 1B90
	UInt8			byte1B91;				// 1B91
	UInt16			overrideFlags;			// 1B92 bit 3 used for adding a debounce to the MenuMode control
	UInt8			keyBinds[28];			// 1B94
	UInt8			mouseBinds[28];			// 1BB0
	UInt8			joystickBinds[28];		// 1BCC
	UInt8			controllerBinds[28];	// 1BE8

	void ResetMouseButtons();
	void ResetPressedControls();

	bool GetControlState(ControlCode code, KeyState state) { return ((bool (__thiscall*)(OSInputGlobals*, ControlCode, KeyState))(0xA24660))(this, code, state); }
	void SetControlHeld(ControlCode code) { ((void(__thiscall*)(OSInputGlobals*, ControlCode))(0xA24280))(this, code); };
	bool GetMouseState(int buttonID, KeyState state) { return ((bool(__thiscall*)(OSInputGlobals*, int, KeyState))(0xA23A50))(this, buttonID, state); };
	bool SetKeybindIfValid(ControlCode whichBind, OSInputGlobals::Device device, UInt8 scancode) { return ((bool(__thiscall*)(OSInputGlobals*, ControlCode whichBind, OSInputGlobals::Device device, UInt8 scancode))(0xA23A50))(this, whichBind, device, scancode); };
	bool GetKeyState(int key, KeyState state) { return 	((bool(__thiscall*)(OSInputGlobals*, int, KeyState))(0xA24180))(this, key, state);};
	static OSInputGlobals* GetSingleton() { return *(OSInputGlobals * *)(0x11F35CC); }
};
STATIC_ASSERT(sizeof(OSInputGlobals) == 0x1C04);

#if 0
#include "GameTypes.h"

class TESGameSound;
class NiAVObject;

// 58
class TESGameSound
{
public:
	TESGameSound();
	~TESGameSound();

	UInt32			unk00[3];	// 00
	UInt32			hashKey;	// 0C
	UInt32			unk10[4];	// 10
	float			x;			// 20
	float			y;			// 24
	float			z;			// 28
	UInt32			unk2C[4];	// 2C
	float			unk3C;		// 3C
	UInt32			unk40[3];	// 40
	const char *	name;		// 4C
	UInt32			unk50;		// 50
	UInt32			unk54;		// 54
};

// 328
class OSSoundGlobals
{
public:
	OSSoundGlobals();
	~OSSoundGlobals();

	enum
	{
		kFlags_HasDSound =		1 << 0,
		kFlags_HasHardware3D =	1 << 2,
	};
	
	typedef NiTPointerMap <TESGameSound>	TESGameSoundMap;
	typedef NiTPointerMap <NiAVObject>		NiAVObjectMap;

	UInt32					unk000;						// 000
	UInt32					unk004;						// 004
	IDirectSound8			* dsoundInterface;			// 008
	IDirectSoundBuffer8		* primaryBufferInterface;	// 00C
	DSCAPS					soundCaps;					// 010
	UInt32					unk070;						// 070
	UInt32					unk074;						// 074
	IDirectSound3DListener	* listenerInterface;		// 078
	UInt32					unk07C[(0x0A4-0x07C) >> 2];	// 07C
	UInt8					unk0A4;						// 0A4
	UInt8					unk0A5;						// 0A5
	UInt8					unk0A6;						// 0A6
	UInt8					pad0A7;						// 0A7
	UInt32					unk0A8;						// 0A8
	UInt32					flags;						// 0AC - flags?
	UInt32					unk0B0;						// 0B0
	float					unk0B4;						// 0B4
	float					masterVolume;				// 0B8
	float					footVolume;					// 0BC
	float					voiceVolume;				// 0C0
	float					effectsVolume;				// 0C4
	UInt32					unk0C8;						// 0C8 - time
	UInt32					unk0CC;						// 0CC - time
	UInt32					unk0D0;						// 0D0 - time
	UInt32					unk0D4[(0x0DC-0x0D4) >> 2];	// 0D4
	UInt32					unk0DC;						// 0DC
	UInt32					unk0E0[(0x2F0-0x0E0) >> 2];	// 0E0
	float					musicVolume;				// 2F0
	UInt32					unk2F4;						// 2F4
	float					musicVolume2;				// 2F8
	UInt32					unk2FC;						// 2FC
	TESGameSoundMap			* gameSoundMap;				// 300
	NiAVObjectMap			* niObjectMap;				// 304
	NiTPointerList <void>	* soundMessageMap;			// 308 - AudioManager::SoundMessage *
	UInt32					unk30C[(0x320-0x30C) >> 2];	// 30C
	void					* soundMessageList;			// 320
	UInt32					unk324;						// 324
};

STATIC_ASSERT(sizeof(OSSoundGlobals) == 0x328);
#endif

class OSSoundGlobals {
};

// A4
class OSGlobals
{
public:
	OSGlobals();
	~OSGlobals();

	UInt8			oneMore;			// 00
	UInt8			quitGame;			// 01	The seven are initialized to 0, this one is set by QQQ
	UInt8			exitToMainMenu;		// 02
	UInt8			unk03;				// 03
	UInt8			unk04;				// 04
	UInt8			unk05;				// 05
	UInt8			isFlyCam;				// 06	This looks promising as TFC status byte
	bool			freezeTime;			// 07
	HWND			window;				// 08
	HINSTANCE		procInstance;		// 0C
	UInt32			mainThreadID;		// 10
	HANDLE			mainThreadHandle;	// 14
	UInt32			*unk18;				// 18	ScrapHeapManager::Buffer*
	UInt32			unk1C;				// 1C
	OSInputGlobals	*input;				// 20
	OSSoundGlobals	*sound;				// 24
	UInt32			unk28;				// 28	relates to unk18
	UInt32 unk2C;
	UInt32 unk30;
	UInt32 unk34;
	UInt32 unk38;
	UInt32 unk3C;
	UInt32 unk40;
	UInt32 unk44;
	UInt32 unk48;
	UInt32 unk4C;
	UInt32 unk50;
	UInt32 unk54;
	UInt32 unk58;
	UInt32 unk5C;
	UInt32 unk60;
	UInt32 unk64;
	UInt32 unk68;
	UInt32 unk6C;
	UInt32 unk70;
	UInt32 unk74;
	UInt32 unk78;
	UInt32 unk7C;
	UInt32 unk80;
	UInt32 unk84;
	UInt32 unk88;
	UInt32 ptr8C;
	UInt32 unk90;
	UInt32 unk94;
	UInt32 unk98;
	UInt8 byte9C;
	UInt8 gap9D[3];
	NiCamera* cameraA0;

	static OSGlobals* GetSingleton() { return *(OSGlobals * *)0x011DEA0C; };
};
STATIC_ASSERT(sizeof(OSGlobals) == 0x0A4);	// found in oldWinMain 0x0086AF4B

extern OSGlobals ** g_osGlobals;
extern OSInputGlobals** g_OSInputGlobals;
