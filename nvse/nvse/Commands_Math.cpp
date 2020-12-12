#include "Commands_Math.h"

#include "GameAPI.h"

bool Cmd_Exp_Execute(COMMAND_ARGS)
{
	*result = 0;

	float arg = 0;
	if(!ExtractArgs(EXTRACT_ARGS, &arg))
		return true;

	*result = exp(arg);

	return true;
}

bool Cmd_Log10_Execute(COMMAND_ARGS)
{
	*result = 0;

	float arg = 0;
	if(!ExtractArgs(EXTRACT_ARGS, &arg))
		return true;

	*result = log10(arg);

	return true;
}

bool Cmd_Floor_Execute(COMMAND_ARGS)
{
	float arg;
	if (ExtractArgs(EXTRACT_ARGS, &arg))
		*result = ifloor(arg);
	else *result = 0;
	return true;
}

bool Cmd_Ceil_Execute(COMMAND_ARGS)
{
	float arg;
	if (ExtractArgs(EXTRACT_ARGS, &arg))
		*result = iceil(arg);
	else *result = 0;
	return true;
}

bool Cmd_LeftShift_Execute(COMMAND_ARGS)
{
	*result = 0;

	UInt32	lhs = 0;
	UInt32	rhs = 0;

	if(!ExtractArgs(EXTRACT_ARGS, &lhs, &rhs))
		return true;

	if(rhs >= 32)
		*result = 0;
	else
		*result = lhs << rhs;

	return true;
}

bool Cmd_RightShift_Execute(COMMAND_ARGS)
{
	*result = 0;

	UInt32	lhs = 0;
	UInt32	rhs = 0;

	if(!ExtractArgs(EXTRACT_ARGS, &lhs, &rhs))
		return true;

	if(rhs >= 32)
		*result = 0;
	else
		*result = lhs >> rhs;

	return true;
}

bool Cmd_LogicalAnd_Execute(COMMAND_ARGS)
{
	*result = 0;

	UInt32	lhs = 0;
	UInt32	rhs = 0;

	if(!ExtractArgs(EXTRACT_ARGS, &lhs, &rhs))
		return true;

	*result = lhs & rhs;

	return true;
}

bool Cmd_LogicalOr_Execute(COMMAND_ARGS)
{
	*result = 0;

	UInt32	lhs = 0;
	UInt32	rhs = 0;

	if(!ExtractArgs(EXTRACT_ARGS, &lhs, &rhs))
		return true;

	*result = lhs | rhs;

	return true;
}

bool Cmd_LogicalXor_Execute(COMMAND_ARGS)
{
	*result = 0;

	UInt32	lhs = 0;
	UInt32	rhs = 0;

	if(!ExtractArgs(EXTRACT_ARGS, &lhs, &rhs))
		return true;

	*result = lhs ^ rhs;

	return true;
}

#pragma warning (push)
#pragma warning (disable : 4319)

bool Cmd_LogicalNot_Execute(COMMAND_ARGS)
{
	*result = 0;

	UInt32	lhs = 0;

	if(!ExtractArgs(EXTRACT_ARGS, &lhs))
		return true;

	*result = (double)(~lhs);

	return true;
}

#pragma warning (pop)

bool Cmd_Fmod_Execute(COMMAND_ARGS)
{
	*result = 0;
	float x = 0.0;
	float n = 0.0;
	float offset = 0.0;

	ExtractArgs(EXTRACT_ARGS, &x, &n, &offset);

	float answer = x - n * floor(x/n);
	if (offset != 0) {
		float bump = n * floor(offset/n);
		float bound = offset - bump;
		answer += bump;
		bool bBigger = (n > 0);
		if ( (bBigger && answer < bound) || (!bBigger && answer > bound) ) {
			answer += n;
		}
	}
	*result = answer;

	return true;
}

bool Cmd_Rand_Execute(COMMAND_ARGS)
{
	*result = 0;

	float rangeMin = 0;
	float rangeMax = 0;
	if(!ExtractArgs(EXTRACT_ARGS, &rangeMin, &rangeMax)) return true;

	if(rangeMax < rangeMin)
	{
		float	temp = rangeMin;
		rangeMin = rangeMax;
		rangeMax = temp;
	}

	float	range = rangeMax - rangeMin;

	double	value = MersenneTwister::genrand_real2() * range;
	value += rangeMin;

	*result = value;

	return true;
}

bool Cmd_Pow_Execute(COMMAND_ARGS)
{
	*result = 0;

	float f1 = 0;
	float f2 = 0;

	if(!ExtractArgs(EXTRACT_ARGS, &f1, &f2)) return true;

	*result = pow(f1,f2);

	return true;
}

bool Cmd_SetBit_Execute(COMMAND_ARGS)
{
	*result = 0;

	UInt32	data = 0;
	UInt32	idx = 0;
	UInt32	doSet = 1;	// default value

	if(!ExtractArgs(EXTRACT_ARGS, &data, &idx, &doSet))
		return true;

	if(doSet)
		data |= (1 << idx);
	else
		data &= ~(1 << idx);

	*result = data;

	return true;
}

bool Cmd_ClearBit_Execute(COMMAND_ARGS)
{
	*result = 0;

	UInt32	data = 0;
	UInt32	idx = 0;

	if(!ExtractArgs(EXTRACT_ARGS, &data, &idx))
		return true;

	data &= ~(1 << idx);

	*result = data;

	return true;
}

