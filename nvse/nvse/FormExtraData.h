#pragma once

class FormExtraData
{
public:
	virtual ~FormExtraData() {};
	static void Add(TESForm* form, const char* name, FormExtraData* formExtraData);
	static void Remove(TESForm* form, const char* name);
	static FormExtraData* Get(TESForm* form, const char* name);
	static void WriteHooks();
};
