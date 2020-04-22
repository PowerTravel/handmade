#include "component_camera.h"
#include "entity_components.h"
#include "math/affine_transformations.h"

// This is a broken function
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
  TIMED_FUNCTION();
	for(u32 Index = 0;  Index < World->NrEntities; ++Index )
	{
		entity* Entity = &World->Entities[Index];

		if( ( Entity->Types & COMPONENT_TYPE_CAMERA ) )
		{
			UpdateViewMatrix( Entity->CameraComponent );
		}
	}
}
