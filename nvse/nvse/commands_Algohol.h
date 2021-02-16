#pragma once

#include "nvse/CommandTable.h"
#include "algohol/paramTypes.h"

DEFINE_CMD_ALT(V3Length, v3len, returns length of given vector3, 0, 3, kParams_Vector3Floats);
DEFINE_CMD_ALT(V3Normalize, v3norm, returns normalized vector3, 0, 6, kParams_Vector3Strings3Floats);
// DEFINE_CMD_ALT(V3Dotproduct, v3dprod, returns dotproduct of two vectors, 0, 9, kParams_Vector6Floats);
DEFINE_CMD_ALT(V3Crossproduct, v3xprod, returns crossproduct of two vectors, 0, 9, kParams_Vector3Strings6Floats);

DEFINE_CMD_ALT(QFromEuler, qfrome, converts euler angles to quaternion. optional flag indicates if the angles came from an actor, 0, 8, kParams_QuatEuler4Strings3Floats1Int);
DEFINE_CMD_ALT(QFromAxisAngle, qfromaa, converts axis-angle rotation to quaternion, 0, 8, kParams_QuatAxisAngle4Strings4Floats);
DEFINE_CMD_ALT(QNormalize, qnorm, returns normalized quaternion, 0, 8, kParams_Quat4Strings4Floats);
DEFINE_CMD_ALT(QMultQuatQuat, qmultq, multiplies two quaternions, 0, 12, kParams_Quat4Strings8Floats);
DEFINE_CMD_ALT(QMultQuatVector3, qmultv3, multiplies vector3 by quaternion, 0, 10, kParams_VectorQuatVector3Strings7Floats);
DEFINE_CMD_ALT(QInterpolate, qint, interpolates between two quaternions; default method is normalized linear interpolation. optional flag indicates spherical linear interpolation, 0, 
			   14, kParams_Quat4Strings9Floats1Int);
DEFINE_CMD_ALT(QToEuler, qtoe, converts quaternion to euler angles. optional flag indicates the output will be used for rotating an actor, 0, 8, kParams_EulerQuat3Strings4Floats1Int);
