#include "gjk_narrow_phase.h"
#include "epa_collision_data.h"
//#include "component_gjk_epa_visualizer.h"
#include "utility_macros.h"

gjk_support CsoSupportFunction( const m4* AModelMat, const collider_mesh* AMesh,
            const m4* BModelMat, const collider_mesh* BMesh, const v3 Direction )
{
  gjk_support Result = {};
  v3 NormalizedDirection = Normalize(Direction);

#if 0
  v4 ADirModelSpace =  RigidInverse(*AModelMat) * V4( NormalizedDirection, 0);
  v4 BDirModelSpace = (RigidInverse(*BModelMat) * V4(-NormalizedDirection, 0));
#else

  // This is needed when transforming covectors, Do we need it here?
  // Since what we pass into here is normals to planes and lines I think we need the transpose.
  v4 ADirModelSpace = RigidInverse(Transpose(RigidInverse(*AModelMat))) * V4( NormalizedDirection,0);
  v4 BDirModelSpace = RigidInverse(Transpose(RigidInverse(*BModelMat))) * V4(-NormalizedDirection,0);
#endif

  r32 AMaxValue     = 0;
  v4 SupportA       = {};
  for(u32 i = 0; i < AMesh->nv; ++i)
  {
    v4 ModelSpacePoint =V4(AMesh->v[i],1);
    r32 sa = ADirModelSpace * ModelSpacePoint;
    if(sa > AMaxValue)
    {
      AMaxValue = sa;
      SupportA = ModelSpacePoint;
    }
  }

  r32 BMaxValue     = 0;
  v4 SupportB       = {};
  for(u32 i = 0; i < BMesh->nv; ++i)
  {
    v4 ModelSpacePoint =V4(BMesh->v[i],1);
    r32 sb = BDirModelSpace * ModelSpacePoint;
    if(sb > BMaxValue)
    {
      BMaxValue = sb;
      SupportB  = ModelSpacePoint;
    }
  }

  Result.A = V3(SupportA);
  Result.B = V3(SupportB);
  Result.S =  V3(*AModelMat * SupportA) - V3(*BModelMat * SupportB);

  return Result;

}

void BlowUpSimplex( const m4* AModelMat, const collider_mesh* AMesh,
                    const m4* BModelMat, const collider_mesh* BMesh,
                    gjk_simplex& Simplex)
{
  switch(Simplex.Dimension)
  {
    // Fall-through intended
    case 1:
    {
      local_persist const v3 SearchDirections[6] = {
        V3(-1,0,0), V3(1,0,0),
        V3(0,-1,0), V3(0,1,0),
        V3(0,0,-1), V3(0,0,1)};

      for (int i = 0; i < 6; ++i)
      {
        gjk_support Support = CsoSupportFunction( AModelMat, AMesh,
                                              BModelMat, BMesh,
                                              SearchDirections[i] );
        if(Norm(Support.S - Simplex.SP[0].S) >= 10E-4)
        {
          Simplex.SP[1] = Support;
          Simplex.Dimension = 2;
          break;
        }
      }
    }
    case 2:
    {
      local_persist const v3 PrincipalAxis[3] = {
        V3(1,0,0),
        V3(0,1,0),
        V3(0,0,1)};

      const v3 LineVector = Simplex.SP[1].S - Simplex.SP[0].S;

      // Find least significant axis
      r32 LineVecPart = Abs(LineVector.X);
      u32 LineVecAxis = 0;
      if(LineVecPart > Abs(LineVector.Y))
      {
        LineVecPart = Abs(LineVector.Y);
        LineVecAxis = 1;
      }
      if(LineVecPart > Abs(LineVector.Z))
      {
        LineVecPart = Abs(LineVector.Z);
        LineVecAxis = 2;
      }

      v3 SearchDirection = CrossProduct(LineVector, PrincipalAxis[LineVecAxis]);

      const m4 RotMat = GetRotationMatrix( Pi32/3, V4(LineVector,0) );

      for(u32 i = 0; i < 6; ++i)
      {
        gjk_support Support = CsoSupportFunction( AModelMat, AMesh,
                                              BModelMat, BMesh,
                                              SearchDirection );
        if(Norm(Support.S) >= 10E-4)
        {
          Simplex.SP[2] = Support;
          Simplex.Dimension = 3;
          break;
        }
        SearchDirection = V3(RotMat * V4(SearchDirection,0));
      }

    }
    case 3:
    {
      const v3 p0 = Simplex.SP[1].S - Simplex.SP[0].S;
      const v3 p1 = Simplex.SP[2].S - Simplex.SP[0].S;
      const v3 SearchDirection = CrossProduct(p0,p1);

      gjk_support Support = CsoSupportFunction( AModelMat, AMesh,
                                            BModelMat, BMesh,
                                            SearchDirection );
      const v3 p2 = Support.S - Simplex.SP[0].S;

      r32 Det = p2 * CrossProduct(p0,p1);
      if(Det <= 10E-4)
      {
        Support = CsoSupportFunction( AModelMat, AMesh,
                                  BModelMat, BMesh,
                                 -SearchDirection );
      }

      Simplex.SP[3] = Support;
      Simplex.Dimension = 4;
    }
  }
}

