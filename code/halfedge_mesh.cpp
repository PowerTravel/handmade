
#include "halfedge_mesh.h"

#define F32_TOL 10E-7

// Todo: Cache the face normal?
v3 GetNormal(epa_face* Face)
{
  epa_halfedge* E0 = Face->Edge;
  epa_halfedge* E1 = Face->Edge->Next;
  epa_halfedge* E2 = Face->Edge->Next->Next;
  return GetPlaneNormal(E0->TargetVertex->P.S,
                        E1->TargetVertex->P.S,
                        E2->TargetVertex->P.S);
}

internal void
GetFacePoints(epa_face* Face, v3* P0, v3* P1, v3* P2)
{
  *P0 = Face->Edge->TargetVertex->P.S;
  *P1 = Face->Edge->Next->TargetVertex->P.S;
  *P2 = Face->Edge->Next->Next->TargetVertex->P.S;;
};

r32 GetFaceArea(epa_face* Face)
{
  v3 P0, P1, P2;
  GetFacePoints(Face, &P0, &P1, &P2);
  v3 v0 = P1-P0;
  v3 v1 = P2-P0;
  r32 Result = Norm(CrossProduct(v0,v1))*0.5f;
  return Result;
}

epa_face* GetFaceSeenByPoint(epa_mesh* Mesh, gjk_support* Point)
{
  epa_face* Face = Mesh->Faces;
  epa_face* Result = 0;
  while(Face)
  {
    // Note Jakob(): To check if a given face can be "seen" by a point
    // you want to form a vector from a vertex of said face to the point
    // and check the dot product of that with the face normal.
    //   if((Point - PointOnFace) * Face->Normal) > 0)
    const v3 PointOnFace = Face->Edge->TargetVertex->P.S;
    const v3 FaceToPoint = (Point->S - PointOnFace);
    const v3 FaceNormal = GetNormal(Face);
    const r32 DotProduct = FaceNormal*FaceToPoint;

    if(DotProduct >= 0)
    {
      Result = Face;
      break;
    }
    Face = Face->Next;
  }
  return Result;
}

/*
 *                               ^
 *                               | |
 *                        NextB2 | |PrevB1
 *                               | |
 *                               | |
 *                                 v
 *                             ^ P1  \
 *                            / /   ^ \
 *                           / /     \ \
 *                          / /       \ \
 *                         / /         \ \
 *                    B2  / / A2     A1 \ \ B1
 *                       / /             \ \
 *                      / /               \ \
 *                     / /                 \ \
 *      PrevB2          v        A0           v     NextB1
 * -----------------> P2-------------------> P0 ---------------->
 * <----------------    <-------------------    <----------------
 *                  ^ /          B0           ^ \
 *                 / / NextB0           PrevB0 \ \
 *                / /                           \ \
 *                 v                             v
 */
void DissconnectEdge(epa_halfedge* Edge)
{
  Assert(!Edge->LeftFace && !Edge->Opposite->LeftFace)

  epa_halfedge* A = Edge;
  epa_halfedge* PrevA = A->Previous;
  epa_halfedge* NextA = A->Next;

  epa_halfedge* B = Edge->Opposite;
  epa_halfedge* PrevB = B->Previous;
  epa_halfedge* NextB = B->Next;

  epa_vertex* Pa = A->TargetVertex;
  if(Pa->OutgoingEdge == B)
  {
    Pa->OutgoingEdge = NextA;
  }

  epa_vertex* Pb = B->TargetVertex;
  if(Pb->OutgoingEdge == A)
  {
    Pb->OutgoingEdge = NextB;
  }

  PrevB->Next = NextA;
  NextA->Previous = PrevB;

  PrevA->Next = NextB;
  NextB->Previous = PrevA;

  A->Next = 0;
  A->Previous = 0;
  B->Next = 0;
  B->Previous = 0;

  Assert(Pa->OutgoingEdge);
  Assert(Pb->OutgoingEdge);
}


