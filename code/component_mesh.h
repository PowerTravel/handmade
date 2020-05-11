#pragma once

struct mesh_data;
struct mesh_indeces;
struct component_mesh
{
  u32 VAO;
  u32 VBO;
  mesh_data* Data;
  mesh_indeces* Indeces;
};