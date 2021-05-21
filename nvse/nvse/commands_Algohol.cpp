#include "nvse/PluginAPI.h"
#include "nvse/CommandTable.h"
#include "nvse/GameAPI.h"
#include "nvse/GameForms.h"
#include "nvse/GameObjects.h"

#include "commands_algohol.h"
#include "algohol/algMath.h"
#include "algohol/algTypes.h"
#include "algohol/paramTypes.h"

#define VBUFSIZ 64	//	buffer size for variable names

void setVarByName( Script *scriptObj, ScriptEventList *eventList, const char *var_name, float value )
{
	for (auto entry = scriptObj->varList.Begin(); !entry.End(); entry.Next())
	{
		if ( !strcmp(entry->name.CStr(), var_name ) )
		{
			eventList->GetVariable(entry->idx)->data = value;
			break;
		}
	}
}

///////////////////////////////////////////
///			Vector3 commands
///////////////////////////////////////////

bool Cmd_V3Length_Execute( COMMAND_ARGS )
{
	Vector3 v;

	if( !ExtractArgs(	EXTRACT_ARGS,
						&v.x, &v.y, &v.z ) )
		return true;

	*result = v.Magnitude();
	return true;
}

bool Cmd_V3Normalize_Execute( COMMAND_ARGS )
{
	char vector_x_name[VBUFSIZ], vector_y_name[VBUFSIZ], vector_z_name[VBUFSIZ];
	Vector3 v;

	if( !ExtractArgs(	EXTRACT_ARGS,
						&vector_x_name, &vector_y_name, &vector_z_name,
						&v.x, &v.y, &v.z ) )
		return true;

	V3Normalize( v );

	setVarByName( scriptObj, eventList, vector_x_name, v.x );
	setVarByName( scriptObj, eventList, vector_y_name, v.y );
	setVarByName( scriptObj, eventList, vector_z_name, v.z );
	return true;
}

/*bool Cmd_V3Dotproduct_Execute( COMMAND_ARGS )
{
	Vector3 v1, v2;

	if( !ExtractArgs(	EXTRACT_ARGS,
						&v1.x, &v1.y, &v1.z,
						&v2.x, &v2.y, &v2.z ) )
		return true;

	*result = V3Dotproduct( v1, v2 );
	return true;
}*/

bool Cmd_V3Crossproduct_Execute( COMMAND_ARGS )
{
	char vector_x_name[VBUFSIZ], vector_y_name[VBUFSIZ], vector_z_name[VBUFSIZ];
	Vector3 v1, v2;

	if( !ExtractArgs(	EXTRACT_ARGS,
						&vector_x_name, &vector_y_name, &vector_z_name,
						&v1.x, &v1.y, &v1.z,
						&v2.x, &v2.y, &v2.z ) )
		return true;

	Vector3 out = V3Crossproduct( v1, v2 );

	setVarByName( scriptObj, eventList, vector_x_name, out.x );
	setVarByName( scriptObj, eventList, vector_y_name, out.y );
	setVarByName( scriptObj, eventList, vector_z_name, out.z );
	return true;
}

///////////////////////////////////////////
///			Quaternion commands
///////////////////////////////////////////

bool Cmd_QFromEuler_Execute( COMMAND_ARGS )
{
	char quat_w_name[VBUFSIZ], quat_x_name[VBUFSIZ], quat_y_name[VBUFSIZ], quat_z_name[VBUFSIZ];
	Euler e;
	int actorFlag = 0;

	if( !ExtractArgs(	EXTRACT_ARGS,
						&quat_w_name, &quat_x_name, &quat_y_name, &quat_z_name,
						&e.elevation, &e.bank, &e.heading,
						&actorFlag ) )
		return true;

	Quat out = fromEuler( e, actorFlag );

	setVarByName( scriptObj, eventList, quat_w_name, out.w );
	setVarByName( scriptObj, eventList, quat_x_name, out.x );
	setVarByName( scriptObj, eventList, quat_y_name, out.y );
	setVarByName( scriptObj, eventList, quat_z_name, out.z );
	return true;
}

bool Cmd_QFromAxisAngle_Execute( COMMAND_ARGS )
{
	char quat_w_name[VBUFSIZ], quat_x_name[VBUFSIZ], quat_y_name[VBUFSIZ], quat_z_name[VBUFSIZ];
	Vector3 axis;
	float angle;

	if( !ExtractArgs(	EXTRACT_ARGS,
						&quat_w_name, &quat_x_name, &quat_y_name, &quat_z_name,
						&axis.x, &axis.y, &axis.z, &angle ) )
		return true;

	Quat out = fromAxisAngle( axis, angle );

	setVarByName( scriptObj, eventList, quat_w_name, out.w );
	setVarByName( scriptObj, eventList, quat_x_name, out.x );
	setVarByName( scriptObj, eventList, quat_y_name, out.y );
	setVarByName( scriptObj, eventList, quat_z_name, out.z );
	return true;
}

