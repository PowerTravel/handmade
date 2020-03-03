
#include "vector_math.h"
#include "utility_macros.h"
#include "aabb.h"
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
getBaumgarteCoefficient(r32 dt, r32 Scalar, r32 PenetrationDepth)
{
  r32 Baumgarte = - (Scalar / dt) * PenetrationDepth;
  return Baumgarte;
}

internal inline r32
getRestitutionCoefficient(v3 V[], r32 Scalar, v3 Normal)
{
  r32 Restitution = Scalar * ((V[0] + V[1] + V[2] + V[3]) * Normal);
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

struct constraint_data
{
  contact_data ContactData;
  v3 J[4];
  v3 InvMJ[4];
  v3 V[4];
  r32 AccumulatedLambda;
  entity* A;
  entity* B;
  constraint_data* Next;
};

void SpatialSystemUpdate( world* World, platform_api* API)
{
  r32 dt =  World->dtForFrame;
  local_persist v3 CollisionPoint = {};
  memory_arena* Arena = &World->Arena;
  tile_map* TileMap = &World->TileMap;


  temporary_memory TempMem1 = BeginTemporaryMemory( Arena );

  memory_index DebugPrintMemorySize = Megabytes(2);
  char* const DebugPrintMemory = (char*) PushArray(Arena, DebugPrintMemorySize, char);
  char* Scanner = DebugPrintMemory;

  aabb_tree BroadPhaseTree = {};

  for(u32 Index = 0;  Index < World->NrEntities; ++Index )
  {
    entity* E = &World->Entities[Index];
    if( E->Types & COMPONENT_TYPE_DYNAMICS )
    {
      component_spatial*   S = E->SpatialComponent;
      component_collider*  C = E->ColliderComponent;
      component_dynamics*  D = E->DynamicsComponent;
      v3  Position        = GetPosition(S);
      v3  LinearVelocity  = D->LinearVelocity;
      v3  AngularVelocity = D->AngularVelocity;
      r32 Mass            = D->Mass;

      v3 Gravity = V3(0,-10,0);
      v3 LinearAcceleration = Gravity * Mass;
      LinearVelocity        = LinearVelocity + dt * LinearAcceleration;
      Translate(dt * LinearVelocity, S);

      v3 AngularAcceleration = {};
      AngularVelocity    = AngularVelocity + dt * AngularAcceleration;
      Rotate(Norm(dt*AngularVelocity), Normalize(dt*AngularVelocity) ,S);

      D->LinearVelocity  = LinearVelocity;
      D->AngularVelocity = AngularVelocity;
    }
    if( E->Types & COMPONENT_TYPE_COLLIDER )
    {
      aabb3f AABBWorldSpace = {};
      GetTransformedAABBFromColliderMesh( E->ColliderComponent, E->SpatialComponent->ModelMatrix, &AABBWorldSpace );
      AABBTreeInsert( Arena, &BroadPhaseTree, E, AABBWorldSpace );

      E->ColliderComponent->IsColliding = false;

      if(API)
      {
        // Print Tree
        s64 RemainingSize = DebugPrintMemorySize + DebugPrintMemory - Scanner;
        Scanner += GetPrintableTree( Arena, &BroadPhaseTree, RemainingSize, Scanner);
        RemainingSize = DebugPrintMemorySize + DebugPrintMemory - Scanner;
        Scanner += str::itoa(-1,RemainingSize,Scanner);
        *Scanner++ = ' ';
        aabb3f zero = {};
        Scanner += AABBToString(&zero, 64, Scanner);
        *Scanner++ = '\n';
      }
    }

    if( E->Types & COMPONENT_TYPE_GJK_EPA_VISUALIZER)
    {
      component_gjk_epa_visualizer* Vis = E->GjkEpaVisualizerComponent;
      Assert(Vis->A);
      Assert(Vis->B);
      Assert(Vis->A->Types & COMPONENT_TYPE_COLLIDER);
      Assert(Vis->B->Types & COMPONENT_TYPE_COLLIDER);
      gjk_collision_result NarrowPhaseResult = GJKCollisionDetection(
          &Vis->A->SpatialComponent->ModelMatrix,
          Vis->A->ColliderComponent->Mesh,
          &Vis->B->SpatialComponent->ModelMatrix,
          Vis->B->ColliderComponent->Mesh,
          Vis);

      EPACollisionResolution(&World->Arena,
        &Vis->A->SpatialComponent->ModelMatrix,
        Vis->A->ColliderComponent->Mesh,
        &Vis->B->SpatialComponent->ModelMatrix,
        Vis->B->ColliderComponent->Mesh,
        NarrowPhaseResult.Simplex);
    }
  }

  broad_phase_result_stack* const BroadPhaseResult = GetCollisionPairs( Arena, &BroadPhaseTree );
  broad_phase_result_stack* ColliderPair = BroadPhaseResult;
  constraint_data* ConstraintHead = 0;
  while( ColliderPair )
  {
    entity* A = ColliderPair->A;
    entity* B = ColliderPair->B;

    if( (A->Types & COMPONENT_TYPE_DYNAMICS)  ||
        (B->Types & COMPONENT_TYPE_DYNAMICS))
    {
      component_spatial*   SA = A->SpatialComponent;
      component_collider*  CA = A->ColliderComponent;
      component_dynamics*  DA = A->DynamicsComponent;

      component_spatial*   SB = B->SpatialComponent;
      component_collider*  CB = B->ColliderComponent;
      component_dynamics*  DB = B->DynamicsComponent;

      gjk_collision_result NarrowPhaseResult = GJKCollisionDetection(
          &SA->ModelMatrix, CA->Mesh,
          &SB->ModelMatrix, CB->Mesh);
      if(NarrowPhaseResult.ContainsOrigin)
      {
        constraint_data* ConstraintData = (constraint_data*) PushStruct(Arena, constraint_data);

        ConstraintData->ContactData = EPACollisionResolution(&World->Arena,  &SA->ModelMatrix, CA->Mesh,
                                                                             &SB->ModelMatrix, CB->Mesh,
                                                                             NarrowPhaseResult.Simplex);
        ConstraintData->AccumulatedLambda = 0;
        ConstraintData->A = A;
        ConstraintData->B = B;
        ConstraintData->Next = ConstraintHead;
        ConstraintHead = ConstraintData;

        CA->IsColliding = true;
        CB->IsColliding = true;
        CA->CollisionPoint = ConstraintData->ContactData.A_ContactModelSpace;
        CB->CollisionPoint = ConstraintData->ContactData.B_ContactModelSpace;
        v3 va = {};
        v3 wa = {};
        v3 vb = {};
        v3 wb = {};
        r32 ma = 10e37;
        r32 mb = 10e37; // Solve stationary objects with stationary constraint?
        if(A->Types & COMPONENT_TYPE_DYNAMICS)
        {
          va = DA->LinearVelocity;
          wa = DA->AngularVelocity;
          ma = DA->Mass;
        }

        if(B->Types & COMPONENT_TYPE_DYNAMICS)
        {
          vb = DB->LinearVelocity;
          wb = DB->AngularVelocity;
          mb = DB->Mass;
        }
        const v3& ra = ConstraintData->ContactData.A_ContactModelSpace;
        const v3& rb = ConstraintData->ContactData.B_ContactModelSpace;
        const v3& n  = ConstraintData->ContactData.ContactNormal;
        ConstraintData->J[0] = -n;
        ConstraintData->J[1] = -CrossProduct(ra, n);
        ConstraintData->J[2] = n;
        ConstraintData->J[3] = CrossProduct(rb, n);
        ConstraintData->V[0] = va;
        ConstraintData->V[1] = wa;
        ConstraintData->V[2] = vb;
        ConstraintData->V[3] = wb;
        m3 InvM[4] = {};
        GetAABBInverseMassMatrix( &CA->AABB, ma, &InvM[0], &InvM[1]);
        GetAABBInverseMassMatrix( &CB->AABB, mb, &InvM[2], &InvM[3]);
        MultiplyDiagonalM12V12(InvM, ConstraintData->J, ConstraintData->InvMJ);

      }else{
        // Breakpoint Spot
        int cc =10;
      }
    }
    ColliderPair = ColliderPair->Previous;
  }

  for (int i = 0; i < 4; ++i)
  {
    constraint_data* Constraint = ConstraintHead;
    while(Constraint)
    {
      contact_data* ContactData = &Constraint->ContactData;
      r32 PenetrationDepth = ContactData->PenetrationDepth;
      v3  ContactNormal    = ContactData->ContactNormal;
      r32 Baumgarte        = getBaumgarteCoefficient(dt, 0.9, PenetrationDepth);
      r32 Restitution      = getRestitutionCoefficient(Constraint->V, 0.0f, ContactNormal);
      r32 Lambda           = GetLambda( Constraint->V, Constraint->J, Constraint->InvMJ, Baumgarte, Restitution);

      r32 OldCumulativeLambda = Constraint->AccumulatedLambda;
      Constraint->AccumulatedLambda += Lambda;
      Constraint->AccumulatedLambda = Constraint->AccumulatedLambda <=0 ? 0 : Constraint->AccumulatedLambda;
      r32 LambdaDiff = Constraint->AccumulatedLambda - OldCumulativeLambda;

      v3 DeltaV[4] = {};
      ScaleV12(LambdaDiff,  Constraint->InvMJ, DeltaV);

      Constraint->V[0] += DeltaV[0];
      Constraint->V[1] += DeltaV[1];
      Constraint->V[2] += DeltaV[2];
      Constraint->V[3] += DeltaV[3];

      Constraint = Constraint->Next;
    }
  }

  constraint_data* Constraint = ConstraintHead;
  while(Constraint)
  {
    contact_data* ContactData = &Constraint->ContactData;
    if(Constraint->A->Types & COMPONENT_TYPE_DYNAMICS)
    {
      entity* A = Constraint->A;
      A->DynamicsComponent->LinearVelocity = Constraint->V[0];
      v3 DeltaX = dt * A->DynamicsComponent->LinearVelocity;
      Translate(DeltaX, A->SpatialComponent);

      //A->DynamicsComponent->AngularVelocity = dt * Constraint->V[1];
      //v3 DeltaRot = dt * Constraint->V[1];
      //Rotate(Norm(DeltaRot), Normalize(DeltaRot), A->SpatialComponent);
    }

    if(Constraint->B->Types & COMPONENT_TYPE_DYNAMICS)
    {
      entity* B = Constraint->B;
      B->DynamicsComponent->LinearVelocity = Constraint->V[2];
      v3 DeltaX = dt * B->DynamicsComponent->LinearVelocity;
      Translate(DeltaX, B->SpatialComponent);

      //B->DynamicsComponent->AngularVelocity =dt * Constraint->V[3];
      //v3 DeltaRot = dt * Constraint->V[3];
      //Rotate(Norm(DeltaRot), Normalize(DeltaRot), B->SpatialComponent);
    }

    Constraint = Constraint->Next;
  }

  EndTemporaryMemory( TempMem1 );
  CheckArena(Arena);
}