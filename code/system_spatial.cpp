#include "component_spatial.h"
#include "component_collider.h"
#include "component_dynamics.h"
#include "component_gjk_epa_visualizer.h"
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
  entity* A;
  entity* B;
  b32 InContact;
  contact_data NewContact;
};

internal void
DoCollisionDetectionWork(void* Data)
{
  collision_detection_work* Work = (collision_detection_work*) Data;
  m4 ModelMatrixA = GetModelMatrix(Work->A->SpatialComponent);
  m4 ModelMatrixB = GetModelMatrix(Work->B->SpatialComponent);
  gjk_collision_result NarrowPhaseResult = GJKCollisionDetection(
      &ModelMatrixA, Work->A->ColliderComponent->Mesh,
      &ModelMatrixB, Work->B->ColliderComponent->Mesh);

  Work->InContact = NarrowPhaseResult.ContainsOrigin;
  if (Work->InContact)
  {
    Work->NewContact = EPACollisionResolution(Work->TransientArena, &ModelMatrixA, Work->A->ColliderComponent->Mesh,
                                                                    &ModelMatrixB, Work->B->ColliderComponent->Mesh,
                                                                    &NarrowPhaseResult.Simplex);
  }
}


#define DO_GET_CONTACT_NONSENSE_LINEARLY(i) ((contact_data_list*) ((u8*) ( World->Contacts ) + i * ( sizeof(contact_data_list) + 4*sizeof(contact_data))));

