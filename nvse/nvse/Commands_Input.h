#pragma once

#include "CommandTable.h"
#include "ParamInfos.h"

#define DEFINE_INPUT(name, alias, desc) DEFINE_CMD_ALT(name, alias, desc, 0, 1, kParams_OneInt)

DEFINE_CMD_COND(IsKeyPressed, return if a dx scancode is down or up, 0, kParams_OneInt_OneOptionalInt);
DEFINE_INPUT(TapKey, tk, Fakes a key press for one frame);
DEFINE_INPUT(HoldKey, hk, Fakes a key press indefinately);
DEFINE_INPUT(ReleaseKey, rk, Releases a key held down by HoldKey);
DEFINE_CMD_ALT(DisableKey, dk, Prevents a player from using a key, 0, 2, kParams_OneInt_OneOptionalInt);
DEFINE_CMD_ALT(EnableKey, ek, Reenables a key previously disabled with DisableKey, 0, 2, kParams_OneInt_OneOptionalInt);
DEFINE_CMD_ALT(GetNumKeysPressed, gnkp, Returns how many keyboard keys are currently being held down, 0, 0, 0);
DEFINE_INPUT(GetKeyPress, gkp, Returns the scancode of the nth key which is currently being held down);
DEFINE_CMD_ALT(GetNumMouseButtonsPressed, gnmbp, Returns how many mouse buttons are currently being held down, 0, 0, 0);
DEFINE_INPUT(GetMouseButtonPress, gmbp, Returns the code of the nth mouse button which is currently being held down);

DEFINE_COMMAND(GetControl, Returns the key assigned to a control, 0, 1, kParams_OneInt);
DEFINE_COMMAND(GetAltControl, Returns the mouse button assigned to a control, 0, 1, kParams_OneInt);

DEFINE_INPUT(MenuTapKey, mtk, Fakes a key press for one frame in menu mode);
DEFINE_INPUT(MenuHoldKey, mhk, Fakes a key press indefinately in menu mode);
DEFINE_INPUT(MenuReleaseKey, mrk, 	Releases a key held down by MenuHoldKey);
DEFINE_CMD_ALT(DisableControl, dc, disables the key and button bound to a control, 0, 2, kParams_OneInt_OneOptionalInt);
DEFINE_CMD_ALT(EnableControl, ec, enables the key and button assigned to a control, 0, 2, kParams_OneInt_OneOptionalInt);
DEFINE_INPUT(TapControl, tc, taps the key or mouse button assigned to control);
DEFINE_COMMAND(SetControl, assigns a new keycode to the specified keyboard control, 0, 2, kParams_TwoInts);
DEFINE_COMMAND(SetAltControl, assigns a new mouse button code to the specified mouse control, 0, 2, kParams_TwoInts);
DEFINE_COMMAND(SetIsControl, sets a key as a custom control, 0, 2, kParams_TwoInts);
DEFINE_COMMAND(IsControl, returns 1 if key is a game control or 2 if a custom control, 0, 1, kParams_OneInt);
DEFINE_COMMAND(IsKeyDisabled, returns 1 if the key has been disabled by a script, 0, 1, kParams_OneInt);
DEFINE_COMMAND(IsControlDisabled, returns 1 if the control has been disabled by a script, 0, 1, kParams_OneInt);
DEFINE_CMD_COND(IsControlPressed, returns 1 if the control is pressed, 0, kParams_OneInt_OneOptionalInt);

#undef DEFINE_INPUT

#if RUNTIME

void Commands_Input_Init(void);

#endif
