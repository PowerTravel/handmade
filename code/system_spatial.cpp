#include "entity_components.h"
#include "handmade_tile.h"
#include "utility_macros.h"
#include "math/aabb.h"
#include "dynamic_aabb_tree.h"
#include "gjk_narrow_phase.h"
#include "epa_collision_data.h"

#define NEW_CONTACT_THRESHOLD 0.15f
#define PERSISTENT_CONTACT_THRESHOLD 0.15f
#define WARM_STARTING_FRACTION 0.31f

#define FRICTIONAL_COEFFICIENT 0.15f
#define BAUMGARTE_COEFFICIENT  0.25f
#define RESTITUTION_COEFFICIENT 0.0f
#define SLOP 0.012f

#define SLOVER_ITERATIONS 24

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

internal PLATFORM_WORK_QUEUE_CALLBACK(DoCollisionDetectionWork)
{
  TIMED_FUNCTION();
  entity_manager* EM = GlobalGameState->EntityManager;
  contact_manifold* Manifold = (contact_manifold*) Data;

  component_spatial* SpatialA   = (component_spatial*)  GetComponent(EM, Manifold->EntityIDA, COMPONENT_FLAG_SPATIAL);
  component_spatial* SpatialB   = (component_spatial*)  GetComponent(EM, Manifold->EntityIDB, COMPONENT_FLAG_SPATIAL);
  component_collider* ColliderA = (component_collider*) GetComponent(EM, Manifold->EntityIDA, COMPONENT_FLAG_COLLIDER);
  component_collider* ColliderB = (component_collider*) GetComponent(EM, Manifold->EntityIDB, COMPONENT_FLAG_COLLIDER);
  component_dynamics* DynamicsA = (component_dynamics*) GetComponent(EM, Manifold->EntityIDA, COMPONENT_FLAG_DYNAMICS);
  component_dynamics* DynamicsB = (component_dynamics*) GetComponent(EM, Manifold->EntityIDB, COMPONENT_FLAG_DYNAMICS);
  m4 ModelMatrixA = GetModelMatrix(SpatialA);
  m4 ModelMatrixB = GetModelMatrix(SpatialB);
  collider_mesh MeshA = GetColliderMesh(GlobalGameState->AssetManager, ColliderA->Object);
  collider_mesh MeshB = GetColliderMesh(GlobalGameState->AssetManager, ColliderB->Object);

  gjk_collision_result NarrowPhaseResult = GJKCollisionDetection(
      &ModelMatrixA, &MeshA,
      &ModelMatrixB, &MeshB);

  // Todo: Should we give each thread it's own Transient Arena?
  // Note: GlobalGamestate->TransientArena is NOT thread safe, don't use it in any PLATFORM_WORK_QUEUE_CALLBACK.
  memory_arena Arena = {};
  temporary_memory TempMem = BeginTemporaryMemory(&Arena);

  if (NarrowPhaseResult.ContainsOrigin)
  {
    contact_data NewContact = EPACollisionResolution(&Arena,
                                                     &ModelMatrixA, &MeshA,
                                                     &ModelMatrixB, &MeshB,
                                                     &NarrowPhaseResult.Simplex);

    b32 FarEnough = true;
    for (u32 ContactIndex = 0; ContactIndex < Manifold->ContactCount; ++ContactIndex)
    {
      const contact_data* OldContact = Manifold->Contacts + ContactIndex;
      const contact_data_cache* OldContactCache = Manifold->CachedData + ContactIndex;
      const v3 rALocal = NewContact.A_ContactModelSpace - OldContact->A_ContactModelSpace;
      const v3 rBLocal = NewContact.B_ContactModelSpace - OldContact->B_ContactModelSpace;
      const v3 rAGlobal = NewContact.A_ContactWorldSpace - OldContact->A_ContactWorldSpace;
      const v3 rBGlobal = NewContact.B_ContactWorldSpace - OldContact->B_ContactWorldSpace;
      r32 nsrla = Norm(rALocal);
      r32 nsrlb = Norm(rBLocal);
      r32 nsrga = Norm(rAGlobal);
      r32 nsrgb = Norm(rBGlobal);

      const b32 NotFarEnoughAL = Norm(rALocal)  < NEW_CONTACT_THRESHOLD;
      const b32 NotFarEnoughBL = Norm(rBLocal)  < NEW_CONTACT_THRESHOLD;
      const b32 NotFarEnoughAG = Norm(rAGlobal) < NEW_CONTACT_THRESHOLD;
      const b32 NotFarEnoughBG = Norm(rBGlobal) < NEW_CONTACT_THRESHOLD;

      if (NotFarEnoughAG || NotFarEnoughBG)
      {
        FarEnough = false;
        break;
      }
    }

    if (FarEnough)
    {
      contact_data_cache CachedData = {};
      const v3& ra = NewContact.A_ContactModelSpace;
      const v3& rb = NewContact.B_ContactModelSpace;
      const v3& n   = NewContact.ContactNormal;
      const v3& n1  = NewContact.TangentNormalOne;
      const v3& n2  = NewContact.TangentNormalTwo;

      // Todo: Solve stationary objects with stationary constraint?
      r32 ma = R32Max;
      r32 mb = R32Max;
      if (DynamicsA)
      {
        ma = DynamicsA->Mass;
      }

      if (DynamicsB)
      {
        mb = DynamicsB->Mass;
      }

      m3 InvM[4] = {};
      GetAABBInverseMassMatrix( &ColliderA->AABB, ma, &InvM[0], &InvM[1]);
      GetAABBInverseMassMatrix( &ColliderB->AABB, mb, &InvM[2], &InvM[3]);

      CachedData.J[0] = -n;
      CachedData.J[1] = -CrossProduct(ra, n);
      CachedData.J[2] = n;
      CachedData.J[3] = CrossProduct(rb, n);
      MultiplyDiagonalM12V12(InvM, CachedData.J, CachedData.InvMJ);

      CachedData.Jn1[0] = -n1;
      CachedData.Jn1[1] = -CrossProduct(ra, n1);
      CachedData.Jn1[2] = n1;
      CachedData.Jn1[3] = CrossProduct(rb, n1);
      MultiplyDiagonalM12V12(InvM, CachedData.Jn1, CachedData.InvMJn1);

      CachedData.Jn2[0] = -n2;
      CachedData.Jn2[1] = -CrossProduct(ra, n2);
      CachedData.Jn2[2] = n2;
      CachedData.Jn2[3] = CrossProduct(rb, n2);
      MultiplyDiagonalM12V12(InvM, CachedData.Jn2, CachedData.InvMJn2);

      if (Manifold->ContactCount < Manifold->MaxContactCount)
      {
        Manifold->Contacts[Manifold->ContactCount] = NewContact;
        Manifold->CachedData[Manifold->ContactCount] = CachedData;
        Manifold->ContactCount++;
      }else{
        #if 0
        local_persist u32 idx = 0;
        Manifold->Contacts[idx % Manifold->MaxContactCount] = NewContact;
        Manifold->CachedData[idx++ % Manifold->MaxContactCount] = CachedData;
        #else
        Assert(Manifold->MaxContactCount == 4);
        Assert(Manifold->ContactCount == Manifold->MaxContactCount);

        contact_data PossibleContacts[5] = {};
        contact_data_cache PossibleContactCaches[5] = {};
        v3  ContactPoints[4] = {};
        u32 AddedContacts[4] = {};
        utils::Copy(sizeof(PossibleContacts), Manifold->Contacts, PossibleContacts);
        utils::Copy(sizeof(PossibleContactCaches), Manifold->CachedData, PossibleContactCaches);
        PossibleContacts[4] = NewContact;
        PossibleContactCaches[4] = CachedData;

        r32 Depth = 0;
        u32 nrAddedContacts = 0;
        for (u32 i = 0; i < ArrayCount(PossibleContacts); ++i)
        {
          const v3 PossiblePointA = V3( ModelMatrixA * V4(PossibleContacts[i].A_ContactModelSpace,1));
          const v3 PossiblePointB = V3( ModelMatrixB * V4(PossibleContacts[i].B_ContactModelSpace,1));
          const r32 PenetrationDepth = (PossiblePointA - PossiblePointB)*PossibleContacts[i].ContactNormal;
          if(Depth < PenetrationDepth)
          {
            AddedContacts[0] = i;
            Manifold->Contacts[0] = PossibleContacts[i];
            Manifold->CachedData[0] = PossibleContactCaches[i];
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

          const v3 PossiblePointA = V3( ModelMatrixA * V4(PossibleContacts[i].A_ContactModelSpace,1));

          r32 TestLength = Norm(PossiblePointA - ContactPoints[0]);
          if(Length < TestLength)
          {
            AddedContacts[1] = i;
            ContactPoints[1] = PossiblePointA;
            Manifold->Contacts[1] = PossibleContacts[i];
            Manifold->CachedData[1] = PossibleContactCaches[i];
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
          const v3 PossiblePoint = V3( ModelMatrixA * V4(PossibleContacts[i].A_ContactModelSpace,1));
          const v3 ClosestPoint = ClosestPointOnEdge(ContactPoints[0], ContactPoints[1], PossiblePoint);

          const r32 TestLength = Norm(PossiblePoint - ClosestPoint);
          if(Length < TestLength)
          {
            AddedContacts[2] = i;
            ContactPoints[2] = PossiblePoint;
            Manifold->Contacts[2] = PossibleContacts[i];
            Manifold->CachedData[2] = PossibleContactCaches[i];
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

          const v3 PossiblePoint = V3( ModelMatrixA * V4(PossibleContacts[i].A_ContactModelSpace,1));
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
            Manifold->Contacts[3] = PossibleContacts[i];
            Manifold->CachedData[3] = PossibleContactCaches[i];
          }
        }

        if(!FinalPointAdded)
        {
          Manifold->ContactCount = 3;
          Manifold->Contacts[3] = {};
          Manifold->CachedData[3] = {};
        }
        #endif
      }
    }
  }

  EndTemporaryMemory(TempMem);
}

internal void RemoveInvalidContactPoints( contact_manifold* FirstManifold )
{
  TIMED_FUNCTION();
  contact_manifold* Manifold = FirstManifold;
  entity_manager* EM = GlobalGameState->EntityManager;
  while (Manifold)
  {
    component_spatial* SpatialA = (component_spatial*) GetComponent(EM, Manifold->EntityIDA, COMPONENT_FLAG_SPATIAL);
    component_spatial* SpatialB = (component_spatial*) GetComponent(EM, Manifold->EntityIDB, COMPONENT_FLAG_SPATIAL);
    m4 ModelMatrixA = GetModelMatrix(SpatialA);
    m4 ModelMatrixB = GetModelMatrix(SpatialB);

    u32 DstIndex = 0;
    for (u32 SrcIndex = 0;
         SrcIndex < Manifold->ContactCount;
         ++SrcIndex)
    {
      contact_data* SrcContact = Manifold->Contacts + SrcIndex;
      contact_data_cache* SrcCache = Manifold->CachedData + SrcIndex;

      const v3 LocalToGlobalA = V3( ModelMatrixA * V4(SrcContact->A_ContactModelSpace,1));
      const v3 LocalToGlobalB = V3( ModelMatrixB * V4(SrcContact->B_ContactModelSpace,1));

      const v3 rAB = LocalToGlobalB - LocalToGlobalA;

      const v3 rA  = SrcContact->A_ContactWorldSpace - LocalToGlobalA;
      const v3 rB  = SrcContact->B_ContactWorldSpace - LocalToGlobalB;

      const r32 normDot = SrcContact->ContactNormal * rAB;
      const b32 stillPenetrating = normDot <= 0.0f;

      const r32 lenDiffA = NormSq(rA);
      const r32 lenDiffB = NormSq(rB);

      // keep contact point if the collision pair is still colliding at this point
      // and the local positions are not too far from the global
      // positions original acquired from collision detection
      if (stillPenetrating && ((lenDiffA < PERSISTENT_CONTACT_THRESHOLD) && (lenDiffB < PERSISTENT_CONTACT_THRESHOLD)) )
      {
        Assert(SrcIndex>=0);
        Assert(SrcIndex>=DstIndex);

        SrcContact->Persistent = true;
        if(SrcIndex!=DstIndex)
        {
          contact_data* DstContact = Manifold->Contacts + DstIndex;
          contact_data_cache* DstCache = Manifold->CachedData + DstIndex;
          *DstContact = Manifold->Contacts[SrcIndex];
          *DstCache = Manifold->CachedData[SrcIndex];
        }
        ++DstIndex;
      }else{
        SrcContact->Persistent = false;
      }
    }
    Assert(DstIndex <= Manifold->ContactCount);
    Manifold->ContactCount = DstIndex;
    Manifold = Manifold->Next;
  }
}

internal void DoWarmStarting( contact_manifold* FirstManifold  )
{
  TIMED_FUNCTION();
  contact_manifold* Manifold = FirstManifold;
  while (Manifold)
  {
   // Warm starting
    for(u32 k = 0; k < Manifold->ContactCount; ++k)
    {
      contact_data* Contact = &Manifold->Contacts[k];
      contact_data_cache* CachedData = &Manifold->CachedData[k];
      r32 WarmStartingFraction = WARM_STARTING_FRACTION;
      if(!Contact->Persistent) continue;

      v3 DeltaV[4] = {};
      v3 DeltaV1[4] = {};
      v3 DeltaV2[4] = {};

      ScaleV12(WarmStartingFraction * CachedData->AccumulatedLambda,   CachedData->InvMJ, DeltaV);
      ScaleV12(WarmStartingFraction * CachedData->AccumulatedLambdaN1, CachedData->InvMJn1, DeltaV);
      ScaleV12(WarmStartingFraction * CachedData->AccumulatedLambdaN2, CachedData->InvMJn2, DeltaV);
      CachedData->AccumulatedLambda = 0;
      CachedData->AccumulatedLambdaN1 = 0;
      CachedData->AccumulatedLambdaN2 = 0;

      component_dynamics* DynamicsA = (component_dynamics*) GetComponent(GlobalGameState->EntityManager, Manifold->EntityIDA, COMPONENT_FLAG_DYNAMICS);
      component_dynamics* DynamicsB = (component_dynamics*) GetComponent(GlobalGameState->EntityManager, Manifold->EntityIDB, COMPONENT_FLAG_DYNAMICS);
      if(DynamicsA)
      {
        DynamicsA->LinearVelocity  += (DeltaV[0] + DeltaV1[0] + DeltaV[0]);
        DynamicsA->AngularVelocity += (DeltaV[1] + DeltaV1[1] + DeltaV[1]);
      }
      if(DynamicsB)
      {
        DynamicsB->LinearVelocity  += (DeltaV[2] + DeltaV1[2] + DeltaV[2]);
        DynamicsB->AngularVelocity += (DeltaV[3] + DeltaV1[3] + DeltaV[3]);
      }
    }
    Manifold = Manifold->Next;
  }
}

inline void IntegrateVelocities( r32 dt )
{
  TIMED_FUNCTION();

  ScopedTransaction(GlobalGameState->EntityManager);

  component_result* ComponentList = GetComponentsOfType(GlobalGameState->EntityManager, COMPONENT_FLAG_DYNAMICS);
  while(Next(GlobalGameState->EntityManager, ComponentList))
  {
    component_spatial*   S = (component_spatial*) GetComponent(GlobalGameState->EntityManager,  ComponentList, COMPONENT_FLAG_SPATIAL);
    component_dynamics*  D = (component_dynamics*) GetComponent(GlobalGameState->EntityManager, ComponentList, COMPONENT_FLAG_DYNAMICS);

    // Forward euler
    // TODO: Investigate other more stable integration methods
    v3 Gravity = V3(0,-10,0);
    v3 LinearAcceleration = Gravity * D->Mass;
    D->LinearVelocity     += dt * LinearAcceleration;

    v3 AngularAcceleration = {};
    D->AngularVelocity += dt * AngularAcceleration;
  }
}

internal void
CreateAndDoWork( world_contact_chunk* ContactChunk, u32 BroadPhaseResultCount, broad_phase_result_stack* const BroadPhaseResultStack )
{
  TIMED_FUNCTION();
  broad_phase_result_stack* ColliderPair = BroadPhaseResultStack;
  contact_manifold** WorkArray = (contact_manifold**) PushArray(GlobalGameState->TransientArena, BroadPhaseResultCount, contact_manifold* );
  contact_manifold** WorkSlot = WorkArray;
  ContactChunk->FirstManifold = 0;
  while( ColliderPair )
  {
    contact_manifold* Manifold = FindManifoldSlot(ContactChunk, ColliderPair->EntityIDA, ColliderPair->EntityIDB);
    Assert(Manifold);

    // Push the contact to the list.
    Manifold->Next = ContactChunk->FirstManifold;
    ContactChunk->FirstManifold = Manifold;

    // Add it to the work array
    *WorkSlot = Manifold;
#if MULTI_THREADED
    Platform.PlatformAddEntry(Platform.HighPriorityQueue, DoCollisionDetectionWork, (void*) *WorkSlot);
#endif
    ColliderPair = ColliderPair->Previous;
    ++WorkSlot;
  }

#if MULTI_THREADED
  Platform.PlatformCompleteWorkQueue(Platform.HighPriorityQueue);
#else
  u32 CollisionWorkIndex = 0;
  while(CollisionWorkIndex < BroadPhaseResultCount)
  {
    DoCollisionDetectionWork(Platform.HighPriorityQueue, (void*) *(WorkArray + CollisionWorkIndex++));
  }
#endif
}

internal void RemoveNonIntersectingManifolds(world_contact_chunk* ContactChunk)
{
  TIMED_FUNCTION();
  contact_manifold** ManifoldPtr = &ContactChunk->FirstManifold;
  while(*ManifoldPtr)
  {
    contact_manifold* Manifold = *ManifoldPtr;
    if(Manifold->ContactCount == 0)
    {
      // Remove manifolds that are not colliding
      *ManifoldPtr = Manifold->Next;
    }else{
      //DoWarmStarting( World );
      Assert(Manifold != Manifold->Next);
      ManifoldPtr = &(Manifold->Next);
    }
  }
}

internal void
SolveNonPenetrationConstraints(r32 dtForFrame, contact_manifold* FirstManifold)
{
  TIMED_FUNCTION();
  contact_manifold* Manifold = FirstManifold;
  while(Manifold)
  {
    Assert(Manifold->ContactCount>0);
    for(u32 k = 0; k < Manifold->ContactCount; ++k)
    {
      v3 V[4] = {};
      component_spatial* SpatialA = GetSpatialComponent(Manifold->EntityIDA);
      component_spatial* SpatialB = GetSpatialComponent(Manifold->EntityIDB);
      component_dynamics* DynamicsA = GetDynamicsComponent(Manifold->EntityIDA);
      component_dynamics* DynamicsB = GetDynamicsComponent(Manifold->EntityIDB);
      if(DynamicsA)
      {
        V[0] = DynamicsA->LinearVelocity;
        V[1] = DynamicsA->AngularVelocity;
      }
      if(DynamicsB)
      {
        V[2] = DynamicsB->LinearVelocity;
        V[3] = DynamicsB->AngularVelocity;
      }

      contact_data* Contact = &Manifold->Contacts[k];
      contact_data_cache* CachedData = &Manifold->CachedData[k];

      v3 ContactNormal     = Contact->ContactNormal;
      v3 ContactPointDiff  = V3(GetModelMatrix(SpatialA) * V4(Contact->A_ContactModelSpace,1)) -
                             V3(GetModelMatrix(SpatialB) * V4(Contact->B_ContactModelSpace,1));
      r32 PenetrationDepth = ContactPointDiff * ContactNormal;

      r32 Restitution       = getRestitutionCoefficient(V, RESTITUTION_COEFFICIENT, ContactNormal, SLOP);
      r32 Baumgarte         = getBaumgarteCoefficient(dtForFrame, BAUMGARTE_COEFFICIENT,  PenetrationDepth, SLOP);
      r32 Lambda            = GetLambda( V, CachedData->J, CachedData->InvMJ, Baumgarte, Restitution);
      r32 OldCumulativeLambda = CachedData->AccumulatedLambda;
      CachedData->AccumulatedLambda += Lambda;
      CachedData->AccumulatedLambda = Maximum(0, CachedData->AccumulatedLambda);
      r32 LambdaDiff = CachedData->AccumulatedLambda - OldCumulativeLambda;

      v3 DeltaV[4] = {};
      ScaleV12(LambdaDiff, CachedData->InvMJ, DeltaV);

      if(DynamicsA)
      {
        DynamicsA->LinearVelocity  += DeltaV[0];
        DynamicsA->AngularVelocity += DeltaV[1];
      }
      if(DynamicsB)
      {
        DynamicsB->LinearVelocity  += DeltaV[2];
        DynamicsB->AngularVelocity += DeltaV[3];
      }
    }
    Manifold = Manifold->Next;
  }
}

internal void
SolveFrictionalConstraints( contact_manifold* FirstManifold )
{
  TIMED_FUNCTION();
  contact_manifold* Manifold = FirstManifold;
  while(Manifold)
  {
    Assert(Manifold->ContactCount>0);
    component_dynamics* DynamicsA = GetDynamicsComponent(Manifold->EntityIDA);
    component_dynamics* DynamicsB = GetDynamicsComponent(Manifold->EntityIDB);
    for(u32 k = 0; k < Manifold->ContactCount; ++k)
    {
      v3 V[4] = {};
      if(DynamicsA)
      {
        V[0] = DynamicsA->LinearVelocity;
        V[1] = DynamicsA->AngularVelocity;
      }
      if(DynamicsB)
      {
        V[2] = DynamicsB->LinearVelocity;
        V[3] = DynamicsB->AngularVelocity;
      }

      contact_data* Contact = &Manifold->Contacts[k];
      contact_data_cache* CachedData = &Manifold->CachedData[k];

      r32 Kf = FRICTIONAL_COEFFICIENT;
      r32 ClampRange = Kf * CachedData->AccumulatedLambda;

      r32 LambdaN1 = GetLambda( V, CachedData->Jn1, CachedData->InvMJn1, 0, 0);
      r32 LambdaN2 = GetLambda( V, CachedData->Jn2, CachedData->InvMJn2, 0, 0);

      r32 OldCumulativeLambdaN1 = CachedData->AccumulatedLambdaN1;
      r32 OldCumulativeLambdaN2 = CachedData->AccumulatedLambdaN2;

      CachedData->AccumulatedLambdaN1 = Clamp(CachedData->AccumulatedLambdaN1 + LambdaN1, -ClampRange, ClampRange);
      CachedData->AccumulatedLambdaN2 = Clamp(CachedData->AccumulatedLambdaN2 + LambdaN2, -ClampRange, ClampRange);

      r32 LambdaDiffN1 = CachedData->AccumulatedLambdaN1 - OldCumulativeLambdaN1;
      r32 LambdaDiffN2 = CachedData->AccumulatedLambdaN2 - OldCumulativeLambdaN2;
      v3 DeltaV1[4] = {};
      v3 DeltaV2[4] = {};

      ScaleV12(LambdaDiffN1, CachedData->InvMJn1, DeltaV1);
      ScaleV12(LambdaDiffN2, CachedData->InvMJn2, DeltaV2);

      if(DynamicsA)
      {
        DynamicsA->LinearVelocity  += (DeltaV1[0] + DeltaV2[0]);
        DynamicsA->AngularVelocity += (DeltaV1[1] + DeltaV2[1]);
      }
      if(DynamicsB)
      {
        DynamicsB->LinearVelocity  += (DeltaV1[2] + DeltaV2[2]);
        DynamicsB->AngularVelocity += (DeltaV1[3] + DeltaV2[3]);
      }

    }
    Manifold =Manifold->Next;
  }
}

//inline interal void
//AngularVelocityFunction(v4& q0, v4 dq);
//{
//  const v4 q0 = Rotation;
//  const v4 r = V4(AngularVelocity.X, AngularVelocity.Y, AngularVelocity.Z,0);
//  const v4 q1 = 0.5f*QuaternionMultiplication(r, q0);
//}



inline void
TimestepVelocityRungeKutta4(const r32 DeltaTime, const v3 LinearVelocity, const v3 AngularVelocity, component_spatial* c )
{
  Assert(c);
  #if 1
  // Note, This is wrong, why is it working?
  {
    auto dfdt_Lin = []( const v3& LinearVelocity )
    {
      return LinearVelocity;
    };
    v3 k1 = dfdt_Lin(LinearVelocity);
    v3 k2 = dfdt_Lin(LinearVelocity + 0.5f*DeltaTime*k1);
    v3 k3 = dfdt_Lin(LinearVelocity + 0.5f*DeltaTime*k2);
    v3 k4 = dfdt_Lin(LinearVelocity + DeltaTime*k4);
    c->Position += (1/6.f) * DeltaTime * (k1 + 2*k2 + 2*k3 + k4);
  }
  #else
  {
    auto dfdt_Lin = [LinearVelocity]( v3 p )
    {
      return LinearVelocity;
    };
    v3 k1 = dfdt_Lin(c->Position);
    v3 k2 = dfdt_Lin(c->Position + 0.5f*DeltaTime*k1);
    v3 k3 = dfdt_Lin(c->Position + 0.5f*DeltaTime*k2);
    v3 k4 = dfdt_Lin(c->Position + DeltaTime*k4);
    c->Position += (1/6.f) * DeltaTime * (k1 + 2*k2 + 2*k3 + k4);
  }
  #endif
  {
    auto dfdt_Rot = [&AngularVelocity]( const v4& Rotation )
    {
      const v4 q0 = Rotation;
      const v4 r = V4(AngularVelocity.X, AngularVelocity.Y, AngularVelocity.Z,0);
      const v4 q1 = 0.5f*QuaternionMultiplication(r, q0);
      return q1;
    };
    v4 k1 = dfdt_Rot(c->Rotation);
    v4 k2 = dfdt_Rot(c->Rotation + 0.5f*DeltaTime*k1);
    v4 k3 = dfdt_Rot(c->Rotation + 0.5f*DeltaTime*k2);
    v4 k4 = dfdt_Rot(c->Rotation + DeltaTime*k4);
    c->Rotation += (1/6.f) * DeltaTime * (k1 + 2*k2 + 2*k3 + k4);
  }
}

 inline void
 TimestepVelocityForwardEuler(const r32 DeltaTime, const v3 LinearVelocity, const v3 AngularVelocity, component_spatial* c )
 {

  const v4 q0 = c->Rotation;
  const v4 r = V4(AngularVelocity.X, AngularVelocity.Y, AngularVelocity.Z,0);
  const v4 q1 = 0.5f*QuaternionMultiplication(r, q0);

  c->Position += DeltaTime*LinearVelocity;
  c->Rotation += DeltaTime*q1;
 }

inline internal void
IntegratePositions(r32 dtForFrame)
{
  TIMED_FUNCTION();

  ScopedTransaction(GlobalGameState->EntityManager);
  component_result* ComponentList = GetComponentsOfType(GlobalGameState->EntityManager, COMPONENT_FLAG_DYNAMICS);
  while(Next(GlobalGameState->EntityManager,ComponentList))
  {
    component_spatial* S = GetSpatialComponent(ComponentList);
    component_dynamics* D = GetDynamicsComponent(ComponentList);
    #if 1
    TimestepVelocityForwardEuler( dtForFrame, D->LinearVelocity, D->AngularVelocity, S );
    S->Rotation = Normalize(S->Rotation);
    #else
    TimestepVelocityRungeKutta4( dtForFrame, D->LinearVelocity, D->AngularVelocity, S );
    S->Rotation = Normalize(S->Rotation);
    #endif
  }
}

void SpatialSystemUpdate( world* World )
{
  TIMED_FUNCTION();

  world_contact_chunk* WorldContacts =  World->ContactManifolds;

  RemoveInvalidContactPoints( WorldContacts->FirstManifold );

  DoWarmStarting( WorldContacts->FirstManifold );

  IntegrateVelocities( World->dtForFrame );

  World->BroadPhaseTree = BuildBroadPhaseTree( );

  u32 BroadPhaseResultCount = 0;
  broad_phase_result_stack* const BroadPhaseResultStack = GetCollisionPairs( &World->BroadPhaseTree, &BroadPhaseResultCount );

  CreateAndDoWork( WorldContacts, BroadPhaseResultCount, BroadPhaseResultStack );

  RemoveNonIntersectingManifolds(WorldContacts);

  BEGIN_BLOCK(SolveConstraints);
  if(WorldContacts->FirstManifold)
  {
    for (u32 i = 0; i < SLOVER_ITERATIONS; ++i)
    {
      // TODO: Process Constraints MultiThreaded
      SolveNonPenetrationConstraints(World->dtForFrame, WorldContacts->FirstManifold);
      SolveFrictionalConstraints(WorldContacts->FirstManifold);
    }
  }
  END_BLOCK(SolveConstraints);

  IntegratePositions(World->dtForFrame);

}