void SpatialSystemUpdate( world* World,/* platform_work_queue* CollisionQueue,*/ platform_api* API)
{
  r32 dt =  World->dtForFrame;
  memory_arena* PersistentArena = World->PersistentArena;
  memory_arena* TransientArena = World->TransientArena;
  tile_map* TileMap = &World->TileMap;

  temporary_memory TempMem1 = BeginTemporaryMemory( TransientArena );

  memory_index DebugPrintMemorySize = Megabytes(1);
  char* const DebugPrintMemory = (char*) PushArray(TransientArena, DebugPrintMemorySize, char);
  char* Scanner = DebugPrintMemory;

#if 0
  u8* TmpList = (u8*) PushSize(TransientArena, World->NrContacts * ( sizeof(contact_data_list) + 4*sizeof(contact_data) ));
  u32 ManifoldIndex = 0;
  u8* MemoryLocation = TmpList;
  // Remove invalid contacts
  for (u32 i = 0; i < World->NrContacts; ++i)
  {
    umm ByteStride  =  i * ( sizeof(contact_data_list) + 4*sizeof(contact_data));
    contact_data_list* ContactData = (contact_data_list*) ( ( (u8*) World->Contacts ) + ByteStride);
    Assert(ContactData->MaxNrContacts ==4);
    contact_data SavedContacts[4] = {};
    u32 SavedContactIdx = 0;
    m4 ModelMatA = ContactData->A->SpatialComponent->ModelMatrix;
    m4 ModelMatB = ContactData->B->SpatialComponent->ModelMatrix;
    for (u32 j = 0; j < ContactData->NrContacts; ++j)
    {
      contact_data* Contact = &ContactData->Contacts[j];
      const v3 LocalToGlobalA = V3( ModelMatA * V4(Contact->A_ContactModelSpace,1));
      const v3 LocalToGlobalB = V3( ModelMatB * V4(Contact->B_ContactModelSpace,1));

      const v3 rAB = LocalToGlobalB - LocalToGlobalA;

      const v3 rA  = Contact->A_ContactWorldSpace - LocalToGlobalA;
      const v3 rB  = Contact->B_ContactWorldSpace - LocalToGlobalB;

      const r32 normDot = Contact->ContactNormal * rAB;
      const b32 stillPenetrating = normDot <= 0.0f;

      const r32 persistentThresholdSq = 0.01;
      const r32 lenDiffA = NormSq(rA);
      const r32 lenDiffB = NormSq(rB);

      // keep contact point if the collision pair is
      // still colliding at this point, and the local
      // positions are not too far from the global
      // positions original acquired from collision detection
      if (stillPenetrating && (lenDiffA < persistentThresholdSq && lenDiffB < persistentThresholdSq) )
      {
        // Persistent contact
        SavedContacts[SavedContactIdx++] = *Contact;
      }
    }

    if(SavedContactIdx)
    {
      utils::Copy(sizeof(contact_data_list), ContactData, MemoryLocation);
      ((contact_data_list*)MemoryLocation)->NrContacts = SavedContactIdx;
      MemoryLocation+=sizeof(contact_data_list);
      utils::Copy(sizeof(SavedContacts), SavedContacts, MemoryLocation);
      MemoryLocation+=sizeof(SavedContacts);
      ManifoldIndex++;
    }
  }

  utils::Copy(MemoryLocation-TmpList, TmpList, World->Contacts);
  World->NrContacts = ManifoldIndex;
#else
  u32 MemCount = World->NrContacts * ( sizeof(contact_data_list) + 4*sizeof(contact_data) );
  u8* TmpList = (u8*) PushSize(TransientArena, MemCount);
  utils::Copy(World->NrContacts, TmpList, World->Contacts);
  World->NrContacts = 0;
#endif
  aabb_tree BroadPhaseTree = {};

  for(u32 Index = 0;  Index < World->NrEntities; ++Index )
  {
    entity* E = &World->Entities[Index];
    #if 1
    if( E->Types & COMPONENT_TYPE_DYNAMICS )
    {
      #if 0
      v3 Gravity = V3(0,-10,0);
      component_dynamics*  D = E->DynamicsComponent;
      D->LinearVelocity += dt * Gravity * D->Mass;
      #else
      component_spatial*   S = E->SpatialComponent;
      component_collider*  C = E->ColliderComponent;
      component_dynamics*  D = E->DynamicsComponent;
      v3  Position        = S->Position;
      v3  LinearVelocity  = D->LinearVelocity;
      v3  AngularVelocity = D->AngularVelocity;
      r32 Mass            = D->Mass;

      v3 Gravity = V3(0,-10,0);
      //Gravity = V3(0,0,0);
      v3 LinearAcceleration = Gravity * Mass;
      LinearVelocity     = LinearVelocity + dt * LinearAcceleration;

      v3 AngularAcceleration = {};
      AngularVelocity = AngularVelocity + dt * AngularAcceleration;
#if 0
      Translate(dt * LinearVelocity, S);

      Rotate(Norm(dt*AngularVelocity), Normalize(dt*AngularVelocity) ,S);
#endif
      D->LinearVelocity  = LinearVelocity;
      D->AngularVelocity = AngularVelocity;
      #endif
    }
    #endif
    if( E->Types & COMPONENT_TYPE_COLLIDER )
    {
      aabb3f AABBWorldSpace = {};
      GetTransformedAABBFromColliderMesh( E->ColliderComponent, GetModelMatrix(E->SpatialComponent), &AABBWorldSpace );
      AABBTreeInsert( TransientArena, &BroadPhaseTree, E, AABBWorldSpace );
#if 0
      if(API)
      {
        // Print Tree
        s64 RemainingSize = DebugPrintMemorySize + DebugPrintMemory - Scanner;
        Scanner += GetPrintableTree( TransientArena, &BroadPhaseTree, RemainingSize, Scanner);
        RemainingSize = DebugPrintMemorySize + DebugPrintMemory - Scanner;
        Scanner += str::itoa(-1,RemainingSize,Scanner);
        *Scanner++ = ' ';
        aabb3f zero = {};
        Scanner += AABBToString(&zero, 64, Scanner);
        *Scanner++ = '\n';
      }
#endif
    }
  }

  broad_phase_result_stack* const BroadPhaseResult = GetCollisionPairs( TransientArena, &BroadPhaseTree );
  broad_phase_result_stack* ColliderPair = BroadPhaseResult;

#if 1
  collision_detection_work WorkArray[128] = {};
  collision_detection_work* Work = WorkArray;
  u32 CollisionCount = 0;
  while( ColliderPair )
  {
    Work->TransientArena = TransientArena;
    Work->A = ColliderPair->A;
    Work->B = ColliderPair->B;
//    CollisionQueue->AddEntry(CollisionQueue, DoCollisionDetectionWork, Work);

    ColliderPair = ColliderPair->Previous;
    ++Work;
    ++CollisionCount;
  }

//  CollisionQueue->PlatformCompleteWorkQueue(CollisionQueue);
  u32 CollisionIndex = 0;
  while(CollisionIndex < CollisionCount)
  {
    DoCollisionDetectionWork((void*) (WorkArray + CollisionIndex));
    CollisionIndex++;
  }

  CollisionIndex = 0;
  u32 ContactIndex = 0;
  World->NrContacts = 0;
  while(CollisionIndex < CollisionCount)
  {
    collision_detection_work* CurrentWork = WorkArray + CollisionIndex++;
    if(CurrentWork->InContact)
    {
      contact_data_list* ContactData = DO_GET_CONTACT_NONSENSE_LINEARLY(ContactIndex);
      World->NrContacts++;
      ContactData->MaxNrContacts = 4;
      ContactData->NrContacts = 1;
      ContactData->A = CurrentWork->A;
      ContactData->B = CurrentWork->B;
      ContactData->Contacts = (contact_data*) (ContactData + 1);
      *ContactData->Contacts = CurrentWork->NewContact;
      ContactIndex++;
    }
  }
#else
  while( ColliderPair )
  {
    entity* A = ColliderPair->A;
    entity* B = ColliderPair->B;

    if( (A->Types & COMPONENT_TYPE_DYNAMICS) ||
        (B->Types & COMPONENT_TYPE_DYNAMICS) ||
        (A->GjkEpaVisualizerComponent) ||
        (B->GjkEpaVisualizerComponent) )
    {
      component_spatial*   SA = A->SpatialComponent;
      component_collider*  CA = A->ColliderComponent;
      component_dynamics*  DA = A->DynamicsComponent;

      component_spatial*   SB = B->SpatialComponent;
      component_collider*  CB = B->ColliderComponent;
      component_dynamics*  DB = B->DynamicsComponent;

      component_gjk_epa_visualizer* Vis = A->GjkEpaVisualizerComponent;
      Vis = Vis ? Vis : B->GjkEpaVisualizerComponent;

      gjk_collision_result NarrowPhaseResult = GJKCollisionDetection(
          &SA->ModelMatrix, CA->Mesh,
          &SB->ModelMatrix, CB->Mesh,
          Vis);

      if (NarrowPhaseResult.ContainsOrigin)
      {
        contact_data NewContact = EPACollisionResolution(TransientArena, &SA->ModelMatrix, CA->Mesh,
                                                                         &SB->ModelMatrix, CB->Mesh,
                                                                         &NarrowPhaseResult.Simplex,
                                                                         Vis);
        // TODO: Find a smarter way than to check the relevant collision points instead of looping through all of them
        // Loop through all existing contacts and see if we have similar boidies already connecting
        contact_data_list* ContactData = 0;
        for(u32 i = 0; i < World->NrContacts; ++i)
        {
          umm ByteStride  =  i * ( sizeof(contact_data_list) + 4*sizeof(contact_data));
          contact_data_list* WorldContact = (contact_data_list*) ( ( (u8*) World->Contacts ) + ByteStride);
          if(WorldContact->A->id == A->id && WorldContact->B->id == B->id )
          {
            ContactData = WorldContact;
          }
          else if (WorldContact->B->id == A->id && WorldContact->A->id == B->id )
          {
            // Note: Jakob, This can be a source of bugs. Check it.
            contact_data Tmp = {};
            Tmp.A_ContactWorldSpace =  NewContact.B_ContactWorldSpace;
            Tmp.B_ContactWorldSpace =  NewContact.A_ContactWorldSpace;
            Tmp.A_ContactModelSpace =  NewContact.B_ContactModelSpace;
            Tmp.B_ContactModelSpace =  NewContact.A_ContactModelSpace;
            Tmp.ContactNormal       = -NewContact.ContactNormal;
            Tmp.TangentNormalOne    =  NewContact.TangentNormalOne;
            Tmp.TangentNormalTwo    =  NewContact.TangentNormalTwo;
            Tmp.PenetrationDepth    =  NewContact.PenetrationDepth;
            NewContact = Tmp;
            ContactData = WorldContact;
          }
        }

        if(!ContactData)
        {
          Assert(World->NrContacts < World->MaxNrContacts);
          umm ByteStride  =  World->NrContacts * ( sizeof(contact_data_list) + 4*sizeof(contact_data));
          ContactData = (contact_data_list*) ( ( (u8*) World->Contacts ) + ByteStride);
          World->NrContacts++;
          ContactData->A = A;
          ContactData->B = B;

          ContactData->NrContacts = 0;
          ContactData->MaxNrContacts = 4;
          ContactData->Contacts = (contact_data*) (ContactData + 1);

        }

        b32 FarEnough = true;
        for (u32 i = 0; i < ContactData->NrContacts; ++i)
        {
          const contact_data* OldContact = &ContactData->Contacts[i];
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
            //ContactData->Contacts[i] = NewContact;
            break;
          }
        }

        if (FarEnough)
        {
          if (ContactData->NrContacts < ContactData->MaxNrContacts)
          {
            ContactData->Contacts[ContactData->NrContacts++] = NewContact;
          }else{
            #if 1
            local_persist u32 idx = 4;
            ContactData->Contacts[idx % 4] = NewContact;
            idx++;
            #else
            Assert(ContactData->MaxNrContacts == 4);
            Assert(ContactData->NrContacts == ContactData->MaxNrContacts);

            contact_data PossibleContacts[5] = {};
            v3 ContactPoints[4] = {};
            u32 AddedContacts[4] = {};
            utils::Copy(4* sizeof(contact_data), ContactData->Contacts, PossibleContacts );
            PossibleContacts[4] = NewContact;
            const m4 ModelMatA = ContactData->A->SpatialComponent->ModelMatrix;
            const m4 ModelMatB = ContactData->B->SpatialComponent->ModelMatrix;
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
                ContactData->Contacts[0] = PossibleContacts[i];
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
                ContactData->Contacts[1] = PossibleContacts[i];
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
                ContactData->Contacts[2] = PossibleContacts[i];
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
                ContactData->Contacts[3] = PossibleContacts[i];
              }
            }

            if(!FinalPointAdded)
            {
              ContactData->NrContacts = 3;
              ContactData->Contacts[3] = {};
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

  for(u32 i = 0; i < World->NrContacts; ++i)
  {
    contact_data_list* ContactData = DO_GET_CONTACT_NONSENSE_LINEARLY(i);
    entity* A = ContactData->A;
    entity* B = ContactData->B;
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


    for(u32 j = 0; j < ContactData->NrContacts; ++j)
    {
      contact_data* Contact = &ContactData->Contacts[j];
      const v3& ra = Contact->A_ContactModelSpace;
      const v3& rb = Contact->B_ContactModelSpace;
      const v3& n  = Contact->ContactNormal;

      Contact->J[0] = -n;
      Contact->J[1] = -CrossProduct(ra, n);
      Contact->J[2] = n;
      Contact->J[3] = CrossProduct(rb, n);

      MultiplyDiagonalM12V12(InvM, Contact->J, Contact->InvMJ);
    }
  }
  for(u32 j = 0; j < World->NrContacts; ++j)
  {
    contact_data_list* ContactData = DO_GET_CONTACT_NONSENSE_LINEARLY(j);
    for(u32 k = 0; k < ContactData->NrContacts; ++k)
    {
        contact_data* Contact = &ContactData->Contacts[k];
        Contact->AccumulatedLambda = 0;
    }
  }
  if(World->NrContacts)
  {
    for (u32 i = 0; i < 4; ++i)
    {
      // TODO: Process Constraints MultiThreaded?
      for(u32 j = 0; j < World->NrContacts; ++j)
      {
        contact_data_list* ContactData = DO_GET_CONTACT_NONSENSE_LINEARLY(j);
        entity* A = ContactData->A;
        entity* B = ContactData->B;
        for(u32 k = 0; k < ContactData->NrContacts; ++k)
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
          contact_data* Contact = &ContactData->Contacts[k];
          //r32 PenetrationDepth  = Norm(V3(GetModelMatrix(A->SpatialComponent) * V4( Contact->A_ContactModelSpace,1)) -
          //                             V3(GetModelMatrix(B->SpatialComponent) * V4( Contact->B_ContactModelSpace,1)));
          r32 PenetrationDepth  = Norm(V3(GetModelMatrix(A->SpatialComponent) * V4(Contact->A_ContactModelSpace,1)) -
                                       V3(GetModelMatrix(B->SpatialComponent) * V4(Contact->B_ContactModelSpace,1)));
          v3  ContactNormal     = Contact->ContactNormal;
          r32 Restitution       = getRestitutionCoefficient(V, 0.25f, ContactNormal, 0.01);
          r32 Baumgarte         = getBaumgarteCoefficient(dt, 0.25,  PenetrationDepth, 0.01);
          r32 Lambda            = GetLambda( V, Contact->J, Contact->InvMJ, Baumgarte, Restitution);
          r32 OldCumulativeLambda = Contact->AccumulatedLambda;
          Contact->AccumulatedLambda += Lambda;
          Contact->AccumulatedLambda = Contact->AccumulatedLambda <=0 ? 0 : Contact->AccumulatedLambda;
          r32 LambdaDiff = Contact->AccumulatedLambda - OldCumulativeLambda;

          v3 DeltaV[4] = {};
          ScaleV12(LambdaDiff, Contact->InvMJ, DeltaV);

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