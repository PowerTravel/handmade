#include "component_gjk_epa_visualizer.h"
#include "epa_collision_data.h"
#include "gjk_narrow_phase.h"
#include "memory.h"
#include "component_collider.h"


// Todo: Should we store a void* in the structures to be used for metadata?
//       Ex, Face Normals, Support points, Makes this a more general mesh datastructure

struct epa_halfedge;
struct epa_face;

struct epa_vertex
{
  u32 Idx;
  gjk_support P;
  epa_halfedge* OutgoingEdge;
  epa_vertex* Next;
};

struct epa_halfedge
{
  epa_vertex*   TargetVertex;
  epa_face*     LeftFace;
  epa_halfedge* NextEdge;
  epa_halfedge* OppositeEdge;
};

struct epa_face
{
  u32 Idx;
  v3 Normal;
  epa_halfedge* Edge;
  epa_face* Next;
};

struct epa_mesh
{
  u32 VerticeIdxCounter;
  u32 FaceIdxCounter;
  memory_arena* Arena;
  epa_vertex* Vertices;
  epa_face* Faces;
};


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

    Each Face forms a loop of halfedge-pairs [joining together vertices]

    1: Remove all references in the mesh to the Face:
        For each HalfEdge around the Face delete the reference back to it's Face.
        Nothing in the mesh is now pointing TO the Face.

    2: Check if any vertices needs to be removed:
        A verice needs to be removed IF all incident Edges needs to be removed.
        A Edge needs to be removed if none of it's half-edges points to a face
  */

internal inline v3
GetFaceNormal(const epa_face* Face)
{
  TIMED_BLOCK();
  return GetPlaneNormal(Face->Edge->TargetVertex->P.S,
                        Face->Edge->NextEdge->TargetVertex->P.S,
                        Face->Edge->NextEdge->NextEdge->TargetVertex->P.S);
}

internal inline b32
Equals( epa_vertex* A, epa_vertex* B )
{
  return A->P.S == B->P.S;
}

internal inline b32
IsBorderEdge(epa_halfedge* Edge)
{
  return (!Edge->LeftFace &&  Edge->OppositeEdge->LeftFace) ||
         ( Edge->LeftFace && !Edge->OppositeEdge->LeftFace);
}

