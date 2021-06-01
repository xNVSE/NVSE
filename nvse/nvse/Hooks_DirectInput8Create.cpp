#include "Hooks_DirectInput8Create.h"
#include "SafeWrite.h"
#include <queue>

static const GUID GUID_SysMouse		= { 0x6F1D2B60, 0xD5A0, 0x11CF, { 0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00} };
static const GUID GUID_SysKeyboard	= { 0x6F1D2B61, 0xD5A0, 0x11CF, { 0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00} };

typedef HRESULT (_stdcall * CreateDInputProc)(HINSTANCE, DWORD, REFIID, LPVOID, LPUNKNOWN);

DIHookControl			g_diHookData;
FramerateTracker		g_framerateTracker;
static CreateDInputProc	DICreate_RealFunc;

// don't allow modification of these keys
static bool ShouldIgnoreKey(DWORD code)
{
	return false;	// whatever, mods can be malicious in easier ways
}

class FakeDirectInputDevice : public IDirectInputDevice8
{
public:
	FakeDirectInputDevice(IDirectInputDevice8 * device, DWORD type)
		:m_device(device), m_deviceType(type), m_refs(1)
	{
		//
	}

	HRESULT _stdcall QueryInterface (REFIID riid, LPVOID * ppvObj)
	{
		return m_device->QueryInterface(riid,ppvObj);
	}

	ULONG _stdcall AddRef(void)
	{
		m_refs++;

		return m_refs;
	}

	ULONG _stdcall Release(void)
	{
		m_refs--;

		if(!m_refs)
		{
			m_device->Release();
			delete this;
			return 0;
		}
		else
		{
			return m_refs;
		}
	}

	// IDirectInputDevice8A
	HRESULT _stdcall GetCapabilities(LPDIDEVCAPS a) { return m_device->GetCapabilities(a); }
	HRESULT _stdcall EnumObjects(LPDIENUMDEVICEOBJECTSCALLBACKA a,LPVOID b,DWORD c) { return m_device->EnumObjects(a,b,c); }
	HRESULT _stdcall GetProperty(REFGUID a,DIPROPHEADER* b) { return m_device->GetProperty(a,b); }
	HRESULT _stdcall SetProperty(REFGUID a,const DIPROPHEADER* b) { return m_device->SetProperty(a,b); }
	HRESULT _stdcall Acquire(void) { return m_device->Acquire(); }
	HRESULT _stdcall Unacquire(void) { return m_device->Unacquire(); }

	HRESULT _stdcall GetDeviceState(DWORD outDataLen, LPVOID outData)
	{
		if(m_deviceType == kDeviceType_Keyboard)
		{
			// keyboard

			// get raw data
			UInt8	rawData[kMaxMacros];
			HRESULT hr = m_device->GetDeviceState(256, rawData);
			if(hr != DI_OK) return hr;

			DIHookControl::GetSingleton().ProcessKeyboardData(rawData);

			memcpy(outData, rawData, outDataLen < 256 ? outDataLen : 256);

			return hr;
		}
		else
		{
			// mouse

			ASSERT(outDataLen == sizeof(DIMOUSESTATE2));

			g_framerateTracker.Update();

			DIMOUSESTATE2	* mouseState = (DIMOUSESTATE2 *)outData;

			// get raw data
			HRESULT hr = m_device->GetDeviceState(sizeof(DIMOUSESTATE2), mouseState);
			if(hr != DI_OK) return hr;

			DIHookControl::GetSingleton().ProcessMouseData(mouseState);

			return hr;
		}
	}

	// oblivion and on use this for menus and the console
	HRESULT _stdcall GetDeviceData(DWORD dataSize, DIDEVICEOBJECTDATA * outData, DWORD * outDataLen, DWORD flags)
	{
		return DIHookControl::GetSingleton().ProcessBufferedData(m_device, dataSize, outData, outDataLen, flags);
	}

	HRESULT _stdcall SetDataFormat(const DIDATAFORMAT* a) { return m_device->SetDataFormat(a); }
	HRESULT _stdcall SetEventNotification(HANDLE a) { return m_device->SetEventNotification(a); }

