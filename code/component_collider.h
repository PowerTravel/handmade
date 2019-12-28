#pragma once

#include "aabb.h"

struct collider_mesh
{
  u32 nv;   // Nr Vertices
  u32 nvi;  // 3 times nr Vertice Indeces (CCW Triangles)

  v3* v;    // Vertices
  u32* vi;  // Vertex Indeces
};

struct component_collider
{
  // Always in Model Space
  collider_mesh* Mesh;
};

void
SetAABBTriangles( memory_arena* Arena, const aabb3f* AABB, collider_mesh* ColliderMesh )
{
  ColliderMesh->nv  = 8;
  ColliderMesh->nvi = 36;
  ColliderMesh->v  = (v3*) PushArray(Arena,  ColliderMesh->nv, v3);
  ColliderMesh->vi = (u32*) PushArray(Arena, ColliderMesh->nvi, u32);

  GetAABBVertices( AABB, ColliderMesh->v );

  // Z Negative
  ColliderMesh->vi[0] = 1;
  ColliderMesh->vi[1] = 0;
  ColliderMesh->vi[2] = 3;

  ColliderMesh->vi[3] = 1;
  ColliderMesh->vi[4] = 3;
  ColliderMesh->vi[5] = 2;

  // Z Positive
  ColliderMesh->vi[6] = 4;
  ColliderMesh->vi[7] = 5;
  ColliderMesh->vi[8] = 6;

  ColliderMesh->vi[9] = 4;
  ColliderMesh->vi[10] = 6;
  ColliderMesh->vi[11] = 7;

  // X Negative
  ColliderMesh->vi[12] = 0;
  ColliderMesh->vi[13] = 4;
  ColliderMesh->vi[14] = 7;

  ColliderMesh->vi[15] = 0;
  ColliderMesh->vi[16] = 7;
  ColliderMesh->vi[17] = 3;

  // X Positive
  ColliderMesh->vi[18] = 5;
  ColliderMesh->vi[19] = 1;
  ColliderMesh->vi[20] = 2;

  ColliderMesh->vi[21] = 5;
  ColliderMesh->vi[22] = 2;
  ColliderMesh->vi[23] = 6;

  // Y Negative
  ColliderMesh->vi[24] = 0;
  ColliderMesh->vi[25] = 1;
  ColliderMesh->vi[26] = 5;

  ColliderMesh->vi[27] = 0;
  ColliderMesh->vi[28] = 5;
  ColliderMesh->vi[29] = 4;

  // Y Positive
  ColliderMesh->vi[30] = 7;
  ColliderMesh->vi[31] = 6;
  ColliderMesh->vi[32] = 2;

  ColliderMesh->vi[33] = 7;
  ColliderMesh->vi[34] = 2;
  ColliderMesh->vi[35] = 3;
}
internal aabb3f
GetAABB( collider_mesh* Mesh )
{
  Assert(Mesh->nv >= 3);
  v3 Min = Mesh->v[0];
  v3 Max = Mesh->v[0];
  for( u32 Index = 0; Index < Mesh->nv; ++Index )
  {
    const v3 Point = Mesh->v[Index];

    Min.X = (Point.X < Min.X) ? Point.X : Min.X;
    Min.Y = (Point.Y < Min.Y) ? Point.Y : Min.Y;
    Min.Z = (Point.Z < Min.Z) ? Point.Z : Min.Z;

    Max.X = (Point.X > Max.X) ? Point.X : Max.X;
    Max.Y = (Point.Y > Max.Y) ? Point.Y : Max.Y;
    Max.Z = (Point.Z > Max.Z) ? Point.Z : Max.Z;
  }

  aabb3f Result = AABB3f(Min,Max);
  return Result;
}

aabb3f GetAABB( component_collider* ColliderComponent )
{
  Assert(ColliderComponent);
  aabb3f Result = {};
  if(ColliderComponent->Mesh)
  {
    Result = GetAABB(ColliderComponent->Mesh);
  }
  return Result;
}
