#pragma once

#include "component_collider.h"
#include "vector_math.h"
struct simplex_index
{
  u32  Offset;
  u32  Length;
  v3   ClosestPoint;
};


struct component_gjk_epa_visualizer
{
  entity* A;
  entity* B;
  u32 CSOMeshOffset;
  u32 CSOMeshLength;

  u32 VAO;
  u32 VBO;
  u32 UpdateVBO;
  u32 IndexCount;
  u32 Indeces[512];
  u32 VertexCount;
  v3 Vertices[256];
  u32 SimplexCount;
  simplex_index Simplex[12];
  s32 ActiveSimplexFrame;

  b32 Playback;
  b32 TriggerRecord;

  // Todo (Jakob) Create a more integrated rising/lowering edge fun in platform controller
  b32 PreviousStartButtonState;
  b32 PreviousSelectButtonState;
  b32 PreviousLeftButtonState;
  b32 PreviousRightButtonState;
};

void EpaGjkVisualizerController( entity* Entity )
{
  component_gjk_epa_visualizer* Vis = Entity->GjkEpaVisualizerComponent;
  game_controller_input* Controller = Entity->ControllerComponent->Controller;

  Assert(Vis);
  Assert(Controller);
  if( Controller->IsAnalog )
  {
    if(Controller->DPadLeft.EndedDown && !Vis->PreviousLeftButtonState)
    {
      Vis->ActiveSimplexFrame--;
      if(Vis->ActiveSimplexFrame < 0)
      {
        Vis->ActiveSimplexFrame = Vis->SimplexCount-1;
      }
    }else if(Controller->DPadRight.EndedDown && !Vis->PreviousRightButtonState)
    {
      Vis->ActiveSimplexFrame++;
      if(Vis->ActiveSimplexFrame >= (s32) Vis->SimplexCount)
      {
        Vis->ActiveSimplexFrame = 0;
      }
    }else if(Controller->Start.EndedDown && !Vis->PreviousStartButtonState)
    {
      Vis->Playback = !Vis->Playback;
    }else if(Controller->Select.EndedDown && !Vis->PreviousSelectButtonState)
    {
      Vis->IndexCount = 0;
      Vis->VertexCount = 0;
      Vis->SimplexCount = 0;
      Vis->CSOMeshOffset = 0;
      Vis->CSOMeshLength = 0;

      Vis->ActiveSimplexFrame = 0;
      Vis->TriggerRecord = true;
      Vis->Playback = true;
    }

    Vis->PreviousLeftButtonState   = Controller->DPadLeft.EndedDown;
    Vis->PreviousRightButtonState  = Controller->DPadRight.EndedDown;
    Vis->PreviousStartButtonState  = Controller->Start.EndedDown;
    Vis->PreviousSelectButtonState = Controller->Select.EndedDown;
  }
}