	// in debug builds, force cooperative level so input isn't locked out
	HRESULT _stdcall SetCooperativeLevel(HWND a,DWORD b)
	{
		
#if defined(_DEBUG) || 0
		// b = DISCL_BACKGROUND | DISCL_NONEXCLUSIVE;
#endif
		return m_device->SetCooperativeLevel(a,b);
	}

	HRESULT _stdcall GetObjectInfo(LPDIDEVICEOBJECTINSTANCEA a,DWORD b,DWORD c) { return m_device->GetObjectInfo(a,b,c); }
	HRESULT _stdcall GetDeviceInfo(LPDIDEVICEINSTANCEA a) { return m_device->GetDeviceInfo(a); }
	HRESULT _stdcall RunControlPanel(HWND a,DWORD b) { return m_device->RunControlPanel(a,b); }
	HRESULT _stdcall Initialize(HINSTANCE a,DWORD b,REFGUID c) { return m_device->Initialize(a,b,c); }
	HRESULT _stdcall CreateEffect(REFGUID a,LPCDIEFFECT b,LPDIRECTINPUTEFFECT *c,LPUNKNOWN d) { return m_device->CreateEffect(a,b,c,d); }
	HRESULT _stdcall EnumEffects(LPDIENUMEFFECTSCALLBACKA a,LPVOID b,DWORD c) { return m_device->EnumEffects(a,b,c); }
	HRESULT _stdcall GetEffectInfo(LPDIEFFECTINFOA a,REFGUID b) { return m_device->GetEffectInfo(a,b); }
	HRESULT _stdcall GetForceFeedbackState(LPDWORD a) { return m_device->GetForceFeedbackState(a); }
	HRESULT _stdcall SendForceFeedbackCommand(DWORD a) { return m_device->SendForceFeedbackCommand(a); }
	HRESULT _stdcall EnumCreatedEffectObjects(LPDIENUMCREATEDEFFECTOBJECTSCALLBACK a,LPVOID b,DWORD c) { return m_device->EnumCreatedEffectObjects(a,b,c); }
	HRESULT _stdcall Escape(LPDIEFFESCAPE a) { return m_device->Escape(a); }
	HRESULT _stdcall Poll(void) { return m_device->Poll(); }
	HRESULT _stdcall SendDeviceData(DWORD a,LPCDIDEVICEOBJECTDATA b,LPDWORD c,DWORD d) { return m_device->SendDeviceData(a,b,c,d); }
	HRESULT _stdcall EnumEffectsInFile(LPCSTR a,LPDIENUMEFFECTSINFILECALLBACK b,LPVOID c,DWORD d) { return m_device->EnumEffectsInFile(a,b,c,d); }
	HRESULT _stdcall WriteEffectToFile(LPCSTR a,DWORD b,LPDIFILEEFFECT c,DWORD d) { return m_device->WriteEffectToFile(a,b,c,d); }
	HRESULT _stdcall BuildActionMap(LPDIACTIONFORMATA a,LPCSTR b,DWORD c) { return m_device->BuildActionMap(a,b,c); }
	HRESULT _stdcall SetActionMap(LPDIACTIONFORMATA a,LPCSTR b,DWORD c) { return m_device->SetActionMap(a,b,c); }
	HRESULT _stdcall GetImageInfo(LPDIDEVICEIMAGEINFOHEADERA a) { return m_device->GetImageInfo(a); }

private:
	IDirectInputDevice8	* m_device;
	DWORD				m_deviceType;
	ULONG				m_refs;
};

class FakeDirectInput : public IDirectInput8A {
public:
	/*** Constructor ***/
	FakeDirectInput(IDirectInput8 * obj)
		:m_realDInput(obj), m_refs(1) { }

	/*** IUnknown methods ***/
	HRESULT _stdcall QueryInterface (REFIID riid, LPVOID* ppvObj) { return m_realDInput->QueryInterface(riid, ppvObj); }

	ULONG _stdcall AddRef(void)
	{
		m_refs++;

		return m_refs;
	}

