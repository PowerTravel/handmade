#pragma once

#include "math/vector_math.h"

struct gjk_simplex;
struct entity;
struct memory_arena;

struct joint_constraint
{
  u32 EntityA;
  u32 EntityB;
  v3 LocalAnchorA;
  v3 LocalAnchorB;
  v3 LocalCenterA;
  v3 LocalCenterB;

  v3 rA;
  v3 rB;

    v3 d;
    v3 InvMJ[4];
  v3 Jacobian[4];
  m3 InvMass[4];
  r32 Lambda;
  };

struct contact_data_cache
{
  v3 J[4];
  v3 Jn1[4];
  v3 Jn2[4];
  v3 InvMJ[4];
  v3 InvMJn1[4];
  v3 InvMJn2[4];
  r32 AccumulatedLambda;
  r32 AccumulatedLambdaN1;
  r32 AccumulatedLambdaN2;
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
  b32 Persistent;
  contact_data_cache Cache;
};

template class vector_list<contact_data>;

struct contact_manifold
{
  u32 EntityIDA;
  u32 EntityIDB;
  u32 WorldArrayIndex;
  u32 MaxContactCount;
  vector_list<contact_data> Contacts;
  contact_manifold* Next;
};

struct world_contact_chunk
{
  u32 MaxCount;
  contact_manifold* ManifoldVector;
  contact_manifold* FirstManifold;
};

world_contact_chunk* CreateWorldContactChunk(memory_arena* Arena, u32 MaxCount)
{
  world_contact_chunk* Result = PushStruct(Arena, world_contact_chunk);
  Result->MaxCount = MaxCount;
  Result->ManifoldVector = PushArray(Arena, MaxCount, contact_manifold);
  for (u32 i = 0; i < MaxCount; ++i)
  {
    contact_manifold* Manifold = Result->ManifoldVector + i;
    Manifold->Contacts = vector_list<contact_data>(Arena,5);
  }
  Result->FirstManifold = 0;
  return Result;
}

contact_manifold* FindManifoldSlot(world_contact_chunk* Manifolds, u32 EntityIDA, u32 EntityIDB);
contact_data EPACollisionResolution(memory_arena* Arena,
                                    const m4* AModelMat, const collider_mesh* AMesh,
                                    const m4* BModelMat, const collider_mesh* BMesh, gjk_simplex* Simplex);