struct gjk_partial_result
{
  v3  ClosestPoint;
  r32 Distance;
  gjk_simplex ReducedSimplex;
  b32 Reduced;
};

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

// See if the Vertex (Origin) can be projected onto a triangle. Otherwise reduce to a line (or point).
internal gjk_partial_result
VertexTriangle( const v3& Vertex, gjk_simplex& Simplex, const u32 Index0 = 0, const u32 Index1 = 1, const u32 Index2 = 2)
{
  gjk_partial_result Result = {};

  const v3 Triangle[3]    = {Simplex.SP[Index0].S, Simplex.SP[Index1].S, Simplex.SP[Index2].S};
  const v3 Normal         = GetPlaneNormal(Triangle[0],Triangle[1],Triangle[2]);
  const v3 ProjectedPoint = ProjectPointOntoPlane( Vertex, Triangle[Index0], Normal );

  if( IsVertexInsideTriangle( ProjectedPoint, Normal, Triangle) )
  {
    Result.ClosestPoint = ProjectedPoint;
    Result.Distance = Norm(ProjectedPoint);
    return Result;
  }
  // Check the edges of the triangle. (Reduce)
  const u32 EdgeIndeces[6] = {Index0,Index1,
                              Index1,Index2,
                              Index2,Index0};
  Result = VertexEdge( Vertex, Simplex, EdgeIndeces[0], EdgeIndeces[1]);
  u32 EdgeIndex0 = EdgeIndeces[0];
  u32 EdgeIndex1 = EdgeIndeces[1];
  for(u32 Index = 1; Index < 3; ++Index)
  {
    const u32 BaseIndex = Index*2;
    const gjk_partial_result ResultCandidate = VertexEdge( Vertex, Simplex, EdgeIndeces[BaseIndex], EdgeIndeces[BaseIndex+1] );

    if(ResultCandidate.Distance < Result.Distance)
    {
      EdgeIndex0 = EdgeIndeces[BaseIndex];
      EdgeIndex1 = EdgeIndeces[BaseIndex+1];
      Result = ResultCandidate;
    }
  }

  if(!Result.Reduced)
  {
    // If we did not reduce to a Point in VertexEdge we reduce to a line here.
    Result.Reduced = true;
    Result.ReducedSimplex = {};
    Result.ReducedSimplex.Dimension = 2;
    Result.ReducedSimplex.SP[0] = Simplex.SP[EdgeIndex0];
    Result.ReducedSimplex.SP[1] = Simplex.SP[EdgeIndex1];
  }else{
    // Make sure we reduced to a point
    Assert(Result.Reduced);
    Assert(Result.ReducedSimplex.Dimension == 1);
  }

  return Result;
}

internal b32
IsVertexInsideTetrahedron( const v3& Vertex, const u32 TriangleIndeces[], const gjk_simplex& Simplex )
{
  for(u32 Index = 0; Index < 4; ++Index)
  {
    const u32 BaseIndex = Index*3;

    const u32 idx0 = TriangleIndeces[BaseIndex];
    const u32 idx1 = TriangleIndeces[BaseIndex+1];
    const u32 idx2 = TriangleIndeces[BaseIndex+2];

    const v3 Triangle[3] = {Simplex.SP[idx0].S, Simplex.SP[idx1].S, Simplex.SP[idx2].S};
    const v3 Normal = GetPlaneNormal(Triangle[0],Triangle[1],Triangle[2]);
    const v3 TriangleToVertex =  Vertex - Triangle[0];
    if( TriangleToVertex*Normal > 0)
    {
      return false;
    }
  }
  return true;
}