	ULONG _stdcall Release(void)
	{
		m_refs--;

		if(!m_refs)
		{
			m_realDInput->Release();
			delete this;
			return 0;
		}

		return m_refs;
	}

	/*** IDirectInput8A methods ***/
	HRESULT _stdcall CreateDevice(REFGUID typeGuid, IDirectInputDevice8A ** device, IUnknown * unused)
	{
		if(typeGuid != GUID_SysKeyboard && typeGuid != GUID_SysMouse)
		{
			return m_realDInput->CreateDevice(typeGuid, device, unused);
		}
		else
		{
			IDirectInputDevice8A	* dev;

			HRESULT hr = m_realDInput->CreateDevice(typeGuid, &dev, unused);
			if(hr != DI_OK) return hr;

			*device = new FakeDirectInputDevice(dev, (typeGuid == GUID_SysKeyboard) ? kDeviceType_Keyboard : kDeviceType_Mouse);

			return hr;
		}
	}

	HRESULT _stdcall EnumDevices(DWORD a,LPDIENUMDEVICESCALLBACKA b,void* c,DWORD d) { return m_realDInput->EnumDevices(a,b,c,d); }
	HRESULT _stdcall GetDeviceStatus(REFGUID r) { return m_realDInput->GetDeviceStatus(r); }
	HRESULT _stdcall RunControlPanel(HWND a,DWORD b) { return m_realDInput->RunControlPanel(a,b); }
	HRESULT _stdcall Initialize(HINSTANCE a,DWORD b) { return m_realDInput->Initialize(a,b); }
	HRESULT _stdcall FindDevice(REFGUID a,LPCSTR b,LPGUID c) { return m_realDInput->FindDevice(a,b,c); }
	HRESULT _stdcall EnumDevicesBySemantics(LPCSTR a,LPDIACTIONFORMATA b,LPDIENUMDEVICESBYSEMANTICSCBA c,void* d,DWORD e) { return m_realDInput->EnumDevicesBySemantics(a,b,c,d,e); }
	HRESULT _stdcall ConfigureDevices(LPDICONFIGUREDEVICESCALLBACK a,LPDICONFIGUREDEVICESPARAMSA b,DWORD c,void* d) { return m_realDInput->ConfigureDevices(a,b,c,d); }

private:
	IDirectInput8	* m_realDInput;
	ULONG			m_refs;
};

static HRESULT _stdcall Hook_DirectInput8Create_Execute(HINSTANCE instance, DWORD version, REFIID iid, void * out, IUnknown * outer)
{
	IDirectInput8A	* dinput;
	HRESULT hr = DICreate_RealFunc(instance, version, iid, &dinput, outer);
	if(hr != DI_OK) return hr;

	*((IDirectInput8A**)out) = new FakeDirectInput(dinput);

	_MESSAGE("Hook_DirectInput8Create_Execute");

	return DI_OK;
}

void Hook_DirectInput8Create_Init()
{
	UInt32 thunkAddress = 0x00FDF02C;

	DICreate_RealFunc = (CreateDInputProc)*(DWORD *)thunkAddress;
	SafeWrite32(thunkAddress, (DWORD)Hook_DirectInput8Create_Execute);
}

DIHookControl::DIHookControl()
{
	memset(&m_keys, 0, sizeof(m_keys));
}

bool DIHookControl::IsKeyPressed(UInt32 keycode, UInt32 flags)
{
	if(keycode >= kMaxMacros) return false;

	// default mode
	if(!flags) flags = kFlag_DefaultBackCompat;

	KeyInfo	* info = &m_keys[keycode];

	bool	result = false;
	bool	isMouseButton = keycode >= kMacro_MouseButtonOffset;

	// data sources
	if(flags & kFlag_GameState)		result |= info->gameState;
	if(flags & kFlag_RawState)		result |= info->rawState;
	if(flags & kFlag_InsertedState)	result |= info->insertedState;

	// modifiers
	bool	disable = false;

	if((flags & kFlag_IgnoreDisabled_User) && info->userDisable)
		disable = true;

	if((flags & kFlag_IgnoreDisabled_Script) && info->scriptDisable)
		disable = true;

	if(disable)	result = false;

	return result;
}

