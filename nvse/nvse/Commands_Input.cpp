#include "Commands_Input.h"
#include "GameForms.h"
#include "GameAPI.h"
#include "Hooks_DirectInput8Create.h"
#include "GameOSDepend.h"
#include <string>
#include <map>
#include <set>
#include "GameScript.h"
#include "ArrayVar.h"
#include <Xinput.h>

#define XINPUT_GAMEPAD_GUIDE 0x400

static bool IsKeycodeValid(UInt32 id)		{ return (id < kMaxMacros - 2) && (id != 0xFF); }

uint32_t BS2DX(uint32_t bethesdaCode) {
	return ThisStdCall<uint32_t>(0xA24080, *g_OSInputGlobals, bethesdaCode);
}

uint8_t DX2BS(uint32_t xinputCode) {
	switch (xinputCode) {
		case XINPUT_GAMEPAD_DPAD_UP:
			return 1;
		case XINPUT_GAMEPAD_DPAD_DOWN:
			return 2;
		case XINPUT_GAMEPAD_DPAD_RIGHT:
			return 4;
		case XINPUT_GAMEPAD_DPAD_LEFT:
			return 5;
		case XINPUT_GAMEPAD_START:
			return 6;
		case XINPUT_GAMEPAD_BACK:
			return 7;
		case XINPUT_GAMEPAD_LEFT_THUMB:
			return 8;
		case XINPUT_GAMEPAD_RIGHT_THUMB:
			return 9;
		case XINPUT_GAMEPAD_A:
			return 10;
		case XINPUT_GAMEPAD_B:
			return 11;
		case XINPUT_GAMEPAD_X:
			return 12;
		case XINPUT_GAMEPAD_Y:
			return 13;
		case XINPUT_GAMEPAD_RIGHT_SHOULDER:
			return 14;
		case XINPUT_GAMEPAD_LEFT_SHOULDER:
			return 15;
		default:
			return 30;
	}
}

bool IsDisallowedControllerButton(UInt32 keycode) {
	switch (keycode) {
		case XINPUT_GAMEPAD_DPAD_UP:
		case XINPUT_GAMEPAD_DPAD_DOWN:
		case XINPUT_GAMEPAD_DPAD_RIGHT:
		case XINPUT_GAMEPAD_DPAD_LEFT:
		case XINPUT_GAMEPAD_START:
		case XINPUT_GAMEPAD_BACK:
		case XINPUT_GAMEPAD_GUIDE:
			return true;
		default:
			return false;
	}
}

UInt32 GetControl(UInt32 whichControl, UInt32 type)
{
	OSInputGlobals	* globs = *g_OSInputGlobals;

	if(whichControl >= globs->kMaxControlBinds)
		return 0xFF;

	UInt32	result;

	switch (type)
	{
		case OSInputGlobals::kControlType_Keyboard:
			result = globs->keyBinds[OSInputGlobals::kControlType_Keyboard][whichControl];
			break;

		case OSInputGlobals::kControlType_Mouse:
			result = globs->keyBinds[OSInputGlobals::kControlType_Mouse][whichControl];

			if(result != 0xFF) result += 0x100;
			break;

		case OSInputGlobals::kControlType_Joystick:
			result = globs->keyBinds[OSInputGlobals::kControlType_Joystick][whichControl];
			break;

		case OSInputGlobals::kControlType_Gamepad:
			result = BS2DX(globs->keyBinds[OSInputGlobals::kControlType_Gamepad][whichControl]);
			break;

		default:
			result = 0xFF;
			break;
	}

	return result;
}

void SetControl(UInt32 whichControl, UInt32 type, UInt32 keycode)
{
	OSInputGlobals	* globs = *g_OSInputGlobals;

	if(whichControl >= globs->kMaxControlBinds)
		return;

	UInt8	* binds = globs->keyBinds[type];
	if (type == OSInputGlobals::kControlType_Gamepad) {
		if (IsDisallowedControllerButton(keycode))
			return;

		keycode = DX2BS(keycode);
	}
	else
		keycode = (keycode >= 0x100) ? keycode - 0x100 : keycode;

	// if specified key already used by another control, swap with the new one
	for(UInt32 i = 0; i < OSInputGlobals::kMaxControlBinds; i++)
	{
		if(binds[i] == keycode)
		{
			binds[i] = binds[whichControl];
			break;
		}
	}

	binds[whichControl] = keycode;
}

