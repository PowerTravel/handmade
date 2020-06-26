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
  u32 EntityIDA;
  u32 EntityIDB;
  u32 WorldArrayIndex;
  u32 ContactCount;
  u32 MaxContactCount;
  contact_data Contacts[4];
  contact_data_cache CachedData[4];
  contact_manifold* Next;
};

struct world_contact_chunk
{
  u32 MaxCount;
  u32 Count;
  contact_manifold* Manifolds;
  world_contact_chunk* Next;
};

#if 0
void GetManifoldIndex(world_contact_chunk* Contacts, u32 A, u32 B)
{
  u32 CantorPair = GetCantorPair(a, b);
  u32 ManifoldIndex = CantorPair % World->MaxNrManifolds;
  u32 HashMapCollisions = 0;
  contact_manifold* Manifold = 0;
  entity* SrcA = ColliderPair->A;
  entity* SrcB = ColliderPair->B;
  while( HashMapCollisions < World->MaxNrManifolds )
  {
    contact_manifold* ManifoldArraySlot = World->Manifolds + ManifoldIndex;
    if(!ManifoldArraySlot->A)
    {
      Assert(!ManifoldArraySlot->B)
      // Slot was empty
      Manifold = ManifoldArraySlot;
      ZeroStruct( *Manifold );
      Manifold->A = ColliderPair->A;
      Manifold->B = ColliderPair->B;
      Manifold->MaxContactCount = 4;
      Manifold->WorldArrayIndex = ManifoldIndex;
      break;
    }
    else if(((SrcA->ID == ManifoldArraySlot->A->ID) && (SrcB->ID == ManifoldArraySlot->B->ID)) ||
            ((SrcA->ID == ManifoldArraySlot->B->ID) && (SrcB->ID == ManifoldArraySlot->A->ID)))
    {
      Manifold = ManifoldArraySlot;
      Assert(Manifold->MaxContactCount == 4);
      Assert(Manifold->WorldArrayIndex == ManifoldIndex);
      break;
    }else{
      TIMED_BLOCK(ContactArrayCollisions);
      ++HashMapCollisions;
      Assert( !((SrcA->ID == ManifoldArraySlot->A->ID) && (SrcB->ID == ManifoldArraySlot->B->ID)) &&
              !((SrcB->ID == ManifoldArraySlot->A->ID) && (SrcA->ID == ManifoldArraySlot->B->ID)) );
      ManifoldIndex = (ManifoldIndex+1) % World->MaxNrManifolds;
    }
  }
  Assert(Manifold);
  Assert(HashMapCollisions < World->MaxNrManifolds);
}
#endif

contact_data EPACollisionResolution(memory_arena* TemporaryArena, const m4* AModelMat, const collider_mesh* AMesh,
                                    const m4* BModelMat, const collider_mesh* BMesh, gjk_simplex* Simplex);
