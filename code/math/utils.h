#pragma once

#include "vector_math.h"

inline r32 Parameterize(r32 A, r32 B, r32 Val)
{
  r32 Result = (Val-A)/(B-A);
  return Result;
}


inline r32 Interpolate(r32 A, r32 B, r32 Val)
{
  r32 Result = A + Val * (B-A);
  return Result;
}

inline v2 Interpolate(v2 A, v2 B, r32 Val)
{
  v2 Result = A + Val * (B-A);
  return Result;
}

inline v3 Interpolate(v3 A, v3 B, r32 Val)
{
  v3 Result = A + Val * (B-A);
  return Result;
}

inline v4 Interpolate(v4 A, v4 B, r32 Val)
{
  v4 Result = A + Val * (B-A);
  return Result;
}