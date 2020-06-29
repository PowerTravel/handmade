#pragma once

#include "memory.h"
#include "entity_components.h"
#include "math/vector_math.h"
#include "assets.h"

enum class render_buffer_entry_type
{
  LIGHT,
  RENDER_ASSET,
  OVERLAY_QUAD,
  LINE,
  TEXT
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

struct entry_type_line
{
  v3 Start;
  v3 End;
  u32 MaterialIndex;
  r32 LineThickness;
};

struct entry_type_overlay_quad
{
  v4 Colour;
  u32 TextureIndex;
  u32 ObjectIndex;
  m4 M;
  m4 TM;
};

struct entry_type_text
{
  u32 TextureIndex;
  u32 ObjectIndex;
  m4 M;
  m4 TM;

  v4 Colour;
  // UVScale takes an uv from [0,0],[1,1] (The whole texture) to [0,0][UVDim.X, UVDim.Y]
  v2 UVDim;
  // UVOffset takes an uv from [0,0][UVDimX, UVDimY] (Bottom left) to [UVPos.X,UVPos.X][UVPos.X + UVDimX, UVPos.Y + UVDimY]
  v2 UVPos;
  // QuadDim takes a quad from [-0.5,-0.5],[0.5,0.5] (The Unit Quad) to [-QuadDim.X/2,-QuadDim.Y/2][QuadDim.X/2,QuadDim.Y/2]
  v2 QuadDim;
  // UVOffset takes an uv from [-QuadDim.X/2,-QuadDim.Y/2][QuadDim.X/2,QuadDim.Y/2] to [QuadPos.X-QuadDim.X/2,QuadPos.Y-QuadDim.Y/2][QuadPos.X+QuadDim.X/2,QuadPos.YQuadDim.Y/2]
  v2 QuadPos;
};

struct entry_type_light
{
  v4 Color;
  m4 M;
};

struct entry_type_render_asset
{
  u32 AssetHandle;
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

struct push_buffer_header
{
  render_buffer_entry_type Type;
  u32 RenderState;
  u32 SortKey;
  push_buffer_header* Next;
};

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

  game_asset_manager* AssetManager;
  push_buffer_header* First;
  push_buffer_header* Last;
};

render_group* InitiateRenderGroup(game_state* GameState, r32 ScreenWidth, r32 ScreenHeight);
void ResetRenderGroup(render_group* RenderGroup);