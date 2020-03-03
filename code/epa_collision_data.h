#pragma once
#include "gjk_narrow_phase.h"

struct contact_data
{
  v3 A_ContactWorldSpace;
  v3 B_ContactWorldSpace;
  v3 A_ContactModelSpace;
  v3 B_ContactModelSpace;
  v3 ContactNormal;
  v3 TangentNormalOne;
  v3 TangentNormalTwo;
  r32 PenetrationDepth;
};

contact_data EPACollisionResolution(memory_arena* TemporaryArena, const m4* AModelMat, const collider_mesh* AMesh,
                                    const m4* BModelMat, const collider_mesh* BMesh, gjk_simplex& Simplex,
                                    component_gjk_epa_visualizer* Vis = 0 );
