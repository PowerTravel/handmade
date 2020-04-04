#include "component_spatial.h"
#include "component_collider.h"
#include "component_dynamics.h"
#include "gjk_epa_visualizer.h"
#include "entity_components.h"
#include "handmade_tile.h"
#include "math/vector_math.h"
#include "utility_macros.h"
#include "math/aabb.h"
#include "dynamic_aabb_tree.h"
#include "gjk_narrow_phase.h"
#include "epa_collision_data.h"

list< aabb3f > GetOverlappingWallTiles(memory_arena* Arena, tile_map* TileMap, aabb3f* BoundingBox, v3 CollisionEnvelope = {} )
{
  aabb3f EnvelopedBoundingBox = *BoundingBox;
  EnvelopedBoundingBox.P0 -= CollisionEnvelope;
  EnvelopedBoundingBox.P1 += CollisionEnvelope;

  list< tile_map_position > TilesToTest = list< tile_map_position >( Arena );
  GetIntersectingTiles(TileMap, &TilesToTest, &EnvelopedBoundingBox);

  list< aabb3f > IntersectingWallTiles = list< aabb3f >( Arena );
  for(TilesToTest.First();
     !TilesToTest.IsEnd();
      TilesToTest.Next() )
  {
    tile_map_position TilePosition = TilesToTest.Get();
    tile_contents Content = GetTileContents(TileMap, TilePosition);
    if( (Content.Type == TILE_TYPE_WALL) ||
        (Content.Type == TILE_TYPE_FLOOR) )
    {
      aabb3f WallTile = GetTileAABB( TileMap, TilePosition );
      IntersectingWallTiles.InsertAfter(WallTile);
    }
  }

  return IntersectingWallTiles;
}

v3 GravityForceEquation(v3& a,v3& b,v3& c)
{
  return V3(0,-00,0);
}

internal void GetAABBInverseMassMatrix(aabb3f* AABB, r32 Mass, m3* InverseMass, m3* InverseInertia)
{
  v3 Dim = AABB->P1 - AABB->P0;

  r32 Ix = (1/12.f) * Mass * (Dim.Y*Dim.Y + Dim.Z*Dim.Z);
  r32 Iy = (1/12.f) * Mass * (Dim.X*Dim.X + Dim.Z*Dim.Z);
  r32 Iz = (1/12.f) * Mass * (Dim.X*Dim.X + Dim.Y*Dim.Y);

/*
  m3 Ma = M3(MassA, 0, 0,
              0,MassA, 0,
              0, 0,MassA);

  m3 Mb = M3(MassB, 0, 0,
              0,MassB, 0,
              0, 0,MassB);
*/

  *InverseMass = M3(1/Mass,0,0,
                    0,1/Mass,0,
                    0,0,1/Mass);

  *InverseInertia = M3(1/Ix,0,0,
                       0,1/Iy,0,
                       0,0,1/Iz);
}

internal inline void ScaleV12( r32 Scal, v3* V, v3* Result )
{
  Result[0] = V[0]*Scal;
  Result[1] = V[1]*Scal;
  Result[2] = V[2]*Scal;
  Result[3] = V[3]*Scal;
}

internal inline void MultiplyDiagonalM12V12( m3* M, v3* V, v3* Result )
{
  Result[0] = M[0] * V[0];
  Result[1] = M[1] * V[1];
  Result[2] = M[2] * V[2];
  Result[3] = M[3] * V[3];
}

internal inline r32 DotProductV12xV12( v3* A, v3* B)
{
  r32 Result = (A[0] * B[0]) + (A[1] * B[1]) + (A[2] * B[2]) + (A[3] * B[3]);
  return Result;
}

internal inline r32
getBaumgarteCoefficient(r32 dt, r32 Scalar, r32 PenetrationDepth, r32 Slop)
{
  r32 k = (PenetrationDepth  - Slop) > 0 ? (PenetrationDepth  - Slop) : 0;
  r32 Baumgarte = - (Scalar / dt) * k;
  return Baumgarte;
}

