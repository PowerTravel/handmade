
#ifndef HANDMADE_INTRINSIC_H
#define HANDMADE_INTRINSIC_H

// TODO: Replace math.h with cpu specific instructions
#include <math.h>

inline uint32
SafeTruncateToU32(uint64 Value)
{
    // TODO(casey): Defines for maximum values
    Assert(Value <= 0xFFFFFFFF);
    uint32 Result = (uint32)Value;
    return(Result);
}

inline uint32 
RoundReal64ToUInt32(real32 Real64)
{
	uint64 tmp = (uint64) round( Real64 );
	uint32 Result = SafeTruncateToU32( tmp );
	return Result; 
}

inline uint32 
RoundReal32ToUInt32(real32 Real32)
{
	uint32 Result = (uint32) roundf(Real32);
	return Result; 
}

inline int32 
RoundReal32ToInt32(real32 Real32)
{
	// NOTE(Jakob): ROUNDF IS SLOW!! 
	int32 Result = (int32) roundf(Real32);

	return Result; 
}

inline int32 
TruncateReal32ToInt32(real32 Real32)
{
	int32 Result = (int32) Real32;
	return Result;
}

inline int32 
FloorReal32ToInt32(real32 Real32)
{
	int32 Result = (int32) floorf(Real32);
	return Result;
}

inline real32
Sin(real32 Angle)
{
	real32 Result = sinf(Angle);
	return Result;
}

inline real32
Cos(real32 Angle)
{
	real32 Result = cosf(Angle);
	return Result;
}

inline real32
ATan2(real32 Y, real32 X)
{
	real32 Result = atan2f(Y,X);
	return Result;
}

inline real32
Tan(real32 Angle)
{
	real32 Result = tanf(Angle);
	return Result;
}

inline real32
Sqrt( real32 A )
{
	real32 Result = sqrtf(A);
	return Result;
}

inline real32
Abs( real32 A )
{
	real32 Result = (real32) fabs( A );
	return Result;
}

inline	real32
Pow(real32 Base, real32 Exponent )
{
	real32 Result = (real32) powf( Base, Exponent );
	return Result;	
}

struct bit_scan_result{
	bool32 Found;
	uint32 Index;
};

inline bit_scan_result
FindLeastSignificantSetBit(uint32 Value)
{
	bit_scan_result Result = {};

#if COMPILER_MSVC
	Result.Found = _BitScanForward((unsigned long*)&Result.Index, Value);
#else
	for(uint32 Test = 0; Test < 32; Test++)
	{
		uint32 mask = (1 << Test);
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