bool Cmd_GetBit_Execute(COMMAND_ARGS)
{
	*result = 0;

	UInt32	data = 0;
	UInt32	idx = 0;

	if(!ExtractArgs(EXTRACT_ARGS, &data, &idx))
		return true;

	*result = (data & (1 << idx)) >> idx;

	return true;
}

// trig functions using radians

bool Cmd_rSin_Execute(COMMAND_ARGS)
{
	*result = 0;

	float arg = 0;
	if(!ExtractArgs(EXTRACT_ARGS, &arg))
		return true;

	*result = sin(arg);

	return true;
}

bool Cmd_rCos_Execute(COMMAND_ARGS)
{
	*result = 0;

	float arg = 0;
	if(!ExtractArgs(EXTRACT_ARGS, &arg))
		return true;

	*result = cos(arg);

	return true;
}

bool Cmd_rTan_Execute(COMMAND_ARGS)
{
	*result = 0;

	float arg = 0;
	if(!ExtractArgs(EXTRACT_ARGS, &arg))
		return true;

	*result = tan(arg);

	return true;
}

bool Cmd_ASin_Execute(COMMAND_ARGS)
{
	*result = 0;

	float arg = 0;
	if(!ExtractArgs(EXTRACT_ARGS, &arg))
		return true;

	*result = asin(arg);

	return true;
}

bool Cmd_ACos_Execute(COMMAND_ARGS)
{
	*result = 0;

	float arg = 0;
	if(!ExtractArgs(EXTRACT_ARGS, &arg))
		return true;

	*result = acos(arg);

	return true;
}

bool Cmd_ATan_Execute(COMMAND_ARGS)
{
	*result = 0;

	float arg = 0;
	if(!ExtractArgs(EXTRACT_ARGS, &arg))
		return true;

	*result = atan(arg);

	return true;
}

bool Cmd_ATan2_Execute(COMMAND_ARGS)
{
	*result = 0;

	float f1 = 0;
	float f2 = 0;
	if(!ExtractArgs(EXTRACT_ARGS, &f1, &f2)) return true;

	*result = atan2(f1,f2);

	return true;
}
bool Cmd_Sinh_Execute(COMMAND_ARGS)
{
	*result = 0;

	float arg = 0;
	if(!ExtractArgs(EXTRACT_ARGS, &arg))
		return true;

	*result = sinh(arg);

	return true;
}
bool Cmd_Cosh_Execute(COMMAND_ARGS)
{
	*result = 0;

	float arg = 0;
	if(!ExtractArgs(EXTRACT_ARGS, &arg))
		return true;

	*result = cosh(arg);

	return true;
}
bool Cmd_Tanh_Execute(COMMAND_ARGS)
{
	*result = 0;

	float arg = 0;
	if(!ExtractArgs(EXTRACT_ARGS, &arg))
		return true;

	*result = tanh(arg);

	return true;
}

// trig functions using degrees
#define DEGTORAD 0.01745329252f

bool Cmd_dSin_Execute(COMMAND_ARGS)
{
	*result = 0;

	float arg = 0;
	if(!ExtractArgs(EXTRACT_ARGS, &arg))
		return true;

	*result = sin(arg*DEGTORAD);

	return true;
}
bool Cmd_dCos_Execute(COMMAND_ARGS)
{
	*result = 0;

	float arg = 0;
	if(!ExtractArgs(EXTRACT_ARGS, &arg))
		return true;

	*result = cos(arg*DEGTORAD);

	return true;
}
bool Cmd_dTan_Execute(COMMAND_ARGS)
{
	*result = 0;

	float arg = 0;
	if(!ExtractArgs(EXTRACT_ARGS, &arg))
		return true;

	*result = tan(arg*DEGTORAD);

	return true;
}
bool Cmd_dASin_Execute(COMMAND_ARGS)
{
	*result = 0;

	float arg = 0;
	if(!ExtractArgs(EXTRACT_ARGS, &arg))
		return true;

	*result = asin(arg)/DEGTORAD;

	return true;
}
bool Cmd_dACos_Execute(COMMAND_ARGS)
{
	*result = 0;

	float arg = 0;
	if(!ExtractArgs(EXTRACT_ARGS, &arg))
		return true;

	*result = acos(arg)/DEGTORAD;

	return true;
}
bool Cmd_dATan_Execute(COMMAND_ARGS)
{
	*result = 0;

	float arg = 0;
	if(!ExtractArgs(EXTRACT_ARGS, &arg))
		return true;

	*result = atan(arg)/DEGTORAD;

	return true;
}
bool Cmd_dATan2_Execute(COMMAND_ARGS)
{
	*result = 0;

	float f1 = 0;
	float f2 = 0;
	if(!ExtractArgs(EXTRACT_ARGS, &f1, &f2)) return true;

	*result = atan2(f1,f2)/DEGTORAD;

	return true;
}
bool Cmd_dSinh_Execute(COMMAND_ARGS)
{
	*result = 0;

	float arg = 0;
	if(!ExtractArgs(EXTRACT_ARGS, &arg))
		return true;

	*result = sinh(arg*DEGTORAD);

	return true;
}
bool Cmd_dCosh_Execute(COMMAND_ARGS)
{
	*result = 0;

	float arg = 0;
	if(!ExtractArgs(EXTRACT_ARGS, &arg))
		return true;

	*result = cosh(arg*DEGTORAD);

	return true;
}
bool Cmd_dTanh_Execute(COMMAND_ARGS)
{
	*result = 0;

	float arg = 0;
	if(!ExtractArgs(EXTRACT_ARGS, &arg))
		return true;

	*result = tanh(arg*DEGTORAD);

	return true;
}

