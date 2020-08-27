#pragma once

#include <stdint.h>
#include <float.h>

#define U16Max 65535
#define S32Min ((s32)0x80000000)
#define S32Max ((s32)0x7fffffff)
#define U32Max ((u32)-1)
#define U64Max ((u64)-1)
#define R32Max FLT_MAX
#define R32Min -FLT_MAX

#define Kilobytes(Value)  ((Value)*1024LL)
#define Megabytes(Value)  (Kilobytes(Value)*1024LL)
#define Gigabytes(Value)  (Megabytes(Value)*1024LL)
#define Terrabytes(Value) (Gigabytes(Value)*1024LL)
#define ArrayCount(Array) ( sizeof(Array)/sizeof((Array)[0]) )

#if HANDMADE_SLOW
#define Assert(Expression) if(!(Expression)){ *(int *)0 = 0;}
#else
#define Assert(Expression)
#endif // HANDMADE_SLOW

#define INVALID_CODE_PATH Assert(0);

typedef char c8;
typedef int8_t  s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef s32 b32;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef size_t memory_index;
typedef size_t midx;

typedef uintptr_t umm;
typedef intptr_t smm;

typedef float  r32;
typedef double r64;

// NOTE(casey): Added (Value-Value) here to force integral promotion to the size of Value
#define AlignPow2(Value, Alignment) ((Value + ((Alignment) - 1)) & ~((Value - Value) + (Alignment) - 1))
#define Align4(Value) ((Value + 3) & ~3)
#define Align8(Value) ((Value + 7) & ~7)
#define Align16(Value) ((Value + 15) & ~15)

#define internal		 static
#define local_persist    static
#define global_variable  static

#define Pi32  3.14159265359f
#define Tau32 6.28318530718f

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