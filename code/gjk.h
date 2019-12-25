#pragma once

#include "aabb.h"

// Gilbert-Johnson-Keerthi (GJK) collision detection algorithm
// See: http://www.allenchou.net/2013/12/game-physics-collision-detection-gjk/
struct gjk_support
{
  v3 Support;
  v3 SupportA;
  v3 SupportB;
};

// Assumes ccw direction
union gjk_triangle
{
  struct
  {
    v3 P0,P1,P2;
  };
  v3 P[3];
};

union gjk_tetrahedron
{
  struct
  {
    v3 P0, P1, P2, P3;
  };
  v3 P[4];
};


gjk_support CsoSupportAABB( const m4* AModelMat, const m4* BModelMat, const v3 Direction )
{
  gjk_support Result = {};

  m4 AModelMatInv = RigidInverse(*AModelMat);
  m4 BModelMatInv = RigidInverse(*BModelMat);

  v4 ADirLocalSpace =   AModelMatInv * V4(Direction,0);
  v4 BDirLocalSpace = -(BModelMatInv * V4(Direction,0));

  v3 P0 = V3(-0.5f,-0.5f, -0.5f);
  v3 P1 = V3( 0.5f, 0.5f,  0.5f);
  aabb3f UnitCube = AABB3f(P0,P1);

  v3 UnitCubeVertices[8] = {};
  GetAABBVertices( &UnitCube, UnitCubeVertices);

  r32 AMaxValue = 0;
  r32 BMaxValue = 0;
  v4 SupportA = {};
  v4 SupportB = {};
  for(u32 i = 0; i < 8; ++i)
  {
    v4 Point =V4(UnitCubeVertices[i],1);
    r32 sa = ADirLocalSpace * Point;
    if(sa > AMaxValue)
    {
      AMaxValue = sa;
      SupportA = Point;
    }

    r32 sb = BDirLocalSpace * Point;
    if(sb > BMaxValue)
    {
      BMaxValue = sb;
      SupportB = Point;
    }
  }

  Result.SupportA = V3(*AModelMat * SupportA);
  Result.SupportB = V3(*BModelMat * SupportB);

  Result.Support = Result.SupportA - Result.SupportB;

  return Result;

}


v3 VertexEdge(const v3& Vertex, const v3& EedgePointA, const v3& EdgePointB)
{
  v3 o = EedgePointA;
  v3 d = EdgePointB;
  v3 u = Normalize(d-o);
  r32 uNorm = Norm(d-o);
  v3 v = Vertex;

  r32 ProjectionScalar = (v-o)*u;
  v3 ClosestPointOnEdge = {};
  if(ProjectionScalar <= 0)
  {
    ClosestPointOnEdge = o;
  }else if(ProjectionScalar >= uNorm )
  {
    ClosestPointOnEdge = d;
  }else{
    ClosestPointOnEdge = o + ProjectionScalar * u;
  }

  return ClosestPointOnEdge;
}

inline v3
GetTriangleNormal(const gjk_triangle& Triangle)
{
  v3 v1 = Triangle.P[1] - Triangle.P[0];
  v3 v2 = Triangle.P[2] - Triangle.P[1];
  v3 Result = Normalize( CrossProduct(v1,v2) );
  return Result;
}

// Checks if the Vertex can be projected onto the Triangle.
b32 IsVertexInsideTriangle(const v3& Vertex, const v3& Normal, const gjk_triangle& Triangle)
{
  for( u32 Index = 0;
           Index < 3;
         ++Index )
  {
    v3 v = Vertex - Triangle.P[Index];
    v3 e = Triangle.P[ (Index+1) % 3] - Triangle.P[Index];
    v3 x = CrossProduct(e,v);
    if( (x * Normal) < 0 )
    {
      return false;
    }
  }

  return true;
}

v3 VertexTriangle( const v3& Vertex, const gjk_triangle& Triangle)
{
  v3 Normal = GetTriangleNormal(Triangle);

  // Project the Vertex onto the plane spanned by the triangle
  v3 o = (Vertex - Triangle.P0);
  v3 ProjectionToNormal = (o * Normal) * Normal;
  v3 ProjectedPoint = Vertex - ProjectionToNormal;

  if( IsVertexInsideTriangle( ProjectedPoint, Normal, Triangle) )
  {
    return ProjectedPoint;
  }

  v3 ClosestPointOnTriangle = VertexEdge(Vertex, Triangle.P0, Triangle.P1);
  r32 ShortestContactDistance = Norm( ClosestPointOnTriangle - Vertex );

  for(u32 Index = 1; Index < 3; ++Index)
  {
    v3 ClosestPointCandidate = VertexEdge(Vertex, Triangle.P[Index], Triangle.P[(Index+1)%3]);
    r32 ContactDistance = Norm( ClosestPointCandidate - Vertex );
    if( ContactDistance < ShortestContactDistance )
    {
      ShortestContactDistance = ContactDistance;
      ClosestPointOnTriangle = ClosestPointCandidate;
    }
  }

  return ClosestPointOnTriangle;
}


