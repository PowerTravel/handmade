#include "halfedge_mesh.h"
#include "standalone_utility.h"

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
GetFaceNormal(const gjk_face* Face)
{
  return GetPlaneNormal(Face->Edge->TargetVertex->P.S,
                        Face->Edge->NextEdge->TargetVertex->P.S,
                        Face->Edge->NextEdge->NextEdge->TargetVertex->P.S);
}

internal inline b32
Equals( gjk_vertex* A, gjk_vertex* B )
{
  return A->P.S == B->P.S;
}

internal inline b32
IsBorderEdge(gjk_halfedge* Edge)
{
  return (!Edge->LeftFace &&  Edge->OppositeEdge->LeftFace) ||
         ( Edge->LeftFace && !Edge->OppositeEdge->LeftFace);
}

internal inline b32
Equals(const gjk_halfedge* A, const gjk_halfedge* B)
{
  const v3& TargetA   = A->TargetVertex->P.S;
  const v3& OutgoingA = A->OppositeEdge->TargetVertex->P.S;

  const v3& TargetB   = B->TargetVertex->P.S;
  const v3& OutgoingB = B->OppositeEdge->TargetVertex->P.S;

  return ( TargetA == OutgoingB ) && (OutgoingA == TargetB ) ||
         ( TargetA == TargetB   ) && (OutgoingA == OutgoingB );
}


/*
 * A Tetrahedron is made up of 4 Points assembling into 4 Triangles.
 * The normals of the triangles should point outwards.
 * This function Fixes the winding of the tetrahedron such that
 * Dot( v03, Cross( v01, v02 )) < 0; (Tripple Product)
 * Or in words: Define the bottom triangle to be {p0,p1,p2} with Normal : (p1-p0) x (p2-p0).
 * The Tetrahedron is then considered wound CCW if Dot( p3-p0 , Normal ) < 0.
 * If Dot( p3-p0 , Normal ) == 0 Our p3 lies on the plane of the bottom triangle and
 * we need another point to  be p3.
 */
void FixWindingCCW(gjk_support Support[4])
{
  // Fix Winding so that all triangles go ccw
  v3 v01 = Support[1].S - Support[0].S;
  v3 v02 = Support[2].S - Support[0].S;
  v3 v03 = Support[3].S - Support[0].S;
  const r32 Determinant = v03 * CrossProduct(v01,v02);
  Assert(Abs(Determinant) > 10E-7);

  if(Determinant > 0.f)
  {
  	// Swap first and third Support
    gjk_support Tmp = Support[0];
    Support[0]   = Support[3];
    Support[3]   = Tmp;
  }
}

internal inline gjk_vertex *
PushVertex(gjk_mesh* Mesh)
{
  gjk_vertex* NewVertex = (gjk_vertex*) PushStruct(Mesh->Arena, gjk_vertex );
  NewVertex->Idx =  Mesh->VerticeIdxCounter++;
  NewVertex->Previous = Mesh->Vertices;
  Mesh->Vertices = NewVertex;
  return NewVertex;
}