bool IsControl(UInt32 key)
{
	OSInputGlobals	* globs = *g_OSInputGlobals;
	UInt8			* binds = globs->keyBinds[key >= 0x100 ? OSInputGlobals::kControlType_Mouse : OSInputGlobals::kControlType_Keyboard];

	key = (key >= 0x100) ? key - 0x100 : key;

	for(UInt32 i = 0; i < OSInputGlobals::kMaxControlBinds; i++)
		if(binds[i] == key)
			return true;

	return false;
}

bool Cmd_IsKeyPressed_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32	keycode = 0;
	UInt32	flags = 0;

	if(ExtractArgs(EXTRACT_ARGS, &keycode, &flags))
		return Cmd_IsKeyPressed_Eval(thisObj, (void *)keycode, (void *)flags, result);

	return true;
}

bool Cmd_IsKeyPressed_Eval(COMMAND_ARGS_EVAL)
{
	*result = DIHookControl::GetSingleton().IsKeyPressed((UInt32)arg1, (UInt32)arg2);

	return true;
}

bool Cmd_TapKey_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32	keycode = 0;

	if(ExtractArgs(EXTRACT_ARGS, &keycode))
		DIHookControl::GetSingleton().TapKey(keycode);

	return true;
}

bool Cmd_HoldKey_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32	keycode = 0;

	if(ExtractArgs(EXTRACT_ARGS, &keycode))
		DIHookControl::GetSingleton().SetKeyHeldState(keycode, true);

	return true;
}

bool Cmd_ReleaseKey_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32	keycode = 0;

	if(ExtractArgs(EXTRACT_ARGS, &keycode))
		DIHookControl::GetSingleton().SetKeyHeldState(keycode, false);

	return true;
}

bool Cmd_DisableKey_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32	keycode = 0;
	UInt32	mask = 0;

	if(ExtractArgs(EXTRACT_ARGS, &keycode, &mask))
		DIHookControl::GetSingleton().SetKeyDisableState(keycode, true, mask);

	return true;
}

bool Cmd_EnableKey_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32	keycode = 0;
	UInt32	mask = 0;

	if(ExtractArgs(EXTRACT_ARGS, &keycode, &mask))
		DIHookControl::GetSingleton().SetKeyDisableState(keycode, false, mask);

	return true;
}

bool Cmd_IsKeyDisabled_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 keycode = 0;

	if(ExtractArgs(EXTRACT_ARGS, &keycode))
		*result = DIHookControl::GetSingleton().IsKeyDisabled(keycode);

	return true;
}

bool Cmd_GetNumKeysPressed_Execute(COMMAND_ARGS)
{
	UInt32 flags=0;
	DWORD count=0;

	if(!ExtractArgs(EXTRACT_ARGS, &flags))
		return true;

	for(DWORD d = 0; d < 256; d++)
		if(IsKeycodeValid(d) && DIHookControl::GetSingleton().IsKeyPressed(d, flags))
			count++;

	*result = count;

	if(IsConsoleMode())
		Console_Print("keysPressed: %d", count);

	return true;
}

bool Cmd_GetKeyPress_Execute(COMMAND_ARGS)
{
	*result = -1;
	UInt32 flags=0;
	UInt32 count=0;

	if(!ExtractArgs(EXTRACT_ARGS, &count, &flags))
		return true;

	for(DWORD d = 0; d < 256; d++)
	{
		if(IsKeycodeValid(d) && DIHookControl::GetSingleton().IsKeyPressed(d, flags) && (!count--))
		{
			*result = d;
			break;
		}
	}

	return true;
}

bool Cmd_GetNumMouseButtonsPressed_Execute(COMMAND_ARGS)
{
	DWORD count=0;
	//Include mouse wheel? Probably not...

	for(DWORD d = 256; d < kMaxMacros -2; d++)
		if(IsKeycodeValid(d) && DIHookControl::GetSingleton().IsKeyPressed(d))
			count++;

	*result = count;
	if(IsConsoleMode())
		Console_Print("buttonsPressed: %d", count);

	return true;
}

bool Cmd_GetMouseButtonPress_Execute(COMMAND_ARGS)
{
	*result = -1;
	UInt32 count=0;

	if(!ExtractArgs(EXTRACT_ARGS, &count))
		return true;

	for(DWORD d = 256; d < kMaxMacros - 2; d++)
	{
		if(DIHookControl::GetSingleton().IsKeyPressed(d) && (!count--))
		{
			*result = d;
			break;
		}
	}

	return true;
}

////////////////////////////////////
// Menu versions of input functions
///////////////////////////////////

