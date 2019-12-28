#pragma once

#include "affine_transformations.h"
#include "aabb.h"

struct component_spatial
{
  // Specifies location
  component_spatial() : ModelMatrix(M4Identity()) {}
  m4 ModelMatrix;
};

void Put( const v3 dr, component_spatial* c )
{
  c->ModelMatrix = M4Identity();
  Translate(V4(dr,1), c->ModelMatrix);
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

void Rotate( const r32 Angle, const v3 Axis, component_spatial* c )
{
  Assert(c);
  Rotate( Angle, V4(Axis,0), c->ModelMatrix );
}
