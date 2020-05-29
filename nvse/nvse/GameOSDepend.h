#pragma once

// keeping this in a separate file so we don't need to include dinput/dsound everywhere

#define DIRECTINPUT_VERSION 0x0800
#define DIRECTSOUND_VERSION 0x0800
#include <dinput.h>
//#include <dsound.h>

// 1C00 (1.0) / 1C04 (1.1)
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

	enum
	{
		kMaxControlBinds = 0x1C,
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

	UInt32		unk0000;							// 0000
	UInt32		flags;								// 0004
	void		* unk0008;							// 0008 IDirectInput8* ?
	UInt32		unk000C;							// 000C
	UInt32		unk0010;							// 0010
	UInt32		unk0014;							// 0014
	UInt32		unk0018;							// 0018
	UInt32		unk001C;							// 001C
	UInt32		unk0020;							// 0020
	UInt32		unk0024;							// 0024
	UInt32		unk0028;							// 0028
	void		* unk002C;							// 002C
	void		* unk0030;							// 0030
	UInt32		unk0034[(0x1A30 - 0x0034) >> 2];	// 0034
	UInt32		unk1A30;							// 1A30
	UInt32		unk1A34[(0x1AF8 - 0x1A34) >> 2];	// 1A34
	UInt32		unk1AF8;							// 1AF8
	UInt32		unk1AFC;							// 1AFC
	UInt32		unk1B00;							// 1B00
	UInt32		unk1B04;							// 1B04
	UInt32		unk1B08;							// 1B08
	UInt32		unk1B0C[(0x1B50 - 0x1B0C) >> 2];	// 1B0C
	UInt32		unk1B50;							// 4 bytes added between +2C and here for v1.1.0.35
	UInt32		oldDoubleClickTime;					// 1B50 (1.0) / 1B54 (1.1)
	UInt32		unk1B54[(0x1B90 - 0x1B54) >> 2];	// 1B54 / 1B58	// Byte at 1B88 referenced
	UInt8		keyBinds[kMaxControlBinds];			// 1B90 / 1B94
	UInt8		mouseBinds[kMaxControlBinds];		// 1BAC / 1BB0
	UInt8		joystickBinds[kMaxControlBinds];	// 1BC8 / 1BCC
	UInt32		unk1BE4[(0x1C00 - 0x1BE4) >> 2];	// 1BE4 / 1BE8
};

#if FALLOUT_VERSION < FALLOUT_VERSION_1_1_35

STATIC_ASSERT(sizeof(OSInputGlobals) == 0x1C00);
STATIC_ASSERT(offsetof(OSInputGlobals, mouseBinds) == 0x1BAC);

#else

STATIC_ASSERT(sizeof(OSInputGlobals) == 0x1C04);
STATIC_ASSERT(offsetof(OSInputGlobals, mouseBinds) == 0x1BB0);

#endif

extern OSInputGlobals** g_OSInputGlobals;

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
	UInt8			quitGame;			// 01	// The seven are initialized to 0, this one is set by QQQ
	UInt8			exitToMainMenu;		// 02
	UInt8			unk03;				// 03
	UInt8			unk04;				// 04
	UInt8			unk05;				// 05
	UInt8			unk06;				// 06	// This looks promising as TFC bool byte
	UInt8			unk07;				// 07
	HWND			window;				// 08
	HINSTANCE		procInstance;		// 0C
	UInt32			mainThreadID;		// 10
	HANDLE			mainThreadHandle;	// 14
	UInt32*			unk18;				// 18 ScrapHeapManager::Buffer*
	UInt32			unk1C;				// 1C
	OSInputGlobals	* input;			// 20
	OSSoundGlobals	* sound;			// 24
	UInt32			unk28;				// 28 relates to unk18
	//...
	UInt32*			unk50;				// 50, same object as unk18
	//..
	UInt32			unk60;				// 60 relates to unk50
};

//STATIC_ASSERT(sizeof(OSGlobals) == 0x0A4);	// found in oldWinMain 0x0086AF4B

extern OSGlobals ** g_osGlobals;
