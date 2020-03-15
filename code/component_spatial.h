#pragma once

#include "affine_transformations.h"

struct component_spatial
{
  // Specifies location
  component_spatial() : ModelMatrix(M4Identity()) {}
  m4 ModelMatrix;
};

void Put( const v3 position, const r32 angle, const v3 axis, component_spatial* c )
{
  c->ModelMatrix = M4Identity();
  Translate(V4(position,1), c->ModelMatrix);
  Rotate(angle, V4(axis,0.f), c->ModelMatrix);
}

v3 GetPosition(component_spatial* c)
{
  return V3(GetTranslationFromMatrix( c->ModelMatrix ));
}

void Translate( const v3 dr, component_spatial* c )
{
  Assert(c);
  Translate(V4(dr,1), c->ModelMatrix);
}

void Scale( const v3 ds, component_spatial* c )
{
  Assert(c);
  Scale( V4(ds,0), c->ModelMatrix );
}

void Rotate( const r32 da, const v3 Axis, component_spatial* c )
{
  Assert(c);
  Rotate( da, V4(Axis,0), c->ModelMatrix );
}
