#pragma once

#include "aabb.h"
#include "data_containers.h"

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

struct contact_data
{
  v3 A_ContactWorldSpace;
  v3 B_ContactWorldSpace;
  v3 A_ContactModelSpace;
  v3 B_ContactModelSpace;
  v3 ContactNormal;
  v3 TangentNormalOne;
  v3 TangentNormalTwo;
  r32 PenetrationDepth;
};

struct gjk_halfedge;
struct gjk_face;

struct gjk_vertex
{
  gjk_support P;
  u32 Idx;
  gjk_halfedge* OutgoingEdge;
};

struct gjk_halfedge
{
  gjk_vertex*   TargetVertex;
  gjk_face*     LeftFace;
  gjk_halfedge* NextEdge;
  gjk_halfedge* PreviousEdge;
  gjk_halfedge* OppositeEdge;
};

struct gjk_face
{
  gjk_halfedge* Edge;
};

/* A Tetrahedron is made up of 4 Points assembling into 4 Triangles.
 * The normals of the triangles should point outwards.
 * This function Fixes the winding of the tetrahedron such that
 * Dot( v03, Cross( v01, v02 )) < 0; (Tripple Product)
 * Or in words: Define the bottom triangle to be {p0,p1,p2} with Normal : (p1-p0) x (p2-p0).
 * The Tetrahedron is then considered wound CCW if Dot( p3-p0 , Normal ) < 0.
 * If Dot( p3-p0 , Normal ) == 0 Our p3 lies on the plane of the bottom triangle and
 * we need another point to  be p3.
 */
bool FixWindingCCW(gjk_simplex& Simplex)
{
  // Fix Winding so that all triangles go ccw
  v3 v01 = Simplex.SP[1].S - Simplex.SP[0].S;
  v3 v02 = Simplex.SP[2].S - Simplex.SP[0].S;
  v3 v03 = Simplex.SP[3].S - Simplex.SP[0].S;
  const r32 Determinant = v03 * CrossProduct(v01,v02);
  if(Determinant > 0.f)
  {
    gjk_support Tmp = Simplex.SP[0];
    Simplex.SP[0]   = Simplex.SP[3];
    Simplex.SP[3]   = Tmp;
  }else if (Abs(Determinant < 10E-7)){
    // Determinant is 0
    return false;
  }

  return true;
}

