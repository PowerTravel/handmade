#ifndef COMPONENT_SPATIAL_H
#define COMPONENT_SPATIAL_H

#include "affine_transformations.h"

struct component_spatial
{
	v3  Position;
	v3  Velocity;
	v3  Direction;
	r32 RotationAngle;
	v3  RotationAxis;
	b32 IsDynamic;
	r32 Width;
	r32 Height;
	r32 Depth;
};

inline m4 
GetAsMatrix( component_spatial* SpatialComponent )
{
	if( ! SpatialComponent )
	{
		INVALID_CODE_PATH
		return M4Identity();
	}

	m4 RotationMatrix = GetRotationMatrix(SpatialComponent->RotationAngle, V4( SpatialComponent->RotationAxis, 0 ) );

	m4 TranslationMatrix = GetTranslationMatrix( V4( SpatialComponent->Position , 1 ) );

	m4 Result = TranslationMatrix * RotationMatrix;

	return Result;
}

inline cube 
GetBoundingBox( component_spatial* SpatialComponent )
{
	if( ! SpatialComponent )
	{
		INVALID_CODE_PATH
		return {};
	}
	v3 Pos = SpatialComponent->Position;
	v3 Dim = V3(SpatialComponent->Width, SpatialComponent->Height, SpatialComponent->Depth);
	cube Result = Cube( Pos-Dim/2, Pos+Dim/2 );
	return Result;

}

#endif // COMPONENT_SPATIAL_H