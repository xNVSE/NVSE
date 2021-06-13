#include "Hooks_SaveLoad.h"
#include "Hooks_Dialog.h"
#include "SafeWrite.h"
#include <cstdarg>
#include "Utilities.h"
#include "GameAPI.h"
#include "GameTiles.h"
#include "EventManager.h"
#include "ArrayVar.h"

// This is almost finished but should NOT be used yet :)

#if RUNTIME

#define kHookTextStart 0x00A1307E
#define kHookTextRtn 0x00A1308A
#define kHookTextStrLen 0x00EC6130
#define kHookTileTextStart 0x00A21D48
#define kHookTileTextRtn 0x00A21D4F

static const UInt32 addrHookTextRtn = kHookTextRtn;
static const UInt32 addrHookTextStrLen = kHookTextStrLen;
static const UInt32 addrHookTileTextRtn = kHookTileTextRtn;

static char* __stdcall doTextHook(char* text, FontManager::FontInfo* font)
{
	// we can use an event to call before a text is output
	UInt32 fontID = font ? font->id : 0;
	char* fontName = font ? font->path : 0;
	gLog.Indent();
	_MESSAGE("doTextHook: '%s' for [%08x] (fontCode=%d) '%s'", text, font, fontID, fontName);
	gLog.Outdent();
	return text;
}

static __declspec(naked) int TextHook()
{
	_asm 
	{
		pusha
		push [ebp - 0x07C8]		// this
		push [ebp + 8]
		call doTextHook
		mov [ebp + 8], eax
		popa
		mov edx, [ebp + 8]
		push edx
		call addrHookTextStrLen
		add esp, 4
		jmp addrHookTextRtn
	}
}

static char * lastSpeaker = NULL;
static UInt32 enableAllTileTextHook = 0;
static CRITICAL_SECTION	csTileText;				// trying to avoid what looks like another concurrency issues

char * doTileTextEvent(ArrayID argsArrayId, const char * eventName, char * text, char * tileName)
{
	char lastText[32768];
	const char* senderName = "NVSE";
	char * result = text;

	ArrayVar *arr = g_ArrayMap.Get(argsArrayId);
	arr->SetElementString("tileName", tileName);
	arr->SetElementString("text", result);
	if (EventManager::DispatchUserDefinedEvent(eventName, NULL, argsArrayId, senderName))
	{
		ArrayElement *elem = arr->Get("text", false);
		const char* replacedText;
		if (elem && elem->GetAsString(&replacedText) && (StrCompare(result, replacedText) != 0) && !strcpy_s(lastText, 32767, replacedText))
			result = lastText;
	}
	return result;
}

static char* __stdcall doTileTextHook(char* text, TileText* tile)
{
	::EnterCriticalSection(&csTileText);
	char* result = text;

	char tileName[4096] = "";
	ArrayVar *arr;
	ArrayID argsArrayId = 0;

	if (tile)
		tile->GetComponentFullName(tileName);
	if (strstr(tileName, "\\DM_SpeakerText")) // This a NPC speaking
	{
		arr = g_ArrayMap.Create(kDataType_String, false, 255);
		argsArrayId = arr ? arr->ID() : 0;
		if (argsArrayId) try {
			arr->SetElementString("speakerName", lastSpeaker);
			result = doTileTextEvent(argsArrayId, "OnSpeakerText", result, tileName);
		} catch(...) {
			g_ArrayMap.Delete(argsArrayId);
			argsArrayId = 0;
		}
		if (argsArrayId)
			g_ArrayMap.Delete(argsArrayId);
		gLog.Indent();
		_DMESSAGE("doTileTextHook \"%s\" says '%s' for [%08x] '%s'", lastSpeaker, result, tile, tileName);
		gLog.Outdent();
	}
	if (strstr(tileName, "\\DM_SpeakerNameLabel")) // This the name of the NPC speaking
		lastSpeaker = text;
	if (strstr(tileName, "MenuRoot\\DialogMenu") && strstr(tileName, "\\ListItemText")) // This is a topic
	{
		arr = g_ArrayMap.Create(kDataType_String, false, 255);
		argsArrayId = arr ? arr->ID() : 0;
		if (argsArrayId) try {
			result = doTileTextEvent(argsArrayId, "OnTopic", result, tileName);
		} catch(...) {
			g_ArrayMap.Delete(argsArrayId);
			argsArrayId = 0;
		}
		if (argsArrayId)
			g_ArrayMap.Delete(argsArrayId);
		gLog.Indent();
		_DMESSAGE("doTileTextHook Topic: '%s' for [%08x] '%s'", result, tile, tileName);
		gLog.Outdent();
	}
	if (enableAllTileTextHook && g_gameStarted)
	{
		arr = g_ArrayMap.Create(kDataType_String, false, 255);
		argsArrayId = arr ? arr->ID() : 0;
		if (argsArrayId) try {
			result = doTileTextEvent(argsArrayId, "OnTileText", result, tileName);
		} catch(...) {
			g_ArrayMap.Delete(argsArrayId);
			argsArrayId = 0;
		}
		if (argsArrayId)
			g_ArrayMap.Delete(argsArrayId);
		gLog.Indent();
		_DMESSAGE("doTileTextHook: '%s' for [%08x] '%s'", text, tile, tileName);
		gLog.Outdent();
	}

	::LeaveCriticalSection(&csTileText);
	return result;

}

static __declspec(naked) int TileTextHook()
{
	_asm 
	{
		pushad
		push dword ptr [ebp - 0x01B4]		// this
		push eax				// text
		call doTileTextHook
		mov dword ptr [ebp - 0x018], eax	// valueAsText
		popad
		cmp	dword ptr [ebp - 0x018], 0
		jmp addrHookTileTextRtn
	}
}

void Hook_Dialog_Init(void)
{
	UInt32	enableDialogHook = 0;
	UInt32	enableTextHook = 0;
	UInt32	enableTileTextHook = 1;	// Defaults to true as of v4.6 Beta 3

	if(GetNVSEConfigOption_UInt32("Text", "EnableTextHook", &enableTextHook) && enableTextHook)
	{
		// This looks to be called a lot . Every displayed text ?
		WriteRelJump(kHookTextStart, (UInt32)TextHook);
		_MESSAGE("Text hooked");
	}
	if(GetNVSEConfigOption_UInt32("Text", "EnableTileTextHook", &enableTileTextHook) && enableTileTextHook)
	{
		::InitializeCriticalSection(&csTileText);
		WriteRelJump(kHookTileTextStart, (UInt32)TileTextHook);
		_MESSAGE("TileText hooked");
		if(GetNVSEConfigOption_UInt32("Text", "EnableAllTileTextHook", &enableAllTileTextHook) && enableAllTileTextHook)
			_MESSAGE("All TileText hooked");
	}
}

#endif
