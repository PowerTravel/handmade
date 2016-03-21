
#ifndef HANDMADE_INTRINSIC_H
#define HANDMADE_INTRINSIC_H


// TODO: Replace math.h with cpu specific instructions
#include <math.h>

inline uint32 
RoundReal32ToUInt32(real32 Real32)
{
	uint32 Result = (uint32) (Real32+0.5);
	return Result; 
}

inline int32 
RoundReal32ToInt32(real32 Real32)
{

	int32 Result = (int32) (Real32+0.5);
	if(Real32 < 0)
	{
		Result = (int32) (Real32-0.5);
	}
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

#endif // HANDMADE_INTRINSIC_H