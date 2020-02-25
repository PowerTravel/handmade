#pragma once

#include "entity_components.h"
#include "primitive_meshes.h"

struct component_mesh;
struct component_surface;
struct collider_mesh;

enum class render_type
{
  INDEXED_BUFFER,
  PRIMITIVE,
  LIGHT
};

enum buffer_type
{
  BUFFER_TYPE_POINT,
  BUFFER_TYPE_LINE,
  BUFFER_TYPE_TRIANGLE
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

//  TODO: Separate the sending of data to the gpu and the building of
//        Push Buffers.
//        The push buffer should only contain the vao's vbo's used in the rendering
//        as well as any index buffer interval to be drawn


struct entry_type_indexed_buffer
{
  u32* VAO;
  u32 nvi;   // Nr Indeces
  u32 nv;    // Nr Verices
  u32 nvn;   // Nr Vertice Normals
  u32 nvt;   // Nr Trxture Vertices

  v3* v;     // Vertices
  v3* vn;    // Vertice Normals
  v2* vt;    // Texture Vertices
  u32* vi;   // Vertex Indeces
  u32* ti;   // Texture Indeces
  u32* ni;   // Normal Indeces

  component_surface* Surface;   // Has textures and render materials
  m4 M;                         // Model Matrix Transform
  m4 NM;                        // Normal Model Matrix Transform Transpose(Inv(ModelMatrix))

  u32 BufferType;
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
  render_type Type;
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