void RemoveFace(epa_mesh* Mesh, epa_face* Face)
{
  // Disconnect The Face Fron the Mesh list
  epa_face** FacePtr = &Mesh->Faces;
  epa_face*  FaceTmp = *FacePtr;
  while(*FacePtr != Face)
  {
    epa_face* Tmp = *FacePtr;
    FacePtr = &(*FacePtr)->Next;
  }
  *FacePtr = Face->Next;

  epa_halfedge* A0 = Face->Edge;
  epa_halfedge* A1 = Face->Edge->Next;
  epa_halfedge* A2 = Face->Edge->Previous;

  Assert(A0->LeftFace && A1->LeftFace && A2->LeftFace);
  A0->LeftFace = 0;
  A1->LeftFace = 0;
  A2->LeftFace = 0;

  if((!A0->Opposite->LeftFace) && (!A1->Opposite->LeftFace) && (!A2->Opposite->LeftFace))
  {
    // Assert the border edges form a selfcontained loop.
    Assert(A0->Next->Next->Next == A0);
    DissconnectEdge(A0);
    DissconnectEdge(A1);
    DissconnectEdge(A2);
  }else{
    if(!A0->Opposite->LeftFace)
    {
      DissconnectEdge(A0);
    }
    if(!A1->Opposite->LeftFace)
    {
      DissconnectEdge(A1);
    }
    if(!A2->Opposite->LeftFace)
    {
      DissconnectEdge(A2);
    }
  }
}

void RemoveFacesSeenByPoint(epa_mesh* Mesh, gjk_support* Point)
{
  TIMED_FUNCTION();
  epa_face* Face = 0;
  while((Face = GetFaceSeenByPoint(Mesh, Point)) != NULL)
  {
    #if HANDMADE_SLOW
      r32 FaceArea = GetFaceArea(Face);
      Assert(FaceArea > F32_TOL);
    #endif
    RemoveFace(Mesh,Face);
  }
}



internal inline epa_face *
PushFace(epa_mesh* Mesh)
{
  epa_face* NewFace = (epa_face*) PushStruct(Mesh->Arena, epa_face);
  NewFace->Next = Mesh->Faces;
  Mesh->Faces = NewFace;
  return NewFace;
}

/*
 *           ^   B  \
 *           / /   ^ \
 *          / /     \ \
 *         / /       \ \
 *        / /         \ \
 *       / /           \ \
 *    5 / / 1         0 \ \ 3
 *     / /               \ \
 *    / /                 \ \
 *   / v         2         \ v
 *    C-------------------> A
 *   <------------------------
 *               4
 */
epa_mesh* InitializeMesh(memory_arena* Arena, gjk_support* P0, gjk_support* P1, gjk_support* P2)
{
  epa_mesh* Result = PushStruct(Arena, epa_mesh);
  Result->Arena = Arena;

  // Allocate 6 edges
  epa_halfedge* NewEdges = (epa_halfedge*) PushArray(Result->Arena, 6, epa_halfedge);

  // Connect edges:
  // Internal edges [0,1,2] (Pointing to Face)
  NewEdges[0].Next = &NewEdges[1];
  NewEdges[1].Next = &NewEdges[2];
  NewEdges[2].Next = &NewEdges[0];
  NewEdges[0].Previous = &NewEdges[2];
  NewEdges[1].Previous = &NewEdges[0];
  NewEdges[2].Previous = &NewEdges[1];

  // External Edges [3,4,5]
  NewEdges[3].Next = &NewEdges[4];
  NewEdges[4].Next = &NewEdges[5];
  NewEdges[5].Next = &NewEdges[3];
  NewEdges[3].Previous = &NewEdges[5];
  NewEdges[4].Previous = &NewEdges[3];
  NewEdges[5].Previous = &NewEdges[4];

  // Connect Internal and Externa; Edges
  NewEdges[0].Opposite = &NewEdges[3];
  NewEdges[3].Opposite = &NewEdges[0];
  NewEdges[1].Opposite = &NewEdges[5];
  NewEdges[5].Opposite = &NewEdges[1];
  NewEdges[2].Opposite = &NewEdges[4];
  NewEdges[4].Opposite = &NewEdges[2];

  // Create Face
  epa_face* NewFace = PushFace(Result);
  //Connect internal edges to Face
  NewEdges[0].LeftFace = NewFace;
  NewEdges[1].LeftFace = NewFace;
  NewEdges[2].LeftFace = NewFace;
  // Connect Face to an internal edge
  NewFace->Edge = &NewEdges[0];

  // Create Vertices
#if 1
  epa_vertex* A = (epa_vertex*) PushStruct(Result->Arena, epa_vertex);
  epa_vertex* B = (epa_vertex*) PushStruct(Result->Arena, epa_vertex);
  epa_vertex* C = (epa_vertex*) PushStruct(Result->Arena, epa_vertex);
#else
  epa_vertex *
  PushVertex(epa_mesh* Mesh)
  {
    epa_vertex* NewVertex = (epa_vertex*) PushStruct(Mesh->Arena, epa_vertex);
    NewVertex->Next = Mesh->Vertices;
    Mesh->Vertices = NewVertex;
    return NewVertex;
  }
  epa_vertex* A = PushVertex(Mesh->Arena, epa_vertex);
  epa_vertex* B = PushVertex(Mesh->Arena, epa_vertex);
  epa_vertex* C = PushVertex(Mesh->Arena, epa_vertex);
#endif

  A->P = *P0;
  A->OutgoingEdge = &NewEdges[0];
  B->P = *P1;
  B->OutgoingEdge = &NewEdges[1];
  C->P = *P2;
  C->OutgoingEdge = &NewEdges[2];
  // Connect Edges to Vertecies
  NewEdges[2].TargetVertex = A;
  NewEdges[0].TargetVertex = B;
  NewEdges[1].TargetVertex = C;

  NewEdges[4].TargetVertex = C;
  NewEdges[5].TargetVertex = B;
  NewEdges[3].TargetVertex = A;

  #if HANDMADE_SLOW
    r32 FaceArea = GetFaceArea(NewFace);
    Assert(FaceArea > F32_TOL);
  #endif

  return Result;
};

