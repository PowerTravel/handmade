#ifndef SPRITE_MAPPING_H
#define SPRITE_MAPPING_H

#include "component_sprite_animation.h"


// All sizes are in pixels
void ExtractRegularFrameRow( memory_arena* AssetArena, sprite_series* SpriteSeries, u32 NrFrames,
							 u32 OffsetX /* From Left */, u32 OffsetY /* From Top */,
							 u32 ClipOutWidth, 			  u32 ClipOutHeight, 
							 u32 SpriteSheetWidth,  	  u32 SpriteSheetHeight,
							 b32 InvertX, 			      b32 InvertY,
							 sprite_type Type, sprite_orientation Orientation )
{
	Assert(SpriteSeries);
	SpriteSeries->NrFrames      = NrFrames;
	SpriteSeries->Type          = Type;
	SpriteSeries->Orientation   = Orientation;

	r32 StrideX =  (r32) ClipOutWidth  / (r32) SpriteSheetWidth;
	r32 StrideY =  (r32) ClipOutHeight / (r32) SpriteSheetHeight;

	r32 StartXFromLeft = (r32) OffsetX / (r32) SpriteSheetWidth; 
	r32 StartYFromTop  = (r32) OffsetY / (r32) SpriteSheetHeight; 
	r32 StartYFromBot  = 1 - StartYFromTop; 

	SpriteSeries->Frames = PushArray(AssetArena, SpriteSeries->NrFrames, rect2f);

	for( u32 FrameIdx = 0; FrameIdx < NrFrames; ++FrameIdx )
	{
		Assert( (StartXFromLeft + (FrameIdx+1) * StrideX) <= 1);
		Assert( (StartYFromBot  + StrideY ) <= 1);
		rect2f* Frame = &SpriteSeries->Frames[FrameIdx];

		if(InvertX)
		{
			Frame->X = StartXFromLeft + FrameIdx * StrideX + StrideX;  // From
			Frame->W = -StrideX;
		}else{
			Frame->X = StartXFromLeft + FrameIdx * StrideX;  // From
			Frame->W = StrideX;
		}

		if(InvertY)
		{
			Frame->Y = StartYFromBot + StrideY;  // From
			Frame->H = -StrideY;
		}else{
			Frame->Y = StartYFromBot;  // From
			Frame->H = StrideY;
		}
	}
	
	SpriteSeries->ActiveFrame = SpriteSeries->Frames;

}

