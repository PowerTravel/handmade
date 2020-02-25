#pragma once

// TODO: Add sphere and cylinder
enum class primitive_type
{
  POINT,
  QUAD,
  VOXEL
};

struct opengl_vertex;

void getPrimitive( primitive_type Type, opengl_vertex* GLData );