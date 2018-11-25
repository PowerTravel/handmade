#include "entity_components.h"

void SetOrthoProj( camera_component* Camera, r32 aNear, r32 aFar, r32 aRight, r32 aLeft, r32 aTop, r32 aBottom )
{
	aFar 	   = -aFar;
	aNear 	   = -aNear;
	r32 rlSum  = aRight+aLeft;
	r32 rlDiff = aRight-aLeft;

	r32 tbSum  = aTop+aBottom;
	r32 tbDiff = aTop-aBottom;

	r32 fnSum  = aFar+aNear;
	r32 fnDiff = aFar-aNear;

	Camera->P =  M4( 2/rlDiff,         0,        0, -rlSum/rlDiff, 
				            0,   2/tbDiff,       0, -tbSum/tbDiff, 
				            0,         0, 2/fnDiff, -fnSum/fnDiff,
				            0,         0,        0,             1);
}

void SetOrthoProj( camera_component* Camera, r32 n, r32 f )
{
	r32 AspectRatio = Camera->ScreenWidth/Camera->ScreenHeight;
	r32 scale = - Tan( Camera->AngleOfView * 0.5f * Pi32 / 180.f ) * n;
	r32 r = AspectRatio * scale;
	r32 l = -r;
	r32 t = scale;
	r32 b = -t;
	SetOrthoProj( Camera, n, f, r, l, t, b );
}

void SetPerspectiveProj( camera_component* Camera, r32 aNear, r32 aFar, r32 aRight, r32 aLeft, r32 aTop, r32 aBottom )
{
	r32 rlSum  = aRight+aLeft;
	r32 rlDiff = aRight-aLeft;

	r32 tbSum  = aTop+aBottom;
	r32 tbDiff = aTop-aBottom;

	r32 fnSum  = aFar+aNear;
	r32 fnDiff = aFar-aNear;

	r32 n2 = aNear*2;

	r32 fn2Prod = 2*aFar*aNear;

	// Note(Jakob): The minus sign in M[0,0] is arbitrarily put to fix a weird issue where X-axis movement and projection was
	// inverted. Ie placing the camrea on X looking towards origin rendered Z-axis to the right. Same with movement
	// Putting a minus sign here (and in the orthographic projection) fixes the issue. 
	// The DANGER and probable outcome of this is that we are masking another bug somewhere. I can't however find it. 
	Camera->P =  M4( n2/rlDiff,         0,  rlSum/rlDiff,               0, 
	                          0, n2/tbDiff,  tbSum/tbDiff,               0, 
	                          0,         0, -fnSum/fnDiff, -fn2Prod/fnDiff,
	                          0,         0,            -1,              -0);
}

void SetPerspectiveProj( camera_component* Camera, const r32 n, const r32 f)
{
	r32 AspectRatio = Camera->ScreenWidth/Camera->ScreenHeight;
	r32 scale = Tan( Camera->AngleOfView * 0.5f * Pi32 / 180.f ) * n;
	r32 r     = AspectRatio * scale;
	r32 l     = -r;
	r32 t     = scale;
	r32 b     = -t;

	SetPerspectiveProj( Camera, n, f, r, l, t, b );
} 

//		void setPinholeCamera( r32 filmApertureHeight, r32 filmApertureWidth, 
//			 					 r32 focalLength, 		r32 nearClippingPlane, 
//			 					 r32 inchToMM = 25.4f )
//		{	
//			r32 top = ( nearClippingPlane* filmApertureHeight * inchToMM * 0.5 ) /  (r32) focalLength;
//			r32 bottom = -top;
//			r32 filmAspectRatio = filmApertureWidth / (r32) filmApertureHeight;
//			r32 left = bottom * filmAspectRatio;
//			r32 left = -right;
//
//			setPerspectiveProj( r32 aNear, r32 aFar, r32 aLeft, r32 aRight, r32 aTop, r32 aBottom );
//
//		}
void LookAt( camera_component* Camera, v3 aFrom,  v3 aTo,  v3 aTmp = V3(0,1,0) )
{
	v3 Forward = Normalize(aFrom - aTo);
	v3 Right   = Normalize( CrossProduct(aTmp, Forward) );
	v3 Up      = Normalize( CrossProduct(Forward, Right) );

	m4 CamToWorld; 
	CamToWorld.r0 = V4(Right,	0);
	CamToWorld.r1 = V4(Up,		0);
	CamToWorld.r2 = V4(Forward,	0);
	CamToWorld.r3 = V4(aFrom,	1);
	CamToWorld = Transpose(CamToWorld);

	Camera->V = RigidInverse(CamToWorld);
	AssertIdentity(Camera->V * CamToWorld, 0.001 );
}

void Translate( camera_component* Camera, const v3& DeltaP  )
{
	Camera->DeltaPos += DeltaP;
}

void Rotate( camera_component* Camera, const r32 DeltaAngle, const v3& Axis )
{
	Camera->DeltaRot = GetRotationMatrix( DeltaAngle, V4(Axis, 0) ) * Camera->DeltaRot;
}

