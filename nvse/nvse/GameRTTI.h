#pragma once

using _Fallout_DynamicCast = void*(__cdecl*)(const void* inptr, int VfDelta, const void* SrcType, const void* TargetType, int isReference);

#ifdef RUNTIME
#include "GameRTTI_1_4_0_525.inc"
#else
#include "GameRTTI_EDITOR.inc"
#endif

#define DYNAMIC_CAST(obj, from, to) ( ## to *) Fallout_DynamicCast(obj, 0, RTTI_ ## from, RTTI_ ## to, 0)