#pragma once
#include "gjk.h"

// Todo: Should we store a void* in the structures to be used for metadata?
//       Ex, Face Normals, Support points, Makes this a more general mesh datastructure

struct gjk_halfedge;
struct gjk_face;

struct gjk_vertex
{
  u32 Idx;
  gjk_support P;
  gjk_halfedge* OutgoingEdge;
  gjk_vertex* Previous;
};

struct gjk_halfedge
{
  gjk_vertex*   TargetVertex;
  gjk_face*     LeftFace;
  gjk_halfedge* NextEdge;
  gjk_halfedge* OppositeEdge;
};

struct gjk_face
{
  u32 Idx;
  v3 Normal;
  gjk_halfedge* Edge;
  gjk_face* Previous;
};

struct gjk_mesh
{
  u32 VerticeIdxCounter;
  u32 FaceIdxCounter;
  memory_arena* Arena;
  gjk_vertex* Vertices;
  gjk_face* Faces;
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


// A halfedge must ALWAYS point to it's opposite halfedge. (They always comes in pairs)
// A halfedge pair must ALWAYS point at least one Face.    (No edges are ever alone)
// A halfedge always creates a closed loop 3 long if inside
//  of a face or 3 or longer if outside or a hole.          (We can go ->next for ever)

gjk_mesh* CreateSimplexMesh(memory_arena* Arena, gjk_simplex* Simplex );
gjk_face* GetCLosestFaceToOrigin(gjk_mesh* Mesh, r32* ResultDistance );
b32 IsPointInMesh(gjk_mesh* Mesh, const v3* Point, r32 Tolerance = 10E-7);
internal void FillHole(gjk_mesh* Mesh, gjk_halfedge* MeshEdge, gjk_support* NewPoint);
internal gjk_halfedge* RemoveFacesSeenByPoint(gjk_mesh* Mesh, const v3& Point);

void ExpandPolytype(gjk_mesh* Mesh, gjk_support* Support)
{
  gjk_halfedge* BorderEdge = RemoveFacesSeenByPoint(Mesh, Support->S);
  FillHole( Mesh, BorderEdge, Support);
}


void DebugPrintPolytype(memory_arena* Arena, gjk_mesh* Mesh, b32 Append, platform_api* API );

void DebugPrintEdges(memory_arena* Arena, gjk_mesh* Mesh, b32 Append, platform_api* API );