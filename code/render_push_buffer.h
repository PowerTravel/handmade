#pragma once

#include "entity_components.h"
#include "primitive_meshes.h"
#include "vector_math.h"

struct component_mesh;
struct component_surface;
struct collider_mesh;

enum class render_buffer_entry_type
{
  INDEXED_BUFFER,
  PRIMITIVE,
  LIGHT
};

enum data_type
{
  DATA_TYPE_POINT,
  DATA_TYPE_LINE,
  DATA_TYPE_TRIANGLE
};

enum render_state
{
  RENDER_STATE_CULL_BACK    = 0x1,
  RENDER_STATE_POINTS       = 0x2,
  RENDER_STATE_WIREFRAME    = 0x4,
  RENDER_STATE_FILL         = 0x8
};

struct entry_type_light
{
  v4 Color;
  m4 M;
};

struct render_buffer
{
  b32 Fill;
  u32* VAO;
  u32* VBO;
  u32 nvi;   // Nr Indeces
  u32* vi;   // Vertex Indeces
  u32* ti;   // Texture Indeces
  u32* ni;   // Normal Indeces

  u32 nv;    // Nr Verices
  v3* v;     // Vertices

  u32 nvn;   // Nr Vertice Normals
  v3* vn;    // Vertice Normals

  u32 nvt;   // Nr Trxture Vertices
  v2* vt;    // Texture Vertices
};

struct entry_type_indexed_buffer
{
  u32 DataType;
  render_buffer* Buffer;
  component_surface* Surface;   // Has textures and render materials
  m4 M;                         // Model Matrix Transform
  m4 NM;                        // Normal Model Matrix Transform Transpose(Inv(ModelMatrix))
  u32 ElementStart;
  u32 ElementLength;
};

struct entry_type_primitive
{
  primitive_type PrimitiveType;
  m4 M;   // Transforms for (0,0,0) Vertex
  m4 TM;  // Transforms for (0,0), (1,1) Texture coordinate
  component_surface* Surface;   // Has textures and render materials
};

struct push_buffer_header
{
  render_buffer_entry_type Type;
  u32 RenderState;
  u32 SortKey;
  push_buffer_header* Next;
};

struct game_assets;

struct render_push_buffer
{
  m4 ProjectionMatrix;
  m4 ViewMatrix;
  game_assets* Assets;
  push_buffer_header* First;
};