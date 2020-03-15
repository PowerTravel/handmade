#pragma once

#include "aabb.h"
#include "memory.h"

struct collider_mesh
{
  u32 VAO;
  u32 VBO;

  u32 nv;   // Nr Vertices
  u32 nvi;  // 3 times nr Vertice Indeces (CCW Triangles)

  v3* v;    // Vertices
  u32* vi;  // Vertex Indeces
};

struct component_collider
{
  // Always in Model Space
  collider_mesh* Mesh;
  aabb3f AABB;
};

void SetAABBFromColliderMesh( component_collider* Collider )
{
  collider_mesh* ColliderMesh = Collider->Mesh;
  Assert(ColliderMesh->nv >= 3);
  v3 Min = ColliderMesh->v[0];
  v3 Max = ColliderMesh->v[0];
  for( u32 Index = 0; Index < ColliderMesh->nv; ++Index )
  {
    const v3 Point = ColliderMesh->v[Index];

    Min.X = (Point.X < Min.X) ? Point.X : Min.X;
    Min.Y = (Point.Y < Min.Y) ? Point.Y : Min.Y;
    Min.Z = (Point.Z < Min.Z) ? Point.Z : Min.Z;

    Max.X = (Point.X > Max.X) ? Point.X : Max.X;
    Max.Y = (Point.Y > Max.Y) ? Point.Y : Max.Y;
    Max.Z = (Point.Z > Max.Z) ? Point.Z : Max.Z;
  }

  Collider->AABB = AABB3f(Min,Max);
}

void GetTransformedAABBFromColliderMesh( component_collider* Collider, const m4& TransMat, aabb3f* ReturnAABB )
{
  collider_mesh* ColliderMesh = Collider->Mesh;
  Assert(ColliderMesh->nv >= 3);
  v3 Min = V3( TransMat * V4(ColliderMesh->v[0],1));
  v3 Max = Min;
  for( u32 Index = 0; Index < ColliderMesh->nv; ++Index )
  {
    const v3 Point = V3( TransMat * V4(ColliderMesh->v[Index], 1) );

    Min.X = (Point.X < Min.X) ? Point.X : Min.X;
    Min.Y = (Point.Y < Min.Y) ? Point.Y : Min.Y;
    Min.Z = (Point.Z < Min.Z) ? Point.Z : Min.Z;

    Max.X = (Point.X > Max.X) ? Point.X : Max.X;
    Max.Y = (Point.Y > Max.Y) ? Point.Y : Max.Y;
    Max.Z = (Point.Z > Max.Z) ? Point.Z : Max.Z;
  }

  *ReturnAABB = AABB3f(Min,Max);
}

void SetColliderMeshFromAABB( memory_arena* Arena, component_collider* Collider )
{
  Collider->Mesh = (collider_mesh*) PushStruct(Arena, collider_mesh);
  Collider->Mesh->nv  = 8;
  Collider->Mesh->nvi = 36;
  Collider->Mesh->v  = (v3*)  PushArray(Arena, Collider->Mesh->nv,  v3);
  Collider->Mesh->vi = (u32*) PushArray(Arena, Collider->Mesh->nvi, u32);

  GetAABBVertices( &Collider->AABB, Collider->Mesh->v, Collider->Mesh->vi);
}
