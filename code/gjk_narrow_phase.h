#pragma once

#include "math/aabb.h"
#include "data_containers.h"

struct component_gjk_epa_visualizer;
struct collider_mesh;

// Gilbert-Johnson-Keerthi (GJK) collision detection algorithm
// See: http://www.allenchou.net/2013/12/game-physics-collision-detection-gjk/
struct gjk_support
{
  v3 S; // Combined Support
  v3 A; // Support for A
  v3 B; // Support for B
};

struct gjk_simplex
{
  u32 Dimension;
  gjk_support SP[4];
};

struct gjk_collision_result
{
  gjk_simplex Simplex;
  b32 ContainsOrigin;
};

gjk_support CsoSupportFunction( const m4* AModelMat, const collider_mesh* AMesh,
                                const m4* BModelMat, const collider_mesh* BMesh, const v3 Direction );

void SetPointsAndIndecesForCCWTetrahedron( gjk_simplex* Simplex, u32 TriangleIndeces[]);
void BlowUpSimplex( const m4* AModelMat, const collider_mesh* AMesh,
                    const m4* BModelMat, const collider_mesh* BMesh,
                    gjk_simplex* Simplex);

gjk_collision_result GJKCollisionDetection(const m4* AModelMat, const collider_mesh* AMesh,
                                           const m4* BModelMat, const collider_mesh* BMesh);
