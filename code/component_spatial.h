#pragma once

#include "math/affine_transformations.h"

struct component_spatial
{
  component_spatial(v3 PosInit = V3(0,0,0), v3 ScaleInit = V3(1,1,1), v4 RotInit = V4(0,0,0,1) ) :
        Position(PosInit), Scale(ScaleInit), Rotation(RotInit){};
  v3 Scale;
  v3 Position;
  //  we define the Quaternion as (xi,yj,zk, Scalar)
  //  Some resources define it as (Scalar,xi,yj,zk)
  v4 Rotation;
};

// TODO: Optimize by doing explicit element multiplication
m4 GetModelMatrix( const component_spatial* c )
{
  //TIMED_FUNCTION();
  const m4 Result = M4Identity();
  const m4 Scale = GetScaleMatrix(V4(c->Scale,1));
  const m4 Rotation = GetRotationMatrix(c->Rotation);
  const m4 Translation = GetTranslationMatrix(c->Position);
  return Translation * Rotation * Scale;
}

inline v4
QuaternionMultiplication(const v4 r, const v4 q)
{
  v4 Result = {};
  Result.W = r.W*q.W - r.X*q.X - r.Y*q.Y - r.Z*q.Z;  // scalar
  Result.X = r.W*q.X + r.X*q.W - r.Y*q.Z + r.Z*q.Y;  // x-part (i)
  Result.Y = r.W*q.Y + r.X*q.Z + r.Y*q.W - r.Z*q.X;  // y-part (j)
  Result.Z = r.W*q.Z - r.X*q.Y + r.Y*q.X + r.Z*q.W;  // z-part (k)
  return Result;
}