internal inline gjk_face *
PushFace(gjk_mesh* Mesh)
{
  gjk_face* NewFace = (gjk_face*) PushStruct(Mesh->Arena, gjk_face );
  NewFace->Idx =  Mesh->FaceIdxCounter++;
  NewFace->Previous = Mesh->Faces;
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
 *    4 / / 1         0 \ \ 3
 *     / /               \ \
 *    / /                 \ \
 *   / v         2         \ v
 *    C-------------------> A
 *   <------------------------
 *               5
 */

internal gjk_mesh *
CreateInitialMesh(memory_arena* Arena, gjk_support* a,  gjk_support* b,  gjk_support* c)
{
  gjk_mesh* Result = PushStruct(Arena,gjk_mesh);
  Result->Arena    = Arena;

  // Allocate 6 edges
  gjk_halfedge* NewEdges = (gjk_halfedge*) PushArray(Result->Arena, 6, gjk_halfedge);

  // Connect edges:
  // Internal edges [0,1,2] (Pointing to Face)
  NewEdges[0].NextEdge = &NewEdges[1];
  NewEdges[1].NextEdge = &NewEdges[2];
  NewEdges[2].NextEdge = &NewEdges[0];

  // External Edges [3,4,5]
  NewEdges[3].NextEdge = &NewEdges[4];
  NewEdges[4].NextEdge = &NewEdges[5];
  NewEdges[5].NextEdge = &NewEdges[3];

  // Connect Internal and Externa; Edges
  NewEdges[0].OppositeEdge = &NewEdges[3];
  NewEdges[0].OppositeEdge->OppositeEdge = &NewEdges[0];
  NewEdges[1].OppositeEdge = &NewEdges[5];
  NewEdges[1].OppositeEdge->OppositeEdge = &NewEdges[1];
  NewEdges[2].OppositeEdge = &NewEdges[4];
  NewEdges[2].OppositeEdge->OppositeEdge = &NewEdges[2];


  // Create Face
  gjk_face* NewFace = PushFace(Result);
  //Connect internal edges to Face
  NewEdges[0].LeftFace = NewFace;
  NewEdges[1].LeftFace = NewFace;
  NewEdges[2].LeftFace = NewFace;
  // Connect Face to an internal edge
  NewFace->Edge = &NewEdges[0];

  // Create Vertices
  gjk_vertex* A = PushVertex(Result);
  gjk_vertex* B = PushVertex(Result);
  gjk_vertex* C = PushVertex(Result);
  A->P = *a;
  B->P = *b;
  C->P = *c;
  // Connect Edges to Vertecies
  NewEdges[0].TargetVertex = B;
  NewEdges[1].TargetVertex = C;
  NewEdges[2].TargetVertex = A;
  NewEdges[0].OppositeEdge->TargetVertex = A;
  NewEdges[1].OppositeEdge->TargetVertex = B;
  NewEdges[2].OppositeEdge->TargetVertex = C;
  // Connect Vertecies to edges
  A->OutgoingEdge = &NewEdges[0];
  B->OutgoingEdge = &NewEdges[1];
  C->OutgoingEdge = &NewEdges[2];


  return Result;
};

internal inline void
CalculateFaceNormals(gjk_mesh* Mesh)
{
  gjk_face* Face = Mesh->Faces;
  while(Face)
  {
    Face->Normal = GetFaceNormal(Face);
    Face = Face->Previous;
  }
}

internal inline gjk_halfedge *
GetPreviousEdge( gjk_halfedge* Edge )
{
  // If we are on the border or in a hole we must
  // iterate untill we made the whole round.
  // Holes should not be that big though.
  // If we ever start having big flat meshes with big borders,
  // Look over this function and see if we can do better
  gjk_halfedge* Result = Edge->NextEdge;
  while(Result->NextEdge != Edge )
  {
    Result = Result->NextEdge;
  }
  return Result;
}


internal void
FillHole(gjk_mesh* Mesh, gjk_halfedge* MeshEdge, gjk_support* NewPoint)
{
  Assert( IsBorderEdge(MeshEdge) );
  MeshEdge = !MeshEdge->LeftFace ? MeshEdge : MeshEdge->OppositeEdge;

  gjk_halfedge* BorderEdge = MeshEdge;
  u32 NrBorderEdges = 0;
  do
  {
    ++NrBorderEdges;
    BorderEdge = BorderEdge->NextEdge;
  }while(BorderEdge != MeshEdge);

  Assert( NrBorderEdges >= 3 );
  const u32 NrHalfEdges = 2 * NrBorderEdges;
  gjk_halfedge* NewEdges = (gjk_halfedge*) PushArray(Mesh->Arena, NrHalfEdges, gjk_halfedge);
  // We have designed the for loop below such that even NewEdges points TO NewVertex and
  // Odd NewEdges points away from.
  gjk_vertex* NewVertex = PushVertex(Mesh);
  NewVertex->P = *NewPoint;
  NewVertex->OutgoingEdge = &NewEdges[1];
  BorderEdge = MeshEdge;
  for (u32 Idx = 0; Idx < NrBorderEdges; ++Idx)
  {
    Assert(BorderEdge->LeftFace == 0);
    // Internal NewFace Edges E0,E1,E2;
    gjk_halfedge* E0 = &NewEdges[2*Idx];   // To New Vertex
    gjk_halfedge* E1 = &NewEdges[2*Idx+1]; // From New Vertex
    gjk_halfedge* E2 = BorderEdge;         // Faces Mesh
    BorderEdge = BorderEdge->NextEdge;

    gjk_face* NewFace =PushFace(Mesh);
    NewFace->Edge = E0;

    E0->TargetVertex = NewVertex;
    E0->NextEdge = E1;
    E0->LeftFace = NewFace;

    E1->TargetVertex = E2->OppositeEdge->TargetVertex;
    E1->NextEdge = E2;
    E1->LeftFace = NewFace;

    E2->NextEdge = E0;
    E2->LeftFace = NewFace;

    //u32 NextTriangleIndex = (2*Idx+3) % (2*NrBorderEdges);
    //E0->OppositeEdge = &NewEdges[NextTriangleIndex];
    //E0->OppositeEdge->OppositeEdge = E0;
  }

  for (u32 i = 0; i < NrHalfEdges; ++i)
  {
    gjk_halfedge* Edge0 =  &NewEdges[i];
    if(Edge0->OppositeEdge) continue;

    gjk_vertex* E0From = GetPreviousEdge(Edge0)->TargetVertex;
    gjk_vertex* E0To = Edge0->TargetVertex;

    for (u32 j = 0; j < NrHalfEdges; ++j)
    {
      if(i==j) continue;

      gjk_halfedge* Edge1 =  &NewEdges[j];
      gjk_vertex* E1From = GetPreviousEdge(Edge1)->TargetVertex;
      gjk_vertex* E1To = Edge1->TargetVertex;

      if( (E0From == E1To) && (E0To == E1From)  )
      {
        Edge0->OppositeEdge = Edge1;
        Edge1->OppositeEdge = Edge0;
      }
    }
  }

  CalculateFaceNormals(Mesh);
}


internal inline b32
IsTargetVertexOutgoing( gjk_halfedge* Edge )
{
  gjk_vertex* A = Edge->TargetVertex;
  gjk_vertex* B = A->OutgoingEdge->OppositeEdge->TargetVertex;
  return Equals(A,B);
}

internal inline u32
GetNrIncidentEdges( gjk_vertex* Vertex )
{
  u32 Result = 0;
  gjk_halfedge* NextIncidentEdge = Vertex->OutgoingEdge->OppositeEdge->NextEdge;
  while( !Equals(Vertex->OutgoingEdge, NextIncidentEdge) )
  {
    NextIncidentEdge = NextIncidentEdge->OppositeEdge->NextEdge;
    ++Result;
  }
  return Result;
}


internal inline void DissconectEdge( gjk_halfedge* Edge )
{
  gjk_halfedge* PreviousEdge = GetPreviousEdge(Edge);
  PreviousEdge->NextEdge = Edge->OppositeEdge->NextEdge;

  gjk_halfedge* OppositePreviousEdge = GetPreviousEdge(Edge->OppositeEdge);
  OppositePreviousEdge->NextEdge = Edge->NextEdge;
}

internal gjk_halfedge *
RemoveFacesSeenByPoint(gjk_mesh* Mesh, const v3& Point)
{
  gjk_face** FaceListEntry = &Mesh->Faces;
  gjk_face* FacesToDissconnect = 0;
  while(*FaceListEntry)
  {
    // Note Jakob(): To check if a given face can be "seen" by the new point
    // you want to form a vector from a vertex of said face to the new point
    // and check the dot product of that with the face normal.
    // So I replaced
    //   if( (Point * Face->Normal) > 0)
    // with
    //   if((Point - PointOnFace) * Face->Normal) > 0)
    // Makes sense since even if the Point and face are aligned in the same direction
    // the face can be so far to the side that its no longer 'visible'
    // Hovever by first forming a vector from the point to the face and take the dot
    // with that we correct for this.

    gjk_face* Face  = *FaceListEntry;
    const v3 FaceToPoint = (Point - Face->Edge->TargetVertex->P.S);
    const r32 DotProduct = Face->Normal*FaceToPoint;
    if(DotProduct > 0)
    {
      // Face is now removed from Mesh Face List
      *FaceListEntry = Face->Previous;
      // And added to the dissconnect list
      Face->Previous = FacesToDissconnect;
      FacesToDissconnect = Face;
    }else{
      FaceListEntry = &(*FaceListEntry)->Previous;
    }
  }

  while(FacesToDissconnect)
  {
    // Dissconect face from edges
    gjk_halfedge*  Edges[3] = { FacesToDissconnect->Edge,                      // Edge: A->B
                                FacesToDissconnect->Edge->NextEdge,            // Edge: B->C
                                FacesToDissconnect->Edge->NextEdge->NextEdge}; // Edge: C->A

    gjk_vertex*  Vertices[3]= { Edges[2]->TargetVertex,   // Vertex A
                                Edges[0]->TargetVertex,   // Vertex B
                                Edges[1]->TargetVertex};  // Vertex C

    Assert(FacesToDissconnect->Edge == Edges[2]->NextEdge);

    for(u32 EdgeIdx = 0; EdgeIdx < 3; ++EdgeIdx)
    {
      gjk_halfedge* Edge = Edges[EdgeIdx];
      gjk_vertex* A = Edge->OppositeEdge->TargetVertex;
      gjk_vertex* B = Edge->TargetVertex;
      Edge->LeftFace = 0;
      // Only dissconnect if No face to either side of the edge exists
      if(!Edge->OppositeEdge->LeftFace)
      {
        // Reroute the vertices outgoing edges so they don't point to this edge:
        // If Edge's Source vertex is pointing back to Edge0 we need to re-route it
        if( Equals(A->OutgoingEdge, Edge) )
        {
          A->OutgoingEdge = A->OutgoingEdge->OppositeEdge->NextEdge;
        }

        // If Edge's target vertex is pointing back to Edge we need to re-route it
        if( Equals(B->OutgoingEdge, Edge) )
        {
          B->OutgoingEdge = B->OutgoingEdge->OppositeEdge->NextEdge;
        }

        DissconectEdge(Edge);
      }
    }
    FacesToDissconnect = FacesToDissconnect->Previous;
  }

  gjk_vertex** VertexListEntry = &Mesh->Vertices;
  gjk_halfedge* BorderEdge = 0;
  while(*VertexListEntry)
  {
    gjk_vertex* Vertex = *VertexListEntry;

    // Check if there is only 1 edge attached to the vertex
    if( Vertex->OutgoingEdge == Vertex->OutgoingEdge->OppositeEdge->NextEdge)
    {
      Assert(!Vertex->OutgoingEdge->LeftFace && !Vertex->OutgoingEdge->OppositeEdge->LeftFace);
      *VertexListEntry = Vertex->Previous;
    }else{
      if(!BorderEdge && IsBorderEdge(Vertex->OutgoingEdge))
      {
        BorderEdge = Vertex->OutgoingEdge;
      }
      VertexListEntry = &(*VertexListEntry)->Previous;
    }
  }
  return BorderEdge;
}


b32 IsPointInMesh(const gjk_mesh* Mesh, const v3& Point)
{
  gjk_vertex* Vertex = Mesh->Vertices;
  while(Vertex)
  {
    if(Vertex->P.S == Point)
    {
      return true;
    }
    Vertex = Vertex->Previous;
  }
  return false;
}


internal char*
PaddWithZeros(u32 Count, char* Scanner)
{
  for (u32 i = 0; i < Count; ++i)
  {
    Scanner += str::itoa(0, 16, Scanner);
    if(i+1 < Count)
      *Scanner++ = ' ';
  }
  return Scanner;
}

gjk_mesh* CreateSimplexMesh(memory_arena* Arena, gjk_simplex* Simplex )
{
  gjk_support Tetrahedron[4] = {Simplex->SP[0],Simplex->SP[1],Simplex->SP[2],Simplex->SP[3]};
  FixWindingCCW(Tetrahedron);

  gjk_mesh* Result = CreateInitialMesh(Arena, &Tetrahedron[0], &Tetrahedron[1], &Tetrahedron[2]);

  FillHole( Result, Result->Faces->Edge, &Tetrahedron[3]);

  return Result;
}

gjk_face* GetCLosestFaceToOrigin(gjk_mesh* Mesh, r32* ResultDistance )
{
  // TODO: Add limit macro for max float value
  *ResultDistance = 10E30;
  gjk_face* ResultFace = 0;
  gjk_face* Face = Mesh->Faces;
  while(Face)
  {
    const v3 Projection = ProjectPointOntoPlane( V3(0,0,0), Face->Edge->TargetVertex->P.S, Face->Normal );
    const r32 DistanceToFace = Norm(Projection);
    if( DistanceToFace < *ResultDistance )
    {
      *ResultDistance = DistanceToFace;
      ResultFace = Face;
    }
    Face = Face->Previous;
  }
  return ResultFace;
}

void DebugPrintEdges(memory_arena* Arena, gjk_mesh* Mesh, b32 Append, platform_api* API )
{
  if(!API) return;
  char FilePath[] = "..\\handmade\\code\\matlab\\data\\EPAPolytypeEdges.m";

  gjk_face* Face = Mesh->Faces;
  u32 FaceCount = 0;
  while(Face)
  {
    ++FaceCount;
    Face = Face->Previous;
  }

  temporary_memory TempMem = BeginTemporaryMemory(Arena);
  char* DebugString = (char*) PushArray(Arena, 3*(FaceCount+2)*64, char);
  char* Scanner = DebugString;

  Face = Mesh->Faces;
  while(Face)
  {
    gjk_halfedge* E[3] = {Face->Edge,
                          Face->Edge->NextEdge,
                          Face->Edge->NextEdge->NextEdge};

    //Assert( E[0]->OppositeEdge->TargetVertex->P.S == E[2]->TargetVertex->P.S);
    for (int i = 0; i < 3; ++i)
    {
      b32 Border = (E[i]->OppositeEdge->LeftFace != 0);
      v3 From    =  E[i]->OppositeEdge->TargetVertex->P.S;
      v3 To      =  E[i]->TargetVertex->P.S;
      Scanner += str::itoa(Border, 64, Scanner);
      *Scanner++ = ' ';
      Scanner += str::ToString(From, 4, 64, Scanner);
      *Scanner++ = ' ';
      Scanner += str::ToString(To, 4, 64, Scanner);
      *Scanner++ = '\n';
    }
    Face = Face->Previous;
  }

  thread_context Thread = {};
  if(Append)
  {
    API->DEBUGPlatformAppendToFile(&Thread, FilePath, str::StringLength(DebugString), (void*)DebugString);
  }else{
    API->DEBUGPlatformWriteEntireFile(&Thread, FilePath, str::StringLength(DebugString), (void*)DebugString);
  }

  EndTemporaryMemory(TempMem);
}

void DebugPrintPolytype(memory_arena* Arena, gjk_mesh* Mesh, b32 Append, platform_api* API )
{
  if(!API) return;
  char FilePath[] = "..\\handmade\\code\\matlab\\data\\EPAPolytypeSeries.m";

  gjk_face* Face = Mesh->Faces;
  u32 FaceCount = 0;
  while(Face)
  {
    ++FaceCount;
    Face = Face->Previous;
  }

  gjk_vertex* Vertex = Mesh->Vertices;
  u32 VerticeCount = 0;
  while(Vertex)
  {
    ++VerticeCount;
    Vertex = Vertex->Previous;
  }

  temporary_memory TempMem = BeginTemporaryMemory(Arena);
  char* DebugString = (char*) PushArray(Arena, (VerticeCount + FaceCount+2)*64, char);
  char* Scanner = DebugString;

  // 1 means vertex data
  Scanner += str::itoa(1, 64, Scanner);
  *Scanner++ = ' ';
  Scanner += str::itoa(VerticeCount, 64, Scanner);
  *Scanner++ = ' ';
  Scanner += str::itoa(3, 64, Scanner); // Vertex used to remove faces Hardcoded untill we actually have something other than a simplex
  *Scanner++ = ' ';
  Scanner += str::itoa(0, 64, Scanner); // Padding
  *Scanner++ = '\n';

  Vertex = Mesh->Vertices;
  while(Vertex)
  {
    Scanner += str::itoa(VerticeCount-(Vertex->Idx), 64, Scanner);
    *Scanner++ = ' ';
    Scanner += str::ToString(Vertex->P.S, 4, 64, Scanner);
    *Scanner++ = '\n';
    Vertex = Vertex->Previous;
  }

  // 2 means face index data
  Scanner += str::itoa(2, 64, Scanner);
  *Scanner++ = ' ';
  Scanner += str::itoa(FaceCount, 64, Scanner);
  *Scanner++ = ' ';
  Scanner = PaddWithZeros(2,Scanner); // Padding
  *Scanner++ = '\n';

  Face = Mesh->Faces;
  while(Face)
  {
    Scanner += str::itoa(FaceCount-(Face->Idx), 64, Scanner);
    *Scanner++ = ' ';
    Scanner += str::itoa(VerticeCount-(Face->Edge->TargetVertex->Idx), 64, Scanner);
    *Scanner++ = ' ';
    Scanner += str::itoa(VerticeCount-(Face->Edge->NextEdge->TargetVertex->Idx), 64, Scanner);
    *Scanner++ = ' ';
    Scanner += str::itoa(VerticeCount-(Face->Edge->NextEdge->NextEdge->TargetVertex->Idx), 64, Scanner);
    *Scanner++ = '\n';
    Face = Face->Previous;
  }
/*
  if(RemovedFaceList.GetSize())
  {
    // 3 means removed face  data
    Scanner += str::itoa(3, 64, Scanner);
    *Scanner++ = ' ';
    Scanner += str::itoa(RemovedFaceList.GetSize(), 64, Scanner);
    *Scanner++ = ' ';
    Scanner = PaddWithZeros(2,Scanner); // Padding
    *Scanner++ = '\n';


    FaceIndex = 1;
    for(RemovedFaceList.First(); !RemovedFaceList.IsEnd(); RemovedFaceList.Next())
    {
      gjk_face F = FaceList.Get();
      Scanner += str::itoa(RemovedFaceList.Get(), 64, Scanner);
      *Scanner++ = ' ';
      Scanner = PaddWithZeros(3,Scanner); // Padding
      *Scanner++ = '\n';
    }
  }
  */

  thread_context Thread = {};
  if(Append)
  {
    API->DEBUGPlatformAppendToFile(&Thread, FilePath, str::StringLength(DebugString), (void*)DebugString);
  }else{
    API->DEBUGPlatformWriteEntireFile(&Thread, FilePath, str::StringLength(DebugString), (void*)DebugString);
  }

  EndTemporaryMemory(TempMem);
}