/* A Tetrahedron is made up of 4 Points assembling into 4 Triangles.
 * The normals of the triangles should point outwards.
 * This function Fixes the winding of the tetrahedron such that
 * Dot( v03, Cross( v01, v02 )) < 0; (Tripple Product)
 * Or in words: Define the bottom triangle to be {p0,p1,p2} with Normal : (p1-p0) x (p2-p0).
 * The Tetrahedron is then considered wound CCW if Dot( p3-p0 , Normal ) < 0.
 * If Dot( p3-p0 , Normal ) == 0 Our p3 lies on the plane of the bottom triangle and
 * we need another point to  be p3.
 */

bool FixWindingCCW(gjk_simplex* Simplex)
{
  // Fix Winding so that all triangles go ccw
  v3 v01 = Simplex->SP[1].S - Simplex->SP[0].S;
  v3 v02 = Simplex->SP[2].S - Simplex->SP[0].S;
  v3 v03 = Simplex->SP[3].S - Simplex->SP[0].S;
  const r32 Determinant = v03 * CrossProduct(v01,v02);
  if(Determinant > 0.f)
  {
    gjk_support Tmp = Simplex->SP[0];
    Simplex->SP[0]   = Simplex->SP[3];
    Simplex->SP[3]   = Tmp;
  }else if (Abs(Determinant)< 10E-7){
    // Determinant is 0
    return false;
  }

  return true;
}

// Makes Simplex CCW
void SetPointsAndIndecesForCCWTetrahedron( gjk_simplex* Simplex, u32 TriangleIndeces[])
{
  Assert(Simplex->Dimension == 4);
  b32 ZeroDet = !FixWindingCCW(Simplex);
  Assert(!ZeroDet);
  u32 TI[12] = {0, 1, 2,   // n =  (p1 - p0) x (p2 - p0)
                1, 3, 2,   // n = -(p2 - p1) x (p3 - p1) = (p3 - p1) x (p2 - p1)
                2, 3, 0,   // n =  (p3 - p2) x (p0 - p2)
                3, 1, 0};  // n = -(p0 - p3) x (p1 - p3) = (p1 - p3) x (p0 - p3)
  u32* TO    = TriangleIndeces;
  u32* TIptr = TI;
  for(u32 i = 0; i < ArrayCount(TI); ++i)
  {
    *TO = *TIptr;
    TO++;
    TIptr++;
  }
}

internal gjk_partial_result
VertexTetrahedron( const v3& Vertex, gjk_simplex& Simplex )
{
  gjk_partial_result Result = {};

  u32 TriangleIndeces[12] = {};
  SetPointsAndIndecesForCCWTetrahedron(&Simplex, TriangleIndeces);

  if( IsVertexInsideTetrahedron( Vertex, TriangleIndeces,  Simplex) )
  {
    Result.ClosestPoint = Vertex;
    Result.Distance = Norm(Vertex);
    return Result;
  }


  Result = VertexTriangle( Vertex, Simplex, TriangleIndeces[0], TriangleIndeces[1], TriangleIndeces[2]);
  u32 TriangleIndex0 = TriangleIndeces[0];
  u32 TriangleIndex1 = TriangleIndeces[1];
  u32 TriangleIndex2 = TriangleIndeces[2];
  for(u32 Index = 1; Index < 4; ++Index)
  {
    u32 BaseIndex = 3*Index;
    const gjk_partial_result ResultCandidate =
                            VertexTriangle( Vertex, Simplex,
                            TriangleIndeces[BaseIndex],
                            TriangleIndeces[BaseIndex+1],
                            TriangleIndeces[BaseIndex+2]);

    if(ResultCandidate.Distance < Result.Distance)
    {
      TriangleIndex0 = TriangleIndeces[BaseIndex];
      TriangleIndex1 = TriangleIndeces[BaseIndex+1];
      TriangleIndex2 = TriangleIndeces[BaseIndex+2];
      Result = ResultCandidate;
    }
  }
  if(!Result.Reduced)
  {
    // If we did not reduce to a Line or Point in VertexTriangle we reduce to a Triangle here.
    Result.Reduced = true;
    Result.ReducedSimplex = {};
    Result.ReducedSimplex.Dimension = 3;
    Result.ReducedSimplex.SP[0] = Simplex.SP[TriangleIndex0];
    Result.ReducedSimplex.SP[1] = Simplex.SP[TriangleIndex1];
    Result.ReducedSimplex.SP[2] = Simplex.SP[TriangleIndex2];
  }

  return Result;
}


