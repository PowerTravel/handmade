
#include "vector_math.h"
#include "utility_macros.h"
#include "aabb.h"

#if 0

closest_vertex_pair EdgeEdge( v3 EdgeAStart, v3 EdgeAEnd, v3 EdgeBStart, v3 EdgeBEnd )
{
  Assert(EdgeAStart != EdgeAEnd);
  Assert(EdgeBStart != EdgeBEnd);

  v3& o1 = EdgeAStart;
  v3& d1 = EdgeAEnd;
  v3 u1 = Normalize(d1-o1);

  v3& o2 = EdgeBStart;
  v3& d2 = EdgeBEnd;
  v3 u2 = Normalize(d2-o2);

  r32 k = u1*u2;


  closest_vertex_pair Result = {};

  r32 Tol = 10E-7;

  // Lines are not parallel, unique point exists
  if( Abs(Abs(k)-1) >= Tol )
  {
    r32 Scalar = ( (o2-o1) * (u1 - k * u2) ) / (1 - k*k);

    // This point is the intersection point between the RAYS A and B
    v3 ClosestPointOnRay = o1 + Scalar * u1;


    // Check if ClosestPointOnRay lies on the LINE SEGMENT A or B
    v3 no1 = ClosestPointOnRay - o1;
    r32 so1 = no1 * u1;
    v3 nd1 = ClosestPointOnRay - d1;
    r32 sd1 = nd1 * u1;

    v3 no2 = ClosestPointOnRay - o2;
    r32 so2 = no2 * u2;
    v3 nd2 = ClosestPointOnRay - d2;
    r32 sd2 = nd2 * u2;


    b32 IsOnLineSegmentA = ( (so1 >= 0) && (sd1 <= 0) );

    b32 IsOnLineSegmentB = ( (so2 >= 0) && (sd2 <= 0) );

    if( IsOnLineSegmentA && IsOnLineSegmentB )
    {
      // Point is on Line Segment A
      Result = VertexEdge( ClosestPointOnRay, EdgeBStart, EdgeBEnd );
    }else{

      // Point is not on any of the line segments
      closest_vertex_pair A0B = VertexEdge( EdgeAStart, EdgeBStart, EdgeBEnd);
      r32 ShortestContactDistance = Norm( A0B.P1 - A0B.P0 );
      Result = A0B;

      closest_vertex_pair A1B = VertexEdge( EdgeAEnd,   EdgeBStart, EdgeBEnd);
      r32 ContactDistance = Norm( A1B.P1 - A1B.P0 );
      if( ContactDistance < ShortestContactDistance )
      {
        ShortestContactDistance = ContactDistance;
        Result = A1B;
      }

      closest_vertex_pair B0A = VertexEdge( EdgeBStart, EdgeAStart, EdgeAEnd);
      ContactDistance = Norm( B0A.P1 - B0A.P0 );
      if( ContactDistance < ShortestContactDistance )
      {
        ShortestContactDistance = ContactDistance;
        Result.P0 = B0A.P1;
        Result.P1 = B0A.P0;
      }

      closest_vertex_pair B1A = VertexEdge( EdgeBEnd,   EdgeAStart, EdgeAEnd);
      ContactDistance = Norm( B1A.P1 - B1A.P0 );
      if( ContactDistance < ShortestContactDistance )
      {
        ShortestContactDistance = ContactDistance;
        Result.P0 = B1A.P1;
        Result.P1 = B1A.P0;
      }
    }

  // Lines are parallel, may not have a unique contact point
  }else{

    if(k < 0)
    {
      // Note (Jakob): This aligns A with B.
      //         Not sure if it's necessary, all unit tests
      //         works without it but I can't know for certain
      //         if omitting this step will cause unforseen
      //         asymmetry problems so just for saftey we align them.
      v3 TmpPoint = EdgeBStart;
      EdgeBStart = EdgeBEnd;
      EdgeBEnd = TmpPoint;
    }

    closest_vertex_pair A0B = VertexEdge( EdgeAStart, EdgeBStart, EdgeBEnd);
    v3 A0Bn = A0B.P1-A0B.P0;
    b32 A0Bp = Abs(A0Bn * u1) < Tol;

    closest_vertex_pair A1B = VertexEdge( EdgeAEnd,   EdgeBStart, EdgeBEnd);
    v3 A1Bn = A1B.P1-A1B.P0;
    b32 A1Bp = Abs(A1Bn * u1) < Tol;

    closest_vertex_pair B0A = VertexEdge( EdgeBStart, EdgeAStart, EdgeAEnd);
    v3 B0An = B0A.P1-B0A.P0;
    b32 B0Ap = Abs(B0An * u2) < Tol;

    closest_vertex_pair B1A = VertexEdge( EdgeBEnd,   EdgeAStart, EdgeAEnd);
    v3 B1An = B1A.P1-B1A.P0;
    b32 B1Ap = Abs(B1An * u2) < Tol;

    // Five different cases when lines are parallel
    if( ! (A0Bp || A1Bp || B0Ap || B1Ap) )
    {
      // Separate:
      //           |--A--|
      //  |--B--|

      r32 A0BMinDist = Norm( A0Bn );
      r32 A1BMinDist = Norm( A1Bn );
      if( A0BMinDist < A1BMinDist )
      {
        Result = A0B;
      }else{
        Result = A1B;
      }
    }else if(A0Bp && A1Bp){

      //  A inside B
      //      |----A----|
      //  |--------B--------|
      Result.P0 = (A0B.P0 + A1B.P0)/2;
      Result.P1 = (A0B.P1 + A1B.P1)/2;
    }else if(B0Ap && B1Ap){

      //  B inside A
      //  |--------A--------|
      //      |----B----|
      Result.P0 = (B0A.P1 + B1A.P1)/2;
      Result.P1 = (B0A.P0 + B1A.P0)/2;
    }else if(A0Bp && B1Ap){

      // B to the left of A
      //      |----A----|
      // |----B----|
      Result.P0 = (A0B.P0 + B1A.P1)/2;
      Result.P1 = (A0B.P1 + B1A.P0)/2;
    }else if(B0Ap && A1Bp){

      // B to the Right of A
      // |----A----|
      //      |----B----|
      Result.P0 = (B0A.P1 + A1B.P0)/2;
      Result.P1 = (B0A.P0 + A1B.P1)/2;
    }else{
      INVALID_CODE_PATH
    }

  }

  return Result;
}


