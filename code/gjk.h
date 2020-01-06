#pragma once

#include "aabb.h"
#include "data_containers.h"

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

struct gjk_partial_result
{
  v3  ClosestPoint;
  r32 Distance;
  gjk_simplex ReducedSimplex;
  b32 Reduced;
};

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

struct gjk_halfedge;
struct gjk_face;

struct gjk_vertex
{
  gjk_support P;
  u32 Idx;
  gjk_halfedge* OutgoingEdge;
};

struct gjk_halfedge
{
  gjk_vertex*   TargetVertex;
  gjk_face*     LeftFace;
  gjk_halfedge* NextEdge;
  gjk_halfedge* PreviousEdge;
  gjk_halfedge* OppositeEdge;
};

struct gjk_face
{
  gjk_halfedge* Edge;
};

gjk_collision_result GJKCollisionDetection(const m4* AModelMat, const collider_mesh* AMesh,
                                           const m4* BModelMat, const collider_mesh* BMesh,
                                          memory_arena* TemporaryArena = 0, platform_api* API = 0);

contact_data EPACollisionResolution(memory_arena* TemporaryArena, const m4* AModelMat, const collider_mesh* AMesh,
                                    const m4* BModelMat, const collider_mesh* BMesh, gjk_simplex& Simplex,
                                    platform_api* API = 0 );