internal b32
IsPointOnSimplexSurface(const gjk_simplex& Simplex, const v3& Point)
{
  r32 Tol = 10E-7;
  switch(Simplex.Dimension)
  {
    case 1:
    {
      // Check if new point is the same as old point
      return Point == Simplex.SP[0].S;
    }break;
    case 2:
    {
      // Check if new point lies on the line
      const v3& v0 = Simplex.SP[1].S - Simplex.SP[0].S;
      const v3& v1 = Point - Simplex.SP[0].S;
      const v3& cross = CrossProduct(v0,v1);
      const r32 lenSq = (cross.X*cross.X + cross.Y*cross.Y + cross.Z*cross.X);
      return Abs(lenSq) < Tol;
    }break;
    case 3:
    {
      // Check if new point lies on face
      const v3& v0 = Simplex.SP[1].S - Simplex.SP[0].S;
      const v3& v1 = Simplex.SP[2].S - Simplex.SP[0].S;
      const v3& v2 = Point - Simplex.SP[0].S;
      const r32 det = v2 * CrossProduct(v0,v1);
      return Abs(det) < Tol;
    }break;
  }

  return false;
}

void DEBUG_GJKCollisionDetectionSequenceToFile(gjk_simplex& Simplex,
                                               gjk_partial_result* Partial,
                                               b32 append,
                                               memory_arena* TemporaryArena,
                                               platform_api* API)
{
  if(!API || !TemporaryArena) return;

  char FilePath[] = "..\\handmade\\code\\matlab\\data\\GJKSimplexSeries.m";
  temporary_memory TempMem = BeginTemporaryMemory(TemporaryArena);
  char* DebugString = (char*) PushArray(TemporaryArena, (Simplex.Dimension+2)*64, char);
  char* Scanner = DebugString;

  for (u32 i = 0; i < Simplex.Dimension; ++i)
  {
    gjk_support SP = Simplex.SP[i];
    Scanner += str::itoa(i+1, 64, Scanner);
    *Scanner++ = ' ';
    Scanner += str::ToString(SP.S, 4, 64, Scanner);
    *Scanner++ = '\n';
  }

  if(Partial)
  {
    Scanner += str::itoa(Simplex.Dimension+1, 64, Scanner);
    *Scanner++ = ' ';
    Scanner += str::ToString(Partial->ClosestPoint, 4, 64, Scanner);
  }else{
    Scanner += str::itoa(Simplex.Dimension+1, 64, Scanner);
    *Scanner++ = ' ';
    Scanner += str::ToString(V3(0,0,0), 4, 64, Scanner);
  }

  thread_context Thread = {};
  if(append)
  {
    if(Simplex.Dimension)
    {
      Scanner += str::itoa(-1, 64, Scanner);
      *Scanner++ = ' ';
      Scanner = PaddWithZeros(3, Scanner);
      *Scanner++ = '\n';
    }
    API->DEBUGPlatformAppendToFile(&Thread, FilePath, str::StringLength(DebugString), (void*)DebugString);
  }else{
    API->DEBUGPlatformWriteEntireFile(&Thread, FilePath, str::StringLength(DebugString), (void*)DebugString);
  }

  EndTemporaryMemory(TempMem);
}