bool Cmd_MenuTapKey_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32	keycode = 0;

	if(!ExtractArgs(EXTRACT_ARGS, &keycode))
		return true;

	if(keycode < 256)
		DIHookControl::GetSingleton().BufferedKeyTap(keycode);

	return true;
}

bool Cmd_MenuHoldKey_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32	keycode = 0;

	if(ExtractArgs(EXTRACT_ARGS, &keycode) && keycode < 256)
		DIHookControl::GetSingleton().BufferedKeyPress(keycode);

	return true;
}

bool Cmd_MenuReleaseKey_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32	keycode = 0;

	if(ExtractArgs(EXTRACT_ARGS, &keycode) && keycode < 256)
		DIHookControl::GetSingleton().BufferedKeyRelease(keycode);

	return true;
}

//////////////////////////
// Controls
/////////////////////////

static Bitfield8 s_disabledControls[OSInputGlobals::kMaxControlBinds];

bool SetControlDisableState_Execute(COMMAND_ARGS, bool bDisable)
{
	*result = 0;
	UInt32	ctrl = 0;
	UInt32	mask = DIHookControl::kDisable_All;

	if(ExtractArgs(EXTRACT_ARGS, &ctrl, &mask))
	{
		if(ctrl < OSInputGlobals::kMaxControlBinds)
		{
			s_disabledControls[ctrl].Write(mask, bDisable);
			DIHookControl::GetSingleton().SetKeyDisableState(GetControl(ctrl, OSInputGlobals::kControlType_Keyboard), bDisable, mask);
			DIHookControl::GetSingleton().SetKeyDisableState(GetControl(ctrl, OSInputGlobals::kControlType_Mouse), bDisable, mask);
		}
	}

	return true;
}

bool Cmd_DisableControl_Execute(COMMAND_ARGS)
{
	return SetControlDisableState_Execute(PASS_COMMAND_ARGS, true);
}

bool Cmd_EnableControl_Execute(COMMAND_ARGS)
{
	return SetControlDisableState_Execute(PASS_COMMAND_ARGS, false);
}

bool Cmd_IsControlDisabled_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 ctrl = 0;

	if(ExtractArgs(EXTRACT_ARGS, &ctrl) && ctrl < OSInputGlobals::kMaxControlBinds)
		*result = s_disabledControls[ctrl].Get() ? 1 : 0;

	return true;
}

bool Cmd_GetControl_Execute(COMMAND_ARGS)
{
	UInt32 whichControl = 0;
	UInt32 whichDevice = OSInputGlobals::kControlType_Keyboard;
	*result = -1;

	if(!ExtractArgs(EXTRACT_ARGS, &whichControl, &whichDevice) || whichDevice < OSInputGlobals::kControlType_Keyboard || whichDevice > OSInputGlobals::kControlType_Gamepad)
		return true;

	UInt32 ctrl = GetControl(whichControl, whichDevice);
	*result = (ctrl == 0xFF) ? -1.0 : ctrl;

	if(IsConsoleMode())
		Console_Print("GetControl %d >> %.0f", whichControl, *result);

	return true;
}

bool Cmd_GetAltControl_Execute(COMMAND_ARGS)
{
	UInt32 whichControl = 0;
	*result = -1;

	if(!ExtractArgs(EXTRACT_ARGS, &whichControl))
		return true;

	UInt8 ctrl = GetControl(whichControl, OSInputGlobals::kControlType_Mouse);
	*result = (ctrl == 0xFF) ? -1.0 : ctrl;

	if(IsConsoleMode())
		Console_Print("GetAltControl %d >> %.0f", whichControl, *result);

	return true;
}

bool Cmd_IsControlPressed_Eval(COMMAND_ARGS_EVAL)
{
	*result = 0;

	UInt32 ctrl = (UInt32)arg1;
	UInt32 keycode = GetControl(ctrl, OSInputGlobals::kControlType_Keyboard);

	if(keycode != 0xFF && DIHookControl::GetSingleton().IsKeyPressed(keycode))
		*result = 1;
	else
	{
		keycode = GetControl(ctrl, OSInputGlobals::kControlType_Mouse);
		if(keycode != 0xFF && DIHookControl::GetSingleton().IsKeyPressed(keycode))
			*result = 1;
	}

	return true;
}

bool Cmd_IsControlPressed_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 ctrl, flags = 1;

	if (ExtractArgs(EXTRACT_ARGS, &ctrl, &flags))
		return Cmd_IsControlPressed_Eval(thisObj, (void*)ctrl, 0, result);

	return true;
}