internal inline r32
getRestitutionCoefficient(v3 V[], r32 Scalar, v3 Normal, r32 Slop)
{
  r32 ClosingSpeed = ((V[0] + V[1] + V[2] + V[3]) * Normal);
  ClosingSpeed = (ClosingSpeed - Slop) > 0 ? (ClosingSpeed - Slop) : 0;
  r32 Restitution = Scalar * ClosingSpeed;
  return Restitution;
}

internal r32 GetLambda(  v3 V[], v3 J[], v3 InvMJ[],
                               r32 BaumgarteCoefficient, r32 RestitutionCoefficient)
{
  r32 Bias = BaumgarteCoefficient + RestitutionCoefficient;
  r32 Numerator   = -(DotProductV12xV12( J, V) + Bias);
  r32 Denominator =   DotProductV12xV12( J, InvMJ);

  r32 Result = Numerator / Denominator;
  return Result;
}

v3 ClosestPointOnEdge(const v3& EdgeStart, const v3& EdgeEnd, const v3& Point)
{
  const v3 EdgeDirection = Normalize(EdgeEnd-EdgeStart);
  const r32 EdgeLength = Norm(EdgeEnd-EdgeStart);
  const r32 ProjectionScalar = (Point-EdgeStart)*EdgeDirection;

  v3 ClosestPointOnEdge = {};
  if(ProjectionScalar <= 0)
  {
    ClosestPointOnEdge = EdgeStart;
  }else if(ProjectionScalar >= EdgeLength )
  {
    ClosestPointOnEdge = EdgeEnd;
  }else{
    ClosestPointOnEdge = EdgeStart + ProjectionScalar * EdgeDirection;
  }

  return ClosestPointOnEdge;
}

struct collision_detection_work
{
  memory_arena* TransientArena;
  contact_manifold* Manifold;
};

