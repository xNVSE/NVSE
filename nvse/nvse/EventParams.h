#pragma once
#include "PluginAPI.h"

using EventParamType = NVSEEventManagerInterface::ParamType;

static EventParamType kEventParams_GameEvent[2] =
{
	EventParamType::eParamType_AnyForm, EventParamType::eParamType_AnyForm
};

static EventParamType kEventParams_OneRef[1] =
{
	EventParamType::eParamType_AnyForm,
};

static EventParamType kEventParams_TwoRefs[] =
{
	EventParamType::eParamType_AnyForm,
	EventParamType::eParamType_AnyForm,
};

static EventParamType kEventParams_OneInt[] =
{
	EventParamType::eParamType_Int,
};

static EventParamType kEventParams_TwoInts[2] =
{
	EventParamType::eParamType_Int, EventParamType::eParamType_Int
};

static EventParamType kEventParams_OneInt_OneRef[2] =
{
	EventParamType::eParamType_Int, EventParamType::eParamType_AnyForm
};

static EventParamType kEventParams_OneForm[1] =
{
	EventParamType::eParamType_AnyForm,
};

static EventParamType kEventParams_OneString[1] =
{
	EventParamType::eParamType_String
};

static EventParamType kEventParams_OneFloat_OneForm[2] =
{
	 EventParamType::eParamType_Float, EventParamType::eParamType_AnyForm
};

static EventParamType kEventParams_OneRef_OneInt[2] =
{
	EventParamType::eParamType_AnyForm, EventParamType::eParamType_Int
};

static EventParamType kEventParams_OneArray[1] =
{
	EventParamType::eParamType_Array
};

static EventParamType kEventParams_OneForm_OneInt[2] =
{
	EventParamType::eParamType_AnyForm,
	EventParamType::eParamType_Int
};

static EventParamType kEventParams_OneInt_OneFloat_OneArray_OneString_OneForm_OneReference_OneBaseform[] =
{
	EventParamType::eParamType_Int,
	EventParamType::eParamType_Float,
	EventParamType::eParamType_Array,
	EventParamType::eParamType_String,
	EventParamType::eParamType_AnyForm,
	EventParamType::eParamType_Reference,
	EventParamType::eParamType_BaseForm,
};

static EventParamType kEventParams_OneReference_OneInt[] =
{
	EventParamType::eParamType_Reference,
	EventParamType::eParamType_Int,
};

static EventParamType kEventParams_OneBaseForm[] =
{
	EventParamType::eParamType_BaseForm,
};

static EventParamType kEventParams_OneForm_OneReference_OneInt[] =
{
	EventParamType::eParamType_AnyForm,
	EventParamType::eParamType_Reference,
	EventParamType::eParamType_Int,
};
static EventParamType kEventParams_OneBaseForm_OneReference_OneInt[] =
{
	EventParamType::eParamType_BaseForm,
	EventParamType::eParamType_Reference,
	EventParamType::eParamType_Int,
};
static EventParamType kEventParams_OneBaseForm_OneReference[] =
{
	EventParamType::eParamType_BaseForm,
	EventParamType::eParamType_Reference,
};
static EventParamType kEventParams_OneReference_OneBaseForm[] =
{
	EventParamType::eParamType_Reference,
	EventParamType::eParamType_BaseForm,
};
static EventParamType kEventParams_OneReference_OneBaseForm_OneReference[] =
{
	EventParamType::eParamType_Reference,
	EventParamType::eParamType_BaseForm,
	EventParamType::eParamType_Reference,
};

static EventParamType kEventParams_ThreeStrings_OneFloat[] =
{
	EventParamType::eParamType_String,
	EventParamType::eParamType_String,
	EventParamType::eParamType_String,
	EventParamType::eParamType_Float,
};

static EventParamType kEventParams_TwoRefs_OneInt[] =
{
	EventParamType::eParamType_AnyForm,
	EventParamType::eParamType_AnyForm,
	EventParamType::eParamType_Int,
};