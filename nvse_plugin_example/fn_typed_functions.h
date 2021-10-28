#pragma once

DEFINE_COMMAND_PLUGIN(ExamplePlugin_ReturnForm, "Returns the form that was given.", false, kParams_OneForm)

#if RUNTIME
bool Cmd_ExamplePlugin_ReturnForm_Execute(COMMAND_ARGS)
{
	*result = 0;
	TESForm* form;

	/*
	 * We will return the form's refID as the result.
	 * To this end, we have to cast "result" as a UInt32* register,
	 * since the Ref datatype assumes that the data is UInt32.
	 */
	
	if (ExtractArgsEx(EXTRACT_ARGS_EX, &form))
		*(UInt32*)result = form->refID;
	
	return true;
}
#endif


DEFINE_COMMAND_PLUGIN(ExamplePlugin_ReturnString, "Returns the string that was given.", false, kParams_OneString)

#if RUNTIME
bool Cmd_ExamplePlugin_ReturnString_Execute(COMMAND_ARGS)
{
	*result = 0;
	
	// Have to specify the expected size of the string and allocate that much space.
	char stringBuffer[0x80];

	if (ExtractArgsEx(EXTRACT_ARGS_EX, &stringBuffer))
		g_stringInterface->Assign(PASS_COMMAND_ARGS, stringBuffer);

	// Note that the Assign function handles setting "result" on its own.
	
	return true;
}
#endif

// Define a parameter to interpret an int as an array, since a plugin cannot accept an array-type argument yet.
static ParamInfo kParams_OneArray[1] =
{
	{	"array",	kParamType_Integer,	0 },
};

// Shorthand aliases.
#if RUNTIME
using NVSEArrayVar = NVSEArrayVarInterface::Array;
using NVSEArrayElement = NVSEArrayVarInterface::Element;
#endif

DEFINE_COMMAND_PLUGIN(ExamplePlugin_ReturnArray, "Returns the array that was given.", false, kParams_OneArray)

#if RUNTIME
bool Cmd_ExamplePlugin_ReturnArray_Execute(COMMAND_ARGS)
{
	*result = 0;
	UInt32 arrID;
	
	if (ExtractArgsEx(EXTRACT_ARGS_EX, &arrID))
	{
		NVSEArrayVar* srcArray = g_arrayInterface->LookupArrayByID(arrID);
		g_arrayInterface->AssignCommandResult(srcArray, result);
	}

	return true;
}
#endif