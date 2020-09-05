#include "nvse/CommandTable.h"

ParamInfo kParams_Vector3Floats[3] =
{
	{"vector_x", kParamType_Float, 0},
	{"vector_y", kParamType_Float, 0},
	{"vector_z", kParamType_Float, 0}
};

ParamInfo kParams_Vector3Strings3Floats[6] =
{
	{"vector_x_name", kParamType_String, 0},
	{"vector_y_name", kParamType_String, 0},
	{"vector_z_name", kParamType_String, 0},
	{"vector_x", kParamType_Float, 0},
	{"vector_y", kParamType_Float, 0},
	{"vector_z", kParamType_Float, 0}
};

ParamInfo kParams_QuatEuler4Strings3Floats1Int[8] =
{
	{"quat_w_name", kParamType_String, 0},
	{"quat_x_name", kParamType_String, 0},
	{"quat_y_name", kParamType_String, 0},
	{"quat_z_name", kParamType_String, 0},
	{"elevation", kParamType_Float, 0},
	{"bank", kParamType_Float, 0},
	{"heading", kParamType_Float, 0},
	{"actorFlag", kParamType_Integer, 1}
};

ParamInfo kParams_EulerQuat3Strings4Floats1Int[8] =
{
	{"elevation_name", kParamType_String, 0},
	{"bank_name", kParamType_String, 0},
	{"heading_name", kParamType_String, 0},
	{"quat_w", kParamType_Float, 0},
	{"quat_x", kParamType_Float, 0},
	{"quat_y", kParamType_Float, 0},
	{"quat_z", kParamType_Float, 0},
	{"actorFlag", kParamType_Integer, 1}
};

ParamInfo kParams_QuatAxisAngle4Strings4Floats[8] =
{
	{"quat_w_name", kParamType_String, 0},
	{"quat_x_name", kParamType_String, 0},
	{"quat_y_name", kParamType_String, 0},
	{"quat_z_name", kParamType_String, 0},
	{"axis_x", kParamType_Float, 0},
	{"axis_y", kParamType_Float, 0},
	{"axis_z", kParamType_Float, 0},
	{"angle", kParamType_Float, 0}
};

ParamInfo kParams_Quat4Strings4Floats[8] =
{
	{"quat_w_name", kParamType_String, 0},
	{"quat_x_name", kParamType_String, 0},
	{"quat_y_name", kParamType_String, 0},
	{"quat_z_name", kParamType_String, 0},
	{"quat_w", kParamType_Float, 0},
	{"quat_x", kParamType_Float, 0},
	{"quat_y", kParamType_Float, 0},
	{"quat_z", kParamType_Float, 0}
};

ParamInfo kParams_Vector3Strings6Floats[9] =
{
	{"vector_x_name", kParamType_String, 0},
	{"vector_y_name", kParamType_String, 0},
	{"vector_z_name", kParamType_String, 0},
	{"vectorA_x", kParamType_Float, 0},
	{"vectorA_y", kParamType_Float, 0},
	{"vectorA_z", kParamType_Float, 0},
	{"vectorB_x", kParamType_Float, 0},
	{"vectorB_y", kParamType_Float, 0},
	{"vectorB_z", kParamType_Float, 0}
};

ParamInfo kParams_VectorQuatVector3Strings7Floats[10] =
{
	{"vector_x_name", kParamType_String, 0},
	{"vector_y_name", kParamType_String, 0},
	{"vector_z_name", kParamType_String, 0},
	{"quat_w", kParamType_Float, 0},
	{"quat_x", kParamType_Float, 0},
	{"quat_y", kParamType_Float, 0},
	{"quat_z", kParamType_Float, 0},
	{"vector_x", kParamType_Float, 0},
	{"vector_y", kParamType_Float, 0},
	{"vector_z", kParamType_Float, 0}
};

ParamInfo kParams_Quat4Strings8Floats[12] =
{
	{"quat_w_name", kParamType_String, 0},
	{"quat_x_name", kParamType_String, 0},
	{"quat_y_name", kParamType_String, 0},
	{"quat_z_name", kParamType_String, 0},
	{"quatA_w", kParamType_Float, 0},
	{"quatA_x", kParamType_Float, 0},
	{"quatA_y", kParamType_Float, 0},
	{"quatA_z", kParamType_Float, 0},
	{"quatB_w", kParamType_Float, 0},
	{"quatB_x", kParamType_Float, 0},
	{"quatB_y", kParamType_Float, 0},
	{"quatB_z", kParamType_Float, 0}
};

ParamInfo kParams_Quat4Strings9Floats1Int[14] =
{
	{"quat_w_name", kParamType_String, 0},
	{"quat_x_name", kParamType_String, 0},
	{"quat_y_name", kParamType_String, 0},
	{"quat_z_name", kParamType_String, 0},
	{"quatA_w", kParamType_Float, 0},
	{"quatA_x", kParamType_Float, 0},
	{"quatA_y", kParamType_Float, 0},
	{"quatA_z", kParamType_Float, 0},
	{"quatB_w", kParamType_Float, 0},
	{"quatB_x", kParamType_Float, 0},
	{"quatB_y", kParamType_Float, 0},
	{"quatB_z", kParamType_Float, 0},
	{"ratio", kParamType_Float, 0},
	{"flag", kParamType_Integer, 1}
};