bool DIHookControl::IsKeyDisabled(UInt32 keycode)
{
	if(keycode >= kMaxMacros) return false;

	KeyInfo	* info = &m_keys[keycode];

	return info->userDisable || info->scriptDisable;
}

bool DIHookControl::IsKeyHeld(UInt32 keycode)
{
	if(keycode >= kMaxMacros) return false;

	return m_keys[keycode].hold;
}

bool DIHookControl::IsKeyTapped(UInt32 keycode)
{
	if(keycode >= kMaxMacros) return false;

	return m_keys[keycode].tap;
}

void DIHookControl::SetKeyDisableState(UInt32 keycode, bool bDisable, UInt32 mask)
{
	if(!mask) mask = kDisable_All;	// default mask value

	if(keycode < kMaxMacros)
	{
		KeyInfo	* info = &m_keys[keycode];

		if(mask & kDisable_User)	info->userDisable = bDisable;
		if(mask & kDisable_Script)	info->scriptDisable = bDisable;
	}
}

void DIHookControl::SetKeyHeldState(UInt32 keycode, bool bHold)
{
	if(keycode < kMaxMacros)
		m_keys[keycode].hold = bHold;
}

void DIHookControl::TapKey(UInt32 keycode)
{
	if(keycode < kMaxMacros)
		m_keys[keycode].tap = true;
}

void DIHookControl::BufferedKeyTap(UInt32 key)
{
	DIDEVICEOBJECTDATA data;

	data.uAppData = -1;
	data.dwTimeStamp = GetTickCount();
	data.dwSequence = 0;	// engine doesn't appear to use this and we can't fake it easily
	data.dwOfs = key;
	data.dwData = 0x80;

	// key down
	m_bufferedPresses.push(data);

	// key up
	data.dwData = 0x00;
	m_bufferedPresses.push(data);
}

void DIHookControl::BufferedKeyPress(UInt32 key)
{
	DIDEVICEOBJECTDATA data;

	data.uAppData = -1;
	data.dwTimeStamp = GetTickCount();
	data.dwSequence = 0;
	data.dwOfs = key;
	data.dwData = 0x80;

	m_bufferedPresses.push(data);
}

void DIHookControl::BufferedKeyRelease(UInt32 key)
{
	DIDEVICEOBJECTDATA data;

	data.uAppData = -1;
	data.dwTimeStamp = GetTickCount();
	data.dwSequence = 0;
	data.dwOfs = key;
	data.dwData = 0x00;

	m_bufferedPresses.push(data);
}

void DIHookControl::ProcessKeyboardData(UInt8 * data)
{
	// process keys
	for(UInt32 idx = 0; idx < 256; idx++)
	{
		bool	keyDown = data[idx] != 0;

		keyDown = m_keys[idx].Process(keyDown, idx);

		data[idx] = keyDown ? 0x80 : 0x00;
	}
}

void DIHookControl::ProcessMouseData(DIMOUSESTATE2 * data)
{
	// process buttons
	for(UInt32 idx = 0; idx < 8; idx++)
	{
		UInt32	macroIdx = kMacro_MouseButtonOffset + idx;
		bool	keyDown = data->rgbButtons[idx] != 0;

		keyDown = m_keys[macroIdx].Process(keyDown, macroIdx);

		data->rgbButtons[idx] = keyDown ? 0x80 : 0x00;
	}

	// process mouse wheel
	UInt8	wheelState[2];	// 0 = +, 1 = -

	wheelState[0] = data->lZ > 0 ? 0x80 : 0x00;
	wheelState[1] = data->lZ < 0 ? 0x80 : 0x00;

	for(UInt32 idx = 0; idx < 2; idx++)
	{
		UInt32	macroIdx = kMacro_MouseWheelOffset + idx;
		bool	keyDown = wheelState[idx] != 0;

		keyDown = m_keys[macroIdx].Process(keyDown, macroIdx);

		wheelState[idx] = keyDown ? 0x80 : 0x00;
	}

	// wheel state not transferred back
}