#endif


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


struct collision_data
{
  b32 FirstCollision;
  b32 Intersecting;
  v3 CollisionDistance;
  v3  CollisionNormal;
  r32 FirstHitPercentage;
};

collision_data FindEarliestCollision( aabb3f& A, list<aabb3f>& BList, v3& Step )
{
  collision_data Result = {};
  Result.FirstHitPercentage = 1;
  r32 HitPercentage = 1;

  for(BList.First();
     !BList.IsEnd();
      BList.Next())
  {
    aabb3f B = BList.Get();
    if(SweeptAABB(A, B, Step, HitPercentage, Result.CollisionNormal))
    {
      Result.Intersecting = true;
      if( (HitPercentage < Result.FirstHitPercentage) && (HitPercentage > 0) )
      {
          Result.FirstHitPercentage = HitPercentage;
          Result.FirstCollision     = true;
      }
    }
  }

  if(Result.FirstCollision)
  {
    Result.CollisionDistance = Step * Result.FirstHitPercentage;
  }

  return Result;
}

struct timestep_progression
{
  v3 P0;
  v3 P1;
  v3 Velocity;

  r32 DistanceToTravel;
  r32 DistanceTraveled;
};


timestep_progression ForwardEuler( r32 dt, r32 Mass, v3 Position, v3 Velocity, v3 ExternalForce, v3 (*ForceEquation)(v3&,v3&,v3&) )
{
  timestep_progression Result = {};

  v3 f  = {};
  if(ForceEquation)
  {
    f = ForceEquation(Position, Velocity, ExternalForce);
  }

  v3 Acceleration = f * Mass;
  Result.Velocity = Velocity + dt * Acceleration;

  Result.P0 = Position;
  Result.P1 = Position + dt * Result.Velocity;

  Result.DistanceToTravel = Norm(Result.P1 - Result.P0);
  Result.DistanceTraveled = 0;
  return Result;
}

void Reflect(v3 PointOfImpact, v3 CollisionNormal,  timestep_progression* TP )
{
  Assert(TP->DistanceToTravel > 0);
  Assert(TP->DistanceToTravel > TP->DistanceTraveled);
  Assert(Norm(CollisionNormal) == 1);

  r32  DistanceToCollision = Norm( PointOfImpact - TP->P0 );
  r32  RemainingDistance   = Norm( TP->P1 - PointOfImpact );

  v3 Perpendicular = ( TP->Velocity * CollisionNormal) * CollisionNormal;
  v3 Parallel  = TP->Velocity - Perpendicular;
  v3 ReflectedVelocity = Parallel - Perpendicular;

  TP->DistanceTraveled += DistanceToCollision;
  TP->P0 = PointOfImpact;
  TP->P1 = PointOfImpact + RemainingDistance * Normalize( ReflectedVelocity );
  TP->Velocity = ReflectedVelocity;

  Assert( TP->DistanceTraveled <= TP->DistanceToTravel );
}

