#pragma once
#include "ParamInfos.h"
#include "CommandTable.h"
#include "ScriptUtils.h"

DEFINE_COMMAND(ar_Construct, creates a new array, 0, 1, kParams_OneString);

static ParamInfo kParams_OneArray[] =
{
	{	"array",	kNVSEParamType_Array,	0	},
};

static ParamInfo kParams_OneArray_OneString_OneBool[] =
{
	{	"array",	kNVSEParamType_Array,0	},
	{	"string",	kNVSEParamType_String,0	},
	{	"bool",	kNVSEParamType_Boolean,1	},
};

DEFINE_COMMAND_EXP(ar_Size, returns the size of an array or -1 if the array does not exist, 0, kParams_OneArray);
DEFINE_COMMAND_EXP(ar_Packed, returns if the array is packed (array) or not (map), 0, kParams_OneArray);
DEFINE_COMMAND_EXP(ar_Dump, dumps the contents of an array for debugging purposes., 0, kParams_OneArray);
DEFINE_COMMAND_EXP(ar_DumpF, dumps the contents of an array to a file, 0, kParams_OneArray_OneString_OneBool);
DEFINE_COMMAND(ar_DumpID, dumps an array given an integer ID, 0, 1, kParams_OneInt);

static ParamInfo kParams_ar_Erase[] =
{
	{	"array",			kNVSEParamType_Array,	0	},
	{	"index or range",	kNVSEParamType_ArrayIndex | kNVSEParamType_Slice,	1	},
};

DEFINE_COMMAND_EXP(ar_Erase, erases an element or range of elements from an array, 0, kParams_ar_Erase);

static ParamInfo kParams_OneAssignableArray[] =
{
	{	"array",	kNVSEParamType_ArrayVarOrElement,	0	},
};

static ParamInfo kParams_ar_Sort[] =
{
	{	"array",		kNVSEParamType_Array,	0	},
	{	"bDescending",	kNVSEParamType_Number,	1	},
};

static ParamInfo kNVSEParams_ar_CustomSort[] =
{
	{	"array",		kNVSEParamType_Array,	0	},
	{	"comparator",	kNVSEParamType_Form,	0	},
	{	"bDescending",	kNVSEParamType_Number,	1	},
};

DEFINE_COMMAND_EXP(ar_Sort, returns an array containing the source array elements in sorted order, 0, kParams_ar_Sort);
DEFINE_COMMAND_EXP(ar_CustomSort, returns an array containing the source array elements sorted using the passed function script, 0, kNVSEParams_ar_CustomSort);
DEFINE_COMMAND_EXP(ar_SortAlpha, returns an array containing the source array elements in alphabetical order, 0, kParams_ar_Sort);

static ParamInfo kParams_ar_Find[] =
{
	{	"valueToFind",	kNVSEParamType_BasicType,	0	},
	{	"array",		kNVSEParamType_Array,		0	},
	{	"range",		kNVSEParamType_Slice,		1	},
};

DEFINE_COMMAND_EXP(ar_Find, finds the first occurence of a value within an array, 0, kParams_ar_Find);
DEFINE_COMMAND_EXP(ar_First, returns the key of the first element in an array, 0, kParams_OneArray);
DEFINE_COMMAND_EXP(ar_Last, returns the key of the last element in an array, 0, kParams_OneArray);

static ParamInfo kParams_ArrayAndKey[] =
{
	{	"array",			kNVSEParamType_Array,		0	},
	{	"array index",	kNVSEParamType_ArrayIndex,	0	},
};

DEFINE_COMMAND_EXP(ar_Next, returns the key of the element immediately following the specified key, 0, kParams_ArrayAndKey);
DEFINE_COMMAND_EXP(ar_Prev, returns the key of the element immediately preceding the specified key, 0, kParams_ArrayAndKey);

DEFINE_COMMAND_EXP(ar_Keys, returns an Array containing the keys of the specified array, 0, kParams_OneArray);
DEFINE_COMMAND_EXP(ar_HasKey, returns 1 if the array contains an element with the specified key, 0, kParams_ArrayAndKey);

DEFINE_COMMAND_EXP(ar_BadStringIndex, returns the return value of a bad string array index, 0, NULL);
DEFINE_COMMAND_EXP(ar_BadNumericIndex, returns the return value of a bad numeric array index, 0, NULL);

