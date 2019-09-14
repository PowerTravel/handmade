#include "component_controller.h"

void ControllerSystemUpdate( world* World )
{
	for(u32 Index = 0;  Index < World->NrEntities; ++Index )
	{
		entity* E = &World->Entities[Index];
		if(E->Types & COMPONENT_TYPE_CONTROLLER)
		{
			Assert(E->ControllerComponent->ControllerMappingFunction);

			E->ControllerComponent->ControllerMappingFunction(E);
		}
	}
}

void CameraController( entity* CameraEntity )
{
	component_camera* Camera = CameraEntity->CameraComponent;
	game_controller_input* Controller = CameraEntity->ControllerComponent->Controller;
	Assert(Camera);
	Assert(Controller);
	if( Controller->IsAnalog )
	{
		b32 hasMoved = false;
		r32 dr = 0.05;
		r32 da = 0.05;
		r32 Length = 1;
		
		if(Controller->RightStickUp.EndedDown)
		{
			TranslateCamera(Camera, V3(0,dr,0));
		}
		if(Controller->RightStickDown.EndedDown)
		{
			TranslateCamera(Camera, V3(0,-dr,0));	
		}		
		if(Controller->RightStickLeft.EndedDown)
		{
			TranslateCamera(Camera, V3(-dr,0,0));
		}
		if(Controller->RightStickRight.EndedDown)
		{
			TranslateCamera(Camera, V3(dr,0,0));	
		}
		if(Controller->RightTrigger.EndedDown)
		{
			TranslateCamera(Camera, V3(0,0,-dr));
		}
		if(Controller->LeftTrigger.EndedDown)
		{
			TranslateCamera(Camera, V3( 0, 0, dr));
		}
		if(Controller->A.EndedDown)
		{
			// at Z, top is Y, X is Right
			LookAt( Camera, V3(0,0,1),V3(0,0,0));
		}
		if(Controller->B.EndedDown)
		{
			// at X, top is Y, X is Left
			LookAt( Camera, Normalize( V3(0,0,1) ),V3(1,1,0), V3(0,0,1));			
		}
		if(Controller->X.EndedDown)
		{
			// at X, top is Y, X is Left
	//		LookAt( Camera, V3(0,0,1), V3(0,0,0));
		}
		if(Controller->Y.EndedDown)
		{
			LookAt( Camera,  V3(1,1,1), V3(0,0,0));
		}
		if(Controller->RightShoulder.EndedDown)
		{
			r32 NearClippingPlane = -100;
			r32 FarClippingPlane = 100;
			SetOrthoProj( Camera, NearClippingPlane, FarClippingPlane, 8, -8, 4.5, -4.5 );
		}
		if(Controller->LeftShoulder.EndedDown)
		{
			r32 NearClippingPlane = 0.1;
			r32 FarClippingPlane = 100;
			SetPerspectiveProj( Camera, NearClippingPlane, FarClippingPlane );
		}
	}
}

void HeroController( entity* HeroEntity )
{
	game_controller_input* Controller = HeroEntity->ControllerComponent->Controller;
	component_spatial* Spatial =  HeroEntity->SpatialComponent;
	
	Assert(Spatial);
	Assert(Controller);

	if( Controller->IsAnalog )
	{
		r32 ImpulseStrength = 100;
		v2 StickAverage = V2(0,0);
		if(Controller->LeftStickLeft.EndedDown || Controller->LeftStickRight.EndedDown )
		{
			StickAverage.X = Controller->LeftStickAverageX;
		}

		if(Controller->LeftStickUp.EndedDown || Controller->LeftStickDown.EndedDown)
		{
			StickAverage.Y = Controller->LeftStickAverageY;
		}

		Spatial->ExternalForce = ImpulseStrength * V3( StickAverage ); 

		if(Controller->X.EndedDown)
		{
			// at X, top is Y, X is Left
			Spatial->Position = V3(3,3,0);
			Spatial->Velocity = V3(0,0,0);
			Spatial->ExternalForce = {};
		}
	}
}