internal void
DoCollisionDetectionWork(void* Data)
{
  collision_detection_work* Work = (collision_detection_work*) Data;
  entity* A = Work->Manifold->A;
  entity* B = Work->Manifold->B;
  contact_manifold* Manifold = Work->Manifold;
  m4 ModelMatrixA = GetModelMatrix(A->SpatialComponent);
  m4 ModelMatrixB = GetModelMatrix(B->SpatialComponent);
  collider_mesh* MeshA = A->ColliderComponent->Mesh;
  collider_mesh* MeshB = B->ColliderComponent->Mesh;
  gjk_collision_result NarrowPhaseResult = GJKCollisionDetection(
      &ModelMatrixA, MeshA,
      &ModelMatrixB, MeshB);

  if (NarrowPhaseResult.ContainsOrigin)
  {
    contact_data NewContact = EPACollisionResolution(Work->TransientArena, &ModelMatrixA, A->ColliderComponent->Mesh,
                                                                           &ModelMatrixB, B->ColliderComponent->Mesh,
                                                                           &NarrowPhaseResult.Simplex);
    // For now store only one
    Manifold->ContactCount  = 1;
    Manifold->Contacts[0]   = NewContact;
    Manifold->CashedData[0] = {};
#if 0
        b32 FarEnough = true;
        for (u32 i = 0; i < CollisionManifold->ContactCount; ++i)
        {
          const contact_data* OldContact = &CollisionManifold->Contacts[i];
          const v3 rALocal = NewContact.A_ContactModelSpace - OldContact->A_ContactModelSpace;
          const v3 rBLocal = NewContact.B_ContactModelSpace - OldContact->B_ContactModelSpace;
          const v3 rAGlobal = NewContact.A_ContactWorldSpace - OldContact->A_ContactWorldSpace;
          const v3 rBGlobal = NewContact.B_ContactWorldSpace - OldContact->B_ContactWorldSpace;
          r32 nsrla = Norm(rALocal);
          r32 nsrlb = Norm(rBLocal);
          r32 nsrga = Norm(rAGlobal);
          r32 nsrgb = Norm(rBGlobal);

          const b32 NotFarEnoughAL = Norm(rALocal)  < 0.01;
          const b32 NotFarEnoughBL = Norm(rBLocal)  < 0.01;
          const b32 NotFarEnoughAG = Norm(rAGlobal) < 0.01;
          const b32 NotFarEnoughBG = Norm(rBGlobal) < 0.01;

          if (NotFarEnoughAG || NotFarEnoughBG)
          {
            FarEnough = false;
            //CollisionManifold->Contacts[i] = NewContact;
            break;
          }
        }

        if (FarEnough)
        {
          if (CollisionManifold->ContactCount < CollisionManifold->MaxNrContacts)
          {
            CollisionManifold->Contacts[CollisionManifold->ContactCount++] = NewContact;
          }else{
            #if 1
            local_persist u32 idx = 4;
            CollisionManifold->Contacts[idx % 4] = NewContact;
            idx++;
            #else
            Assert(CollisionManifold->MaxNrContacts == 4);
            Assert(CollisionManifold->ContactCount == CollisionManifold->MaxNrContacts);

            contact_data PossibleContacts[5] = {};
            v3 ContactPoints[4] = {};
            u32 AddedContacts[4] = {};
            utils::Copy(4* sizeof(contact_data), CollisionManifold->Contacts, PossibleContacts );
            PossibleContacts[4] = NewContact;
            const m4 ModelMatA = CollisionManifold->A->SpatialComponent->ModelMatrix;
            const m4 ModelMatB = CollisionManifold->B->SpatialComponent->ModelMatrix;
            r32 Depth = 0;
            u32 nrAddedContacts = 0;
            for (u32 i = 0; i < ArrayCount(PossibleContacts); ++i)
            {
              const v3 PossiblePointA = V3( ModelMatA * V4(PossibleContacts[i].A_ContactModelSpace,1));
              const v3 PossiblePointB = V3( ModelMatB * V4(PossibleContacts[i].B_ContactModelSpace,1));
              const r32 PenetrationDepth = Norm(PossiblePointB - PossiblePointA);
              if(Depth < PenetrationDepth)
              {
                AddedContacts[0] = i;
                CollisionManifold->Contacts[0] = PossibleContacts[i];
                Depth = PenetrationDepth;
                ContactPoints[0] = PossiblePointA;
              }
            }

            r32 Length = 0;
            for (u32 i = 0; i < ArrayCount(PossibleContacts); ++i)
            {
              if(i == AddedContacts[0])
              {
                continue;
              }

              const v3 PossiblePointA = V3( ModelMatA * V4(PossibleContacts[i].A_ContactModelSpace,1));

              r32 TestLength = Norm(PossiblePointA - ContactPoints[0]);
              if(Length < TestLength)
              {
                AddedContacts[1] = i;
                ContactPoints[1] = PossiblePointA;
                CollisionManifold->Contacts[1] = PossibleContacts[i];
                Length = TestLength;
              }
            }

            Length = 0;
            for (u32 i = 0; i < ArrayCount(PossibleContacts); ++i)
            {
              if(i == AddedContacts[0] || i == AddedContacts[1])
              {
                continue;
              }
              const v3 PossiblePoint = V3( ModelMatA * V4(PossibleContacts[i].A_ContactModelSpace,1));
              const v3 ClosestPoint = ClosestPointOnEdge(ContactPoints[0], ContactPoints[1], PossiblePoint);

              const r32 TestLength = Norm(PossiblePoint - ClosestPoint);
              if(Length < TestLength)
              {
                AddedContacts[2] = i;
                ContactPoints[2] = PossiblePoint;
                CollisionManifold->Contacts[2] = PossibleContacts[i];
                Length = TestLength;
              }
            }

            Length = 0;
            b32 FinalPointAdded = false;
            for (u32 i = 0; i < ArrayCount(PossibleContacts); ++i)
            {
              if(i == AddedContacts[0] || i == AddedContacts[1] || i == AddedContacts[2])
              {
                continue;
              }

              const v3 PossiblePoint = V3( ModelMatA * V4(PossibleContacts[i].A_ContactModelSpace,1));
              const v3 a = ContactPoints[0];
              const v3 b = ContactPoints[1];
              const v3 c = ContactPoints[2];
              const v3 v = PossiblePoint;
              const v3 Normal = GetPlaneNormal(a,b,c);
              const v3 ProjectedPoint = ProjectPointOntoPlane(v, a, Normal);
              const v3 Coords = GetBaryocentricCoordinates( a, b, c, Normal, ProjectedPoint);
              const b32 InsideTriangle = (Coords.E[0] >= 0) && (Coords.E[0] <= 1) &&
                                         (Coords.E[1] >= 0) && (Coords.E[1] <= 1) &&
                                         (Coords.E[2] >= 0) && (Coords.E[2] <= 1);
              if(InsideTriangle)
              {
                continue;
              }

              r32 CumuLength = Norm(v-a) + Norm(v-b) + Norm(v-c);
              if(CumuLength > Length)
              {
                FinalPointAdded = true;
                AddedContacts[3] = i;
                CollisionManifold->Contacts[3] = PossibleContacts[i];
              }
            }

            if(!FinalPointAdded)
            {
              CollisionManifold->ContactCount = 3;
              CollisionManifold->Contacts[3] = {};
            }
            #endif
          }
        }
      }

      if(Vis)
      {
        Vis->TriggerRecord = false;
      }
    }
    ColliderPair = ColliderPair->Previous;
  }