DEFINE_COMMAND_EXP(ar_Copy, returns a shallow copy of an array, 0, kParams_OneArray);
DEFINE_COMMAND_EXP(ar_DeepCopy, returns a deep copy of an array copying any arrays contained in that array, 0, kParams_OneArray);

DEFINE_COMMAND(ar_Null, makes an array variable refer to no array, 0, 0, NULL);

#if DW_NIF_SCRIPT
DEFINE_COMMAND(InstallModelMapHook, nothing, 0, 0, NULL);
#endif

static ParamInfo kNVSEParams_ar_Resize[3] =
{
	{	"array",		kNVSEParamType_Array,		0	},
	{	"newSize",		kNVSEParamType_Number,		0	},
	{	"padding",		kNVSEParamType_BasicType,	1	},
};

DEFINE_COMMAND_EXP(ar_Resize, resizes an array padding additional elements with zero or a supplied value, 0, kNVSEParams_ar_Resize);

static ParamInfo kNVSEParams_ar_Insert[3] =
{
	{	"array",		kNVSEParamType_Array,		0	},
	{	"index",		kNVSEParamType_Number,		0	},
	{	"toInsert",		kNVSEParamType_BasicType,	0	},
};

DEFINE_COMMAND_EXP(ar_Insert, inserts a single element in an array, 0, kNVSEParams_ar_Insert);

static ParamInfo kNVSEParams_ar_InsertRange[3] =
{
	{	"array",		kNVSEParamType_Array,		0	},
	{	"index",		kNVSEParamType_Number,		0	},
	{	"range",		kNVSEParamType_Array,		0	},
};

DEFINE_COMMAND_EXP(ar_InsertRange, inserts a range of elements into an array, 0, kNVSEParams_ar_InsertRange);

static ParamInfo kNVSEParams_ar_Append[2] =
{
	{	"array",		kNVSEParamType_Array,		0	},
	{	"toInsert",		kNVSEParamType_BasicType,	0	},
};

DEFINE_COMMAND_EXP(ar_Append, appends an element to an Array, 0, kNVSEParams_ar_Append);

static ParamInfo kNVSEParams_List[20] =
{
	{	"element",	kNVSEParamType_BasicType,	1	},
	{	"element",	kNVSEParamType_BasicType,	1	},
	{	"element",	kNVSEParamType_BasicType,	1	},
	{	"element",	kNVSEParamType_BasicType,	1	},
	{	"element",	kNVSEParamType_BasicType,	1	},
	{	"element",	kNVSEParamType_BasicType,	1	},
	{	"element",	kNVSEParamType_BasicType,	1	},
	{	"element",	kNVSEParamType_BasicType,	1	},
	{	"element",	kNVSEParamType_BasicType,	1	},
	{	"element",	kNVSEParamType_BasicType,	1	},
	{	"element",	kNVSEParamType_BasicType,	1	},
	{	"element",	kNVSEParamType_BasicType,	1	},
	{	"element",	kNVSEParamType_BasicType,	1	},
	{	"element",	kNVSEParamType_BasicType,	1	},
	{	"element",	kNVSEParamType_BasicType,	1	},
	{	"element",	kNVSEParamType_BasicType,	1	},
	{	"element",	kNVSEParamType_BasicType,	1	},
	{	"element",	kNVSEParamType_BasicType,	1	},
	{	"element",	kNVSEParamType_BasicType,	1	},
	{	"element",	kNVSEParamType_BasicType,	1	},
};

DEFINE_COMMAND_EXP(ar_List, creates an array given a list of elements, 0, kNVSEParams_List);