v3 HeroForceEquation( v3& Position, v3& Velocity, v3& ExternalForce )
{
  r32 WindDampingFactor = 2.f;
  r32 VelDot = Velocity * Velocity;
  v3 WindResistance = {};
  if(VelDot != 0)
  {
    WindResistance = WindDampingFactor * VelDot * Normalize(Velocity);
  }

  r32 FrictionDampingFactor = 2.f;
  v3 Friction = FrictionDampingFactor * Velocity;

  v3 Force = ExternalForce - WindResistance - Friction;

  return Force;
}

v3 GravityForceEquation(v3& a,v3& b,v3& c)
{
  return V3(0,-1,0);
}
#if 1
void SpatialSystemUpdate( world* World )
{
  v3 Gravity = V3( 0, -9, 0 );
  r32 dt =  World->dtForFrame;
  local_persist v3 CollisionPoint = {};
  memory_arena* Arena = &World->Arena;
  tile_map* TileMap = &World->TileMap;

  for(u32 Index = 0;  Index < World->NrEntities; ++Index )
  {
    entity* E = &World->Entities[Index];

    if( E->Types & COMPONENT_TYPE_DYNAMICS )
    {

#if 0
      temporary_memory TempMem = BeginTemporaryMemory( Arena );

      component_spatial*   S = E->SpatialComponent;
      component_collider*  C = E->ColliderComponent;
      component_dynamics*  D = E->DynamicsComponent;

      v3  Position      = GetPosition(S);
      v3  Velocity      = D->Velocity;
      r32 Mass          = D->Mass;
      v3  ExternalForce = D->ExternalForce + V3(0,-10,0);

      timestep_progression TP = ForwardEuler(dt, Mass, Position, Velocity, ExternalForce, HeroForceEquation);

      v3 Dim = V3(GetWidth(&C->AABB), GetHeight(&C->AABB), GetDepth(&C->AABB));
      while( TP.DistanceTraveled < TP.DistanceToTravel )
      {
        aabb3f A0 = AABB3f( TP.P0, TP.P0+Dim );
        aabb3f A1 = AABB3f( TP.P1, TP.P1+Dim );
        aabb3f IntersectionBoundingBox = MergeAABB(A0, A1);

        v3 CollisionEnvelope = V3(1E-7,1E-7,0);
        list< aabb3f > PossibleCollisionAABBs = GetOverlappingWallTiles(Arena, TileMap, &IntersectionBoundingBox, CollisionEnvelope);
        v3 dR = TP.P1-TP.P0;
        collision_data CD = FindEarliestCollision( A0, PossibleCollisionAABBs, dR );

        if(CD.FirstCollision && (V3(0,1,0) * CD.CollisionNormal) > 0 )
        {
          CollisionPoint = TP.P0 + CD.CollisionDistance;
        }

        v3 NormalForce = V3(0,0,0);
        if(CD.Intersecting)
        {
          TP.DistanceTraveled = TP.DistanceToTravel;
          TP.P1.Y = CollisionPoint.Y;
          TP.Velocity.Y = 0;
        }else{
          TP.DistanceTraveled += Norm(TP.P1-TP.P0);
        }
      }

      Put(TP.P1,S);
      D->Velocity = TP.Velocity;
      D->ExternalForce = {};

      EndTemporaryMemory(TempMem);
#else


      temporary_memory TempMem = BeginTemporaryMemory( Arena );
      component_spatial*   S = E->SpatialComponent;
      component_collider*  C = E->ColliderComponent;
      component_dynamics*  D = E->DynamicsComponent;



      for(u32 IndexB = 0;  IndexB < World->NrEntities; ++IndexB )
      {
        entity* Eb = &World->Entities[IndexB];
        if( !( (E->id == 1 ) && (Eb->id== 2) ) )
        {
          continue;
        }

        component_spatial*   Sb = Eb->SpatialComponent;
        component_collider*  Cb = Eb->ColliderComponent;
        component_dynamics*  Db = Eb->DynamicsComponent;
        gjk_collision_result cr = GJKCollisionDetection( &S->ModelMatrix,  C->Mesh, &Sb->ModelMatrix, Cb->Mesh);
        if(cr.ContainsOrigin)
        {
          int a = 0;
        }
      }
      v3  Position      = GetPosition(S);
      v3  Velocity      = D->Velocity;
      r32 Mass          = D->Mass;
      v3  ExternalForce = D->ExternalForce + V3(0,-10,0);

      timestep_progression TP = ForwardEuler(dt, Mass, Position, Velocity, ExternalForce, GravityForceEquation);

      Put(TP.P1,S);
      D->Velocity = TP.Velocity;
      D->ExternalForce = {};

      EndTemporaryMemory( TempMem );
#endif
    }

//    if( E->Types & COMPONENT_TYPE_DYNAMICS )
//    {
//      // Dynamics Requires Spatial and Collision
//      Assert(E->SpatialComponent && E->ColliderComponent && E->DynamicsComponent);
//      v3 Position = V3(GetTranslationFromMatrix( E->SpatialComponent->ModelMatrix ));
//      v3& Velocity = E->DynamicsComponent->Velocity;
//      r32 Mass = E->DynamicsComponent->Mass;
//      timestep_progression timestep =  ForwardEuler( dt, Mass, Position, Velocity, V3(0,0,0), GravityForceEquation);
//      Translate(timestep.P1-timestep.P0,E->SpatialComponent);
//      Velocity = timestep.Velocity;
//    }
  }

  CheckArena(Arena);
}


