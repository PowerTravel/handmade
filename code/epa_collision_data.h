#pragma once

#include "math/vector_math.h"

struct gjk_simplex;
struct entity;
struct memory_arena;

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

struct contact_manifold
{
  entity* A;
  entity* B;
  u32 WorldArrayIndex;
  u32 ContactCount;
  u32 MaxContactCount;
  contact_data Contacts[4];
  contact_data_cache CachedData[4];
  contact_manifold* Next;
};


contact_data EPACollisionResolution(memory_arena* TemporaryArena, const m4* AModelMat, const collider_mesh* AMesh,
                                    const m4* BModelMat, const collider_mesh* BMesh, gjk_simplex* Simplex);
