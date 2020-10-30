#include "component_camera.h"
#include "handmade.h"
#include "entity_components.h"
/*
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
*/
void FlyingCameraController( component_controller* Controller, component_camera* Camera )
{
  Assert(Camera);
  Assert(Controller);

  game_controller_input* Input = Controller->Controller;
  keyboard_input* Keyboard = Controller->Keyboard;
  if( Input->IsAnalog )
  {
    b32 hasMoved = false;
    r32 dr = 0.05;
    if(Keyboard->Key_SHIFT.EndedDown){
      dr = 0.1;
    }
    r32 da = 0.05;
    r32 Length = 3;
    if(Input->LeftStickLeft.EndedDown || Keyboard->Key_A.EndedDown )
    {
      TranslateCamera( Camera, V3(-dr,0,0));
      hasMoved = true;
    }
    if(Input->LeftStickRight.EndedDown || Keyboard->Key_D.EndedDown)
    {
      TranslateCamera( Camera, V3(dr,0,0));
      hasMoved = true;
    }
    if(Input->LeftStickUp.EndedDown || Keyboard->Key_R.EndedDown)
    {
      TranslateCamera( Camera, V3(0,dr,0));
      hasMoved = true;
    }
    if(Input->LeftStickDown.EndedDown || Keyboard->Key_F.EndedDown)
    {
      TranslateCamera( Camera, V3(0,-dr,0));
      hasMoved = true;
    }
    if( GlobalGameState->Input->MouseButton[PlatformMouseButton_Right].EndedDown )
    {
      //v4 WorldDownCamCoord = Camera->V * V4(-1,0,0,0);
      RotateCamera( Camera, 1.5f*GlobalGameState->Input->MouseDY, V3(1,0,0) );
      hasMoved = true;
    }else{
      if(Input->RightStickUp.EndedDown || Keyboard->Key_UP.EndedDown)
      {
        RotateCamera( Camera, da, V3(1,0,0) );
        hasMoved = true;
      }
      if(Input->RightStickDown.EndedDown || Keyboard->Key_DOWN.EndedDown)
      {
        RotateCamera( Camera, da, V3(-1,0,0) );
        hasMoved = true;
      }
    }
    if( GlobalGameState->Input->MouseButton[PlatformMouseButton_Right].EndedDown )
    {
      v4 WorldDownCamCoord = Camera->V * V4(0,-1,0,0);
      RotateCamera( Camera, 1.5f*GlobalGameState->Input->MouseDX, V3( WorldDownCamCoord ) );
      hasMoved = true;
    }else{
      if(Input->RightStickLeft.EndedDown || Keyboard->Key_LEFT.EndedDown)
      {
        v4 WorldUpCamCoord = Camera->V * V4(0,1,0,0);
        RotateCamera( Camera, da, V3( WorldUpCamCoord) );
        hasMoved = true;
      }
      if(Input->RightStickRight.EndedDown || Keyboard->Key_RIGHT.EndedDown)
      {
        v4 WorldDownCamCoord = Camera->V * V4(0,-1,0,0);
        RotateCamera( Camera, da, V3( WorldDownCamCoord ) );
        hasMoved = true;
      }  
    }
    
    
    if(Input->RightTrigger.EndedDown || Keyboard->Key_W.EndedDown)
    {
      TranslateCamera(Camera, V3(0,0,-dr));
      hasMoved = true;
    }
    if(Input->LeftTrigger.EndedDown || Keyboard->Key_S.EndedDown)
    {
      TranslateCamera(Camera, V3( 0, 0, dr));
      hasMoved = true;
    }
    if(Input->A.EndedDown)
    {
      // at Z, top is Y, X is Right
      LookAt( Camera, Length*V3(0,1,0),V3(0,0,0),V3(0,0,-1));
    }
    if(Input->B.EndedDown)
    {
      // at X, top is Y, X is Left
      LookAt( Camera, Length * Normalize( V3(1,0,0) ),V3(0,0,0));
    }
    if(Input->X.EndedDown)
    {
      // at X, top is Y, X is Left
      LookAt( Camera, Length*V3(0,0,1), V3(0,0,0));
    }
    if(Input->Y.EndedDown)
    {
      // at Y, top is X is up, X is Left
      LookAt( Camera, Length*V3(0,1,1),V3(0,0,0), V3(1,0,0));
    }
    if(Input->RightShoulder.EndedDown)
    {
      SetOrthoProj( Camera, -3, 3 );
    }
    if(Input->LeftShoulder.EndedDown)
    {
      SetPerspectiveProj( Camera, 0.1, 1000 );
    }

    if(hasMoved)
    {
      UpdateViewMatrix(  Camera );
    }
  }
}

void HeroController( component_controller*  Controller,
                     component_spatial*     Spatial,
                     component_dynamics*    Dynamics,
                     component_camera*      Camera )
{
  Assert(Spatial);
  Assert(Dynamics);
  Assert(Controller);
  Assert(Camera);

  game_controller_input* Input = Controller->Controller;
  if( Input->IsAnalog )
  {
    r32 ImpulseStrength = 20;
    r32 JumpImpulse = 0;
    v2 StickAverage = V2(0,0);

    if(Input->LeftStickLeft.EndedDown || Input->LeftStickRight.EndedDown )
    {
      StickAverage.X = Input->LeftStickAverageX;
    }

    if(Input->A.EndedDown)
    {
      JumpImpulse = 4;
    }

    Dynamics->ExternalForce = ImpulseStrength * (V3(StickAverage, 0) + V3(0,JumpImpulse,0));

    if(Input->X.EndedDown)
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

void ControllerSystemUpdate( world* World )
{
  TIMED_FUNCTION();
  entity_manager* EM = GlobalGameState->EntityManager;
  ScopedTransaction(EM);
  component_result* ComponentList = GetComponentsOfType(EM, COMPONENT_FLAG_CAMERA);
  while(Next(EM, ComponentList))
  {
    component_controller* Controller = (component_controller*) GetComponent(EM, ComponentList, COMPONENT_FLAG_CONTROLLER);
    switch (Controller->Type)
    {
      case ControllerType_FlyingCamera:
      {
        component_camera* Camera   = (component_camera*) GetComponent(EM, ComponentList, COMPONENT_FLAG_CAMERA);
        FlyingCameraController(Controller, Camera);
      }break;
      case ControllerType_Hero:
      {
        component_spatial*     Spatial  = (component_spatial*) GetComponent(EM, ComponentList, COMPONENT_FLAG_SPATIAL);
        component_dynamics*    Dynamics = (component_dynamics*) GetComponent(EM, ComponentList, COMPONENT_FLAG_DYNAMICS);
        component_camera*      Camera   = (component_camera*) GetComponent(EM, ComponentList, COMPONENT_FLAG_CAMERA);
        HeroController(Controller, Spatial, Dynamics, Camera);
      }break;
    }
  }
}
