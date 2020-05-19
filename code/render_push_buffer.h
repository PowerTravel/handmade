#pragma once

#include "memory.h"
#include "entity_components.h"
#include "primitive_meshes.h"
#include "math/vector_math.h"
#include "standalone_utility.h"
#include "assets.h"


struct component_mesh;
struct component_surface;
struct collider_mesh;

enum class render_buffer_entry_type
{
  INDEXED_BUFFER,
  PRIMITIVE,
  LIGHT,
  ASSET_TEST,
  OVERLAY_QUAD
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

struct entry_type_overlay_quad
{
  v4 Colour;
  u32 TextureHandle;
  u32 MeshHandle;
  m4 M;
  m4 TM;
};

struct entry_type_light
{
  v4 Color;
  m4 M;
};

struct entry_type_asset_test
{
  u32 MeshHandle;
  u32 MaterialHandle;
  m4 M;
  m4 NM;
  m4 TM;
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
  // Get rid of surface component. Use textures OR colours.
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

struct render_group
{
  r32 ScreenWidth;
  r32 ScreenHeight;
  b32 Initialized;

  m4 ProjectionMatrix;
  m4 ViewMatrix;

  u32 ElementCount;
  memory_arena Arena;
  temporary_memory PushBufferMemory;

  game_assets* Assets;
  game_asset_manager* AssetManager;
  push_buffer_header* First;
  push_buffer_header* Last;
};

render_group* InitiateRenderGroup(game_state* GameState, r32 ScreenWidth, r32 ScreenHeight);
void ResetRenderGroup(render_group* RenderGroup);