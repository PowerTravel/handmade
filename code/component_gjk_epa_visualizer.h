#pragma once

#include "component_collider.h"
#include "vector_math.h"
struct simplex_index
{
  u32  Offset;
  u32  Length;
  v3   ClosestPoint;
};

struct memory_arena;
struct component_gjk_epa_visualizer
{
  memory_arena* Arena;

  entity* A;
  entity* B;

  //simplex_vertice_list* CSOVertices;
  u32 VAO;
  u32 IndexCount;
  u32 Indeces[512];
  u32 VertexCount;
  v3 Vertices[64];
  u32 NrFrames;
  simplex_index Simplex[12];
  u32 ActiveSimplexFrame;

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
      Vis->ActiveSimplexFrame = (--Vis->ActiveSimplexFrame % Vis->NrFrames);
    }else if(Controller->DPadRight.EndedDown && !Vis->PreviousRightButtonState)
    {
      Vis->ActiveSimplexFrame = (++Vis->ActiveSimplexFrame % Vis->NrFrames);
    }else if(Controller->Start.EndedDown && !Vis->PreviousStartButtonState)
    {
      Vis->Playback = false;
      Vis->ActiveSimplexFrame = 0;
    }else if(Controller->Select.EndedDown && !Vis->PreviousSelectButtonState)
    {
      Vis->TriggerRecord = true;
      Vis->Playback = true;
    }

    Vis->PreviousLeftButtonState   = Controller->DPadLeft.EndedDown;
    Vis->PreviousRightButtonState  = Controller->DPadRight.EndedDown;
    Vis->PreviousStartButtonState  = Controller->Start.EndedDown;
    Vis->PreviousSelectButtonState = Controller->Select.EndedDown;
  }
}
