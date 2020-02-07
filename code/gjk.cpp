#include "gjk.h"
#include "halfedge_mesh.h"

internal gjk_support
CsoSupportAABB( const m4* AModelMat, const collider_mesh* AMesh,
            const m4* BModelMat, const collider_mesh* BMesh, const v3 Direction )
{
  gjk_support Result = {};

  v4 ADirModelSpace = RigidInverse(*AModelMat) * V4(Direction,0);
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

  v4 BDirModelSpace = (RigidInverse(*BModelMat) * V4(-Direction,0));
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
internal void
SetPointsAndIndecesForCCWTetrahedron( gjk_simplex* Simplex, u32 TriangleIndeces[])
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

internal void
BlowUpSimplex( const m4* AModelMat, const collider_mesh* AMesh,
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
        gjk_support Support = CsoSupportAABB( AModelMat, AMesh,
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

      // Find leas significant axis
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
        gjk_support Support = CsoSupportAABB( AModelMat, AMesh,
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
      const v3 v0 = Simplex.SP[1].S - Simplex.SP[0].S;
      const v3 v1 = Simplex.SP[2].S - Simplex.SP[0].S;
      const v3 SearchDirection = CrossProduct(v0,v1);

      gjk_support Support = CsoSupportAABB( AModelMat, AMesh,
                                            BModelMat, BMesh,
                                            SearchDirection );
      if(Norm(Support.S) <= 10E-4)
      {
        Support = CsoSupportAABB( AModelMat, AMesh,
                                  BModelMat, BMesh,
                                 -SearchDirection );
      }

      Simplex.SP[3] = Support;
      Simplex.Dimension = 4;
    }
  }
}

contact_data EPACollisionResolution(memory_arena* TemporaryArena, const m4* AModelMat, const collider_mesh* AMesh,
                                    const m4* BModelMat, const collider_mesh* BMesh,
                                    gjk_simplex& Simplex,
                                    platform_api* API )
{
  temporary_memory TempMem = BeginTemporaryMemory(TemporaryArena);

  BlowUpSimplex(AModelMat, AMesh,
                BModelMat, BMesh,
                Simplex);


  gjk_mesh* Mesh = CreateSimplexMesh(TemporaryArena, &Simplex);

  // Get the first new point
  r32 DistanceClosestToFace = 0;
  gjk_face* ClosestFace = GetCLosestFaceToOrigin( Mesh, &DistanceClosestToFace );
  Assert(ClosestFace);

  r32 PreviousDistanceClosestToFace = DistanceClosestToFace + 100;
  gjk_face* PreviousClosestFace = ClosestFace;
  u32 Tries = 0;
  // Note (Jakob): Somethimes the EPA does not converge on ONE solution and instead
  //               start to cycle between different points with some period.
  //               If that happens we want to exit out at the closest result
  //               within that period.
  //               TriesUntilGivingUp can be tweaked, lower number gives faster result,
  //               higher number gives more precise result (If the period is longer than
  //               TriesUntilGivingUp we may not find the closest point).
  //               However usually the EPA converges and if not has a period of 2.
  const u32 TriesUntilGivingUp = 2;


  while(Tries++ < TriesUntilGivingUp)
  {

    gjk_support SupportPoint = CsoSupportAABB( AModelMat, AMesh,
                                               BModelMat, BMesh,
                                               ClosestFace->Normal);

    if(IsPointInMesh(Mesh,SupportPoint.S))
    {
      // If a point gets repeated we use the previous closest face.
      // I can't be sure this always works so chekc here if we get
      // weird EPA results.
      ClosestFace = PreviousClosestFace;
      break;
    }

    gjk_halfedge* BorderEdge = RemoveFacesSeenByPoint(Mesh, SupportPoint.S);
    if(!BorderEdge)
    {
      // If BorderEdge is NULL it means the new SupportPoint must be on the border
      // of the CSO.
      // It could also be inside the Polytype but I don't think so.
      // If it's inside it means that there is a point further away in that direction.
      // But the the Support function should have returned that point instead.
      // The point SHOULD therefore be on the face of ClosestFace since that was the
      // direction we were las looking in.
      // So exit with closest face.
      break;
    }
    FillHole( Mesh, BorderEdge, &SupportPoint);

    PreviousDistanceClosestToFace = DistanceClosestToFace;
    PreviousClosestFace = ClosestFace;
    ClosestFace = GetCLosestFaceToOrigin( Mesh, &DistanceClosestToFace );

    if(Abs(DistanceClosestToFace - PreviousDistanceClosestToFace) > 10E-4)
    {
      Tries = 0;
    }
  }

  gjk_support A = ClosestFace->Edge->TargetVertex->P;
  gjk_support B = ClosestFace->Edge->NextEdge->TargetVertex->P;
  gjk_support C = ClosestFace->Edge->NextEdge->NextEdge->TargetVertex->P;
  v3 P = ClosestFace->Normal * DistanceClosestToFace;

  // The Length of the cross product between vectors A and B gives the area of the
  // paralellogram with sides by A and B.
  r32 FaceArea = 0.5f * Norm( CrossProduct( B.S - A.S, C.S - A.S) );
  r32 SubAreaA = 0.5f * Norm( CrossProduct( C.S - B.S,   P - B.S) );
  r32 SubAreaB = 0.5f * Norm( CrossProduct( A.S - C.S,   P - C.S) );
  r32 SubAreaC = 0.5f * Norm( CrossProduct( B.S - A.S,   P - A.S) );

  // The Baryocentric coordinates
  r32 LambdaA = SubAreaA / FaceArea;
  r32 LambdaB = SubAreaB / FaceArea;
  r32 LambdaC = SubAreaC / FaceArea;

  // Check that some baryocentric coordinate conditions hold.
  // IE that P is within triangle ABC;
  r32 LambdaSum = LambdaA + LambdaB + LambdaC;
  if((Abs( LambdaSum - 1) > 10E-7)       ||
     !( (LambdaA >= 0) && (LambdaA <= 1)) ||
     !( (LambdaB >= 0) && (LambdaB <= 1)) ||
     !( (LambdaC >= 0) && (LambdaC <= 1)))
  {
    DebugPrintEdges(TemporaryArena, Mesh, false, API );
    Assert(0);
  }

  gjk_support InterpolatedSupport = {};
  InterpolatedSupport.S = P;
  InterpolatedSupport.A = LambdaA * A.A + LambdaB * B.A + LambdaC * C.A;
  InterpolatedSupport.B = LambdaA * A.B + LambdaB * B.B + LambdaC * C.B;

  EndTemporaryMemory(TempMem);

  contact_data ContactData = {};

  v3 Tangent1 = V3(0.0f, ClosestFace->Normal.Z, -ClosestFace->Normal.Y);
  if ( ClosestFace->Normal.X >= 0.57735f)
  {
    Tangent1 = V3(ClosestFace->Normal.Y, -ClosestFace->Normal.X, 0.0f);
  }
  Normalize(Tangent1);

  ContactData.A_ContactWorldSpace = V3(*AModelMat * V4(InterpolatedSupport.A,1));
  ContactData.B_ContactWorldSpace = V3(*BModelMat * V4(InterpolatedSupport.B,1));
  ContactData.A_ContactModelSpace = InterpolatedSupport.A;
  ContactData.B_ContactModelSpace = InterpolatedSupport.B;
  ContactData.ContactNormal       = ClosestFace->Normal;
  ContactData.TangentNormalOne    = Tangent1;
  ContactData.TangentNormalTwo    = CrossProduct(ClosestFace->Normal, Tangent1);
  ContactData.PenetrationDepth    = DistanceClosestToFace;
  return ContactData;

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

void DEBUG_GJKCollisionDetectionSequenceToFile(gjk_simplex& Simplex, b32 append,
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

  if(Simplex.Dimension)
  {
    Scanner += str::itoa(-1, 64, Scanner);
    *Scanner++ = ' ';
    Scanner = PaddWithZeros(3, Scanner);
    *Scanner++ = '\n';
  }

  thread_context Thread = {};
  if(append)
  {
    API->DEBUGPlatformAppendToFile(&Thread, FilePath, str::StringLength(DebugString), (void*)DebugString);
  }else{
    API->DEBUGPlatformWriteEntireFile(&Thread, FilePath, str::StringLength(DebugString), (void*)DebugString);
  }

  EndTemporaryMemory(TempMem);
}

gjk_collision_result GJKCollisionDetection(const m4* AModelMat, const collider_mesh* AMesh,
                                           const m4* BModelMat, const collider_mesh* BMesh,
                                           memory_arena* TemporaryArena, platform_api* API )
{
  const v3 Origin = V3(0,0,0);
  gjk_collision_result Result = {};
  gjk_simplex& Simplex = Result.Simplex;

  gjk_partial_result PartialResult = {};
  PartialResult.ClosestPoint = V3(1,0,0);
  DEBUG_GJKCollisionDetectionSequenceToFile(Simplex, false, TemporaryArena, API);
  while(true)
  {
    r32 PreviousDistance = PartialResult.Distance;
    // Add a CSO to the simplex
    gjk_support SupportPoint = CsoSupportAABB( AModelMat, AMesh, BModelMat, BMesh, Origin-PartialResult.ClosestPoint );
    Assert(Simplex.Dimension <= 3);
    for(u32 i = 0; i < Simplex.Dimension; ++i)
    {
      if(SupportPoint.S == Simplex.SP[i].S)
      {
        // If we start adding duplicate points
        // we are not going to get closer to the origin.
        // Thus we return the Simplex
        return Result;
      }
    }

    Simplex.SP[Simplex.Dimension++] = SupportPoint;
    DEBUG_GJKCollisionDetectionSequenceToFile(Simplex, true, TemporaryArena, API);
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