r32 GetDistanceToFace(epa_face* Face)
{
  #if HANDMADE_SLOW
    r32 FaceArea = GetFaceArea(Face);
    Assert(FaceArea > F32_TOL*F32_TOL);
  #endif
  v3 P0, P1, P2;
  GetFacePoints(Face, &P0, &P1, &P2);
  const v3 Normal = GetPlaneNormal(P0, P1, P2);
  const v3 ProjectedPoint = ProjectPointOntoPlane( V3(0,0,0), P0, Normal );
  const v3 Coords = GetBaryocentricCoordinates( P0, P1, P2, Normal, ProjectedPoint);
  // Note: Baryocentric coordinates introduce allot of numerical instability
  //       especially for excentric triangles (which we have alot of).
  //       Be careful when using them.
  const b32 InsideTriangle = (Coords.E[0] >= 0) && (Coords.E[0] <= 1) &&
                             (Coords.E[1] >= 0) && (Coords.E[1] <= 1) &&
                             (Coords.E[2] >= 0) && (Coords.E[2] <= 1);
  r32 Result = Norm(ProjectedPoint);
  if( !InsideTriangle )
  {
    epa_halfedge* Edge = Face->Edge;
    while(Edge->Next != Face->Edge)
    {
      // Todo: Use GetBaryocentricCoordinates to figure out which Edge is closest
      //       without looping over all the edges?
      const v3 o = Edge->Previous->TargetVertex->P.S;
      const v3 d = Edge->TargetVertex->P.S;
      const v3 u = Normalize(d-o);
      const r32 uNorm = Norm(d-o);
      const v3 v = ProjectedPoint;

      const r32 ProjectionScalar = (v-o)*u;
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

      r32 DistanceToLine = Norm(ClosestPointOnEdge);
      if(DistanceToLine < Result)
      {
        Result = DistanceToLine;
      }

      Edge = Edge->Next;
    }
  }
  return Result;
}


/*
 * AddNewPoint(epa_mesh* Mesh, epa_halfedge* BorderEdge, gjk_support* Point)
 * REQURES That:
 *              - A Valid Mesh
 *              - NewPoint does NOT exist in the mesh
 *              - BorderEdge does NOT point to a face
 *
 * THEN the output is ALSO a valid mesh.
 *
 *
 *                            NewPoint
 *                            ^  P1  \
 *                            / /   ^ \
 *                           / /     \ \
 *                          / /       \ \
 *                         / /         \ \
 *                   Ext2 / /Int2   Int1\ \ Ext1
 *                       / /             \ \
 *                      / /               \ \
 *                     / /                 \ \
 *   PrevBorder         v       Int0          v    NextBorder
 * -----------------> P2-------------------> P0 ---------------->
 * <----------------    <-------------------    <----------------
 *
 */