bool Cmd_QNormalize_Execute( COMMAND_ARGS )
{
	char quat_w_name[VBUFSIZ], quat_x_name[VBUFSIZ], quat_y_name[VBUFSIZ], quat_z_name[VBUFSIZ];
	Quat q;

	if( !ExtractArgs(	EXTRACT_ARGS,
						&quat_w_name, &quat_x_name, &quat_y_name, &quat_z_name,
						&q.w, &q.x, &q.y, &q.z ) )
		return true;

	q.normalize();

	setVarByName( scriptObj, eventList, quat_w_name, q.w );
	setVarByName( scriptObj, eventList, quat_x_name, q.x );
	setVarByName( scriptObj, eventList, quat_y_name, q.y );
	setVarByName( scriptObj, eventList, quat_z_name, q.z );
	return true;
}

bool Cmd_QMultQuatQuat_Execute( COMMAND_ARGS )
{
	char quat_w_name[VBUFSIZ], quat_x_name[VBUFSIZ], quat_y_name[VBUFSIZ], quat_z_name[VBUFSIZ];
	Quat q1, q2;

	if( !ExtractArgs(	EXTRACT_ARGS,
						&quat_w_name, &quat_x_name, &quat_y_name, &quat_z_name,
						&q1.w, &q1.x, &q1.y, &q1.z,
						&q2.w, &q2.x, &q2.y, &q2.z ) )
		return true;

	Quat out = q1 * q2;

	setVarByName( scriptObj, eventList, quat_w_name, out.w );
	setVarByName( scriptObj, eventList, quat_x_name, out.x );
	setVarByName( scriptObj, eventList, quat_y_name, out.y );
	setVarByName( scriptObj, eventList, quat_z_name, out.z );
	return true;
}

bool Cmd_QMultQuatVector3_Execute( COMMAND_ARGS )
{
	char vector_x_name[VBUFSIZ], vector_y_name[VBUFSIZ], vector_z_name[VBUFSIZ];
	Quat q;
	Vector3 v;

	if( !ExtractArgs(	EXTRACT_ARGS,
						&vector_x_name, &vector_y_name, &vector_z_name,
						&q.w, &q.x, &q.y, &q.z,
						&v.x, &v.y, &v.z ) )
		return true;

	Vector3 out = q * v;

	setVarByName( scriptObj, eventList, vector_x_name, out.x );
	setVarByName( scriptObj, eventList, vector_y_name, out.y );
	setVarByName( scriptObj, eventList, vector_z_name, out.z );
	return true;
}

bool Cmd_QInterpolate_Execute( COMMAND_ARGS )
{
	char quat_w_name[VBUFSIZ], quat_x_name[VBUFSIZ], quat_y_name[VBUFSIZ], quat_z_name[VBUFSIZ];
	Quat q1, q2;
	float t;
	int slerpFlag = 0;

	if( !ExtractArgs(	EXTRACT_ARGS,
						&quat_w_name, &quat_x_name, &quat_y_name, &quat_z_name,
						&q1.w, &q1.x, &q1.y, &q1.z,
						&q2.w, &q2.x, &q2.y, &q2.z,
						&t, &slerpFlag ) )
		return true;

	Quat out;
	if ( !slerpFlag )
		out = nlerp( q1, q2, t );
	else
		out = slerp( q1, q2, t );

	setVarByName( scriptObj, eventList, quat_w_name, out.w );
	setVarByName( scriptObj, eventList, quat_x_name, out.x );
	setVarByName( scriptObj, eventList, quat_y_name, out.y );
	setVarByName( scriptObj, eventList, quat_z_name, out.z );
	return true;
}

bool Cmd_QToEuler_Execute( COMMAND_ARGS )
{
	char heading_name[VBUFSIZ], attitude_name[VBUFSIZ], bank_name[VBUFSIZ];
	Quat q;
	int actorFlag = 0;

	if( !ExtractArgs(	EXTRACT_ARGS,
						&attitude_name, &bank_name, &heading_name,
						&q.w, &q.x, &q.y, &q.z,
						&actorFlag ) )
		return true;

	Euler out = fromQuat( q, actorFlag );

	setVarByName( scriptObj, eventList, attitude_name, out.elevation );
	setVarByName( scriptObj, eventList, bank_name, out.bank );
	setVarByName( scriptObj, eventList, heading_name, out.heading );
	return true;
}