inline void
GetTriangles(const gjk_tetrahedron& Tetrahedron, gjk_triangle Triangles[])
{
  Assert(ArrayCount(Triangles) == 4);
  Triangles[0] = {Tetrahedron.P0, Tetrahedron.P1, Tetrahedron.P2};
  Triangles[1] = {Tetrahedron.P0, Tetrahedron.P1, Tetrahedron.P3};
  Triangles[2] = {Tetrahedron.P1, Tetrahedron.P2, Tetrahedron.P3};
  Triangles[3] = {Tetrahedron.P2, Tetrahedron.P0, Tetrahedron.P3};
}

b32 IsVertexInsideTetrahedron( const v3& Vertex, const gjk_tetrahedron& Tetrahedron )
{
  gjk_triangle Triangles[4] = {};
  GetTriangles(Tetrahedron, Triangles);

  for(u32 Index = 0; Index < 4; ++Index)
  {
    const v3 Normal = GetTriangleNormal(Triangles[Index]);
    v3 TrianglePoint = Triangles[Index].P0;
    if(TrianglePoint*Normal > 0)
    {
      return false;
    }
  }
  return true;
}

struct gjk_partial_collision_data
{
  b32 ContainsOrigin;
  gjk_triangle Triangle;
  v3 Direction;
};

b32 GJKCollisionDetectionAABB( const m4* AModelMat, const m4* BModelMat, gjk_partial_collision_data& Data )
{
  const v3 Origin = {};
  gjk_triangle Triangle = Data.Triangle;
  const v3 Direction          = Data.Direction;

  gjk_tetrahedron Tetrahedron = {};

  gjk_support SupportPoints = CsoSupportAABB( AModelMat, BModelMat, Direction );
  Tetrahedron.P0 = Triangle.P0;
  Tetrahedron.P1 = Triangle.P1;
  Tetrahedron.P2 = Triangle.P2;
  Tetrahedron.P3 = SupportPoints.Support;

  if(IsVertexInsideTetrahedron(Origin, Tetrahedron))
  {
    Data.ContainsOrigin = true;
    return true;
  }

  // Remove the point furthest from the origin
  r32 Normal = 0;
  u32 FurthestPointIndex  = 0;
  for(u32 Index = 0; Index < 4; ++Index)
  {
    r32 NewNormal = Norm(Tetrahedron.P[Index]);
    if(NewNormal > Normal)
    {
      Normal = NewNormal;
      FurthestPointIndex = Index;
    }
  }

  Triangle.P0 = Tetrahedron.P[(FurthestPointIndex+1)%4];
  Triangle.P1 = Tetrahedron.P[(FurthestPointIndex+2)%4];
  Triangle.P2 = Tetrahedron.P[(FurthestPointIndex+3)%4];

  // Get a new Direction
  v3 PointOnTriangle = VertexTriangle(Origin, Triangle);
  v3 NewDirection  = Origin - PointOnTriangle;
  r32 NewDistance = Norm(NewDirection);

  if( NewDistance < 10E-7 )
  {
    Data.ContainsOrigin = true;
    return true;
  }

  if( NewDistance >= Norm(Direction) )
  {
    Data.ContainsOrigin = false;
    return true;
  }

  Data.ContainsOrigin = false;
  Data.Triangle       = Triangle;
  Data.Direction      = NewDirection;

  return false;
}

// Assumes the AABB in model space is the cube going from V3(-1/2,-1/2,0) to V3(1/2,1/2,0)
b32 GJKCollisionDetectionAABB( const m4* AModelMat, const m4* BModelMat)
{
  const v3 Origin = V3(0,0,0);

  gjk_triangle Triangle = {};

  gjk_support SupportPoints = CsoSupportAABB( AModelMat, BModelMat, V3(1,0,0) );
  Triangle.P0 = SupportPoints.Support;

  v3 FirstDirection = Origin - Triangle.P0;
  r32 FirstDistance = Norm(FirstDirection);
  if( FirstDistance < 10E-7 )
  {
    return true;
  }

  SupportPoints = CsoSupportAABB( AModelMat, BModelMat, FirstDirection );
  Triangle.P1 = SupportPoints.Support;

  v3 PointOnEdge = VertexEdge(Origin, Triangle.P0, Triangle.P1);
  v3 Direction  = Origin - PointOnEdge;
  r32 Distance = Norm(Direction);

  if( Distance < 10E-7 )
  {
    return true;
  }else if(Distance >= FirstDistance){
    return false;
  }

  FirstDirection = Direction;
  FirstDistance = Distance;
  SupportPoints = CsoSupportAABB( AModelMat, BModelMat, FirstDirection );
  Triangle.P2 = SupportPoints.Support;

  v3 PointOnTriangle = VertexTriangle(Origin, Triangle);
  Direction  = Origin - PointOnTriangle;
  Distance = Norm(Direction);

  if( Distance < 10E-7 )
  {
    return true;
  }else if(Distance >= FirstDistance){
    return false;
  }

  gjk_partial_collision_data GJKData = {};
  GJKData.Triangle  = Triangle;
  GJKData.Direction = Direction;
  b32 Stop = false;
  while(!Stop)
  {
    Stop = GJKCollisionDetectionAABB( AModelMat, BModelMat, GJKData);
  }

  return GJKData.ContainsOrigin;
}