internal epa_vertex* AddNewPoint(epa_mesh* Mesh, epa_halfedge* BorderEdge, gjk_support* Point)
{
  Assert(!BorderEdge->LeftFace);
  Assert(BorderEdge->Opposite->LeftFace);

  // Store the serial edges of NextBorderEdge before we reconstruct the mesh
  epa_halfedge* NextBorderEdge = BorderEdge->Next;
  epa_halfedge* PreviousBorderEdge = BorderEdge->Previous;

  epa_halfedge* InternalEdge0 = BorderEdge;
  epa_halfedge* InternalEdge1 = PushStruct(Mesh->Arena, epa_halfedge);
  epa_halfedge* InternalEdge2 = PushStruct(Mesh->Arena, epa_halfedge);
  epa_halfedge* ExternalEdge1 = PushStruct(Mesh->Arena, epa_halfedge);
  epa_halfedge* ExternalEdge2 = PushStruct(Mesh->Arena, epa_halfedge);

  // Hook up Internal with External Edges (Parallel)
  ExternalEdge1->Opposite = InternalEdge1;
  InternalEdge1->Opposite = ExternalEdge1;
  ExternalEdge2->Opposite = InternalEdge2;
  InternalEdge2->Opposite = ExternalEdge2;

  // Hook up Internal Edges (Form the triangle)
  InternalEdge0->Next = InternalEdge1;
  InternalEdge0->Previous = InternalEdge2;
  InternalEdge1->Next = InternalEdge2;
  InternalEdge1->Previous = InternalEdge0;
  InternalEdge2->Next = InternalEdge0;
  InternalEdge2->Previous = InternalEdge1;

  // Hook up External Edges Serial (Connect to the rest of the mesh)
  PreviousBorderEdge->Next = ExternalEdge2;
  ExternalEdge2->Next = ExternalEdge1;
  ExternalEdge1->Next = NextBorderEdge;
  NextBorderEdge->Previous = ExternalEdge1;
  ExternalEdge1->Previous = ExternalEdge2;
  ExternalEdge2->Previous = PreviousBorderEdge;


  // Hook up a new Face to the Internal Edges
  epa_face* Face = PushFace(Mesh);
  Face->Edge = InternalEdge0;
  InternalEdge0->LeftFace = Face;
  InternalEdge1->LeftFace = Face;
  InternalEdge2->LeftFace = Face;

  // Pick out the relevant points
  epa_vertex* P0 = InternalEdge0->TargetVertex;
  epa_vertex* P1 = PushStruct(Mesh->Arena, epa_vertex);
  epa_vertex* P2 = PreviousBorderEdge->TargetVertex;

  // Initialize the new point
  P1->P = *Point;
  P1->OutgoingEdge = InternalEdge2;

  Assert(P0->P.S != P1->P.S);
  Assert(P1->P.S != P2->P.S);
  Assert(P2->P.S != P0->P.S);

  Assert(InternalEdge2);
  // Make our edges point to the points
  InternalEdge1->TargetVertex = P1;
  InternalEdge2->TargetVertex = P2;
  ExternalEdge1->TargetVertex = P0;
  ExternalEdge2->TargetVertex = P1;

  Assert(P0->OutgoingEdge);
  Assert(P1->OutgoingEdge);
  Assert(P2->OutgoingEdge);

  // Some Sanity checks that the new External edges are hooked up okay to the rest of the mesh
  Assert(InternalEdge0 == InternalEdge0->Opposite->Opposite);
  Assert(InternalEdge1 == InternalEdge1->Opposite->Opposite);
  Assert(InternalEdge2 == InternalEdge2->Opposite->Opposite);
  Assert(InternalEdge0->Opposite == InternalEdge0->Opposite->Opposite->Opposite);
  Assert(ExternalEdge1 == ExternalEdge1->Opposite->Opposite);
  Assert(ExternalEdge2 == ExternalEdge2->Opposite->Opposite);

  Assert(InternalEdge0->Next == InternalEdge1);
  Assert(InternalEdge1->Next == InternalEdge2);
  Assert(InternalEdge2->Next == InternalEdge0);
  Assert(InternalEdge0->Previous == InternalEdge2);
  Assert(InternalEdge2->Previous == InternalEdge1);
  Assert(InternalEdge1->Previous == InternalEdge0);

  Assert(PreviousBorderEdge->Next == ExternalEdge2);
  Assert(ExternalEdge2->Next == ExternalEdge1);
  Assert(ExternalEdge1->Next == NextBorderEdge);
  Assert(NextBorderEdge->Previous == ExternalEdge1);
  Assert(ExternalEdge1->Previous == ExternalEdge2);
  Assert(ExternalEdge2->Previous == PreviousBorderEdge);

  Assert(InternalEdge0->LeftFace == Face);
  Assert(InternalEdge1->LeftFace == Face);
  Assert(InternalEdge2->LeftFace == Face);
  Assert(InternalEdge0->Opposite);
  Assert(!ExternalEdge1->LeftFace);
  Assert(!ExternalEdge2->LeftFace);
  Assert((Face->Edge == InternalEdge0) || (Face->Edge == InternalEdge1) || (Face->Edge == InternalEdge2));

  Assert(InternalEdge0->TargetVertex == P0);
  Assert(InternalEdge1->TargetVertex == P1);
  Assert(InternalEdge2->TargetVertex == P2);
  Assert(InternalEdge0->Opposite->TargetVertex == P2);
  Assert(ExternalEdge1->TargetVertex == P0);
  Assert(ExternalEdge2->TargetVertex == P1);

#if HANDMADE_SLOW
  r32 FaceArea = GetFaceArea(Face);
  Assert(FaceArea > F32_TOL*F32_TOL);
#endif

  return P1;
}

