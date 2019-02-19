#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

#ifndef UINT32_MAX
#define UINT32_MAX 0xffffffff
#endif

#define OffsetOf(type, Member) (uintptr_t) &( ( (type* )0 )->Member )

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

typedef uintptr_t umm;
typedef intptr_t smm;

typedef float  r32;
typedef double r64;

#define Maximum(A, B) ((A > B) ? (A) : (B))
#define Minimum(A, B) ((A < B) ? (A) : (B))

#define U32Max ( (u32)-1 )

// NOTE(casey): Added (Value-Value) here to force integral promotion to the size of Value
#define AlignPow2(Value, Alignment) ((Value + ((Alignment) - 1)) & ~((Value - Value) + (Alignment) - 1))
#define Align4(Value) ((Value + 3) & ~3)
#define Align8(Value) ((Value + 7) & ~7)
#define Align16(Value) ((Value + 15) & ~15)

#define internal		 static
#define local_persist    static
#define global_variable  static

#define Pi32 3.14159265359f

#include "vector_math.h"

union rect
{
	struct{
		r32 X,Y,W,H;
	};
	struct{
		r32 X0,Y0,X1,Y1;
	};
};

rect Rect( r32 a, r32 b, r32 c, r32 d )
{
	rect Result = {a,b,c,d};
	return Result;
}

struct cube
{
	struct{
		r32 X,Y,Z,W,H,D;
	};
	v3 P0, P1;
};

inline cube Cube( v3 P0, v3 P1 )
{
	cube Result = {};
	Result.P0 = P0;
	Result.P1 = P1;
	return Result;
};

#endif
