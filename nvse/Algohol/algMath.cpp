#include "algMath.h"

#define DEGTORAD 0.01745329252f
#define RADTODEG 57.2957795131f

void V3Normalize( Vector3 &v )
{
	float len = v.Magnitude();
	if ( len > 0.0f )
		len = 1.0f / len;
	else
		len = 0.0f;
	v.x *= len;
	v.y *= len;
	v.z *= len;
}

Vector3 V3Crossproduct( Vector3 va, Vector3 vb )
{
	Vector3 out;
	out.x = vb.y * va.z - vb.z * va.y;
	out.y = vb.z * va.x - vb.x * va.z;
	out.z = vb.x * va.y - vb.y * va.x;
	return out;
}

void Quat::normalize( void )
{
	float len = sqrtf( w*w + x*x + y*y + z*z );
	if ( len > 0.0f )
		len = 1.0f / len;
	else
		len = 0.0f;
	w *= len;
	x *= len;
	y *= len;
	z *= len;
}

float QDotproduct( Quat q1, Quat q2 )
{
	return q1.w * q2.w + q1.x * q2.x + q1.y * q2.y + q1.z * q2.z;
}

Quat fromAxisAngle( Vector3 axis, float angle )
{
	V3Normalize( axis );
	angle /= 2 * RADTODEG;
	float s = sinf( angle );
	return Quat( cosf( angle ),
				 axis.x * s,
				 axis.y * s,
				 axis.z * s );
}

Quat fromEuler( Euler e, int flag )
{
	float c1 = cosf( e.elevation * DEGTORAD / 2 );
	float c3 = cosf( e.heading * DEGTORAD / 2 );
	float s1 = sinf( e.elevation * DEGTORAD / 2 );
	float s3 = sinf( e.heading * DEGTORAD / 2 );

	if ( !flag )
	{
		float c2 = cosf( e.bank * DEGTORAD / 2 );
		float s2 = sinf( e.bank * DEGTORAD / 2 );
		float c1c2 = c1 * c2;
		float s1s2 = s1 * s2;

		return Quat( c1c2 * c3 + s1s2 * s3,
					 s1 * c2 * c3 - c1 * s2 * s3,
					 c1 * s2 * c3 + s1 * c2 * s3,
					 c1c2 * s3 - s1s2 * c3 );
	}
	else
		return Quat( c1 * c3,
					 s1 * c3,
					-s1 * s3,
					 c1 * s3 );
}

Quat nlerp( Quat q1, Quat q2, float t )
{
	float cosHalfTheta = QDotproduct( q1, q2 );
	if ( cosHalfTheta < 0 )
	{
		q1.w = -q1.w;
		q1.x = -q1.x;
		q1.y = -q1.y;
		q1.z = -q1.z;
	}

	Quat out = q1 + ( q2 - q1 ) * t;
	out.normalize();
	return out;
}

Quat slerp( Quat q1, Quat q2, float t )
{
	q1.normalize();
	q2.normalize();
	float cosHalfTheta = QDotproduct( q1, q2 );

	if ( fabs( cosHalfTheta ) >= 1.0f )
		return q1;

	if ( cosHalfTheta < 0 )
	{
		q1.w = -q1.w;
		q1.x = -q1.x;
		q1.y = -q1.y;
		q1.z = -q1.z;
	}

	float halfTheta = acosf( cosHalfTheta );
	float sinHalfTheta = sqrtf( 1.0f - cosHalfTheta * cosHalfTheta );

	if ( fabs( sinHalfTheta ) < 0.001f )
		return q1 * 0.5f + q2 * 0.5f;

	float ratioA = sinf( ( 1.0f - t ) * halfTheta ) / sinHalfTheta;
	float ratioB = sinf( t * halfTheta ) / sinHalfTheta; 
	return q1 * ratioA + q2 * ratioB;
}

Euler fromQuat( Quat q, int flag )
{
	Euler out;
	float sqw = q.w * q.w;
	float sqx = q.x * q.x;
	float sqy = q.y * q.y;
	float sqz = q.z * q.z;
	float unit = sqw + sqx + sqy + sqz;

	if ( !flag )
	{
		float test = q.x*q.z - q.w*q.y;

		//	Check gimbal lock at north pole
		if ( test > 0.49992f * unit )
		{
			out.elevation	= -atan2f( 2.0f * q.x*q.y - 2.0f * q.w*q.z , sqw - sqx + sqy - sqz ) * RADTODEG;
			out.bank		= -90.0f;
			out.heading		= 0.0f;
			return out;
		}
		//	Check gimbal lock at south pole
		if ( test < -0.49992f * unit )
		{
			out.elevation	= -atan2f( 2.0f * q.w*q.z - 2.0f * q.x*q.y , sqw - sqx + sqy - sqz ) * RADTODEG;
			out.bank		= 90.0f;
			out.heading		= 0.0f;
			return out;
		}
		out.elevation	= -atan2f( -2.0f * q.x*q.w - 2.0f * q.y*q.z , sqw - sqx - sqy + sqz ) * RADTODEG;
		out.bank		= -asinf( ( 2.0f * test ) / unit ) * RADTODEG;
		out.heading		= -atan2f( -2.0f * q.x*q.y - 2.0f * q.w*q.z , sqw + sqx - sqy - sqz ) * RADTODEG;
		return out;
	}
	else
	{
		out.elevation	= -asinf( 2.0f * q.y*q.z - 2.0f * q.w*q.x ) * RADTODEG;
		out.bank		= 0.0f;
		out.heading		= -atan2f( 2.0f * q.x*q.y - 2.0f * q.w*q.z , sqw + sqx - sqy - sqz ) * RADTODEG;
		return out;
	}
}