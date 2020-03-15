#pragma once

#include "math/vector_math.h"

struct mesh_data
{
  u32 nv;    // Nr Verices
  u32 nvn;   // Nr Vertice Normals
  u32 nvt;   // Nr Trxture Vertices

  v3* v;     // Vertices
  v3* vn;    // Vertice Normals
  v2* vt;    // Texture Vertices
};

struct mesh_indeces
{
  u32 Count;  // 3 times Nr Triangles
  u32* vi;    // Vertex Indeces
  u32* ti;    // Texture Indeces
  u32* ni;    // Normal Indeces
};

struct component_mesh
{
  u32 VAO;
  u32 VBO;
  mesh_data* Data;
  mesh_indeces Indeces;
};