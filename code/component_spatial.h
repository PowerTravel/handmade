#ifndef COMPONENT_SPATIAL_H
#define COMPONENT_SPATIAL_H

#include "affine_transformations.h"
#include "aabb.h"

struct component_spatial
{
	v3  Position;
	v3  Velocity;
	v3 	ExternalForce;
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
	}

	m4 RotationMatrix = GetRotationMatrix(SpatialComponent->RotationAngle, V4( SpatialComponent->RotationAxis, 0 ) );

	m4 TranslationMatrix = GetTranslationMatrix( V4( SpatialComponent->Position , 1 ) );

	m4 Result = TranslationMatrix * RotationMatrix;

	return Result;
}

inline aabb3f 
GetBoundingBox( component_spatial* SpatialComponent )
{
	if( ! SpatialComponent )
	{
		INVALID_CODE_PATH
	}
	v3 Pos = SpatialComponent->Position;
	v3 Dim = V3(SpatialComponent->Width, SpatialComponent->Height, SpatialComponent->Depth);
	aabb3f Result = AABB3f( Pos, Pos+Dim );
	return Result;

}

#endif // COMPONENT_SPATIAL_H