/*
 * AddNewEdge(epa_mesh* Mesh, epa_halfedge* BorderEdge)
 * REQURES That:
 *              - A Valid Mesh
 *              - BorderEdge does NOT point to a face
 *              - BorderEdge->Next does NOT point to a face
 *              - BorderEdge and Next are NOT parallell
 *
 * THEN the output is ALSO a valid mesh.
 *
 *            ---------------->         ------------->
 *            <----------------   P1   <--------------
 *          ^ \               ^ /  X ^ \X Previous Border Edge
 *           \ \             / /  New \ \
 *            \ \    Face   / /  Face  \ \
 *             \ \         / /          \ \
 *              \ \       / /Int2    Int1\X\
 *               \ \     / /Border     New\ \
 *                \ \   / /Edge        Edge\ \
 *                 \ v / /                  \ \
 *                      v X      Int0      X   v   Next Border Edge
 *                    P2 -------------------> P0 --------------->
 *                      <--------------------    <---------------
 *
 */

void AddNewEdge(epa_mesh* Mesh, epa_halfedge* BorderEdge, v3 DebugAssertVertex)
{
  Assert(BorderEdge->Opposite->LeftFace);
  Assert(!BorderEdge->LeftFace);
  Assert(!BorderEdge->Next->LeftFace);
  Assert(!BorderEdge->Previous->LeftFace);

  Assert(BorderEdge->TargetVertex->P.S != DebugAssertVertex);
  Assert(BorderEdge->Next->TargetVertex->P.S != DebugAssertVertex);
  Assert(BorderEdge->Previous->TargetVertex->P.S == DebugAssertVertex);
  Assert(BorderEdge->Opposite->TargetVertex->P.S == DebugAssertVertex);
  epa_face* Face = PushFace(Mesh);

  // Store the serial edges of NextBorderEdge before we reconstruct the mesh
  epa_halfedge* NextBorderEdge = BorderEdge->Next->Next;
  epa_halfedge* PreviousBorderEdge = BorderEdge->Previous;

  if(NextBorderEdge == PreviousBorderEdge)
  {
    // Hook up a new Face to the Internal Edges
    epa_halfedge* InternalEdge0 = BorderEdge->Next;
    epa_halfedge* InternalEdge1 = BorderEdge->Previous;
    epa_halfedge* InternalEdge2 = BorderEdge;
    Assert(!InternalEdge0->LeftFace);
    Assert(!InternalEdge1->LeftFace);
    Assert(!InternalEdge2->LeftFace);

    // Pick out the relevant points
    epa_vertex* P0 = InternalEdge0->TargetVertex;
    epa_vertex* P1 = InternalEdge1->TargetVertex;
    epa_vertex* P2 = InternalEdge2->TargetVertex;

    Assert(P1->P.S == DebugAssertVertex)
    Assert(InternalEdge0->TargetVertex->P.S == P0->P.S);
    Assert(InternalEdge0->Opposite->TargetVertex->P.S == P2->P.S);
    Assert(InternalEdge1->TargetVertex->P.S == P1->P.S);
    Assert(InternalEdge1->Opposite->TargetVertex->P.S == P0->P.S);
    Assert(InternalEdge2->TargetVertex->P.S == P2->P.S);
    Assert(InternalEdge2->Opposite->TargetVertex->P.S == P1->P.S);

    Assert(InternalEdge2->Next == InternalEdge0);
    Assert(InternalEdge0->Previous == InternalEdge2);

    Face->Edge = InternalEdge0;
    InternalEdge0->LeftFace = Face;
    InternalEdge1->LeftFace = Face;
    InternalEdge2->LeftFace = Face;
  }else{

    epa_halfedge* InternalEdge0 = BorderEdge->Next;
    epa_halfedge* InternalEdge1 = PushStruct(Mesh->Arena, epa_halfedge);
    epa_halfedge* ExternalEdge1 = PushStruct(Mesh->Arena, epa_halfedge);
    epa_halfedge* InternalEdge2 = BorderEdge;

    Assert(!NextBorderEdge->LeftFace);
    Assert(!PreviousBorderEdge->LeftFace);

    // Hook up Internal with External Edges (Parallel)
    ExternalEdge1->Opposite = InternalEdge1;
    InternalEdge1->Opposite = ExternalEdge1;

    // Hook up Internal Edges (Form the inside of the triangle)
    InternalEdge0->Next = InternalEdge1;
    InternalEdge1->Next = InternalEdge2;
    Assert(InternalEdge2->Next = InternalEdge0);

    Assert(InternalEdge0->Previous == InternalEdge2);
    InternalEdge2->Previous = InternalEdge1;
    InternalEdge1->Previous = InternalEdge0;

    // Hook up External Edges Serial (Connect to the rest of the mesh)
    PreviousBorderEdge->Next = ExternalEdge1;
    ExternalEdge1->Next = NextBorderEdge;
    NextBorderEdge->Previous = ExternalEdge1;
    ExternalEdge1->Previous = PreviousBorderEdge;

    // Hook up the new Face to the Internal Edges
    Face->Edge = InternalEdge0;
    Assert(!InternalEdge0->LeftFace);
    InternalEdge0->LeftFace = Face;
    Assert(!InternalEdge1->LeftFace);
    InternalEdge1->LeftFace = Face;
    Assert(!InternalEdge2->LeftFace);
    InternalEdge2->LeftFace = Face;

    // Pick out the relevant points
    epa_vertex* P0 = InternalEdge0->TargetVertex;
    epa_vertex* P1 = InternalEdge2->Opposite->TargetVertex;
    epa_vertex* P2 = InternalEdge2->TargetVertex;

    Assert(InternalEdge2->Opposite->TargetVertex->P.S == PreviousBorderEdge->TargetVertex->P.S);
    Assert(P1->P.S == DebugAssertVertex)
    Assert(P0->P.S != P1->P.S);
    Assert(P1->P.S != P2->P.S);
    Assert(P2->P.S != P0->P.S);

    // Make our edges point to the points
    Assert(InternalEdge0->TargetVertex->P.S == P0->P.S);
    Assert(InternalEdge0->Opposite->TargetVertex->P.S == P2->P.S);
    InternalEdge1->TargetVertex = P1;
    ExternalEdge1->TargetVertex = P0;
    Assert(InternalEdge2->TargetVertex->P.S == P2->P.S);
    Assert(InternalEdge2->Opposite->TargetVertex->P.S == P1->P.S);
  }

  #if HANDMADE_SLOW
    r32 FaceArea = GetFaceArea(Face);
    Assert(FaceArea > F32_TOL);
  #endif

}


