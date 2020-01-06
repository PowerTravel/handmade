
#include "vector_math.h"
#include "utility_macros.h"
#include "aabb.h"
#include "dynamic_aabb_tree.h"

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

aabb3f GetAABBWorldSpace( entity* E )
{
  Assert(E->SpatialComponent);
  Assert(E->ColliderComponent);
  component_spatial*  S = E->SpatialComponent;
  component_collider* C = E->ColliderComponent;

  m4 M = S->ModelMatrix;
  aabb3f AABB = C->AABB;

  aabb3f Result = {};
  Result.P0 =   V3( M * V4(AABB.P0,1));
  Result.P1 =   V3( M * V4(AABB.P1,1));

  return Result;
}


v3 GravityForceEquation(v3& a,v3& b,v3& c)
{
  return V3(0,-00,0);
}

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

      v3  Position      = GetPosition(S);
      // TODO: We should try and calculate collisions BEFORE doing euler and bake Velocity into here
      //       Maybe it fixes the positional drift.
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
      aabb3f AABBWorldSpace = GetAABBWorldSpace(E);
      AABBTreeInsert( Arena, &BroadPhaseTree, E, AABBWorldSpace );

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
  local_persist b32 printed = false;
  if(API && !printed)
  {
    printed = true;
    thread_context Thread = {};
    API->DEBUGPlatformWriteEntireFile(&Thread, "..\\handmade\\Tree.m", str::StringLength(DebugPrintMemory), (void*)DebugPrintMemory);
  }


  broad_phase_result_stack* BroadPhaseResult = GetCollisionPairs( Arena, &BroadPhaseTree );

  while( BroadPhaseResult )
  {
    entity* A = BroadPhaseResult->A;
    entity* B = BroadPhaseResult->B;

    if( (A->Types & COMPONENT_TYPE_DYNAMICS)  ||
        (B->Types & COMPONENT_TYPE_DYNAMICS))
    {
      temporary_memory TempMem2 = BeginTemporaryMemory( Arena );
      component_spatial*   SA = A->SpatialComponent;
      component_collider*  CA = A->ColliderComponent;
      component_dynamics*  DA = A->DynamicsComponent;

      component_spatial*   SB = B->SpatialComponent;
      component_collider*  CB = B->ColliderComponent;
      component_dynamics*  DB = B->DynamicsComponent;

      gjk_collision_result NarrowPhaseResult = GJKCollisionDetection( &SA->ModelMatrix,  CA->Mesh, &SB->ModelMatrix, CB->Mesh, Arena, API);
      if(NarrowPhaseResult.ContainsOrigin)
      {
        contact_data ContactData = EPACollisionResolution(&World->Arena,  &SA->ModelMatrix, CA->Mesh,
                                                                          &SB->ModelMatrix, CB->Mesh,
                                                                          NarrowPhaseResult.Simplex,
                                                                          API);
        // Positional Constraint: Pa - Pb >= 0
        // Velocity   Constraint: JV + T = 0
        v3 pa = ContactData.A_ContactWorldSpace;
        v3 pb = ContactData.B_ContactWorldSpace;
        v3 n  = ContactData.ContactNormal;
        v3 ra = ContactData.A_ContactModelSpace;
        v3 va = {};
        v3 wa = {};
        v3 rb = ContactData.B_ContactModelSpace;
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

        v3 J[4] = {-n, -CrossProduct(ra,n), n, CrossProduct(rb,n) };
        v3 V[4] = {va, wa, vb, wb};

        v3 ADim = (CA->AABB.P1 - CA->AABB.P0);
        v3 BDim = (CB->AABB.P1 - CB->AABB.P0);

        r32 Ixa = (1/12.f) * ma * (ADim.Y*ADim.Y + ADim.Z*ADim.Z);
        r32 Iya = (1/12.f) * ma * (ADim.X*ADim.X + ADim.Z*ADim.Z);
        r32 Iza = (1/12.f) * ma * (ADim.X*ADim.X + ADim.Y*ADim.Y);

        r32 Ixb = (1/12.f) * mb * (BDim.Y*BDim.Y + BDim.Z*BDim.Z);
        r32 Iyb = (1/12.f) * mb * (BDim.X*BDim.X + BDim.Z*BDim.Z);
        r32 Izb = (1/12.f) * mb * (BDim.X*BDim.X + BDim.Y*BDim.Y);


        m3 Ma = M3(ma, 0, 0,
                    0,ma, 0,
                    0, 0,ma);

        m3 Mb = M3(mb, 0, 0,
                    0,mb, 0,
                    0, 0,mb);

        m3 Ia = M3(Ixa, 0, 0,
                    0,Iya, 0,
                    0, 0,Iza);

        m3 Ib = M3(Ixb, 0, 0,
                    0,Iyb, 0,
                    0, 0,Izb);

        m3 MaInv = M3(1/ma, 0, 0,
                       0,1/ma, 0,
                       0, 0,1/ma);

        m3 MbInv = M3(1/mb, 0, 0,
                        0,1/mb, 0,
                        0, 0,1/mb);

        m3 IaInv = M3(1/Ixa, 0, 0,
                        0,1/Iya, 0,
                        0, 0,1/Iza);

        m3 IbInv = M3(1/Ixb, 0, 0,
                        0, 1/Iyb, 0,
                        0,  0,1/Izb);


        m3 M[4]  = {Ma, Ia, Mb, Ib};
        m3 MInv[4] = {MaInv, IaInv, MbInv, IbInv}; // Works cause of diagonal matrix

        v3 MInvJ[4] = {MInv[0] * J[0],
                       MInv[1] * J[1],
                       MInv[2] * J[2],
                       MInv[3] * J[3]};

        r32 Restitution = 1.f * ((V[0] + V[1] + V[2] + V[3]) * n);
        //Baumgarte term:
        r32 Baumgarte = - (0.5f / dt) * ((pb-pa) * (-n));

        r32 Bias = Baumgarte + Restitution;

        r32 Denominator = (J[0] * MInvJ[0]) + (J[1] * MInvJ[1]) + (J[2] * MInvJ[2]) + (J[3] * MInvJ[3]);

        r32 Numerator = -((J[0] * V[0]) + (J[1] * V[1]) + (J[2] * V[2]) + (J[3] * V[3]) + Bias);

        Assert(Denominator > 10E-7);
        r32 Lambda = Numerator / Denominator;

        v3 DeltaV[4] = {(MInvJ[0])*Lambda, (MInvJ[1])*Lambda, (MInvJ[2])*Lambda, (MInvJ[3])*Lambda};

         if(A->Types & COMPONENT_TYPE_DYNAMICS)
         {
           DA->LinearVelocity += DeltaV[0];
           //DA->AngularVelocity += DeltaV[1];
           //Translate(-ContactData.ContactNormal * ContactData.PenetrationDepth,SA);
         }

         if(B->Types & COMPONENT_TYPE_DYNAMICS)
         {
            DB->LinearVelocity += DeltaV[2];
            //DB->AngularVelocity += DeltaV[3];
            //DB->AngularVelocity = ContactData.ContactNormal*Norm(DB->LinearVelocity);
           //Translate(ContactData.ContactNormal * ContactData.PenetrationDepth,SB);
         }

      }
      EndTemporaryMemory( TempMem2 );
    }
    BroadPhaseResult = BroadPhaseResult->Previous;
  }


  EndTemporaryMemory( TempMem1 );
  CheckArena(Arena);
}