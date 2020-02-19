#pragma once

#include "component_collider.h"
struct simplex_list
{
  v3 ClosestPointOnSurface;
  collider_mesh Mesh;
  simplex_list* Next;
  simplex_list* Previous;
};

struct polytype_edge
{
  v3 P0;
  v3 P1;
  b32 IsBorder;
};
/*
struct polytype_face_list
{
  u32 PolytypeEdgeCount;
  polytype_edge* Edges;
  polytype_list* Next;
  polytype_list* Previous;
};
*/
struct memory_arena;
struct component_gjk_epa_visualizer
{
  memory_arena* Arena;

  entity* A;
  entity* B;

  simplex_list* GJKSimplexSeries;
  simplex_list* ActiveSimplexFrame;

  b32 Playback;
  b32 TriggerRecord;

  // Todo (Jakob) Create a more integrated rising/lowering edge fun in platform controller
  b32 PreviousStartButtonState;
  b32 PreviousSelectButtonState;
  b32 PreviousLeftButtonState;
  b32 PreviousRightButtonState;
//  simplex_list* EPAPolytypeSeries;
//  simplex_list* ActivePolytypeFrame;
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
      Vis->ActiveSimplexFrame = Vis->ActiveSimplexFrame->Previous;
    }else if(Controller->DPadRight.EndedDown && !Vis->PreviousRightButtonState)
    {
      Vis->ActiveSimplexFrame = Vis->ActiveSimplexFrame->Next;
    }else if(Controller->Start.EndedDown && !Vis->PreviousStartButtonState)
    {
      Vis->Playback = false;
      Vis->GJKSimplexSeries = 0;
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
