#include "component_camera.h"
#include "handmade.h"
#include "entity_components.h"
#include "gjk_epa_visualizer.h"

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
  //    LookAt( Camera, V3(0,0,1), V3(0,0,0));
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

void FlyingCameraController( entity* CameraEntity )
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
    r32 Length = 3;
    if(Controller->LeftStickLeft.EndedDown)
    {
      TranslateCamera( Camera, V3(-dr,0,0));
      hasMoved = true;
    }
    if(Controller->LeftStickRight.EndedDown)
    {
      TranslateCamera( Camera, V3(dr,0,0));
      hasMoved = true;
    }
    if(Controller->LeftStickUp.EndedDown)
    {
      TranslateCamera( Camera, V3(0,dr,0));
      hasMoved = true;
    }
    if(Controller->LeftStickDown.EndedDown)
    {
      TranslateCamera( Camera, V3(0,-dr,0));
      hasMoved = true;
    }
    if(Controller->RightStickUp.EndedDown)
    {
      RotateCamera( Camera, da, V3(1,0,0) );
      hasMoved = true;
    }
    if(Controller->RightStickDown.EndedDown)
    {
      RotateCamera( Camera, da, V3(-1,0,0) );
      hasMoved = true;
    }
    if(Controller->RightStickLeft.EndedDown)
    {
      v4 WorldUpCamCoord = Camera->V * V4(0,1,0,0);
      RotateCamera( Camera, da, V3( WorldUpCamCoord) );
      hasMoved = true;
    }
    if(Controller->RightStickRight.EndedDown)
    {
      v4 WorldDownCamCoord = Camera->V * V4(0,-1,0,0);
      RotateCamera( Camera, da, V3( WorldDownCamCoord ) );
      hasMoved = true;
    }
    if(Controller->RightTrigger.EndedDown)
    {
      TranslateCamera(Camera, V3(0,0,-dr));
      hasMoved = true;
    }
    if(Controller->LeftTrigger.EndedDown)
    {
      TranslateCamera(Camera, V3( 0, 0, dr));
      hasMoved = true;
    }
    if(Controller->A.EndedDown)
    {
      // at Z, top is Y, X is Right
      LookAt( Camera, Length*V3(0,1,0),V3(0,0,0),V3(0,0,-1));
    }
    if(Controller->B.EndedDown)
    {
      // at X, top is Y, X is Left
      LookAt( Camera, Length * Normalize( V3(1,0,0) ),V3(0,0,0));
    }
    if(Controller->X.EndedDown)
    {
      // at X, top is Y, X is Left
      LookAt( Camera, Length*V3(0,0,1), V3(0,0,0));
    }
    if(Controller->Y.EndedDown)
    {
      // at Y, top is X is up, X is Left
      LookAt( Camera, Length*V3(0,1,1),V3(0,0,0), V3(1,0,0));
    }
    if(Controller->RightShoulder.EndedDown)
    {
      SetOrthoProj( Camera, -3, 3 );
    }
    if(Controller->LeftShoulder.EndedDown)
    {
      SetPerspectiveProj( Camera, 0.1, 1000 );
    }

    if(hasMoved)
    {
      UpdateViewMatrix(  Camera );
    }
  }
}

void HeroController( entity* HeroEntity )
{
  game_controller_input* Controller = HeroEntity->ControllerComponent->Controller;
  component_spatial*     Spatial  = HeroEntity->SpatialComponent;
  component_dynamics*    Dynamics = HeroEntity->DynamicsComponent;
  component_camera*      Camera   = HeroEntity->CameraComponent;

  Assert(Spatial);
  Assert(Dynamics);
  Assert(Controller);
  Assert(Camera);

  if( Controller->IsAnalog )
  {
    r32 ImpulseStrength = 20;
    r32 JumpImpulse = 0;
    v2 StickAverage = V2(0,0);

    if(Controller->LeftStickLeft.EndedDown || Controller->LeftStickRight.EndedDown )
    {
      StickAverage.X = Controller->LeftStickAverageX;
    }

    if(Controller->A.EndedDown)
    {
      JumpImpulse = 4;
    }

    Dynamics->ExternalForce = ImpulseStrength * (V3(StickAverage, 0) + V3(0,JumpImpulse,0));

    if(Controller->X.EndedDown)
    {
      *Spatial = component_spatial(V3(0,1,0));
      Dynamics->ExternalForce  = V3(0,0,0);
      Dynamics->LinearVelocity = V3(0,0,0);
    }

    v3 to = Spatial->Position;
    v3 from = to + V3(0,0,3);
    LookAt(Camera, from, to);
  }

}

void EpaGjkVisualizerController( entity* Entity )
{
  gjk_epa_visualizer* Vis = GlobalVis;
  game_controller_input* Controller = Entity->ControllerComponent->Controller;

  Assert(Vis);
  Assert(Controller);

  Vis->TriggerRecord=false;
  if(Vis->UpdateVBO)
  {
    Vis->UpdateVBO=false;
  }

  if( Controller->IsAnalog )
  {
    if(Controller->DPadLeft.EndedDown && !Vis->PreviousLeftButtonState)
    {
      if(Vis->ActiveEPAFrame == 0)
      {
        if(Vis->ActiveSimplexFrame > 0)
        {
          Vis->ActiveSimplexFrame--;
        }
      }

      if(Vis->ActiveEPAFrame > 0)
      {
        Vis->ActiveEPAFrame--;
      }

    }else if(Controller->DPadRight.EndedDown && !Vis->PreviousRightButtonState)
    {
      if(Vis->ActiveSimplexFrame == (s32) Vis->SimplexCount)
      {
        if(Vis->ActiveEPAFrame < (s32) Vis->EPACount-1)
        {
          Vis->ActiveEPAFrame++;
        }
      }

      if(Vis->ActiveSimplexFrame < (s32) Vis->SimplexCount)
      {
        Vis->ActiveSimplexFrame++;
      }
    }else if(Controller->Start.EndedDown && !Vis->PreviousStartButtonState)
    {
      Vis->Playback = !Vis->Playback;
    }else if(Controller->Select.EndedDown && !Vis->PreviousSelectButtonState)
    {
      ResetEPA(Vis);
    }

    Vis->PreviousLeftButtonState   = Controller->DPadLeft.EndedDown;
    Vis->PreviousRightButtonState  = Controller->DPadRight.EndedDown;
    Vis->PreviousStartButtonState  = Controller->Start.EndedDown;
    Vis->PreviousSelectButtonState = Controller->Select.EndedDown;
  }
}

void ControllerSystemUpdate( world* World )
{
  TIMED_FUNCTION();
  for (u32 Index = 0;  Index < World->NrEntities; ++Index )
  {
    entity* E = &World->Entities[Index];
    if (E->ControllerComponent)
    {
      switch (E->ControllerComponent->Type)
      {
        case ControllerType_FlyingCamera:
        {
          FlyingCameraController(E);
        }break;
        case ControllerType_EpaGjkVisualizer:
        {
          EpaGjkVisualizerController(E);
        }break;
        case ControllerType_Hero:
        {
          HeroController(E);
        }break;
      }
    }
  }
}
