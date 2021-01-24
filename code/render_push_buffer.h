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
  TEXT,
  OVERLAY_TEXTURED_QUAD,
  NEW_LEVEL,
  COUNT
};

enum data_type
{
  DATA_TYPE_POINT,
  DATA_TYPE_LINE,
  DATA_TYPE_TRIANGLE
};

enum render_state
{
  RENDER_STATE_NONE         = 0x0,
  RENDER_STATE_CULL_BACK    = 0x1,
  RENDER_STATE_POINTS       = 0x2,
  RENDER_STATE_WIREFRAME    = 0x4,
  RENDER_STATE_FILL         = 0x8
};

struct entry_type_line
{
  v3 Start;
  v3 End;
  material_handle MaterialHandle;
  r32 LineThickness;
};

struct entry_type_overlay_line
{
  v4 Colour;
  v2 p0;
  v2 p1;
  r32 Thickness;
};

struct entry_type_overlay_quad
{
  v4 Colour;
  rect2f QuadRect;
};

struct entry_type_overlay_textured_quad
{
  bitmap_handle Handle;
  rect2f TextureRect;
  rect2f QuadRect;
};

struct entry_type_text
{
  rect2f QuadRect;
  rect2f UVRect;
  v4 Colour;
  bitmap_handle BitmapHandle;
};

struct entry_type_light
{
  v4 Color;
  m4 M;
};

struct entry_type_render_asset
{
  object_handle Object;
  bitmap_handle Bitmap;
  material_handle Material;

  m4 M;
  m4 NM;
  m4 TM;
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
  m4 ProjectionMatrix;
  m4 ViewMatrix;

  u32 ElementCount;
  memory_arena Arena;
  temporary_memory PushBufferMemory;

  push_buffer_header* First;
  push_buffer_header* Last;

  u32 BufferCounts[16];
};

void ResetRenderGroup(render_group* RenderGroup)
{
  EndTemporaryMemory(RenderGroup->PushBufferMemory);
  RenderGroup->PushBufferMemory = BeginTemporaryMemory(&RenderGroup->Arena);

  RenderGroup->ProjectionMatrix = M4Identity();
  RenderGroup->ViewMatrix = M4Identity();
  RenderGroup->ElementCount = 0;
  RenderGroup->First = 0;
  RenderGroup->Last = 0;

  ZeroArray(ArrayCount(RenderGroup->BufferCounts), RenderGroup->BufferCounts);
}
