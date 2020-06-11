#include "nvse/GameSettings.h"

GameSettingCollection **g_GameSettingCollection = (GameSettingCollection**)0x11C8048;
IniSettingCollection **g_INISettingCollection = (IniSettingCollection**)0x11F96A0;
IniSettingCollection **g_INIPrefCollection = (IniSettingCollection**)0x11F35A0;
IniSettingCollection **g_INIBlendSettingCollection = (IniSettingCollection**)0x11CC694;
IniSettingCollection **g_INIRendererSettingCollection = (IniSettingCollection**)0x11F35A4;

UInt32 Setting::GetType()
{
	if (name)
	{
		switch (*name | 0x20)
		{
		case 'b':
			return kSetting_Bool;
		case 'c':
			return kSetting_c;
		case 'i':
			return kSetting_Integer;
		case 'u':
			return kSetting_Unsigned;
		case 'f':
			return kSetting_Float;
		case 's':
			return kSetting_String;
		case 'r':
			return kSetting_r;
		case 'a':
			return kSetting_a;
		default:
			break;
		}
	}
	return kSetting_Other;
}

bool Setting::ValidType()
{
	switch (*name | 0x20)
	{
		case 'b':
		case 'f':
		case 'i':
		case 's':
		case 'u':
			return true;
		default:
			return false;
	}
}

void Setting::Get(double *out)
{
	switch (*name | 0x20)
	{
		case 'b':
		case 'u':
			*out = data.uint;
			break;
		case 'f':
			*out = data.f;
			break;
		case 'i':
			*out = data.i;
			break;
		default:
			break;
	}
}

void Setting::Set(float newVal)
{
	switch (*name | 0x20)
	{
		case 'b':
			data.uint = newVal ? 1 : 0;
			break;
		case 'f':
			data.f = newVal;
			break;
		case 'i':
			data.i = (int)newVal;
			break;
		case 'u':
			data.uint = (UInt32)newVal;
			break;
		default:
			break;
	}
}

void Setting::Set(const char *strVal, bool doFree)
{
	if (doFree) GameHeapFree(data.str);
	UInt32 length = StrLen(strVal) + 1;
	data.str = (char*)GameHeapAlloc(length);
	MemCopy(data.str, strVal, length);
}

__declspec(naked) bool GameSettingCollection::GetGameSetting(const char *settingName, Setting **out)
{
	__asm
	{
		add		ecx, 0x10C
		jmp		kNiTMapLookupAddr
	}
}

GameSettingCollection *GameSettingCollection::GetSingleton()
{
	return *g_GameSettingCollection;
}

IniSettingCollection *IniSettingCollection::GetIniSettings()
{
	return *g_INISettingCollection;
}

IniSettingCollection *IniSettingCollection::GetIniPrefs()
{
	return *g_INIPrefCollection;
}