void RecordGJKFrame( component_gjk_epa_visualizer* Vis, gjk_simplex* Simplex, const v3& ClosestPointOnSurface )
{
  if(!Vis || !Vis->TriggerRecord) return;
  Assert(Vis->IndexCount < ArrayCount(Vis->Indeces));
  Assert(Vis->VertexCount < ArrayCount(Vis->Vertices));

  simplex_index* SI = &Vis->Simplex[Vis->NrFrames++];
  SI->ClosestPoint = ClosestPointOnSurface;
  SI->Offset = Vis->IndexCount;
  SI->Length = Simplex->Dimension;
  for (u32 i = 0; i < SI->Length; ++i)
  {
    v3& Vertex = Simplex->SP[i].S;
    b32 Unique = true;
    u32 VerticeIndex = 0;
    while(VerticeIndex < Vis->VertexCount)
    {
      if(Vertex == Vis->Vertices[VerticeIndex])
      {
        Unique = false;
        break;
      }
      VerticeIndex++;
    }

    if(Unique)
    {
      Assert(ArrayCount(Vis->Vertices) > VerticeIndex);
      Vis->Vertices[Vis->VertexCount++] = Vertex;
    }
    Vis->Indeces[SI->Offset+i] = VerticeIndex;
  }

  // Expand the 4 vertices to 4 triangles
  if(SI->Length==4)
  {
    u32 VerticeIdx[4] = {};
    utils::Copy(sizeof(VerticeIdx), &Vis->Indeces[SI->Offset], VerticeIdx);

    // Fix Winding so that all triangles go ccw
    v3 v01 = Vis->Vertices[VerticeIdx[1]] - Vis->Vertices[VerticeIdx[0]];
    v3 v02 = Vis->Vertices[VerticeIdx[2]] - Vis->Vertices[VerticeIdx[0]];
    v3 v03 = Vis->Vertices[VerticeIdx[3]] - Vis->Vertices[VerticeIdx[0]];
    const r32 Determinant = v03 * CrossProduct(v01,v02);
    Assert(Abs(Determinant) > 10E-7);
    if(Determinant > 0.f)
    {
      // Swap first and third Support
      u32 Tmp = VerticeIdx[0];
      VerticeIdx[0] = VerticeIdx[3];
      VerticeIdx[3] = Tmp;
    }

    u32 CCWSimplexIdx[12] = {};
    // 0,1,2 is a outwards facing triangle
    CCWSimplexIdx[ 0] = VerticeIdx[0];
    CCWSimplexIdx[ 1] = VerticeIdx[1];
    CCWSimplexIdx[ 2] = VerticeIdx[2];

    // All the other triangles must go around the
    // first triangle with 3rd index in the middle
    CCWSimplexIdx[ 3] = VerticeIdx[0];
    CCWSimplexIdx[ 4] = VerticeIdx[3];
    CCWSimplexIdx[ 5] = VerticeIdx[1];

    CCWSimplexIdx[ 6] = VerticeIdx[1];
    CCWSimplexIdx[ 7] = VerticeIdx[3];
    CCWSimplexIdx[ 8] = VerticeIdx[2];

    CCWSimplexIdx[ 9] = VerticeIdx[2];
    CCWSimplexIdx[10] = VerticeIdx[3];
    CCWSimplexIdx[11] = VerticeIdx[0];

    utils::Copy(sizeof(CCWSimplexIdx), CCWSimplexIdx, &Vis->Indeces[SI->Offset]);
    SI->Length=12;
  }

  Vis->IndexCount = SI->Offset + SI->Length;
}

gjk_collision_result GJKCollisionDetection(const m4* AModelMat, const collider_mesh* AMesh,
                                           const m4* BModelMat, const collider_mesh* BMesh,
                                           component_gjk_epa_visualizer* Vis)
{
  b32 ShouldRecord = Vis && Vis->TriggerRecord;
  const v3 Origin = V3(0,0,0);
  gjk_collision_result Result = {};
  gjk_simplex& Simplex = Result.Simplex;

  gjk_partial_result PartialResult = {};
  PartialResult.ClosestPoint = V3(1,0,0);
  while(true)
  {
    r32 PreviousDistance = PartialResult.Distance;
    // Add a CSO to the simplex
    gjk_support SupportPoint = CsoSupportFunction( AModelMat, AMesh, BModelMat, BMesh, Origin-PartialResult.ClosestPoint );
    Assert(Simplex.Dimension <= 3);
    if(IsPointOnSimplexSurface(Simplex, SupportPoint.S))
    {
      // If we start adding degenerate points
      // we are not going to get closer to the origin.
      // Thus we return the Simplex
      return Result;
    }
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

    if(ShouldRecord)
    {
      RecordGJKFrame(Vis, &Simplex, PartialResult.ClosestPoint);
    }

    if(PartialResult.Reduced)
    {
      Simplex = PartialResult.ReducedSimplex;
    }

    Result.ContainsOrigin = (PartialResult.Distance < 10E-7);

    if( Result.ContainsOrigin || (PartialResult.Distance >= PreviousDistance))
    {
      if(Vis)
      {
        Vis->TriggerRecord = false;
      }
      return Result;
    }
  }

  INVALID_CODE_PATH
  return Result;
}