#ifndef COMPONENT_SPRITE_ANIMATION_H
#define	COMPONENT_SPRITE_ANIMATION_H

struct bitmap
{
	u32   Handle;
	u32   Width;
	u32   Height;
	void* Pixels;
};

enum sprite_type
{
	SPRITE_TYPE_NA,
	SPRITE_TYPE_TILESET_GROUND,
	SPRITE_TYPE_TILESET_WALL,
	SPRITE_TYPE_TILESET_CORNER,
	SPRITE_TYPE_IDLE,
	SPRITE_TYPE_WALK
};


enum sprite_orientation
{
	SPRITE_ORIENTATION_NA,
	// Cardinal Directions x 6
	SPRITE_ORIENTATION_P00, // +X,   ,     -  Right,    ,
	SPRITE_ORIENTATION_M00, // -X,   ,     -  Left ,    ,
	SPRITE_ORIENTATION_0P0, //   , +Y,     -       , Top,
	SPRITE_ORIENTATION_0M0, //   , -Y,     -       , Bot, 
	SPRITE_ORIENTATION_00P, //   ,   , +Z  -       ,    , Front
	SPRITE_ORIENTATION_00M, //   ,   , -Z  -       ,    , Back
	
	// Edges x 12
	SPRITE_ORIENTATION_PP0, // +X, +Y,     -  Right, Top,
	SPRITE_ORIENTATION_PM0, // +X, -Y,     -  Right, Bot,
	SPRITE_ORIENTATION_P0P, // +X,   , +Z  -  Right,    , Front
	SPRITE_ORIENTATION_P0M, // +X,   , -Z  -  Right,    , Back

	SPRITE_ORIENTATION_MP0, // -X, +Y,     -  Left,  Top,
	SPRITE_ORIENTATION_MM0, // -X, -Y,     -  Left,  Bot,
	SPRITE_ORIENTATION_M0P, // -X,   , +Z  -  Left,     , Front
	SPRITE_ORIENTATION_M0M, // -X,   , -Z  -  Left,     , Back

	SPRITE_ORIENTATION_0PP, //   , +Y, +Z  -      ,   Top, Front
	SPRITE_ORIENTATION_0PM, //   , +Y, -Z  -      ,   Top, Back
	SPRITE_ORIENTATION_0MP, //   , -Y, +Z  -      ,   Bot, Front,
	SPRITE_ORIENTATION_0MM, //   , -Y, -Z  -      ,   Bot, Back

	// Corners x 8
	SPRITE_ORIENTATION_PPP, // +X, +Y, +Z  -  Right, Top, Front
	SPRITE_ORIENTATION_PPM, // +X, +Y, -Z  -  Right, Top, Back
	SPRITE_ORIENTATION_PMP, // +X, -Y, +Z  -  Right, Bot, Front
	SPRITE_ORIENTATION_PMM, // +X, -Y, -Z  -  Right, Bot, Back

	SPRITE_ORIENTATION_MPP, // -X, +Y, +Z  -  Left,  Top, Front
	SPRITE_ORIENTATION_MPM, // -X, +Y, -Z  -  Left,  Top, Back
	SPRITE_ORIENTATION_MMP, // -X, -Y, +Z  -  Left,  Bot, Front
	SPRITE_ORIENTATION_MMM, // -X, -Y, -Z  -  Left,  Bot, Back
};

struct sprite_series
{
	u32 NrFrames;
	sprite_type Type;
	sprite_orientation Orientation;

	// Texture Coordinates for the sprite series
	rect2f* Frames;

	rect2f* ActiveFrame;
};

struct component_sprite_animation
{
	bitmap* Bitmap;
	u32 NrEntries;
	sprite_series* Animations;
	sprite_series* ActiveSeries;
};


sprite_series* GetSeriesWithTypeAndOrientation(component_sprite_animation* AnimationComponent, sprite_type Type, sprite_orientation Orientation )
{
	u32 Index = 0;
	while( Index < AnimationComponent->NrEntries )
	{
		sprite_series* Animation = AnimationComponent->Animations + Index++;
		if( ( Animation->Type == Type) && (Animation->Orientation == Orientation) )
		{
			return Animation;
		}
	}

	return 0;
}

#endif // COMPONENT_SPRITE_ANIMATION_H