#pragma once

#include "entity_components.h"
//#include "component_camera.h"
//#include "component_surface.h"
//#include "component_light.h"
//#include "component_collider.h"

struct component_mesh;
struct component_surface;
struct collider_mesh;

enum class render_type
{
  POINT_BUFFER,    //
  LINE_BUFFER,     //
  TRIANGLE_BUFFER, //
  QUAD,            // Predefined Primitives
  POINT,           // Predefined Primitives
  LINE,            // Predefined Primitives
  LIGHT            // Special
};

struct entry_type_light
{
  v4 Color;
  m4 M;
};

struct entry_type_triangle_buffer
{
  component_mesh* Mesh;         // Has Vertice Texture and normal data
  collider_mesh*  ColliderMesh; // Has only vertice data
  component_surface* Surface;   // Has textures and render materials
  m4 M;                         // Model Matrix Transform
  m4 NM;                        // Normal Model Matrix Transform Transpose(Inv(ModelMatrix))
};

struct entry_type_point_buffer
{
  component_mesh* Mesh;         // Has Vertice Texture and normal data
  collider_mesh*  ColliderMesh; // Has only vertice data
  m4 M;                         // Model Matrix Transform
};

struct entry_type_line_buffer
{
  component_mesh* Mesh;         // Has Vertice Texture and normal data
  collider_mesh*  ColliderMesh; // Has only vertice data
  m4 M;                         // Model Matrix Transform
};

struct entry_type_point
{
  m4 M;   // Transforms for (0,0,0) Vertex
};

struct entry_type_quad
{
  component_surface* Surface;   // Has textures and render materials
  m4 M;   // Transforms for (-0.5,-0.5,0), (0.5,0.5,0) Vertecies
  m4 TM;  // Transforms for (0,0), (1,1) Texture coordinate
};

enum render_state
{
  RENDER_STATE_CULL_BACK    = 0x1,
  RENDER_STATE_POINTS       = 0x2,
  RENDER_STATE_WIREFRAME    = 0x4,
  RENDER_STATE_FILL         = 0x8
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