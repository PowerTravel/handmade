
#ifndef HANDMADE_INTRINSIC_H
#define HANDMADE_INTRINSIC_H

// TODO: Replace math.h with cpu specific instructions
#include <math.h>

inline u32
SafeTruncateToU32( u64 Value )
{
    // TODO(casey): Defines for maximum values
    Assert(Value <= 0xFFFFFFFF);
    u32 Result = (u32)Value;
    return(Result);
}

inline u32 
RoundReal64ToUInt32( r32 Real64 )
{
	u64 tmp = (u64) round( Real64 );
	u32 Result = SafeTruncateToU32( tmp );
	return Result; 
}

inline u32 
RoundReal32ToUInt32( r32 Real32 )
{
	u32 Result = (u32) roundf( Real32 );
	return Result; 
}

inline s32 
RoundReal32ToInt32( r32 Real32 )
{
	// NOTE(Jakob): ROUNDF IS SLOW!! 
	s32 Result = (s32) roundf( Real32 );

	return Result; 
}

inline s32 
TruncateReal32ToInt32( r32 Real32 )
{
	s32 Result = (s32) Real32;
	return Result;
}

inline s32 
FloorReal32ToInt32( r32 Real32 )
{
	s32 Result = (s32) floorf( Real32 );
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


#endif // HANDMADE_INTRINSIC_H