#else


void SpatialSystemUpdate( world* World )
{
  r32 dt =  World->dtForFrame;
  local_persist v3 CollisionPoint = {};
  memory_arena* Arena = &World->Arena;
  temporary_memory TempMem = BeginTemporaryMemory( Arena );
  tile_map* TileMap = &World->TileMap;

  component_spatial*   S = 0;
  component_collider*  C = 0;
  component_dynamics*  D = 0;

  for(u32 Index = 0;  Index < World->NrEntities; ++Index )
  {
    entity* E = &World->Entities[Index];

    if( E->Types & COMPONENT_TYPE_DYNAMICS )
    {
      S = E->SpatialComponent;
      C = E->ColliderComponent;
      D = E->DynamicsComponent;
      break;
    }
  }

  v3  Position      = GetPosition(S);
  v3  Velocity      = D->Velocity;
  r32 Mass          = D->Mass;
  v3  ExternalForce = D->ExternalForce - V3(0,5,0);
  v3 Dim = V3(GetWidth(&C->AABB), GetHeight(&C->AABB), GetDepth(&C->AABB));

  timestep_progression TP = ForwardEuler(dt, Mass, Position, Velocity, ExternalForce, HeroForceEquation);

  aabb3f A0 = AABB3f( TP.P0, TP.P0+Dim );
  aabb3f A1 = AABB3f( TP.P1, TP.P1+Dim );
  aabb3f IntersectionBoundingBox = MergeAABB(A0, A1);

  v3 CollisionEnvelope = V3(1E-7,1E-7,0);
  list< aabb3f > PossibleCollisionAABBs = GetOverlappingWallTiles(Arena, TileMap, &IntersectionBoundingBox, CollisionEnvelope);

  v3 dR = TP.P1-TP.P0;

  r32 FirstHitPercentage = 1;
  r32 HitPercentage  = 0;
  v3  CollisionNormal = {};
  b32 IsIntersecting = false;

  for(PossibleCollisionAABBs.First();
     !PossibleCollisionAABBs.IsEnd();
      PossibleCollisionAABBs.Next())
  {
    aabb3f B = PossibleCollisionAABBs.Get();
    IsIntersecting = SweeptAABB(A0, B, dR, HitPercentage, CollisionNormal);
    if(IsIntersecting)
    {
      if( (HitPercentage < FirstHitPercentage) && (HitPercentage > 0) )
      {
        FirstHitPercentage = HitPercentage;
      }
    }
  }


  if(FirstHitPercentage > 0 && FirstHitPercentage < 1)
  {
    CollisionPoint = TP.P0 + dR * FirstHitPercentage;
	TP.P1 = CollisionPoint;
  }

  Put(TP.P1, S);
  D->Velocity = TP.Velocity;
  D->ExternalForce = {};

  EndTemporaryMemory(TempMem);

  CheckArena( Arena );
}
#endif