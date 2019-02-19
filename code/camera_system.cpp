#include "entity_components.h"
#include "affine_transformations.h"

void TranslateCamera( component_camera* Camera, const v3& DeltaP  )
{
	Camera->DeltaPos += DeltaP;
}

void RotateCamera( component_camera* Camera, const r32 DeltaAngle, const v3& Axis )
{
	Camera->DeltaRot = GetRotationMatrix( DeltaAngle, V4(Axis, 0) ) * Camera->DeltaRot;
}

void UpdateViewMatrix(  component_camera* Camera )
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

void UpdateViewMatrixAngularMovement(  component_camera* Camera )
{
	m4 CamToWorld = RigidInverse( Camera->V );
	AssertIdentity( CamToWorld*Camera->V,0.001 );

	v4 NewUpInWorldCoord  = Camera->V * V4(0,1,0,0);

	v4 NewPosInWorldCoord  = Column( CamToWorld, 3 ) + CamToWorld*V4( Camera->DeltaPos, 0 );

	LookAt(Camera, V3(NewPosInWorldCoord), V3(0,0,0), V3(NewUpInWorldCoord) );

	Camera->DeltaRot = M4Identity();
	Camera->DeltaPos = V3( 0, 0, 0 );
}


void CameraSystemUpdate( world* World )
{
	for(u32 Index = 0;  Index < World->NrEntities; ++Index )
	{
		entity* Entity = &World->Entities[Index];

		if( ( Entity->Types & COMPONENT_TYPE_CAMERA ) &&
			( Entity->Types & COMPONENT_TYPE_CONTROLLER )  )
		{
			UpdateViewMatrix( Entity->CameraComponent );
		}
	}
}