static ParamInfo kNVSEParams_PairList[20] =
{
	{	"key-value pair",	kNVSEParamType_Pair,	0	},
	{	"key-value pair",	kNVSEParamType_Pair,	1	},
	{	"key-value pair",	kNVSEParamType_Pair,	1	},
	{	"key-value pair",	kNVSEParamType_Pair,	1	},
	{	"key-value pair",	kNVSEParamType_Pair,	1	},
	{	"key-value pair",	kNVSEParamType_Pair,	1	},
	{	"key-value pair",	kNVSEParamType_Pair,	1	},
	{	"key-value pair",	kNVSEParamType_Pair,	1	},
	{	"key-value pair",	kNVSEParamType_Pair,	1	},
	{	"key-value pair",	kNVSEParamType_Pair,	1	},
	{	"key-value pair",	kNVSEParamType_Pair,	1	},
	{	"key-value pair",	kNVSEParamType_Pair,	1	},
	{	"key-value pair",	kNVSEParamType_Pair,	1	},
	{	"key-value pair",	kNVSEParamType_Pair,	1	},
	{	"key-value pair",	kNVSEParamType_Pair,	1	},
	{	"key-value pair",	kNVSEParamType_Pair,	1	},
	{	"key-value pair",	kNVSEParamType_Pair,	1	},
	{	"key-value pair",	kNVSEParamType_Pair,	1	},
	{	"key-value pair",	kNVSEParamType_Pair,	1	},
	{	"key-value pair",	kNVSEParamType_Pair,	1	},
};

DEFINE_COMMAND_EXP(ar_Map, creates a Map or Stringmap given a list of key-value pairs, 0, kNVSEParams_PairList);

static ParamInfo kNVSEParams_ar_Range[3] =
{
	{	"start",	kNVSEParamType_Number,	0	},
	{	"end",		kNVSEParamType_Number,	0	},
	{	"step",		kNVSEParamType_Number, 1	},
};

DEFINE_COMMAND_EXP(ar_Range, returns an array containing numbers beginning with 'start' up to and including 'end' in intervals 'step', 0, kNVSEParams_ar_Range);

static ParamInfo kNVSEParams_OneArray_OneFunction[2] =
{
	{	"array",	kNVSEParamType_Array,	0	},
	{	"condition user defined function",		kNVSEParamType_Form,	0	},
};

DEFINE_COMMAND_EXP(ar_FindWhere, "finds first element in array which satisfies a condition", false, kNVSEParams_OneArray_OneFunction);
DEFINE_COMMAND_EXP(ar_Filter, "filters an array on a condition", false, kNVSEParams_OneArray_OneFunction);
DEFINE_COMMAND_EXP(ar_MapTo, "transforms an array into a new array from a script", false, kNVSEParams_OneArray_OneFunction);
DEFINE_COMMAND_EXP(ar_ForEach, "calls a function on each element of array", false, kNVSEParams_OneArray_OneFunction);
DEFINE_COMMAND_EXP(ar_Any, "checks if any of the elements in array satisfies a function script", false, kNVSEParams_OneArray_OneFunction);
DEFINE_COMMAND_EXP(ar_All, "checks if all of the elements in array satisfy a function script", false, kNVSEParams_OneArray_OneFunction);

static ParamInfo kNVSEParams_OneInt_OneFunction_OneOptionalFunction[3] =
{
	{	"int",	kNVSEParamType_Number,	0	},
	{	"condition user defined function",		kNVSEParamType_Form,	0	},
	{	"condition user defined function",		kNVSEParamType_Form,	1	},
};

DEFINE_COMMAND_EXP(ar_Generate, "creates a new array from a function called each time for each element", false, kNVSEParams_OneInt_OneFunction_OneOptionalFunction);

static ParamInfo kNVSEParams_OneInt_OneElem[2] =
{
	{	"int",	kNVSEParamType_Number,	0	},
	{	"element",		kNVSEParamType_BasicType,	0	},
};

DEFINE_CMD_ALT_EXP(ar_Init, ar_Initialize, "creates a new array with x amount of elements, all set to a single value", false, kNVSEParams_OneInt_OneElem);

static ParamInfo kNVSEParams_TwoArrays[2] =
{
	{	"array",	kNVSEParamType_Array,	0	},
	{	"array",		kNVSEParamType_Array,	0	},
};

DEFINE_CMD_ALT_EXP(ar_DeepEquals, ar_DeepCompare, "checks if every element and sub-elements are identical between two arrays.", false, kNVSEParams_TwoArrays);

static ParamInfo kNVSEParams_OneArray[] =
{
	{	"array",	kNVSEParamType_Array,	0	},
};

DEFINE_COMMAND_EXP(ar_Unique, "returns a new array with no duplicate elements from the source array", false, kNVSEParams_OneArray);