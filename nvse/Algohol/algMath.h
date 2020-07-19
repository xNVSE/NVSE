#include "algTypes.h"


void V3Normalize( Vector3 &v );
Vector3 V3Crossproduct( Vector3 va, Vector3 vb );

Quat fromEuler( Euler e, int flag );
Quat fromAxisAngle( Vector3 axis, float angle );
Quat nlerp( Quat q1, Quat q2, float t );
Quat slerp( Quat q1, Quat q2, float t );

Euler fromQuat( Quat q, int flag );