void ExtractPrinny( memory_arena* AssetArena, bitmap* Bitmap, component_sprite_animation* AnimationComponent )
{
	Assert(AnimationComponent);
	
	AnimationComponent->NrEntries = 8;
	AnimationComponent->Bitmap = Bitmap;

	Assert(AnimationComponent->Bitmap);

	AnimationComponent->Animations = PushArray(AssetArena, AnimationComponent->NrEntries, sprite_series);

	u32 NrFrames            = 7;
	u32 FrameWidthInPixels  = 32;
	u32 FrameHeightInPixels = 64;
	u32 BitmapWidth  = Bitmap->Width;
	u32 BitmapHeight = Bitmap->Height;

	u32 Row = 0; // From Top 

	// First Row Normal (idle front left)
	ExtractRegularFrameRow( AssetArena,         &AnimationComponent->Animations[0], 7,
							0,                  FrameHeightInPixels, 
							FrameWidthInPixels, (Row+1) * FrameHeightInPixels,
							BitmapWidth,   	 BitmapHeight,
							false, 				false,
							SPRITE_TYPE_IDLE,   SPRITE_ORIENTATION_M0P);
	// First Row Inverted X (idle front right)
	ExtractRegularFrameRow( AssetArena,          &AnimationComponent->Animations[1], 7,
							0,                   (Row+1) * FrameHeightInPixels, 
							FrameWidthInPixels,  FrameHeightInPixels,
							BitmapWidth,    	 BitmapHeight,
							true, 				 false,
							SPRITE_TYPE_IDLE,    SPRITE_ORIENTATION_P0P );

	Row = 1;
	// Second Row Inverted X (walk front left)
	ExtractRegularFrameRow( AssetArena,          &AnimationComponent->Animations[2], 7,
							0,                   (Row+1) * FrameHeightInPixels, 
							FrameWidthInPixels,  FrameHeightInPixels,
							BitmapWidth,    	 BitmapHeight,
							false, 				 false,
							SPRITE_TYPE_WALK,    SPRITE_ORIENTATION_M0P );

	// Second Row Inverted X (walk front right)
	ExtractRegularFrameRow( AssetArena,          &AnimationComponent->Animations[3], 7,
							0,                   (Row+1) * FrameHeightInPixels, 
							FrameWidthInPixels,  FrameHeightInPixels,
							BitmapWidth,    	 BitmapHeight,
							true, 				 false,
							SPRITE_TYPE_WALK,    SPRITE_ORIENTATION_P0P );

	Row = 2;
	// Second Row Inverted X (Idle Back left)
	ExtractRegularFrameRow( AssetArena,          &AnimationComponent->Animations[4], 7,
							0,                   (Row+1) * FrameHeightInPixels, 
							FrameWidthInPixels,  FrameHeightInPixels,
							BitmapWidth,    	 BitmapHeight,
							false, 				 false,
							SPRITE_TYPE_IDLE,    SPRITE_ORIENTATION_M0M );

	// Second Row Inverted X (Idle Back right)
	ExtractRegularFrameRow( AssetArena,          &AnimationComponent->Animations[5], 7,
							0,                   (Row+1) * FrameHeightInPixels, 
							FrameWidthInPixels,  FrameHeightInPixels,
							BitmapWidth,    	 BitmapHeight,
							true, 				 false,
							SPRITE_TYPE_IDLE,    SPRITE_ORIENTATION_P0M );

	Row = 3;
	// Second Row Inverted X (Walk Back left)
	ExtractRegularFrameRow( AssetArena,          &AnimationComponent->Animations[6], 7,
							0,                   (Row+1) * FrameHeightInPixels, 
							FrameWidthInPixels,  FrameHeightInPixels,
							BitmapWidth,    	 BitmapHeight,
							false, 				 false,
							SPRITE_TYPE_WALK,    SPRITE_ORIENTATION_M0M );

	// Second Row Inverted X (Walk Back right)
	ExtractRegularFrameRow( AssetArena,          &AnimationComponent->Animations[7], 7,
							0,                   (Row+1) * FrameHeightInPixels, 
							FrameWidthInPixels,  FrameHeightInPixels,
							BitmapWidth,    	 BitmapHeight,
							true, 				 false,
							SPRITE_TYPE_WALK,    SPRITE_ORIENTATION_P0M );


	AnimationComponent->ActiveSeries = AnimationComponent->Animations;
}
#if 0
	s32 PrinnyIdx = 0;
	for( s32 RowIdx = PrinnyStartY; RowIdx < (PrinnyStartY+NrPrinnyY); ++RowIdx )
	{
		for( s32 ColumnIdx = PrinnyStartX; ColumnIdx < (PrinnyStartX+NrPrinnyX); ++ColumnIdx )
		{
			sprite* Sprite = 0;

			switch(RowIdx)
			{

				case 0:
				{ 
					Sprite = SpriteAnimation->Frames + ColumnIdx;
				}break;
				case 1:
				{
					if(!Assets->PrinnySet.PrinnyWalkFront)
					{
						Assets->PrinnySet.PrinnyWalkFrontFrames = 7;
						Assert(ColumnIdx <= (s32) Assets->PrinnySet.PrinnyWalkFrontFrames);
						Assets->PrinnySet.PrinnyWalkFront = (sprite*) PushArray(AssetArena, Assets->PrinnySet.PrinnyWalkFrontFrames, sprite);
					}
					Sprite = Assets->PrinnySet.PrinnyWalkFront + ColumnIdx;
				}break;
				case 2:
				{
					if(!Assets->PrinnySet.PrinnyIdleBack)
					{
						Assets->PrinnySet.PrinnyIdleBackFrames = 7;
						Assert(ColumnIdx <= (s32) Assets->PrinnySet.PrinnyIdleBackFrames);
						Assets->PrinnySet.PrinnyIdleBack = (sprite*) PushArray(AssetArena, Assets->PrinnySet.PrinnyIdleBackFrames, sprite);
					}
					Sprite = Assets->PrinnySet.PrinnyIdleBack + ColumnIdx;
				}break;
				case 3:
				{
					if(!Assets->PrinnySet.PrinnyWalkBack)
					{
						Assets->PrinnySet.PrinnyWalkBackFrames = 7;
						Assert(ColumnIdx <= (s32) Assets->PrinnySet.PrinnyWalkBackFrames);
						Assets->PrinnySet.PrinnyWalkBack = (sprite*) PushArray(AssetArena, Assets->PrinnySet.PrinnyWalkBackFrames, sprite);
					}

					Sprite = Assets->PrinnySet.PrinnyWalkBack + ColumnIdx;

				}break;
			}

			Assert(Sprite);
			Sprite->Bitmap = PrinnySet;
			Sprite->OriginX = (r32) ColumnIdx*((r32) PrinnyWidthInPixels) /(r32)PrinnySet->Width;
			Sprite->OriginY = (r32) 1.f - (RowIdx+1) * ((r32) PrinnyHeightInPixels)/(r32)PrinnySet->Height;
			Sprite->Width = (r32) PrinnyWidthInPixels  / (r32) PrinnySet->Width;
			Sprite->Height = (r32) PrinnyHeightInPixels / (r32) PrinnySet->Height;
		}
	}

	bitmap* TileSet = LoadTGA( Thread, &GameState->AssetArena,
				 Memory->PlatformAPI.DEBUGPlatformReadEntireFile,
				 Memory->PlatformAPI.DEBUGPlatformFreeFileMemory,
				 "..\\handmade\\data\\Isometric\\sheet.tga" );

	Assert(!TileSet->Handle);

	s32 FloorWidthInPixels  = 16;
	s32 FloorHeightInPixels = 16;


	s32 FloorStartX = 6;
	s32 FloorStartY = 0;
	s32 NrFloorTilesX = 7;
	s32 NrFloorTilesY = 2;

	World->Assets->NrFloorTiles = NrFloorTilesX*NrFloorTilesY;
	World->Assets->FloorTiles = PushArray( &GameState->AssetArena, World->Assets->NrFloorTiles, sprite );

	s32 SpriteIdx = 0;
	for( s32 RowIdx = FloorStartY; RowIdx <  (s32) (FloorStartY+NrFloorTilesY); ++RowIdx )
	{
		for( s32 ColumnIdx = FloorStartX; ColumnIdx < (s32) (FloorStartX + NrFloorTilesX); ++ColumnIdx )
		{
			sprite& Sprite = World->Assets->FloorTiles[SpriteIdx++];
			Sprite.Bitmap  = TileSet;
			Sprite.OriginX = (r32) ColumnIdx*((r32) FloorWidthInPixels) /(r32)TileSet->Width;
			Sprite.OriginY = (r32) 1.f - (RowIdx+1) * ((r32) FloorHeightInPixels)/(r32)TileSet->Height;
			Sprite.Width   = (r32) FloorWidthInPixels  / (r32) TileSet->Width;
			Sprite.Height  = (r32) FloorHeightInPixels / (r32) TileSet->Height;
		}
	}

	World->Assets->NrEdgeTiles = 8;
	World->Assets->EdgeTiles = PushArray( &GameState->AssetArena, World->Assets->NrEdgeTiles, sprite );
	sprite* EdgeTiles = World->Assets->EdgeTiles;


	sprite* TopLeftDrop = &EdgeTiles[0];
	TopLeftDrop->OriginX =  3 * FloorWidthInPixels  / (r32) TileSet->Width;
	TopLeftDrop->OriginY =  1 - FloorHeightInPixels / (r32) TileSet->Height;
	TopLeftDrop->Width =  FloorWidthInPixels      / (r32) TileSet->Width;
	TopLeftDrop->Height =  FloorHeightInPixels     / (r32) TileSet->Height;
	TopLeftDrop->Bitmap = TileSet;

	sprite* TopRightDrop = &EdgeTiles[1];
	TopRightDrop->OriginX =  4 * FloorWidthInPixels  / (r32) TileSet->Width;
	TopRightDrop->OriginY =  1 - FloorHeightInPixels / (r32) TileSet->Height;
	TopRightDrop->Width =  FloorWidthInPixels      / (r32) TileSet->Width;
	TopRightDrop->Height =  FloorHeightInPixels     / (r32) TileSet->Height;
	TopRightDrop->Bitmap = TileSet;

	sprite* BottomLeftDrop = &EdgeTiles[2];
	BottomLeftDrop->OriginX =  3 * FloorWidthInPixels      / (r32) TileSet->Width;
	BottomLeftDrop->OriginY =  1 - 2 * FloorHeightInPixels / (r32) TileSet->Height;
	BottomLeftDrop->Width =  FloorWidthInPixels          / (r32) TileSet->Width;
	BottomLeftDrop->Height =  FloorHeightInPixels         / (r32) TileSet->Height;
	BottomLeftDrop->Bitmap = TileSet;

	sprite* BottomRightDrop = &EdgeTiles[3];
	BottomRightDrop->OriginX =  4 * FloorWidthInPixels      / (r32) TileSet->Width;
	BottomRightDrop->OriginY =  1 - 2 * FloorHeightInPixels / (r32) TileSet->Height;
	BottomRightDrop->Width =  FloorWidthInPixels          / (r32) TileSet->Width;
	BottomRightDrop->Height =  FloorHeightInPixels         / (r32) TileSet->Height;
	BottomRightDrop->Bitmap = TileSet;

	sprite* LeftDrop = &EdgeTiles[4];
	LeftDrop->OriginX =  2 * FloorWidthInPixels      / (r32) TileSet->Width;
	LeftDrop->OriginY =  1 - 2 * FloorHeightInPixels / (r32) TileSet->Height;
	LeftDrop->Width =  FloorWidthInPixels          / (r32) TileSet->Width;
	LeftDrop->Height =  FloorHeightInPixels         / (r32) TileSet->Height;
	LeftDrop->Bitmap = TileSet;

	sprite* RightDrop = &EdgeTiles[5];
	RightDrop->OriginX =  0;
	RightDrop->OriginY =  1 - 2 * FloorHeightInPixels / (r32) TileSet->Height;
	RightDrop->Width =  FloorWidthInPixels          / (r32) TileSet->Width;
	RightDrop->Height =  FloorHeightInPixels         / (r32) TileSet->Height;
	RightDrop->Bitmap = TileSet;

	sprite* TopDrop = &EdgeTiles[6];
	TopDrop->OriginX =  1 * FloorWidthInPixels      / (r32) TileSet->Width;
	TopDrop->OriginY =  1 - 3 * FloorHeightInPixels / (r32) TileSet->Height;
	TopDrop->Width =  FloorWidthInPixels          / (r32) TileSet->Width;
	TopDrop->Height =  FloorHeightInPixels         / (r32) TileSet->Height;
	TopDrop->Bitmap = TileSet;

	sprite* BottomDrop = &EdgeTiles[7];
	BottomDrop->OriginX =  1 * FloorWidthInPixels      / (r32) TileSet->Width;
	BottomDrop->OriginY =  1 - 1 * FloorHeightInPixels / (r32) TileSet->Height;
	BottomDrop->Width =  FloorWidthInPixels          / (r32) TileSet->Width;
	BottomDrop->Height =  FloorHeightInPixels         / (r32) TileSet->Height;
	BottomDrop->Bitmap = TileSet;


	r32 TilesPerRoomWidth = 16;
	r32 TilesPerRoomHeight = 9;

#endif
#endif // SPRITE_MAPPING_H