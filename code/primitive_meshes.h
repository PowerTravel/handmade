#pragma once

// TODO: Add sphere and cylinder
enum class primitive_type
{
  SQUARE,
  BOX
};

struct opengl_vertex;

void getPrimitive( primitive_type Type, opengl_vertex* GLData );