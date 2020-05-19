#pragma once

#include "math/aabb.h"
#include "memory.h"

struct component_collider
{
  // Always in Model Space
  // collider_mesh* Mesh;
  aabb3f AABB;
  u32 MeshHandle;
};

void GetTransformedAABBFromColliderMesh( component_collider* Collider, const m4& TransMat, aabb3f* ReturnAABB )
{
  v3 Min = V3( TransMat * V4(Collider->AABB.P0,1));
  v3 Max = Min;
  v3 AABBVertices[8] = {};
  GetAABBVertices(&Collider->AABB, AABBVertices);
  for( u32 Index = 0; Index < ArrayCount(AABBVertices); ++Index )
  {
    const v3 Point = V3( TransMat * V4(AABBVertices[Index], 1) );

    Min.X = (Point.X < Min.X) ? Point.X : Min.X;
    Min.Y = (Point.Y < Min.Y) ? Point.Y : Min.Y;
    Min.Z = (Point.Z < Min.Z) ? Point.Z : Min.Z;

    Max.X = (Point.X > Max.X) ? Point.X : Max.X;
    Max.Y = (Point.Y > Max.Y) ? Point.Y : Max.Y;
    Max.Z = (Point.Z > Max.Z) ? Point.Z : Max.Z;
  }

  *ReturnAABB = AABB3f(Min,Max);
}