// Makes Simplex CCW
internal void
SetPointsAndIndecesForCCWTetrahedron( gjk_simplex& Simplex, u32 TriangleIndeces[])
{
  Assert(Simplex.Dimension == 4);
  b32 ZeroDet = !FixWindingCCW(Simplex);
  if(ZeroDet)
  {
    // TODO:
    // This means the tetrahedron is a plane
    // Figure out how to deal with this if it happens.
    Assert(0);
  }
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
/*                   P3                    f1: {P0, P1, P2}  CCW
 *                  /|\                    f2: {P1, P3, P2}  CCW
 *                 / | \                   f3: {P2, P3, P0}  CCW
 *                /  |  \                  f4: {P3, P1, P0}  CCW
 *               /   |   \
 *              /    |  -------f1
 *             /     |     \
 *            /      |      \
 *     f3----/---    |   ----\----f2
 *          /        |        \
 *         /        _|P0       \
 *        /       _/   \_       \
 *       /     _/         \_     \
 *      /   _/               \_   \
 *     / _/         f1          \_ \
 *  P2//__________________________\_\ P1
 */




// Split up the creation of faces and the connection into these two steps.

// Step one: (All data is created and allocated at this step)
//  Create one or more faces.
//    A face is created by giving 3 CCW ordered Points. Out comes three half edges and one face.
//  CreateFace(A, B, C) gives  A -> E0 -> B -> E1 -> C -> E2 -> A

// Step two: (Edges are updated in this step)
// Join the faces together by 'attaching' the half edges.

// Step three: (can be squished into step two):
// Walk around the edges and joint the neighbouring edges together creating a closed surface.


internal inline v3
GetPlaneNormal(const v3 PointsOnPlane[])
{
  const v3 v1 = PointsOnPlane[1] - PointsOnPlane[0];
  const v3 v2 = PointsOnPlane[2] - PointsOnPlane[0];
  const v3 Result = Normalize( CrossProduct(v1,v2) );
  return Result;
}

internal inline v3
GetFaceNormal(const gjk_face& Face)
{
  const v3 Triangle[3] = {Face.Edge->PreviousEdge->TargetVertex->P.S,
                          Face.Edge->TargetVertex->P.S,
                          Face.Edge->NextEdge->TargetVertex->P.S};
  return GetPlaneNormal(Triangle);
}

internal v3 ProjectPointOntoPlane( const v3& Vertex, const v3& PointOnPlane, const v3& Normal )
{
  // Project the Vertex onto the plane spanned by the triangle
  const v3 o = (Vertex - PointOnPlane);
  const v3 ProjectionToNormal = (o * Normal) * Normal;
  const v3 ProjectedPoint = Vertex - ProjectionToNormal;
  return ProjectedPoint;
}

gjk_face CreateFace(memory_arena* Arena, gjk_vertex* A, gjk_vertex* B, gjk_vertex* C)
{
  gjk_halfedge* E = (gjk_halfedge*) PushArray(Arena, 3, gjk_halfedge);
  gjk_face*     F = (gjk_face*)     PushStruct(Arena, gjk_face);

  F->Edge = &E[0];

  E[0].TargetVertex = B;
  E[0].LeftFace = F;
  E[0].NextEdge = &E[1];
  E[0].PreviousEdge = &E[2];
  E[0].OppositeEdge = NULL;
  A->OutgoingEdge = !A->OutgoingEdge ? &E[0] : A->OutgoingEdge;

  E[1].TargetVertex = C;
  E[1].LeftFace = F;
  E[1].NextEdge = &E[2];
  E[1].PreviousEdge = &E[0];
  E[1].OppositeEdge = NULL;
  B->OutgoingEdge = !B->OutgoingEdge ? &E[1] : B->OutgoingEdge;

  E[2].TargetVertex = A;
  E[2].LeftFace = F;
  E[2].NextEdge = &E[0];
  E[2].PreviousEdge = &E[1];
  E[2].OppositeEdge = NULL;
  C->OutgoingEdge = !C->OutgoingEdge ? &E[2] : C->OutgoingEdge;

  return *F;
}

void JoinEdges(gjk_halfedge* A, gjk_halfedge* B)
{
  Assert(A->PreviousEdge->TargetVertex->P.S == B->TargetVertex->P.S);
  Assert(B->PreviousEdge->TargetVertex->P.S == A->TargetVertex->P.S);
  A->OppositeEdge = B;
  B->OppositeEdge = A;
}


// A halfedge must ALWAYS point to it's opposite halfedge. (They always comes in pairs)
// A halfedge pair must ALWAYS point at least one Face.    (No edges are ever alone)
// A halfedge always creates a closed loop 3 long if inside
//  of a face or 3 or longer if outside or a hole.          (We can go ->next for ever)
inline internal void
DissconectEdge( gjk_halfedge* Edge )
{
  gjk_halfedge* OppositeEdge = Edge->OppositeEdge;

  Edge->PreviousEdge->NextEdge     = OppositeEdge->NextEdge;
  Edge->NextEdge->PreviousEdge     = OppositeEdge->PreviousEdge;

  OppositeEdge->PreviousEdge->NextEdge     = Edge->NextEdge;
  OppositeEdge->NextEdge->PreviousEdge     = Edge->PreviousEdge;
}

b32 JoinFaces(gjk_face A, gjk_face B)
{
  gjk_halfedge* AE = A.Edge;
  gjk_halfedge* BE = B.Edge;
  for(u32 i = 0; i < 3; ++i)
  {
    v3& AP0 = AE->PreviousEdge->TargetVertex->P.S;
    v3& AP1 = AE->TargetVertex->P.S;
    for(u32 j = 0; j < 3; ++j)
    {
      v3& BP0 = BE->PreviousEdge->TargetVertex->P.S;
      v3& BP1 = BE->TargetVertex->P.S;
      if( (BP0 == AP1) && (AP0 == BP1) )
      {
        JoinEdges(AE, BE);
        return true;
      }
      BE = BE->NextEdge;
    }
    AE = AE->NextEdge;
  }
  return false;
}

inline b32
Equals(const gjk_vertex* A, const gjk_vertex* B)
{
  return A->P.S == B->P.S;
}

inline b32
Equals(const gjk_halfedge* A, const gjk_halfedge* B)
{
  gjk_vertex* TargetA   = A->TargetVertex;
  gjk_vertex* OutgoingA = A->OppositeEdge->TargetVertex;

  gjk_vertex* TargetB   = B->TargetVertex;
  gjk_vertex* OutgoingB = B->OppositeEdge->TargetVertex;

  return (Equals(TargetA, OutgoingB) != 0) && (Equals(OutgoingA, TargetB)   != 0) ||
         (Equals(TargetA, TargetB)   != 0) && (Equals(OutgoingA, OutgoingB) != 0);
}


struct debug_face
{
  u32 F[3];
};
struct debug_vertice
{
  u32 Idx;
  v3  V;
};

void ShowDebugStruct(memory_arena* Arena, list<gjk_face> FaceList, list<gjk_vertex> VerticeList )
{
  temporary_memory TempMem = BeginTemporaryMemory(Arena);
  debug_vertice*  Vertices = (debug_vertice*) PushArray(Arena, VerticeList.GetSize(), debug_vertice);
  debug_face* Faces        = (debug_face*)    PushArray(Arena,    FaceList.GetSize(), debug_face);
  u32 VertexIndex = 0;
  for(VerticeList.First(); !VerticeList.IsEnd(); VerticeList.Next())
  {
    gjk_vertex V = VerticeList.Get();
    Vertices[VertexIndex++] = {V.Idx,V.P.S};
  }

  u32 FaceIndex = 0;
  for(FaceList.First(); !FaceList.IsEnd(); FaceList.Next())
  {
    gjk_face F = FaceList.Get();
    Faces[FaceIndex++] = { F.Edge->PreviousEdge->TargetVertex->Idx,
		F.Edge->TargetVertex->Idx,
                           F.Edge->NextEdge->TargetVertex->Idx};
  }

  EndTemporaryMemory(TempMem);
}


void DebugCheckFaces(memory_arena* Arena, list<gjk_face> FaceList, list<gjk_vertex> VerticeList )
{
  temporary_memory TempMem = BeginTemporaryMemory(Arena);
  u32 FaceIndex = 0;
  for(FaceList.First(); !FaceList.IsEnd(); FaceList.Next())
  {
    gjk_face F = FaceList.Get();
    gjk_halfedge* E[3] = {F.Edge->PreviousEdge, F.Edge, F.Edge->NextEdge};
    v3 T[3] = {E[0]->TargetVertex->P.S, E[1]->TargetVertex->P.S, E[2]->TargetVertex->P.S};
    v3 CurrentNormal = GetPlaneNormal(T);
    if(Norm(CurrentNormal) < 10E-2)
    {
      ShowDebugStruct(Arena, FaceList, VerticeList );
    }
  }
  EndTemporaryMemory(TempMem);
}

struct closest_face_result
{
  gjk_face F;
  r32 Distance;
  v3 Normal;
};

closest_face_result GetCLosestFace(memory_arena* TemporaryArena,  list<gjk_face>* FaceList, list<gjk_vertex>* VerticeList )
{
  temporary_memory TempMem = BeginTemporaryMemory(TemporaryArena);
  closest_face_result Result = {};
  list<v3>  Normals(TemporaryArena);
  list<r32> Distances(TemporaryArena);
  list<gjk_face> Faces(TemporaryArena);
  for (FaceList->First(); !FaceList->IsEnd(); FaceList->Next())
  {
    gjk_face Face = FaceList->Get();
    gjk_halfedge* Edges[3] = {Face.Edge, Face.Edge->NextEdge, Face.Edge->NextEdge->NextEdge};

    v3 Triangle[3] = {Edges[0]->TargetVertex->P.S, Edges[1]->TargetVertex->P.S, Edges[2]->TargetVertex->P.S};
    v3 CurrentNormal = *Normals.InsertAfter(GetPlaneNormal(Triangle));
    const v3 ProjectedPoints = ProjectPointOntoPlane( V3(0,0,0), Triangle[0], CurrentNormal );
    Distances.InsertAfter(Norm(ProjectedPoints));
    Faces.InsertAfter(Face);
  }

  Result.F         = *Faces.First();
  Result.Distance  = *Distances.First();
  Result.Normal    = *Normals.First();

  Assert(Normals.GetSize() == Distances.GetSize());
  Assert(  Faces.GetSize() == Distances.GetSize());
  u32 ArrayLength = Faces.GetSize();
  for( u32 i = 1; i < ArrayLength; ++i)
  {
    Normals.Next();
    Distances.Next();
    Faces.Next();
    if(Distances.Get() < Result.Distance)
    {
      Result.F          = Faces.Get();
      Result.Distance   = Distances.Get();
      Result.Normal     = Normals.Get();
    }
  }
  if( Norm(Result.Normal)>10E-7)
  {
	  ShowDebugStruct(TemporaryArena, *FaceList, *VerticeList);
  }
  EndTemporaryMemory(TempMem);
  return Result;
}

contact_data EPACollisionResolution(memory_arena* TemporaryArena, const m4* AModelMat, const collider_mesh* AMesh,
                                      const m4* BModelMat, const collider_mesh* BMesh,
                                      gjk_simplex& Simplex)
{
  temporary_memory TempMem = BeginTemporaryMemory(TemporaryArena);

  BlowUpSimplex(AModelMat, AMesh,
                BModelMat, BMesh,
                Simplex);

  u32 TriangleIndeces[12] = {};
  SetPointsAndIndecesForCCWTetrahedron(Simplex, TriangleIndeces);

  list<gjk_vertex> VerticeList(TemporaryArena);

  gjk_vertex* V[4] = {};
  V[0] = VerticeList.InsertAfter({});
  V[1] = VerticeList.InsertAfter({});
  V[2] = VerticeList.InsertAfter({});
  V[3] = VerticeList.InsertAfter({});

  u32 VertexIDX = 1;
  V[0]->P = Simplex.SP[0];
  V[0]->Idx = VertexIDX++;
  V[1]->P = Simplex.SP[1];
  V[1]->Idx = VertexIDX++;
  V[2]->P = Simplex.SP[2];
  V[2]->Idx = VertexIDX++;
  V[3]->P = Simplex.SP[3];
  V[3]->Idx = VertexIDX++;

  list<gjk_face> FaceList(TemporaryArena);
  FaceList.InsertAfter(CreateFace(TemporaryArena, V[TriangleIndeces[0]] ,V[TriangleIndeces[1]], V[TriangleIndeces[2]]));
  FaceList.InsertAfter(CreateFace(TemporaryArena, V[TriangleIndeces[3]] ,V[TriangleIndeces[4]], V[TriangleIndeces[5]]));
  FaceList.InsertAfter(CreateFace(TemporaryArena, V[TriangleIndeces[6]] ,V[TriangleIndeces[7]], V[TriangleIndeces[8]]));
  FaceList.InsertAfter(CreateFace(TemporaryArena, V[TriangleIndeces[9]] ,V[TriangleIndeces[10]],V[TriangleIndeces[11]]));

  gjk_face  F[4] = {};
  F[0] = *FaceList.First();
  F[1] = *FaceList.Next();
  F[2] = *FaceList.Next();
  F[3] = *FaceList.Next();
  Assert((F[0].Edge) == (F[0].Edge->PreviousEdge->PreviousEdge->PreviousEdge));
  Assert((F[0].Edge) == (F[0].Edge->NextEdge->NextEdge->NextEdge));
  Assert((F[0].Edge->NextEdge) == (F[0].Edge->PreviousEdge->PreviousEdge));
  Assert((F[0].Edge->PreviousEdge) == (F[0].Edge->NextEdge->NextEdge));
  Assert((F[0].Edge->NextEdge->NextEdge) == (F[0].Edge->PreviousEdge));
  Assert((F[0].Edge->PreviousEdge) == (F[0].Edge->NextEdge->NextEdge));
  Assert((F[1].Edge) == (F[1].Edge->PreviousEdge->PreviousEdge->PreviousEdge));
  Assert((F[1].Edge) == (F[1].Edge->NextEdge->NextEdge->NextEdge));
  Assert((F[1].Edge->NextEdge) == (F[1].Edge->PreviousEdge->PreviousEdge));
  Assert((F[1].Edge->PreviousEdge) == (F[1].Edge->NextEdge->NextEdge));
  Assert((F[1].Edge->NextEdge->NextEdge) == (F[1].Edge->PreviousEdge));
  Assert((F[1].Edge->PreviousEdge) == (F[1].Edge->NextEdge->NextEdge));
  Assert((F[2].Edge) == (F[2].Edge->PreviousEdge->PreviousEdge->PreviousEdge));
  Assert((F[2].Edge) == (F[2].Edge->NextEdge->NextEdge->NextEdge));
  Assert((F[2].Edge->NextEdge) == (F[2].Edge->PreviousEdge->PreviousEdge));
  Assert((F[2].Edge->PreviousEdge) == (F[2].Edge->NextEdge->NextEdge));
  Assert((F[2].Edge->NextEdge->NextEdge) == (F[2].Edge->PreviousEdge));
  Assert((F[2].Edge->PreviousEdge) == (F[2].Edge->NextEdge->NextEdge));
  Assert((F[3].Edge) == (F[3].Edge->PreviousEdge->PreviousEdge->PreviousEdge));
  Assert((F[3].Edge) == (F[3].Edge->NextEdge->NextEdge->NextEdge));
  Assert((F[3].Edge->NextEdge) == (F[3].Edge->PreviousEdge->PreviousEdge));
  Assert((F[3].Edge->PreviousEdge) == (F[3].Edge->NextEdge->NextEdge));
  Assert((F[3].Edge->NextEdge->NextEdge) == (F[3].Edge->PreviousEdge));
  Assert((F[3].Edge->PreviousEdge) == (F[3].Edge->NextEdge->NextEdge));


  JoinFaces(F[0], F[1]);
  JoinFaces(F[0], F[2]);
  JoinFaces(F[0], F[3]);
  JoinFaces(F[1], F[2]);
  JoinFaces(F[1], F[3]);
  JoinFaces(F[2], F[3]);


  // Get the first new point
  closest_face_result ClosestFace = GetCLosestFace( TemporaryArena, &FaceList, &VerticeList );

  r32 ShortestNormalDistance = ClosestFace.Distance;
  r32 PreviousShortestNormalDistance = ShortestNormalDistance + 100;

  u32 Tries = 0;
  while(Tries++ < 2)
  {
    DebugCheckFaces(TemporaryArena, FaceList, VerticeList );
    if(Abs(ShortestNormalDistance - PreviousShortestNormalDistance) > 10E-4)
    {
      Tries = 0;
    }
    gjk_vertex* NewVertex = VerticeList.InsertAfter({});
    NewVertex->Idx = VertexIDX++;
    NewVertex->P = CsoSupportAABB( AModelMat, AMesh,
                                   BModelMat, BMesh,
                                   ClosestFace.Normal);
  /*
    How to remove a Face properly:

    Definition:
      Verices: A point in space.
        A valid Verice must point to at least two incident Edges.
        A Verice should be removed if none of it's incident edges point's to any Faces.

      Edge: A half-edge pair joining two vertices.
        A valid Edge consists of:
        - Two opposite halfedges joining together two vertices.
        - At least ONE of the halfedges must point to a face.
        - Each edge must form a closed loop with other edges.
        An edge should be removed if none of its halfedges points to a face.

      Face: Three vertices joined together by three edges.
        A valid Face points to one bounding half-edge.
        A face should be removed if it doesn't point to a half-edge.




    Each Face forms a loop of halfedge-pairs [joining together vertices

    1: Remove all references in the mesh to the Face:
        For each HalfEdge around the Face delete the reference back to it's Face.
        Nothing in the mesh is now pointing TO the Face.

    2: Check if any vertices needs to be removed:
        A verice needs to be removed IF all incident Edges needs to be removed.
        A Edge needs to be removed if none of it's half-edges

  */

    list<gjk_vertex*>   AffectedVertices(TemporaryArena);
    list<gjk_halfedge*> AffectedEdges(TemporaryArena);
    list<gjk_face>      FacesToRemove(TemporaryArena);

    for (FaceList.First(); !FaceList.IsEnd(); FaceList.Next())
    {
      gjk_face* Face = FaceList.GetRef();
      v3 FaceNormal = GetFaceNormal(*Face);
      r32 Nerm = (FaceNormal*NewVertex->P.S);
      if((FaceNormal*(NewVertex->P.S - Face->Edge->TargetVertex->P.S)) > 0)
      {
        FacesToRemove.InsertAfter(*Face);
        FaceList.Remove();
      }
    }

    if(FacesToRemove.GetSize()==0)
    {
      break;
    }

    //ShowDebugStruct(TemporaryArena, FaceList, VerticeList );

    // Populate AffectedVertices and AffectedEdges with unique entries
    for(FacesToRemove.First(); !FacesToRemove.IsEnd(); FacesToRemove.Next())
    {
      gjk_face* Face = FacesToRemove.GetRef();

      gjk_halfedge* EdgesToInsert[3] = {Face->Edge->PreviousEdge,
                                        Face->Edge,
                                        Face->Edge->NextEdge};
      gjk_vertex* VerticesToInsert[3] = {EdgesToInsert[0]->TargetVertex,
                                         EdgesToInsert[1]->TargetVertex,
                                         EdgesToInsert[2]->TargetVertex};

      for(u32 i = 0; i < 3; ++i)
      {
        b32 IsEdgeUnique = true;
        for(AffectedEdges.First(); !AffectedEdges.IsEnd(); AffectedEdges.Next())
        {
          gjk_halfedge* EdgeRef = AffectedEdges.Get();
          if(Equals(EdgesToInsert[i], EdgeRef))
          {
            IsEdgeUnique = false;
            break;
          }
        }
        if(IsEdgeUnique)
        {
          AffectedEdges.First();
          AffectedEdges.InsertAfter(EdgesToInsert[i]);
        }

        b32 IsVertUnique = true;
        for(AffectedVertices.First(); !AffectedVertices.IsEnd(); AffectedVertices.Next())
        {
          if(Equals(VerticesToInsert[i], AffectedVertices.Get()))
          {
            IsVertUnique = false;
            break;
          }
        }
        if(IsVertUnique)
        {
          AffectedVertices.First();
          AffectedVertices.InsertAfter(VerticesToInsert[i]);
        }
      }
    }

    // Dissconect Faces from halfedges
    for(FacesToRemove.First(); !FacesToRemove.IsEnd(); FacesToRemove.Next())
    {
      gjk_face* Face = FacesToRemove.GetRef();
      gjk_halfedge* HalfEdge = Face->Edge;
      Face->Edge = NULL;
      HalfEdge->PreviousEdge->LeftFace = NULL;
      HalfEdge->LeftFace = NULL;
      HalfEdge->NextEdge->LeftFace = NULL;
    }

    // Go throug each affected vertex and see if it should be removed
    // (if all incident edges should be removed)
    for(AffectedVertices.First(); !AffectedVertices.IsEnd(); AffectedVertices.Next())
    {
      gjk_vertex* Vert = AffectedVertices.Get();
      gjk_halfedge* StartingEdge = Vert->OutgoingEdge;
      gjk_halfedge* EdgeToCheck = StartingEdge;
      b32 RemoveVertex = true;
      do
      {
        if( EdgeToCheck->LeftFace ||
            EdgeToCheck->OppositeEdge->LeftFace )
        {
          RemoveVertex = false;
          Vert->OutgoingEdge = EdgeToCheck;
          break;
        }
        EdgeToCheck = EdgeToCheck->OppositeEdge->NextEdge;
      }while( !Equals(StartingEdge, EdgeToCheck) );

      if(RemoveVertex)
      {
        Vert->OutgoingEdge = NULL; // This indicates the vertex can be removed
        AffectedVertices.Remove();
      }
    }

    // Disconnect the Edges which has no faces on either side.
    for(AffectedEdges.First(); !AffectedEdges.IsEnd(); AffectedEdges.Next())
    {
      gjk_halfedge* E  = AffectedEdges.Get();
      // If a halfedge pair no longer points to a face we remove it.
      if( ! E->LeftFace &&
          ! E->OppositeEdge->LeftFace )
      {
        DissconectEdge(E);
        AffectedEdges.Remove();
      }
    }

    // Make sure the AffectedEdges List form one continous hole
    gjk_halfedge* FE  = *AffectedEdges.First();
    Assert(FE->LeftFace == 0);
    Assert(FE->OppositeEdge->LeftFace != 0);
    gjk_halfedge* NE  = FE->NextEdge;
    u32 BorderEdgesSize = AffectedEdges.GetSize();
    u32 BorderIndex = 1;
    while( !Equals(FE,NE) )
    {
      Assert(NE->LeftFace == 0);
      Assert(NE->OppositeEdge->LeftFace != 0);
      Assert(BorderIndex < BorderEdgesSize);
      NE = NE->NextEdge;
      ++BorderIndex;
    }
    Assert(BorderIndex == BorderEdgesSize);

    // Connect new triangles between each border edge and the new point to create faces
    FaceList.Last();
    gjk_vertex* A = FE->OppositeEdge->TargetVertex;
    gjk_vertex* B = FE->TargetVertex;
    v3 VVV[3] = {FE->OppositeEdge->TargetVertex->P.S, FE->TargetVertex->P.S, NewVertex->P.S};
    gjk_face*  FirstNewFace = FaceList.InsertAfter(CreateFace(TemporaryArena,
                                              FE->OppositeEdge->TargetVertex,
                                              FE->TargetVertex, NewVertex));
    gjk_face*  PreviousFace = FirstNewFace;
    JoinFaces(*PreviousFace, *FE->OppositeEdge->LeftFace);
    BorderIndex = 1;
    NE  = FE->NextEdge;
    while( BorderIndex < BorderEdgesSize )
    {
      v3 VVVV[3] = { NE->OppositeEdge->TargetVertex->P.S, NE->TargetVertex->P.S, NewVertex->P.S};
      gjk_face*  NewFace =FaceList.InsertAfter( CreateFace(TemporaryArena,
                                           NE->OppositeEdge->TargetVertex,
                                           NE->TargetVertex, NewVertex));
      JoinFaces(*NewFace, *NE->OppositeEdge->LeftFace);
      JoinFaces(*PreviousFace, *NewFace);
      PreviousFace = NewFace;
      NE = NE->NextEdge;
      //gjk_halfedge* BorderEdgeBase = NewFace->Edge;
      //gjk_halfedge* BorderEdgeBase = NE;
      //
      //JoinFaces(*NewFace, *NE->OppositeEdge->LeftFace);
      ++BorderIndex;
    }
    JoinFaces(*PreviousFace, *FirstNewFace);

    //ShowDebugStruct(TemporaryArena, FaceList, VerticeList );

    ClosestFace = GetCLosestFace( TemporaryArena, &FaceList, &VerticeList );
    PreviousShortestNormalDistance = ShortestNormalDistance;
    ShortestNormalDistance = ClosestFace.Distance;
  }


  gjk_support A = ClosestFace.F.Edge->TargetVertex->P;
  gjk_support B = ClosestFace.F.Edge->NextEdge->TargetVertex->P;
  gjk_support C = ClosestFace.F.Edge->NextEdge->NextEdge->TargetVertex->P;
  v3 P = ClosestFace.Normal * ClosestFace.Distance;

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

  // Check that the point P is inside A, B and C;
  Assert( (LambdaA >= 0) && (LambdaA <= 1) );
  Assert( (LambdaB >= 0) && (LambdaB <= 1) );
  Assert( (LambdaC >= 0) && (LambdaC <= 1) );
  // Check that some baryocentric coordinate conditions hold.
  r32 LambdaSum = LambdaA + LambdaB + LambdaC;
  if(Abs( LambdaSum - 1) > 10E-7 )
  {
    ShowDebugStruct(TemporaryArena, FaceList, VerticeList);
  }
 // v3 LambdaPoint = (LambdaA * A.S + LambdaB * B.S + LambdaC * C.S);
 // Assert( P == LambdaPoint );

  gjk_support InterpolatedSupport = {};
  InterpolatedSupport.S = P;
  InterpolatedSupport.A = LambdaA * A.A + LambdaB * B.A + LambdaC * C.A;
  InterpolatedSupport.B = LambdaA * A.B + LambdaB * B.B + LambdaC * C.B;

  EndTemporaryMemory(TempMem);

  contact_data ContactData = {};

  v3 Tangent1 = V3(0.0f, ClosestFace.Normal.Z, -ClosestFace.Normal.Y);
  if ( ClosestFace.Normal.X >= 0.57735f)
  {
    Tangent1 = V3(ClosestFace.Normal.Y, -ClosestFace.Normal.X, 0.0f);
  }
  Normalize(Tangent1);

  ContactData.A_ContactWorldSpace = V3(*AModelMat * V4(InterpolatedSupport.A,1));
  ContactData.B_ContactWorldSpace = V3(*BModelMat * V4(InterpolatedSupport.B,1));
  ContactData.A_ContactModelSpace = InterpolatedSupport.A;
  ContactData.B_ContactModelSpace = InterpolatedSupport.B;
  ContactData.ContactNormal       = ClosestFace.Normal;
  ContactData.TangentNormalOne    = Tangent1;
  ContactData.TangentNormalTwo    = CrossProduct(ClosestFace.Normal, Tangent1);
  ContactData.PenetrationDepth    = ClosestFace.Distance;
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
  const v3 Normal         = GetPlaneNormal(Triangle);
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
    const v3 Normal = GetPlaneNormal(Triangle);
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
  SetPointsAndIndecesForCCWTetrahedron(Simplex, TriangleIndeces);

  if( IsVertexInsideTetrahedron( Vertex, TriangleIndeces,  Simplex) )
  {
    Result.ClosestPoint = Vertex;
    Result.Distance = Norm(Vertex);
    return Result;
  }


  Result = VertexTriangle( Vertex, Simplex, TriangleIndeces[0], TriangleIndeces[1], TriangleIndeces[2]);
  u32 TriangleIndex0 = TriangleIndeces[0];
  u32 TriangleIndex1 = TriangleIndeces[1];
  u32 TriangleIndec2 = TriangleIndeces[2];
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
      TriangleIndec2 = TriangleIndeces[BaseIndex+2];
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
    Result.ReducedSimplex.SP[2] = Simplex.SP[TriangleIndex1];
  }

  return Result;
}

// Assumes the AABB in model space is the cube going from V3(-1/2,-1/2,-1/2) to V3(1/2,1/2,1/2)
gjk_collision_result GJKCollisionDetection(const m4* AModelMat, const collider_mesh* AMesh,
                                           const m4* BModelMat, const collider_mesh* BMesh )
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