#endif
  }
}

void FireOnceVic(memory_arena* TransientArena, entity* A, entity* B)
{
  local_persist b32 Fired = false;
  if(!Fired && GlobalFireVic )
  {
    Fired = true;
    ResetEPA(GlobalVis);
    m4 ModelMatrixA = GetModelMatrix(A->SpatialComponent);
    m4 ModelMatrixB = GetModelMatrix(B->SpatialComponent);
    gjk_collision_result NarrowPhaseResult = GJKCollisionDetection(
                                                &ModelMatrixA, A->ColliderComponent->Mesh,
                                                &ModelMatrixB, B->ColliderComponent->Mesh);
    contact_data apa = EPACollisionResolution(TransientArena, &ModelMatrixA, A->ColliderComponent->Mesh,
                                                              &ModelMatrixB, B->ColliderComponent->Mesh,
                                                              &NarrowPhaseResult.Simplex);
  }
}
void SpatialSystemUpdate( world* World,/* platform_work_queue* CollisionQueue,*/ platform_api* API)
{
  TIMED_FUNCTION();
  r32 dt =  World->dtForFrame;
  memory_arena* TransientArena = World->TransientArena;

  temporary_memory TempMem1 = BeginTemporaryMemory( TransientArena );

#if 1
  World->FirstContactManifold = 0;
  for(u32 ManifoldIndex = 0;  ManifoldIndex < World->MaxNrManifolds; ++ManifoldIndex )
  {
    contact_manifold* Manifold = World->Manifolds + ManifoldIndex;
    Manifold->ContactCount=0;
    Manifold->A=0;
    Manifold->B=0;
  }
#else
  // Remove invalid contacts
  u32 ManifoldsCounted = 0;
  contact_manifold* Manifold = World->FirstContactManifold;
  while( Manifold )
  {
    m4 ModelMatA = GetModelMatrix(Manifolds->A->SpatialComponent);
    m4 ModelMatB = GetModelMatrix(Manifolds->B->SpatialComponent);

    b32 PersistentContacts = false;
    for (u32 j = 0; j <CollisionManifold->MaxNrContacts; ++j)
    {
      contact_data* Contact = Manifolds->Contacts + j;
      if(!Contact->Valid)
      {
        continue;
      }

      const v3 LocalToGlobalA = V3( ModelMatA * V4(Contact->A_ContactModelSpace,1));
      const v3 LocalToGlobalB = V3( ModelMatB * V4(Contact->B_ContactModelSpace,1));

      const v3 rAB = LocalToGlobalB - LocalToGlobalA;

      const v3 rA  = Contact->A_ContactWorldSpace - LocalToGlobalA;
      const v3 rB  = Contact->B_ContactWorldSpace - LocalToGlobalB;

      const r32 normDot = Contact->ContactNormal * rAB;
      const b32 stillPenetrating = normDot <= 0.0f;

      const r32 persistentThresholdSq = 0.1;
      const r32 lenDiffA = NormSq(rA);
      const r32 lenDiffB = NormSq(rB);

      // keep contact point if the collision pair is still colliding at this point
      // and the local positions are not too far from the global
      // positions original acquired from collision detection
      if (stillPenetrating && ((lenDiffA < persistentThresholdSq) && (lenDiffB < persistentThresholdSq)) )
      {
        Contact->Persistent=true;
      }else{
        ZeroStruct(*Contact);
      }
    }

    if(!PersistentContacts)
    {
      Manifolds->Valid = false;
    }
  }
#endif
  aabb_tree BroadPhaseTree = {};
  for(u32 Index = 0;  Index < World->NrEntities; ++Index )
  {
    entity* E = &World->Entities[Index];
    if( E->DynamicsComponent )
    {
      component_spatial*   S = E->SpatialComponent;
      component_dynamics*  D = E->DynamicsComponent;
      v3  Position           = S->Position;
      v3  LinearVelocity     = D->LinearVelocity;
      v3  AngularVelocity    = D->AngularVelocity;
      r32 Mass               = D->Mass;

      // Forward euler
      // TODO: Investigate other more stable integration methods
      v3 Gravity = V3(0,-10,0);
      v3 LinearAcceleration = Gravity * Mass;
      D->LinearVelocity     += dt * LinearAcceleration;

      // TODO: Can angular acceleration be integrated naively like this?
      //       Don't think so, use rotor-integration, See paper of Michael Boyle
      v3 AngularAcceleration = {};
      D->AngularVelocity += dt * AngularAcceleration;
    }

    if( E->ColliderComponent )
    {
      aabb3f AABBWorldSpace = {};
      GetTransformedAABBFromColliderMesh( E->ColliderComponent, GetModelMatrix(E->SpatialComponent), &AABBWorldSpace );
      // TODO: Don't do a insert every timestep. Update an existing tree
      AABBTreeInsert( TransientArena, &BroadPhaseTree, E, AABBWorldSpace );
    }
  }

  broad_phase_result_stack* const BroadPhaseResult = GetCollisionPairs( TransientArena, &BroadPhaseTree );
  broad_phase_result_stack* ColliderPair = BroadPhaseResult;

  // Todo: Get this number straigth from broad_phase_result_stack
  u32 CollisionWorkCount = 0;
  while( ColliderPair )
  {
    ++CollisionWorkCount;
    ColliderPair = ColliderPair->Previous;
  }

  ColliderPair = BroadPhaseResult;
  collision_detection_work* WorkArray = PushArray(TransientArena, CollisionWorkCount, collision_detection_work);
  collision_detection_work* WorkSlot = WorkArray;
  Assert(World->FirstContactManifold==0);
  u32 Idx = 0;
  while( ColliderPair )
  {
    // Find a manifold memory slot for manifold made up of A and B;
    r32 a = (r32) ColliderPair->A->id;
    r32 b = (r32) ColliderPair->B->id;

    r32 CantorPairR32 =(1/2.f) * (a + b)*(a + b + 1) + b;
    u32 CantorPairA = (u32)CantorPairR32;
    u32 CantorPair = GetCantorPair(ColliderPair->A->id,  ColliderPair->B->id);
    Assert(CantorPair == CantorPairA);
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
      else if(((SrcA->id == ManifoldArraySlot->A->id) && (SrcB->id == ManifoldArraySlot->B->id)) ||
              ((SrcA->id == ManifoldArraySlot->B->id) && (SrcB->id == ManifoldArraySlot->A->id)))
      {
        Manifold = ManifoldArraySlot;
        Assert(Manifold->Next != World->FirstContactManifold);
        Assert(Manifold->MaxContactCount == 4);
        Assert(Manifold->WorldArrayIndex == ManifoldIndex);
        Assert(Manifold->ContactCount == 0);
        Assert(0);
        break;
      }else{
        TIMED_BLOCK("ContactArrayCollisions");
        ++HashMapCollisions;
        Assert( !((SrcA->id == ManifoldArraySlot->A->id) && (SrcB->id == ManifoldArraySlot->B->id)) &&
                !((SrcB->id == ManifoldArraySlot->A->id) && (SrcA->id == ManifoldArraySlot->B->id)) );
        ManifoldIndex = (ManifoldIndex+1) % World->MaxNrManifolds;
      }
    }
    Assert(Manifold);
    Assert(HashMapCollisions < World->MaxNrManifolds);

    // Push the contact to the list.
    Manifold->Next = World->FirstContactManifold;
    World->FirstContactManifold = Manifold;
    Assert( Manifold->Next != World->FirstContactManifold);
    Assert(Manifold != Manifold->Next);

    // Add it to the work array
    WorkSlot->TransientArena = TransientArena;
    WorkSlot->Manifold =  Manifold;
//  CollisionQueue->AddEntry(CollisionQueue, DoCollisionDetectionWork, Work);

    ColliderPair = ColliderPair->Previous;
    ++WorkSlot;
  }

