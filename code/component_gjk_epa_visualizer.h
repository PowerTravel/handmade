#pragma once

#include "component_collider.h"
#include "vector_math.h"
struct simplex_index
{
  u32  Offset;
  u32  Length;
  v3   ClosestPoint;
};

struct epa_index
{
  u32 MeshOffset;
  u32 MeshLength;
  u32 NormalOffset;
  u32 NormalLength;
  v3 ClosestPointOnFace;
  v3 SupportPoint;
  b32 FillMesh;
  u32 ClosestFace;
};

struct component_gjk_epa_visualizer
{
  u32 CSOMeshOffset;
  u32 CSOMeshLength;

  u32 GJKMeshOffset;
  u32 GJKMeshLength;

  u32 EPAMeshOffset;
  u32 EPAMeshLength;

  u32 VAO;
  u32 VBO;
  u32 UpdateVBO;

  u32 IndexCount;
  u32 MaxIndexCount;
  u32* Indeces;

  u32 VertexCount;
  u32 MaxVertexCount;
  v3* Vertices;

  u32 NormalIndexCount;
  u32 NormalMaxIndexCount;
  u32* NormalIndeces;

  u32 NormalCount;
  u32 MaxNormalCount;
  v3* Normals;

  u32 SimplexCount;
  u32 MaxSimplexCount;
  simplex_index* Simplex;
  s32 ActiveSimplexFrame;

  u32 EPACount;
  u32 MaxEPACount;
  epa_index* EPA;
  s32 ActiveEPAFrame;

  b32 Playback;
  b32 TriggerRecord;

  // Todo (Jakob) Create a more integrated rising/lowering edge fun in platform controller
  b32 PreviousStartButtonState;
  b32 PreviousSelectButtonState;
  b32 PreviousLeftButtonState;
  b32 PreviousRightButtonState;
};

void ResetEPA(component_gjk_epa_visualizer* Vis)
{
  Vis->CSOMeshOffset = 0;
  Vis->CSOMeshLength = 0;
  Vis->GJKMeshOffset = 0;
  Vis->GJKMeshLength = 0;
  Vis->EPAMeshOffset = 0;
  Vis->EPAMeshLength = 0;

  Vis->IndexCount = 0;
  Vis->VertexCount = 0;
  Vis->NormalIndexCount = 0;
  Vis->NormalCount = 0;
  Vis->SimplexCount = 0;
  Vis->CSOMeshOffset = 0;
  Vis->CSOMeshLength = 0;
  Vis->ActiveSimplexFrame = 0;
  Vis->EPACount = 0;
  Vis->ActiveEPAFrame = 0;

  for( u32 i = 0; i<Vis->MaxIndexCount; ++i)
  {
    Vis->Indeces[i] = 0;
  }

  Vis->TriggerRecord = true;
  Vis->Playback = true;
  Vis->UpdateVBO = true;
}

void EpaGjkVisualizerController( entity* Entity )
{
  component_gjk_epa_visualizer* Vis = Entity->GjkEpaVisualizerComponent;
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