epa_halfedge* GetBorderEdge(epa_mesh* Mesh)
{
  epa_face* Face = Mesh->Faces;
  while(Face)
  {
    epa_halfedge* Edge = Face->Edge;
    while(Edge->Next != Face->Edge)
    {
      if(!Edge->Opposite->LeftFace)
      {
        Assert(Edge->LeftFace);
        return Edge->Opposite;
      }
      Edge = Edge->Next;
    }
    Face = Face->Next;
  }
  return 0;
}


// The halfedge returned will always point away from vertex
epa_halfedge* GetBorderEdge(epa_vertex* Vertex)
{
  epa_halfedge* Edge = Vertex->OutgoingEdge;
  epa_halfedge* Result = 0;
  for(;;)
  {
    if(!Edge->LeftFace)
    {
      Result = Edge;
      break;
    }

    Edge = Edge->Opposite->Next;

    if(Edge == Vertex->OutgoingEdge)
    {
      break;
    }
  }

  return Result;
}

void FillHole(epa_mesh* Mesh, gjk_support* NewPoint)
{
  epa_halfedge* BorderEdge = GetBorderEdge(Mesh);
  Assert(BorderEdge);
  epa_vertex* Vertex = AddNewPoint(Mesh, BorderEdge, NewPoint);

  while((BorderEdge = GetBorderEdge(Vertex)) != NULL)
  {
    Assert(BorderEdge->Opposite->TargetVertex->P.S == NewPoint->S);
    Assert(!BorderEdge->LeftFace);

    AddNewEdge(Mesh, BorderEdge, Vertex->P.S);
  }
}


