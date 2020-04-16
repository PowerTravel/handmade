#pragma once

// TODO: Replace math.h with cpu specific instructions
#include <math.h>
#include "types.h"

inline u32 GetThreadID()
{
  // Read the pointer to thread local storage.
  u8* ThreadLocalStorage = (u8*) __readgsqword(0x30);
  u32 ThreadID = *(u32*) (ThreadLocalStorage + 0x48);
  return ThreadID;
}


inline u32
SafeTruncateToU32( u64 Value )
{
    Assert(Value <= UINT32_MAX);
    u32 Result = (u32) Value;
    return(Result);
}

inline r64
Round( r64 Real64 )
{
  r64 Result = round( Real64 );
  return Result;
}

inline r32
Round( r32 Real32 )
{
  // NOTE(Jakob): ROUNDF IS SLOW!!
  r32 Result = roundf( Real32 );
  return Result;
}

inline r32
Roof( r32 Real32 )
{
  r32 Result = ceilf( Real32 );
  return Result;
}

inline r32
Floor( r32 Real32 )
{
  r32 Result = floorf( Real32 );
  return Result;
}

inline r32
Sin( r32 Angle )
{
  r32 Result = sinf( Angle );
  return Result;
}

inline r32
Cos( r32 Angle )
{
  r32 Result = cosf( Angle );
  return Result;
}

inline r32
ATan2( r32 Y, r32 X )
{
  r32 Result = atan2f( Y, X );
  return Result;
}

inline r32
Tan( r32 Angle )
{
  r32 Result = tanf( Angle );
  return Result;
}

inline r32
Sqrt( r32 A )
{
  r32 Result = sqrtf( A );
  return Result;
}

inline r32
Abs( r32 A )
{
  r32 Result = (r32) fabs( A );
  return Result;
}

inline r32
Pow( r32 Base, r32 Exponent )
{
  r32 Result = (r32) powf( Base, Exponent );
  return Result;
}

struct bit_scan_result
{
  b32 Found;
  u32 Index;
};

inline bit_scan_result
FindLeastSignificantSetBit( u32 Value )
{
  bit_scan_result Result = {};

#if COMPILER_MSVC
  Result.Found = _BitScanForward( (unsigned long*) &Result.Index, Value);
#else
  for(u32 Test = 0; Test < 32; Test++)
  {
    u32 mask = (1 << Test);
    if( (Value & mask ) != 0)
    {
      Result.Index = Test;
      Result.Found = true;
      break;
    }
  }

#endif
  return Result;
}