//  CollisionQueue->PlatformCompleteWorkQueue(CollisionQueue);
  u32 CollisionWorkIndex = 0;
  while(CollisionWorkIndex < CollisionWorkCount)
  {
    DoCollisionDetectionWork((void*) (WorkArray + CollisionWorkIndex++));
  }

  contact_manifold** ManifoldPtr = &World->FirstContactManifold;
  while(*ManifoldPtr)
  {
    contact_manifold* Manifold = *ManifoldPtr;
    if(Manifold->ContactCount == 0)
    {
      // Remove manifolds that are not colliding
      *ManifoldPtr = Manifold->Next;
    }else{
      Assert(Manifold->ContactCount>0);
      entity* A = Manifold->A;
      entity* B = Manifold->B;

      v3 va = {};
      v3 wa = {};
      v3 vb = {};
      v3 wb = {};
      r32 ma = 10e37;
      r32 mb = 10e37; // Solve stationary objects with stationary constraint?
      if (A->DynamicsComponent)
      {
        va = A->DynamicsComponent->LinearVelocity;
        wa = A->DynamicsComponent->AngularVelocity;
        ma = A->DynamicsComponent->Mass;
      }

      if (B->DynamicsComponent)
      {
        vb = B->DynamicsComponent->LinearVelocity;
        wb = B->DynamicsComponent->AngularVelocity;
        mb = B->DynamicsComponent->Mass;
      }

      m3 InvM[4] = {};
      GetAABBInverseMassMatrix( &A->ColliderComponent->AABB, ma, &InvM[0], &InvM[1]);
      GetAABBInverseMassMatrix( &B->ColliderComponent->AABB, mb, &InvM[2], &InvM[3]);

      for(u32 j = 0; j < Manifold->ContactCount; ++j)
      {
        contact_data* Contact = &Manifold->Contacts[j];
        contact_data_cache* CashedData = &Manifold->CashedData[j];
        const v3& ra = Contact->A_ContactModelSpace;
        const v3& rb = Contact->B_ContactModelSpace;
        const v3& n  = Contact->ContactNormal;

        CashedData->J[0] = -n;
        CashedData->J[1] = -CrossProduct(ra, n);
        CashedData->J[2] = n;
        CashedData->J[3] = CrossProduct(rb, n);

        MultiplyDiagonalM12V12(InvM, CashedData->J, CashedData->InvMJ);
      }

      Assert(Manifold != Manifold->Next);
      ManifoldPtr = &(Manifold->Next);
    }
  }

  if(World->FirstContactManifold)
  {
    for (u32 i = 0; i < 10; ++i)
    {
      // TODO: Process Constraints MultiThreaded?
      contact_manifold* Manifold = World->FirstContactManifold;
      while(Manifold)
      {
        Assert(Manifold->ContactCount>0);
        entity* A = Manifold->A;
        entity* B = Manifold->B;
        for(u32 k = 0; k < Manifold->ContactCount; ++k)
        {
          v3 V[4] = {};
          if(A->DynamicsComponent)
          {
            V[0] = A->DynamicsComponent->LinearVelocity;
            V[1] = A->DynamicsComponent->AngularVelocity;
          }
          if(B->DynamicsComponent)
          {
            V[2] = B->DynamicsComponent->LinearVelocity;
            V[3] = B->DynamicsComponent->AngularVelocity;
          }
          contact_data* Contact = &Manifold->Contacts[k];
          contact_data_cache* CashedData = &Manifold->CashedData[k];
          //r32 PenetrationDepth  = Norm(V3(GetModelMatrix(A->SpatialComponent) * V4( Contact->A_ContactModelSpace,1)) -
          //                             V3(GetModelMatrix(B->SpatialComponent) * V4( Contact->B_ContactModelSpace,1)));
          r32 PenetrationDepth  = Norm(V3(GetModelMatrix(A->SpatialComponent) * V4(Contact->A_ContactModelSpace,1)) -
                                       V3(GetModelMatrix(B->SpatialComponent) * V4(Contact->B_ContactModelSpace,1)));
          v3  ContactNormal     = Contact->ContactNormal;
          r32 Restitution       = getRestitutionCoefficient(V, 0.1f, ContactNormal, 0.01);
          r32 Baumgarte         = getBaumgarteCoefficient(dt, 0.25,  PenetrationDepth, 0.01);
          r32 Lambda            = GetLambda( V, CashedData->J, CashedData->InvMJ, Baumgarte, Restitution);
          r32 OldCumulativeLambda = CashedData->AccumulatedLambda;
          CashedData->AccumulatedLambda += Lambda;
          CashedData->AccumulatedLambda = CashedData->AccumulatedLambda <=0 ? 0 : CashedData->AccumulatedLambda;
          r32 LambdaDiff = CashedData->AccumulatedLambda - OldCumulativeLambda;

          v3 DeltaV[4] = {};
          ScaleV12(LambdaDiff, CashedData->InvMJ, DeltaV);

          if(A->DynamicsComponent)
          {
            A->DynamicsComponent->LinearVelocity  += DeltaV[0];
            A->DynamicsComponent->AngularVelocity += DeltaV[1];
          }
          if(B->DynamicsComponent)
          {
            B->DynamicsComponent->LinearVelocity  += DeltaV[2];
            B->DynamicsComponent->AngularVelocity += DeltaV[3];
          }
        }
        Manifold = Manifold->Next;
      }
    }
  }
#if 1
  for(u32 Index = 0;  Index < World->NrEntities; ++Index )
  {
    entity* E = &World->Entities[Index];
    if( E->Types & COMPONENT_TYPE_DYNAMICS )
    {
      component_spatial*   S = E->SpatialComponent;
      component_dynamics*  D = E->DynamicsComponent;
      TimestepVelocity( dt, D->LinearVelocity, D->AngularVelocity, S );
      S->Rotation = Normalize(S->Rotation);
    }
  }
#endif
  EndTemporaryMemory( TempMem1 );
  CheckArena(TransientArena);
}