/* Checks the area of the 4 triangles that result from splitting 2 triangles in the middle
 *     p1           p1
 *     /\          /|\
 *    / 0\        /1|0\
 *  p2----p0 -> p2--|--p0
 *    \1 /        \2|3/
 *     \/          \|/
 *     p3          p3
 */
void DEBUGCheckAreasOfSegmentedTriangle(epa_halfedge* Edge, v3 p)
{
  b32 OnSegment = IsPointOnLinesegment(Edge->Opposite->TargetVertex->P.S,
                                       Edge->TargetVertex->P.S, p);
  v3 p00 = Edge->TargetVertex->P.S;
  v3 p01 = Edge->Next->TargetVertex->P.S;
  v3 n0 = GetPlaneNormal(p00,p01,p);
  r32 NewSignedSubArea0 = n0 * CrossProduct( p01 - p00, p - p00);

  v3 p10 = Edge->Next->TargetVertex->P.S;
  v3 p11 = Edge->Previous->TargetVertex->P.S;
  v3 n1 = GetPlaneNormal(p10,p11,p);
  r32 NewSignedSubArea1 = n1 * CrossProduct( p11 - p10, p - p10);

  v3 p20 = Edge->Opposite->TargetVertex->P.S;
  v3 p21 = Edge->Opposite->Next->TargetVertex->P.S;
  v3 n2 = GetPlaneNormal(p20,p21,p);
  r32 NewSignedSubArea2 = n2 * CrossProduct( p21 - p20, p - p20);

  v3 p30 = Edge->Opposite->Next->TargetVertex->P.S;
  v3 p31 = Edge->Opposite->Previous->TargetVertex->P.S;
  v3 n3 = GetPlaneNormal(p30,p31,p);
  r32 NewSignedSubArea3 = n3 * CrossProduct( p31 - p30, p - p30);

  // How we may wanna segment a line:
  // RemoveFace(Mesh, Edge0->LeftFace );
  // RemoveFace(Mesh, Edge0->Opposite->LeftFace);
  // FillHole(Mesh, Point);
}

b32 GrowMesh(epa_mesh* Mesh, gjk_support* Point)
{
  epa_face* Face = Mesh->Faces;
  while(Face)
  {
    v3 P0, P1, P2;
    v3 P = Point->S;
    GetFacePoints(Face, &P0, &P1, &P2);
    if(P == P0){
      return false;
    }else if(P == P1){
      return false;
    }else if(P == P2){
      return false;
    }else if(IsPointOnLinesegment(P2, P0, P)){
      return false;
    }else if(IsPointOnLinesegment(P0, P1, P)){
      return false;
    }else if(IsPointOnLinesegment(P1, P2, P)){
      return false;
    }
    Face = Face->Next;
  }

  Face = Mesh->Faces;
  while(Face)
  {
    v3 P0, P1, P2;
    GetFacePoints(Face, &P0, &P1, &P2);
    const v3 Normal = GetPlaneNormal(P0, P1, P2);
    const v3 ProjectedPoint = ProjectPointOntoPlane( Point->S, P0, Normal );
    r32 Diff = NormSq(ProjectedPoint-Point->S);
    v3 P = Point->S;
    r32 SignedSubArea0 = Normal * CrossProduct( P2 - P1, P - P1);
    r32 SignedSubArea1 = Normal * CrossProduct( P0 - P2, P - P2);
    r32 SignedSubArea2 = Normal * CrossProduct( P1 - P0, P - P0);

    // Point Lies on the plane of the Face
    if ((Diff <= F32_TOL) && (SignedSubArea0 > F32_TOL) && (SignedSubArea1 > F32_TOL) && (SignedSubArea2 > F32_TOL))
    {
      return false;
    }
    Face = Face->Next;
  }

  RemoveFacesSeenByPoint(Mesh, Point);
  Assert(Mesh->Faces);
  FillHole( Mesh, Point);
  return true;
}