internal inline b32
Equals(const epa_halfedge* A, const epa_halfedge* B)
{
  const v3& TargetA   = A->TargetVertex->P.S;
  const v3& OutgoingA = A->OppositeEdge->TargetVertex->P.S;

  const v3& TargetB   = B->TargetVertex->P.S;
  const v3& OutgoingB = B->OppositeEdge->TargetVertex->P.S;

  return ( TargetA == OutgoingB ) && (OutgoingA == TargetB ) ||
         ( TargetA == TargetB   ) && (OutgoingA == OutgoingB );
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

internal void DebugPrintEdges(memory_arena* Arena, epa_mesh* Mesh, b32 Append, platform_api* API )
{
  if(!API) return;
  char FilePath[] = "..\\handmade\\code\\matlab\\data\\EPAPolytypeEdges.m";

  epa_face* Face = Mesh->Faces;
  u32 FaceCount = 0;
  while(Face)
  {
    ++FaceCount;
    Face = Face->Next;
  }

  temporary_memory TempMem = BeginTemporaryMemory(Arena);
  char* DebugString = (char*) PushArray(Arena, 3*(FaceCount+2)*64, char);
  char* Scanner = DebugString;

  Face = Mesh->Faces;
  while(Face)
  {
    epa_halfedge* E[3] = {Face->Edge,
                          Face->Edge->NextEdge,
                          Face->Edge->NextEdge->NextEdge};

    //Assert( E[0]->OppositeEdge->TargetVertex->P.S == E[2]->TargetVertex->P.S);
    for (int i = 0; i < 3; ++i)
    {
      b32 Border = (E[i]->OppositeEdge->LeftFace != 0) && (E[i]->LeftFace != 0);
      v3 From    =  E[i]->OppositeEdge->TargetVertex->P.S;
      v3 To      =  E[i]->TargetVertex->P.S;
      Scanner += str::itoa(Border, 64, Scanner);
      *Scanner++ = ' ';
      Scanner += str::ToString(From, 4, 64, Scanner);
      *Scanner++ = ' ';
      Scanner += str::ToString(To, 4, 64, Scanner);
      *Scanner++ = '\n';
    }
    Face = Face->Next;
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
  TIMED_BLOCK();
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
    Support[0] = Support[3];
    Support[3] = Tmp;
  }
}

internal inline epa_vertex *
PushVertex(epa_mesh* Mesh)
{
  epa_vertex* NewVertex = (epa_vertex*) PushStruct(Mesh->Arena, epa_vertex );
  NewVertex->Idx =  Mesh->VerticeIdxCounter++;
  NewVertex->Next = Mesh->Vertices;
  Mesh->Vertices = NewVertex;
  return NewVertex;
}

internal inline epa_face *
PushFace(epa_mesh* Mesh)
{
  epa_face* NewFace = (epa_face*) PushStruct(Mesh->Arena, epa_face );
  NewFace->Idx =  Mesh->FaceIdxCounter++;
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
 *    4 / / 1         0 \ \ 3
 *     / /               \ \
 *    / /                 \ \
 *   / v         2         \ v
 *    C-------------------> A
 *   <------------------------
 *               5
 */

internal epa_mesh *
CreateInitialMesh(memory_arena* Arena, gjk_support* a,  gjk_support* b,  gjk_support* c)
{
  TIMED_BLOCK();
  epa_mesh* Result = PushStruct(Arena,epa_mesh);
  Result->Arena    = Arena;

  // Allocate 6 edges
  epa_halfedge* NewEdges = (epa_halfedge*) PushArray(Result->Arena, 6, epa_halfedge);

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
  epa_face* NewFace = PushFace(Result);
  //Connect internal edges to Face
  NewEdges[0].LeftFace = NewFace;
  NewEdges[1].LeftFace = NewFace;
  NewEdges[2].LeftFace = NewFace;
  // Connect Face to an internal edge
  NewFace->Edge = &NewEdges[0];

  // Create Vertices
  epa_vertex* A = PushVertex(Result);
  epa_vertex* B = PushVertex(Result);
  epa_vertex* C = PushVertex(Result);
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
CalculateFaceNormals(epa_mesh* Mesh)
{
  epa_face* Face = Mesh->Faces;
  while(Face)
  {
    Face->Normal = GetFaceNormal(Face);
    Face = Face->Next;
  }
}

internal inline epa_halfedge *
GetPreviousEdge( epa_halfedge* Edge )
{
  // If we are on the border or in a hole we must
  // iterate untill we made the whole round.
  // Holes should not be that big though.
  // If we ever start having big flat meshes with big borders,
  // Look over this function and see if we can do better
  epa_halfedge* Result = Edge->NextEdge;
  while(Result->NextEdge != Edge )
  {
    Result = Result->NextEdge;
  }
  return Result;
}


internal void
FillHole(epa_mesh* Mesh, epa_halfedge* MeshEdge, gjk_support* NewPoint)
{
  TIMED_BLOCK();
  Assert( IsBorderEdge(MeshEdge) );
  MeshEdge = !MeshEdge->LeftFace ? MeshEdge : MeshEdge->OppositeEdge;

  epa_halfedge* BorderEdge = MeshEdge;
  u32 NrBorderEdges = 0;
  do
  {
    ++NrBorderEdges;
    BorderEdge = BorderEdge->NextEdge;
  }while(BorderEdge != MeshEdge);

  Assert( NrBorderEdges >= 3 );
  const u32 NrHalfEdges = 2 * NrBorderEdges;
  epa_halfedge* NewEdges = (epa_halfedge*) PushArray(Mesh->Arena, NrHalfEdges, epa_halfedge);
  // We have designed the for loop below such that even NewEdges points TO NewVertex and
  // Odd NewEdges points away from.
  epa_vertex* NewVertex = PushVertex(Mesh);
  NewVertex->P = *NewPoint;
  NewVertex->OutgoingEdge = &NewEdges[1];
  BorderEdge = MeshEdge;
  for (u32 Idx = 0; Idx < NrBorderEdges; ++Idx)
  {
    Assert(BorderEdge->LeftFace == 0);
    // Internal NewFace Edges E0,E1,E2;
    epa_halfedge* E0 = &NewEdges[2*Idx];   // To New Vertex
    epa_halfedge* E1 = &NewEdges[2*Idx+1]; // From New Vertex
    epa_halfedge* E2 = BorderEdge;         // Faces Mesh
    BorderEdge = BorderEdge->NextEdge;

    epa_face* NewFace =PushFace(Mesh);
    NewFace->Edge = E0;

    E0->TargetVertex = NewVertex;
    E0->NextEdge = E1;
    E0->LeftFace = NewFace;

    E1->TargetVertex = E2->OppositeEdge->TargetVertex;
    E1->NextEdge = E2;
    E1->LeftFace = NewFace;

    E2->NextEdge = E0;
    E2->LeftFace = NewFace;

    v3 v01 = E1->TargetVertex->P.S - E0->TargetVertex->P.S;
    v3 v02 = E2->TargetVertex->P.S - E0->TargetVertex->P.S;
    r32 Area = 0.5f*Norm(CrossProduct(v01,v02));
    if(Area <= 10e-5)
    {
      int cc = 10;
    }

  }

  for (u32 i = 0; i < NrHalfEdges; ++i)
  {
    epa_halfedge* Edge0 =  &NewEdges[i];
    if(Edge0->OppositeEdge) continue;

    epa_vertex* E0From = GetPreviousEdge(Edge0)->TargetVertex;
    epa_vertex* E0To = Edge0->TargetVertex;

    for (u32 j = 1; j < NrHalfEdges; ++j)
    {
      if(i==j) continue;
      epa_halfedge* Edge1 =  &NewEdges[j];
      if(Edge1->OppositeEdge) continue;

      epa_vertex* E1From = GetPreviousEdge(Edge1)->TargetVertex;
      epa_vertex* E1To = Edge1->TargetVertex;

      if( (E0From == E1To) && (E0To == E1From)  )
      {
        Edge0->OppositeEdge = Edge1;
        Edge1->OppositeEdge = Edge0;
        continue;
      }
    }
  }

  CalculateFaceNormals(Mesh);
}


internal inline b32
IsTargetVertexOutgoing( epa_halfedge* Edge )
{
  epa_vertex* A = Edge->TargetVertex;
  epa_vertex* B = A->OutgoingEdge->OppositeEdge->TargetVertex;
  return Equals(A,B);
}

internal inline u32
GetNrIncidentEdges( epa_vertex* Vertex )
{
  u32 Result = 0;
  epa_halfedge* NextIncidentEdge = Vertex->OutgoingEdge->OppositeEdge->NextEdge;
  while( !Equals(Vertex->OutgoingEdge, NextIncidentEdge) )
  {
    NextIncidentEdge = NextIncidentEdge->OppositeEdge->NextEdge;
    ++Result;
  }
  return Result;
}

internal inline void DissconectEdge( epa_halfedge* Edge )
{
  epa_halfedge* PreviousEdge = GetPreviousEdge(Edge);
  PreviousEdge->NextEdge = Edge->OppositeEdge->NextEdge;

  epa_halfedge* OppositePreviousEdge = GetPreviousEdge(Edge->OppositeEdge);
  OppositePreviousEdge->NextEdge = Edge->NextEdge;
}

internal inline void getFacePoints(epa_face* Face, v3* P0, v3* P1, v3* P2)
{
  *P0 = Face->Edge->TargetVertex->P.S;
  *P1 = Face->Edge->NextEdge->TargetVertex->P.S;
  *P2 = Face->Edge->NextEdge->NextEdge->TargetVertex->P.S;
}

internal epa_halfedge *
RemoveFacesSeenByPoint(epa_mesh* Mesh, const v3& Point)
{
  TIMED_BLOCK();
  epa_face** FaceListEntry = &Mesh->Faces;
  epa_face* FacesToDissconnect = 0;
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
    epa_face* Face  = *FaceListEntry;

    const v3 FaceToPoint = (Point - Face->Edge->TargetVertex->P.S);
    const r32 DotProduct = Face->Normal*FaceToPoint;
    if(DotProduct >= 0)
    {
      // Face is now removed from Mesh Face List
      *FaceListEntry = Face->Next;
      // And added to the dissconnect list
      Face->Next = FacesToDissconnect;
      FacesToDissconnect = Face;
    }else{
      FaceListEntry = &(*FaceListEntry)->Next;
    }
  }

  while(FacesToDissconnect)
  {
    // Dissconect face from edges
    epa_halfedge*  Edges[3] = { FacesToDissconnect->Edge,                      // Edge: A->B
                                FacesToDissconnect->Edge->NextEdge,            // Edge: B->C
                                FacesToDissconnect->Edge->NextEdge->NextEdge}; // Edge: C->A

    epa_vertex*  Vertices[3]= { Edges[2]->TargetVertex,   // Vertex A
                                Edges[0]->TargetVertex,   // Vertex B
                                Edges[1]->TargetVertex};  // Vertex C

    Assert(FacesToDissconnect->Edge == Edges[2]->NextEdge);

    for(u32 EdgeIdx = 0; EdgeIdx < 3; ++EdgeIdx)
    {
      epa_halfedge* Edge = Edges[EdgeIdx];
      epa_vertex* A = Edge->OppositeEdge->TargetVertex;
      epa_vertex* B = Edge->TargetVertex;
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
    FacesToDissconnect = FacesToDissconnect->Next;
  }

  epa_vertex** VertexListEntry = &Mesh->Vertices;
  epa_halfedge* BorderEdge = 0;
  while(*VertexListEntry)
  {
    epa_vertex* Vertex = *VertexListEntry;

    // Check if there is only 1 edge attached to the vertex
    if( Vertex->OutgoingEdge == Vertex->OutgoingEdge->OppositeEdge->NextEdge)
    {
      Assert(!Vertex->OutgoingEdge->LeftFace && !Vertex->OutgoingEdge->OppositeEdge->LeftFace);
      *VertexListEntry = Vertex->Next;
    }else{
      if(!BorderEdge && IsBorderEdge(Vertex->OutgoingEdge))
      {
        BorderEdge = Vertex->OutgoingEdge;
      }
      VertexListEntry = &(*VertexListEntry)->Next;
    }
  }
  return BorderEdge;
}

internal b32
IsPointOnMeshSurface(epa_mesh* Mesh, const v3& Point)
{
  TIMED_BLOCK();
  epa_face* Face = Mesh->Faces;
  while(Face)
  {
    v3 p0,p1,p2;
    getFacePoints(Face, &p0, &p1, &p2);

    // Tripple Dot Product
    r32 Determinant = (Point-p0) * CrossProduct( (p1-p0), (p2-p0) );
    if( Abs(Determinant) <= 10E-4 )
    {
      // Point is in the plane of the face
      v3 Coords = GetBaryocentricCoordinates( p0, p1, p2, Face->Normal, Point);
      if( (Coords.E[0] >= 0) && (Coords.E[0] <= 1) &&
          (Coords.E[1] >= 0) && (Coords.E[1] <= 1) &&
          (Coords.E[2] >= 0) && (Coords.E[2] <= 1))
      {
        // Point is inside the face triangle
        return true;
      }
    }
    Face = Face->Next;
  }
  return false;
}

internal epa_mesh *
CreateSimplexMesh(memory_arena* Arena, gjk_simplex* Simplex )
{
  gjk_support Tetrahedron[4] = {Simplex->SP[0],Simplex->SP[1],Simplex->SP[2],Simplex->SP[3]};
  FixWindingCCW(Tetrahedron);

  epa_mesh* Result = CreateInitialMesh(Arena, &Tetrahedron[0], &Tetrahedron[1], &Tetrahedron[2]);

  FillHole( Result, Result->Faces->Edge, &Tetrahedron[3]);

  return Result;
}

internal epa_face *
GetCLosestFaceToOrigin(epa_mesh* Mesh, r32* ResultDistance )
{
  // TODO: Add limit macro for max float value
  *ResultDistance = 10E30;
  epa_face* ResultFace = 0;
  epa_face* Face = Mesh->Faces;
  while(Face)
  {
    v3 p0,p1,p2;
    getFacePoints(Face, &p0, &p1, &p2);
    const v3 Projection = ProjectPointOntoPlane( V3(0,0,0), p0, Face->Normal );
    const r32 DistanceToFace = Norm(Projection);
    const v3 Coords = GetBaryocentricCoordinates( p0, p1, p2, Face->Normal, Projection);
    const b32 InsideTriangle = (Coords.E[0] >= 0) && (Coords.E[0] <= 1) &&
                               (Coords.E[1] >= 0) && (Coords.E[1] <= 1) &&
                               (Coords.E[2] >= 0) && (Coords.E[2] <= 1);

    if( InsideTriangle && (DistanceToFace < *ResultDistance))
    {
      *ResultDistance = DistanceToFace;
      ResultFace = Face;
    }
    Face = Face->Next;
  }
  return ResultFace;
}

void RecordFrame(epa_mesh* Mesh, component_gjk_epa_visualizer* Vis, epa_face* ClosestFaceToOrigin, v3 SupportPoint = V3(0,0,0), b32 RenderFilled = false)
{
  TIMED_BLOCK();
  if(!Vis) return;
  if(!Vis->TriggerRecord)
  {
    Vis->UpdateVBO = false;
    return;
  }

  epa_index* EPA = &Vis->EPA[Vis->EPACount++];
  EPA->FillMesh = RenderFilled;
  EPA->ClosestPointOnFace = ProjectPointOntoPlane(
    V3(0,0,0), ClosestFaceToOrigin->Edge->TargetVertex->P.S, ClosestFaceToOrigin->Normal );

  EPA->SupportPoint = SupportPoint;

  Vis->NormalIndexCount = Vis->IndexCount;
  Vis->NormalCount = Vis->VertexCount;

  EPA->ClosestFace = Vis->IndexCount;
  epa_halfedge* ce0 = ClosestFaceToOrigin->Edge;
  epa_halfedge* ce1 = ce0->NextEdge;
  epa_halfedge* ce2 = ce1->NextEdge;
  Vis->Indeces[ Vis->IndexCount++] = Vis->VertexCount;
  Vis->Vertices[Vis->VertexCount++] = ce0->TargetVertex->P.S;
  Vis->Indeces[ Vis->IndexCount++] = Vis->VertexCount;
  Vis->Vertices[Vis->VertexCount++] = ce1->TargetVertex->P.S;
  Vis->Indeces[ Vis->IndexCount++] = Vis->VertexCount;
  Vis->Vertices[Vis->VertexCount++] = ce2->TargetVertex->P.S;

  Vis->NormalIndeces[ Vis->NormalIndexCount++] = Vis->NormalCount;
  Vis->Normals[Vis->NormalCount++] = ClosestFaceToOrigin->Normal;
  Vis->NormalIndeces[ Vis->NormalIndexCount++] = Vis->NormalCount;
  Vis->Normals[Vis->NormalCount++] = ClosestFaceToOrigin->Normal;
  Vis->NormalIndeces[ Vis->NormalIndexCount++] = Vis->NormalCount;
  Vis->Normals[Vis->NormalCount++] = ClosestFaceToOrigin->Normal;

  EPA->MeshOffset = Vis->IndexCount;
  EPA->NormalOffset = Vis->NormalCount;
  epa_face* Face = Mesh->Faces;
  while(Face)
  {
    epa_halfedge* e0 = Face->Edge;
    epa_halfedge* e1 = e0->NextEdge;
    epa_halfedge* e2 = e1->NextEdge;

    Vis->NormalIndeces[ Vis->NormalIndexCount++] = Vis->NormalCount;
    Vis->Normals[Vis->NormalCount++] = Face->Normal;
    Vis->NormalIndeces[ Vis->NormalIndexCount++] = Vis->NormalCount;
    Vis->Normals[Vis->NormalCount++] = Face->Normal;
    Vis->NormalIndeces[ Vis->NormalIndexCount++] = Vis->NormalCount;
    Vis->Normals[Vis->NormalCount++] = Face->Normal;

    Vis->Indeces[ Vis->IndexCount++] = Vis->VertexCount;
    Vis->Vertices[Vis->VertexCount++] = e0->TargetVertex->P.S;
    Vis->Indeces[ Vis->IndexCount++] = Vis->VertexCount;
    Vis->Vertices[Vis->VertexCount++] = e1->TargetVertex->P.S;
    Vis->Indeces[ Vis->IndexCount++] = Vis->VertexCount;
    Vis->Vertices[Vis->VertexCount++] = e2->TargetVertex->P.S;

    Face = Face->Next;
  }
  EPA->MeshLength = Vis->IndexCount - EPA->MeshOffset;
  EPA->NormalLength = Vis->NormalIndexCount - EPA->NormalOffset;
}

contact_data EPACollisionResolution(memory_arena* TemporaryArena, const m4* AModelMat, const collider_mesh* AMesh,
                                    const m4* BModelMat, const collider_mesh* BMesh,
                                    gjk_simplex* Simplex, component_gjk_epa_visualizer* Vis)
{
  TIMED_BLOCK()
  temporary_memory TempMem = BeginTemporaryMemory(TemporaryArena);

  BlowUpSimplex(AModelMat, AMesh,
                BModelMat, BMesh,
                Simplex);


  epa_mesh* Mesh = CreateSimplexMesh(TemporaryArena, Simplex);

  // Get the first new point
  r32 DistanceClosestToFace = 0;
  epa_face* ClosestFace = GetCLosestFaceToOrigin( Mesh, &DistanceClosestToFace );
  Assert(ClosestFace);

  r32 PreviousDistanceClosestToFace = DistanceClosestToFace + 100;
  epa_face* PreviousClosestFace = ClosestFace;
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
    gjk_support SupportPoint = CsoSupportFunction( AModelMat, AMesh,
                                                   BModelMat, BMesh,
                                                   ClosestFace->Normal);
    RecordFrame(Mesh, Vis, ClosestFace, SupportPoint.S);
    if(IsPointOnMeshSurface(Mesh,SupportPoint.S))
    {
      // If a new point falls on an face in the polytype we return the
      // ClosestFace.
      break;
    }

    epa_halfedge* BorderEdge = RemoveFacesSeenByPoint(Mesh, SupportPoint.S);
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

  ClosestFace = GetCLosestFaceToOrigin( Mesh, &DistanceClosestToFace );

  gjk_support A = ClosestFace->Edge->TargetVertex->P;
  gjk_support B = ClosestFace->Edge->NextEdge->TargetVertex->P;
  gjk_support C = ClosestFace->Edge->NextEdge->NextEdge->TargetVertex->P;
  v3 P = ClosestFace->Normal * DistanceClosestToFace;

  v3 Coords = GetBaryocentricCoordinates(A.S,B.S,C.S,ClosestFace->Normal,P);

  // Point Outside the triangle
  if( !(Coords.E[0] >= 0) && (Coords.E[0] <= 1) &&
      !(Coords.E[1] >= 0) && (Coords.E[1] <= 1) &&
      !(Coords.E[2] >= 0) && (Coords.E[2] <= 1))
  {
    Assert(0)
  }

  v3 InterpolatedSupportA = Coords.E[0] * A.A + Coords.E[1] * B.A + Coords.E[2] * C.A;
  v3 InterpolatedSupportB = Coords.E[0] * A.B + Coords.E[1] * B.B + Coords.E[2] * C.B;

  v3 Tangent1 = V3(0.0f, ClosestFace->Normal.Z, -ClosestFace->Normal.Y);
  if ( ClosestFace->Normal.X >= 0.57735f)
  {
    Tangent1 = V3(ClosestFace->Normal.Y, -ClosestFace->Normal.X, 0.0f);
  }
  Normalize(Tangent1);

  contact_data ContactData = {};
  ContactData.A_ContactWorldSpace = V3(*AModelMat * V4(InterpolatedSupportA,1));
  ContactData.B_ContactWorldSpace = V3(*BModelMat * V4(InterpolatedSupportB,1));
  ContactData.A_ContactModelSpace = InterpolatedSupportA;
  ContactData.B_ContactModelSpace = InterpolatedSupportB;
  ContactData.ContactNormal       = ClosestFace->Normal;
  ContactData.TangentNormalOne    = Tangent1;
  ContactData.TangentNormalTwo    = CrossProduct(ClosestFace->Normal, Tangent1);
  ContactData.PenetrationDepth    = DistanceClosestToFace;
  EndTemporaryMemory(TempMem);

  RecordFrame(Mesh, Vis, ClosestFace);
  return ContactData;

}
