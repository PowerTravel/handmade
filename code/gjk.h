#pragma once

#include "aabb.h"

// Gilbert-Johnson-Keerthi (GJK) collision detection algorithm
// See: http://www.allenchou.net/2013/12/game-physics-collision-detection-gjk/
struct gjk_support
{
  v3 S; // Combined Support
  v3 A; // Support for A
  v3 B; // Support for B
};

struct gjk_simplex
{
  u32 Dimension;
  gjk_support SP[4];
};

struct gjk_collision_result
{
  gjk_simplex Simplex;
  b32 ContainsOrigin;
};

struct gjk_partial_result
{
  v3  ClosestPoint;
  r32 Distance;
  gjk_simplex ReducedSimplex;
  b32 Reduced;
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

  Result.A = V3(*AModelMat * SupportA);
  Result.B = V3(*BModelMat * SupportB);

  Result.S = Result.A - Result.B;

  return Result;

}

internal gjk_partial_result
VertexEdge(const v3& Vertex, gjk_simplex& Simplex, u32 Index0 = 0, u32 Index1 = 1)
{
  gjk_partial_result Result = {};

  const v3 o = Simplex.SP[Index0].S;
  const v3 d = Simplex.SP[Index1].S;
  const v3 u = Normalize(d-o);
  const r32 uNorm = Norm(d-o);
  const v3 v = Vertex;

  const r32 ProjectionScalar = (v-o)*u;

  v3 ClosestPointOnEdge = {};
  if(ProjectionScalar <= 0)
  {
    ClosestPointOnEdge = o;
    Result.ReducedSimplex.SP[0] = Simplex.SP[Index0];
    Result.ReducedSimplex.Dimension = 1;
    Result.Reduced = true;
  }else if(ProjectionScalar >= uNorm )
  {
    ClosestPointOnEdge = d;
    Result.ReducedSimplex.SP[0] = Simplex.SP[Index1];
    Result.ReducedSimplex.Dimension = 1;
    Result.Reduced = true;
  }else{
    ClosestPointOnEdge = o + ProjectionScalar * u;
  }

  Result.ClosestPoint = ClosestPointOnEdge;
  Result.Distance = Norm(Result.ClosestPoint);

  return Result;
}

internal inline v3
GetTriangleNormal(const v3 Triangle[])
{
  const v3 v1 = Triangle[1] - Triangle[0];
  const v3 v2 = Triangle[2] - Triangle[1];
  const v3 Result = Normalize( CrossProduct(v1,v2) );
  return Result;
}

// Checks if the Vertex can be projected onto the Triangle.
internal inline b32
IsVertexInsideTriangle(const v3& Vertex, const v3& Normal, const v3 Triangle[])
{
  for( u32 Index = 0;
           Index < 3;
         ++Index )
  {
    const v3 v = Vertex - Triangle[Index];
    const v3 e = Triangle[ (Index+1) % 3] - Triangle[Index];
    const v3 x = CrossProduct(e,v);
    if( (x * Normal) < 0 )
    {
      return false;
    }
  }
  return true;
}

internal gjk_partial_result
VertexTriangle( const v3& Vertex, gjk_simplex& Simplex, const u32 Index0 = 0, const u32 Index1 = 1, const u32 Index2 = 2)
{
  gjk_partial_result Result = {};

  const v3  Triangle[3] = {Simplex.SP[Index0].S, Simplex.SP[Index1].S, Simplex.SP[Index2].S};
  const v3  Normal = GetTriangleNormal(Triangle);

  // Project the Vertex onto the plane spanned by the triangle
  const v3 o = (Vertex - Triangle[Index0]);
  const v3 ProjectionToNormal = (o * Normal) * Normal;
  const v3 ProjectedPoint = Vertex - ProjectionToNormal;

  if( IsVertexInsideTriangle( ProjectedPoint, Normal, Triangle) )
  {
    Result.ClosestPoint = ProjectedPoint;
    Result.Distance = Norm(ProjectedPoint);
    return Result;
  }

  const u32 EdgeIndeces[6] = {Index0,Index1,
                              Index1,Index2,
                              Index2,Index0};
  Result = VertexEdge( Vertex, Simplex,  EdgeIndeces[0], EdgeIndeces[1]);
  for(u32 Index = 1; Index < 2; ++Index)
  {
    const u32 BaseIndex = Index*2;
    const gjk_partial_result ResultCandidate = VertexEdge( Vertex, Simplex, EdgeIndeces[BaseIndex], EdgeIndeces[BaseIndex+1] );
    if(ResultCandidate.Distance < Result.Distance)
    {
      Result = ResultCandidate;
    }
  }

  return Result;
}