bool Cmd_TapControl_Execute(COMMAND_ARGS)
{
	//returns false if control is not assigned
	*result = 0;
	UInt32 ctrl = 0;
	UInt32 keycode = 0;

	if(ExtractArgs(EXTRACT_ARGS, &ctrl))
	{
		keycode = GetControl(ctrl, OSInputGlobals::kControlType_Mouse);
		if(!IsKeycodeValid(keycode))
			keycode = GetControl(ctrl, OSInputGlobals::kControlType_Keyboard);

		if(IsKeycodeValid(keycode))
		{
			*result = 1;
			DIHookControl::GetSingleton().TapKey(keycode);
		}
	}

	return true;
}

bool Cmd_SetControl_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 key = 0;
	UInt32 ctrl = 0;
	UInt32 device = OSInputGlobals::kControlType_Keyboard;
	if(ExtractArgs(EXTRACT_ARGS, &ctrl, &key, &device) && device >= OSInputGlobals::kControlType_Keyboard && device <= OSInputGlobals::kControlType_Gamepad)
		SetControl(ctrl, device, key);

	return true;
}

bool Cmd_SetAltControl_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 key = 0;
	UInt32 ctrl = 0;

	if(ExtractArgs(EXTRACT_ARGS, &ctrl, &key))
		SetControl(ctrl, OSInputGlobals::kControlType_Mouse, key);

	return true;
}

// lets scripters register user-defined controls to help avoid conflicts
// key = key/button code, data = set of mod indices of mods which have registered key as a custom control
typedef Map<UInt32, Set<UInt8>> RegisteredControlMap;
static RegisteredControlMap s_registeredControls;

bool Cmd_SetIsControl_Execute(COMMAND_ARGS)
{
	// registers or unregisters a key for a particular mod
	UInt32	key = 0;
	UInt32	bIsControl = 1;
	UInt8	modIndex = scriptObj->GetModIndex();

	*result = 0;

	if (ExtractArgs(EXTRACT_ARGS, &key, &bIsControl) && key < kMaxMacros)
	{
		if (bIsControl)
			s_registeredControls[key].Insert(modIndex);
		else
		{
			Set<UInt8> *modIdxSet = s_registeredControls.GetPtr(key);
			if (modIdxSet) modIdxSet->Erase(modIndex);
		}
	}

	return true;
}

// returns 1 if game-assigned control, 2 is custom mod control, 3 if both, 0 otherwise
bool Cmd_IsControl_Execute(COMMAND_ARGS)
{
	UInt32 key = 0;

	*result = 0;

	if (!ExtractArgs(EXTRACT_ARGS, &key))
		return true;

	// check game controls
	*result = IsControl(key) ? 1 : 0;

	// check mod custom controls
	Set<UInt8> *modIdxSet = s_registeredControls.GetPtr(key);
	if (modIdxSet && !modIdxSet->Empty())
		*result += 2;

	return true;
}


bool Cmd_GetDisabledKeys_Execute(COMMAND_ARGS)
{

	ArrayVar *arr = g_ArrayMap.Create(kDataType_Numeric, true, scriptObj->GetModIndex());
	*result = arr->ID();
	double arrIndex = 0;

	for(UInt32 d = 0; d < kMaxMacros - 2; d++)
	{
		if(IsKeycodeValid(d) && DIHookControl::GetSingleton().IsKeyDisabled(d))
		{
			arr->SetElementNumber(arrIndex, d);
			arrIndex += 1;
		}
	}

	return true;
}


bool Cmd_GetPressedKeys_Execute(COMMAND_ARGS)
{

	UInt32 flags=0;

	if(!ExtractArgs(EXTRACT_ARGS, &flags))
		return true;

	ArrayVar *arr = g_ArrayMap.Create(kDataType_Numeric, true, scriptObj->GetModIndex());
	*result = arr->ID();
	double arrIndex = 0;

	for(UInt32 d = 0; d < kMaxMacros - 2; d++)
	{
		if(IsKeycodeValid(d) && DIHookControl::GetSingleton().IsKeyPressed(d, flags))
		{
			arr->SetElementNumber(arrIndex, d);
			arrIndex += 1;
		}
	}

	return true;
}

#if RUNTIME

void Commands_Input_Init(void)
{
	for(UInt32 i = 0; i < OSInputGlobals::kMaxControlBinds; i++)
		s_disabledControls[i].Clear();
}

#endif