void UpdateViewMatrix(  camera_component* Camera )
{
	m4 CamToWorld = RigidInverse( Camera->V );
	AssertIdentity( CamToWorld*Camera->V,0.001 );

	v4 NewUpInCamCoord    = Column( Camera->DeltaRot, 1 );
	v4 NewUpInWorldCoord  = CamToWorld * NewUpInCamCoord;

	v4 NewAtDirInCamCoord  = Column( Camera->DeltaRot, 2 );
	v4 NewAtDirInWorldCord = CamToWorld * NewAtDirInCamCoord;
	v4 NewPosInWorldCoord  = Column( CamToWorld, 3 ) + CamToWorld*V4( Camera->DeltaPos, 0 );

	v4 NewAtInWorldCoord   = NewPosInWorldCoord-NewAtDirInWorldCord;

	LookAt(Camera, V3(NewPosInWorldCoord), V3(NewAtInWorldCoord), V3(NewUpInWorldCoord) );

	Camera->DeltaRot = M4Identity();
	Camera->DeltaPos = V3( 0, 0, 0 );
}

void UpdateCameraMovement(camera_component* Camera, controller_component* ControllerComponent)
{
	game_controller_input* Controller = ControllerComponent->Controller;
	if( Controller->IsAnalog )
	{
		b32 hasMoved = false;
		r32 dr = 0.05;
		r32 da = 0.05;
		m4& DeltaRot = Camera->DeltaRot;
		v3& DeltaPos = Camera->DeltaPos;
		if(Controller->Start.EndedDown)
		{
			v3 CamPos = V3(0,0,1);
			v3 CamAt =  V3(0,0,0);
			LookAt( Camera, CamPos, CamAt );
		}
		if(Controller->LeftStickLeft.EndedDown)
		{
			DeltaPos += V3(-dr,0,0);
			hasMoved = true;
		}
		if(Controller->LeftStickRight.EndedDown)
		{
			DeltaPos = V3(dr,0,0);
			hasMoved = true;
		}
		if(Controller->LeftStickUp.EndedDown)
		{
			DeltaPos = V3( 0, dr, 0 );
			hasMoved = true;
		}
		if(Controller->LeftStickDown.EndedDown)
		{
			DeltaPos = V3( 0, -dr, 0 );
			hasMoved = true;
		}
		if(Controller->RightStickUp.EndedDown)
		{
			const v4 RotAxis = V4( 1, 0, 0, 0 );
			Rotate( da, RotAxis, DeltaRot );
			hasMoved = true;
		}
		if(Controller->RightStickDown.EndedDown)
		{
			const v4 RotAxis = V4( -1, 0, 0, 0 );
			Rotate( da, RotAxis, DeltaRot );
			hasMoved = true;
		}		
		if(Controller->RightStickLeft.EndedDown)
		{
			const v4 RotAxis = V4( 0, 1, 0, 0);
			Rotate( da, RotAxis, DeltaRot );
			hasMoved = true;
		}
		if(Controller->RightStickRight.EndedDown)
		{
			const v4 RotAxis = V4(0,-1,0,0);
			Rotate( da, RotAxis, DeltaRot );
			hasMoved = true;
		}
		if(Controller->RightTrigger.EndedDown)
		{
			DeltaPos += V3(0,0,-dr);
			hasMoved = true;
		}
		if(Controller->LeftTrigger.EndedDown)
		{
			DeltaPos += V3( 0, 0, dr);
			hasMoved = true;
		}
		if(Controller->A.EndedDown)
		{
			// at Z, top is Y, X is Right
			LookAt( Camera, V3(0,0,3),V3(0,0,0));
		}
		if(Controller->B.EndedDown)
		{
			// at X, top is Y, X is Left
			LookAt( Camera, V3(1,1,1),V3(0,0,0));			
		}
		if(Controller->X.EndedDown)
		{
			// at X, top is Y, X is Left
			LookAt( Camera, V3(3,0,0), V3(0,0,0));
		}
		if(Controller->Y.EndedDown)
		{
			// at Y, top is X is up, X is Left
			LookAt( Camera, V3(0,3,0),V3(0,0,0), V3(1,0,0));
		}
		if(Controller->RightShoulder.EndedDown)
		{
			SetOrthoProj( Camera, -1, 1 );
		}
		if(Controller->LeftShoulder.EndedDown)
		{
			SetPerspectiveProj( Camera, -0.1, -1000 );
		}

		if(hasMoved)
		{
			UpdateViewMatrix(  Camera );
		}
	}
}

void CameraSystemUpdate( world* World )
{
	for(s32 Index = 0;  Index < ArrayCount( World->WorldEntities); ++Index )
	{
		entity* Entity = &World->WorldEntities[Index];

		if( ( Entity->Types & ( COMPONENT_TYPE_CAMERA | COMPONENT_TYPE_CONTROLLER )  ) )
		{
			UpdateCameraMovement( &Entity->CameraComponent, &Entity->ControllerComponent );
		}
	}
}