HRESULT	DIHookControl::ProcessBufferedData(IDirectInputDevice8 * device, DWORD dataSize, DIDEVICEOBJECTDATA * outData, DWORD * outDataLen, DWORD flags)
{
	ASSERT(dataSize == sizeof(DIDEVICEOBJECTDATA));

	// if we have nothing to inject, pass through
	if(m_bufferedPresses.empty())
		return device->GetDeviceData(dataSize, outData, outDataLen, flags);

	UInt32	eventsRequested = *outDataLen;

	// pass down to the device
	UInt32	numRealEvents = eventsRequested;
	HRESULT	hr = device->GetDeviceData(dataSize, outData, &numRealEvents, flags);
	if((hr != DI_OK) && (hr != DI_BUFFEROVERFLOW))
	{
		*outDataLen = numRealEvents;
		return hr;
	}

	// move pointer down and update count
	DIDEVICEOBJECTDATA *	virtualOutData = (DIDEVICEOBJECTDATA *)(((UInt8 *)outData) + (dataSize * numRealEvents));
	UInt32					virtualEventsRequested = eventsRequested - numRealEvents;

	UInt32	numVirtualEvents = 0;

	if(flags & DIGDD_PEEK)
	{
		if(outData)
		{
			// todo: switch from queue to list so we can handle this
			HALT("DIHookControl::ProcessBufferedData: can't handle non-NULL data in peek mode");
		}
		else
		{
			UInt32	virtualEventsAvail = m_bufferedPresses.size();

			if(virtualEventsAvail > virtualEventsRequested)
				numVirtualEvents = virtualEventsRequested;
			else
				numVirtualEvents = virtualEventsAvail;
		}
	}
	else
	{
		for(UInt32 i = 0; i < virtualEventsRequested; i++)
		{
			if(m_bufferedPresses.empty()) break;

			if(outData)
			{
				*virtualOutData = m_bufferedPresses.front();
				virtualOutData = (DIDEVICEOBJECTDATA *)(((UInt8 *)outData) + dataSize);
			}

			m_bufferedPresses.pop();
			numVirtualEvents++;
		}
	}

	*outDataLen = numRealEvents + numVirtualEvents;

	return hr;
}

bool DIHookControl::KeyInfo::Process(bool keyDown, UInt32 idx)
{
	insertedState = false;
	rawState = keyDown;

	// only process whitelisted keys
	if(!ShouldIgnoreKey(idx))
	{
		if(userDisable)
			keyDown = false;

		if(!scriptDisable)
		{
			if(hold)
				insertedState = true;

			if(tap)
			{
				insertedState = true;
				tap = false;
			}

			if(insertedState)
				keyDown = true;
		}
	}

	gameState = keyDown;

	return keyDown;
}

// this code doesn't belong here
FramerateTracker::FramerateTracker()
:m_lastTime(0), m_lastFrameLength(0),
m_frameTimeHistoryIdx(0), m_frameTimeHistoryPrimed(false),
m_averageFrameTime(0)
{
	for(UInt32 i = 0; i < kFrameTimeHistoryLength; i++)
		m_frameTimeHistory[i] = 0;
}

void FramerateTracker::Update(void)
{
	DWORD time = GetTickCount();

	// calculate current frame time
	m_lastFrameLength = (float)(time - m_lastTime) / 1000.0f;
	m_lastTime = time;

	// store in ring buffer
	m_frameTimeHistory[m_frameTimeHistoryIdx % kFrameTimeHistoryLength] = m_lastFrameLength;
	m_frameTimeHistoryIdx++;

	// filled ring buffer? flag it
	if(m_frameTimeHistoryIdx >= kFrameTimeHistoryLength)
		m_frameTimeHistoryPrimed = true;

	// history full?
	if(m_frameTimeHistoryPrimed)
	{
		// calculate and store the average
		float	total = 0;

		for(UInt32 i = 0; i < kFrameTimeHistoryLength; i++)
			total += m_frameTimeHistory[i];

		m_averageFrameTime = total / static_cast<int>(kFrameTimeHistoryLength);
	}
	else
	{
		// report 0 frametime until primed
		m_averageFrameTime = 0;
	}
}
