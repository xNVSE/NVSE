#pragma once

class Quat
{
public:
	Quat()												{ }
	Quat( const Quat & in )								{ w = in.w; x = in.x; y = in.y; z = in.z; }
	Quat( float inW, float inX, float inY, float inZ )	{ w = inW; x = inX; y = inY; z = inZ; }
	~Quat()												{ }

	Quat & operator*=( float s )		{ w *= s; x *= s; y *= s; z *= s;			return *this; }
	Quat & operator+=( const Quat & q )	{ w += q.w; x += q.x; y += q.y; z += q.z;	return *this; }
	Quat & operator-=( const Quat & q )	{ w -= q.w; x -= q.x; y -= q.y; z -= q.z;	return *this; }
	Quat & operator*=( Quat & q )
	{
		float tw = w * q.w - x * q.x - y * q.y - z * q.z;
		float tx = w * q.x + x * q.w - y * q.z + z * q.y;
		float ty = w * q.y + x * q.z + y * q.w - z * q.x;
		float tz = w * q.z - x * q.y + y * q.x + z * q.w;
		w = tw;
		x = tx;
		y = ty;
		z = tz;
		return *this;
	}
	Vector3 operator*( Vector3 & v )
	{
		float t2 = w * x;
		float t3 = w * y;
		float t4 = w * z;
		float t5 = -x * x;
		float t6 = x * y;
		float t7 = x * z;
		float t8 = -y * y;
		float t9 = y * z;
		float t10 = -z * z;
		Vector3 out;
		out.x = 2 * ( ( t8 + t10 ) * v.x + ( t6 + t4 ) * v.y + ( t7 - t3 ) * v.z ) + v.x;
		out.y = 2 * ( ( t6 - t4 ) * v.x + ( t5 + t10 ) * v.y + ( t9 + t2 ) * v.z ) + v.y;
		out.z = 2 * ( ( t7 + t3 ) * v.x + ( t9 - t2 ) * v.y + ( t5 + t8 ) * v.z ) + v.z;
		return out;
	}

	Quat operator*( float s )			const	{ Quat out( *this );	return out *= s; }
	Quat operator+( const Quat & q )	const	{ Quat out( *this );	return out += q; }
	Quat operator-( const Quat & q )	const	{ Quat out( *this );	return out -= q; }
	Quat operator*( Quat & q )			const	{ Quat out( *this );	return out *= q; }

	void normalize( void );

	union
	{
		struct
		{
			float w, x, y, z;
		};
	};
};

class Euler
{
public:
	Euler() { }
	Euler( const Euler & in )
	{
		elevation = in.elevation;
		bank = in.bank;
		heading = in.heading;
	}
	Euler( float inElevation, float inBank, float inHeading )
	{
		elevation = inElevation;
		bank = inBank;
		heading = inHeading;
	}
	~Euler() { }

	union
	{
		struct
		{
			float elevation, bank, heading;
		};
	};
};