internal b32
IsVertexInsideTetrahedron( const v3& Vertex, const v3 Tetrahedron[] )
{
  const v3 Triangles[12] = {Tetrahedron[0], Tetrahedron[1], Tetrahedron[2],
                            Tetrahedron[0], Tetrahedron[1], Tetrahedron[3],
                            Tetrahedron[1], Tetrahedron[2], Tetrahedron[3],
                            Tetrahedron[2], Tetrahedron[0], Tetrahedron[3]};

  for(u32 Index = 0; Index < 4; ++Index)
  {
    const u32 BaseIndex = Index*3;
    const v3 Triangle[3] = {Triangles[BaseIndex],Triangles[BaseIndex + 1],Triangles[BaseIndex + 2]};
    const v3 Normal = GetTriangleNormal(Triangle);
    const v3 TriangleToVertex =  Vertex - Triangle[0];
    if( TriangleToVertex*Normal > 0)
    {
      return false;
    }
  }
  return true;
}

internal gjk_partial_result
VertexTetrahedron( const v3& Vertex, gjk_simplex& Simplex )
{
  gjk_partial_result Result = {};

  const v3 Tetrahedron[4] = {Simplex.SP[0].S, Simplex.SP[1].S, Simplex.SP[2].S, Simplex.SP[3].S};

  if( IsVertexInsideTetrahedron( Vertex, Tetrahedron) )
  {
    Result.ClosestPoint = Vertex;
    Result.Distance = Norm(Vertex);
    return Result;
  }

  const u32 TriangleIndeces[12] = {0, 1, 2,
                                  0, 1, 3,
                                  1, 2, 3,
                                  2, 0, 3};

  Result = VertexTriangle( Vertex, Simplex, TriangleIndeces[0], TriangleIndeces[1], TriangleIndeces[2]);

  for(u32 Index = 1; Index < 3; ++Index)
  {
    u32 BaseIndex = 3*Index;
    const gjk_partial_result ResultCandidate =
                            VertexTriangle( Vertex, Simplex,
                            TriangleIndeces[BaseIndex],
                            TriangleIndeces[BaseIndex+1],
                            TriangleIndeces[BaseIndex+2]);

    if(ResultCandidate.Distance < Result.Distance)
    {
      Result = ResultCandidate;
    }
  }
  return Result;
}

// Assumes the AABB in model space is the cube going from V3(-1/2,-1/2,-1/2) to V3(1/2,1/2,1/2)
gjk_collision_result GJKCollisionDetectionAABB( const m4* AModelMat, const m4* BModelMat)
{
  const v3 Origin = V3(0,0,0);
  gjk_collision_result Result = {};
  gjk_simplex& Simplex = Result.Simplex;

  gjk_partial_result PartialResult = {};
  PartialResult.ClosestPoint = V3(1,0,0);

  while(true)
  {
    r32 PreviousDistance = PartialResult.Distance;
    // Add a CSO to the simplex
    gjk_support SupportPoint = CsoSupportAABB( AModelMat, BModelMat,  Origin-PartialResult.ClosestPoint );
    Simplex.SP[Simplex.Dimension++] = SupportPoint;

    switch(Simplex.Dimension)
    {
      case 1:
      {
        PartialResult.ClosestPoint = Simplex.SP[0].S;
        PartialResult.Distance = Norm(PartialResult.ClosestPoint);
        PreviousDistance = PartialResult.Distance+1;
        PartialResult.ReducedSimplex = {};
        PartialResult.Reduced = false;
      }break;
      case 2:
      {
        PartialResult = VertexEdge(Origin, Simplex);
      }break;
      case 3:
      {
        PartialResult = VertexTriangle(Origin, Simplex);
      }break;
      case 4:
      {
        PartialResult = VertexTetrahedron(Origin, Simplex);
      }break;
      default:
      {
        INVALID_CODE_PATH
      }
    }

    if(PartialResult.Reduced)
    {
      Simplex = PartialResult.ReducedSimplex;
    }

    if( PartialResult.Distance < 10E-7 )
    {
      Result.ContainsOrigin = true;
      return Result;
    }else if( PartialResult.Distance >= PreviousDistance){
      return Result;
    }
  }

  INVALID_CODE_PATH
  return Result;
}

