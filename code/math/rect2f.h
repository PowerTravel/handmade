
#include "math/vector_math.h"

struct u32_pair
{
  u32 a;
  u32 b;
};

struct rect2f
{
	r32 X,Y,W,H;
};

rect2f Rect2f( r32 X, r32 Y, r32 W, r32 H )
{
	rect2f Result = {};
  Result.X = X;
  Result.Y = Y;
  Result.W = W;
  Result.H = H;
	return Result;
};

rect2f Rect2f( v2 Pos, v2 Size )
{
  rect2f Result = {};
  Result.X = Pos.X;
  Result.Y = Pos.Y;
  Result.W = Size.X;
  Result.H = Size.Y;
  return Result;
};

inline rect2f
Shrink(rect2f Rect, r32 dx)
{
  rect2f Result = Rect;
  Result.X += dx;
  Result.Y += dx;
  Result.H -= 2*dx;
  Result.W -= 2*dx;
  return Result;
}

inline b32
Intersects(const rect2f & Rect, v2 P)
{
  b32 Result = (P.X>=Rect.X && (P.X<=Rect.X+Rect.W)) &&
               (P.Y>=Rect.Y && (P.Y<=Rect.Y+Rect.H));
  return Result;
}

inline b32
Intersects(const rect2f& A, const rect2f& B)
{
  b32 Result = ( (A.X+A.W) <  B.X)       ||
               (  A.X      > (B.X+B.W))  ||
               ( (A.Y+A.H) <  B.Y)       ||
               (  A.Y      > (B.Y+B.H));
  return Result;
}

