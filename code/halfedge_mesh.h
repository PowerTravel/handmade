#pragma once

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

#include "memory.h"
struct gjk_support;
struct epa_halfedge;
struct epa_face;


/* Mesh algorithms break IF:
 * It splits in two.          (Could technically happen by improper use of RemoveFace.
 *                             However, since we in theory only should call RemoveFace in
 *                             the function "RemoveFacesSeenByPoint" in epa_collision_data.cpp
 *                             and Point should always be outside of the mesh or on it's surface
 *                             and Mesh should always be convex it should not be able to be split in two.)
 * It has more than one holes (Ditto)
 */


// Todo: * Speed up everything when you know it's working correctly (cache normals, vertex list, don't do unecessary looping)
//       * Right now, if a new CSO-Point is found on the mesh surface
//         we just return immideately. This may not be the right ting to do.
//         When we have better debugging tools. (Epa-visualizer using the log-system)
//         investigate if we need to revise this approach
struct epa_vertex
{
  gjk_support P;
  epa_halfedge* OutgoingEdge;
};

struct epa_halfedge
{
  epa_vertex*   TargetVertex;
  epa_face*     LeftFace;
  epa_halfedge* Next;
  epa_halfedge* Previous;
  epa_halfedge* Opposite;
};

struct epa_face
{
  epa_halfedge* Edge;
  epa_face* Next;
};

struct epa_mesh
{
  memory_arena* Arena;
  epa_face* Faces;
};


epa_mesh* InitializeMesh(memory_arena* Arena, gjk_support* P0, gjk_support* P1, gjk_support* P2);
r32 GetDistanceToFace(epa_face* Face);
v3 GetNormal(epa_face* Face);
void FillHole(epa_mesh* Mesh, gjk_support* NewPoint);
b32 GrowMesh(epa_mesh* Mesh, gjk_support* Point);