#include "render_push_buffer.h"

#define GetRenderCommandsTail( RenderCommands ) ( RenderCommands->PushBuffer + RenderCommands->PushBufferSize )

u8* PushNewHeader(game_render_commands* RenderCommands, push_buffer_header** PreviousEntry, u32 Type)
{
	Assert(PreviousEntry);
	u32 TypeSize = 0;
	switch(Type)
	{
		case RENDER_TYPE_ENTITY:
		{
			TypeSize = sizeof(entry_type_entity);
		}break;
		case RENDER_TYPE_SPRITE:
		{
			TypeSize = sizeof(entry_type_sprite);
		}break;
		case RENDER_TYPE_WIREBOX:
		{
			TypeSize = sizeof(entry_type_wirebox);
		}break;
		case RENDER_TYPE_FLOOR_TILE:
		{
			TypeSize = sizeof(entry_type_floor_tile);
		}break;
		default:
		{
			INVALID_CODE_PATH
		}break;
	}
	Assert(TypeSize);

	render_push_buffer* PushBuffer = (render_push_buffer*) RenderCommands->PushBuffer;
	push_buffer_header* NewEntryHeader = (push_buffer_header*) GetRenderCommandsTail( RenderCommands );
	RenderCommands->PushBufferSize += sizeof(push_buffer_header);
	RenderCommands->PushBufferElementCount++;
	Assert( RenderCommands->PushBufferSize < RenderCommands->MaxPushBufferSize );
	NewEntryHeader->Type = Type;
	NewEntryHeader->Next = 0;
	
	if( ! *PreviousEntry )
	{
		PushBuffer->First = NewEntryHeader;
		*PreviousEntry = PushBuffer->First;
	}else{
		(*PreviousEntry)->Next = NewEntryHeader;
		*PreviousEntry = NewEntryHeader;
	}

	u8* Entry = (u8*)  GetRenderCommandsTail( RenderCommands );
	RenderCommands->PushBufferSize += TypeSize;
	return Entry;
}

void FillRenderPushBuffer( world* World, game_render_commands* RenderCommands )
{
	Assert(RenderCommands);
	Assert(RenderCommands->PushBuffer);
	Assert(RenderCommands->MaxPushBufferSize);
	RenderCommands->PushBufferElementCount = 0;
	RenderCommands->PushBufferSize = 0;
	
	// Reset Render Commands
	RenderCommands->PushBufferElementCount = 0;
	RenderCommands->PushBufferSize = 0;

	render_push_buffer* PushBuffer = (render_push_buffer*) RenderCommands->PushBuffer;
	RenderCommands->PushBufferSize += sizeof(render_push_buffer);
 	PushBuffer->First = 0;
 	PushBuffer->Assets = World->Assets;
	PushBuffer->RenderCommands = RenderCommands;

	push_buffer_header* PreviousEntry = 0;

	for(u32 Index = 0; Index <  World->NrEntities; ++Index )
	{
		entity* Entity = &World->Entities[Index];

		if( Entity->Types & COMPONENT_TYPE_CAMERA )
		{
			PushBuffer->ProjectionMatrix = Entity->CameraComponent->P;
			PushBuffer->ViewMatrix       = Entity->CameraComponent->V;
		}

		if( Entity->Types & COMPONENT_TYPE_MESH )
		{
			entry_type_entity* EntityEntry = (entry_type_entity*) PushNewHeader( RenderCommands, &PreviousEntry, RENDER_TYPE_ENTITY );
			EntityEntry->Entity = Entity;
		}

		if(Entity->Types & COMPONENT_TYPE_SPATIAL )
		{
			entry_type_wirebox* WireEntry = (entry_type_wirebox*) PushNewHeader( RenderCommands, &PreviousEntry, RENDER_TYPE_WIREBOX );
			
			v3 Pos  = Entity->SpatialComponent->Position;

			WireEntry->Rect.X = Pos.X-Entity->SpatialComponent->Width/2;
			WireEntry->Rect.Y = Pos.Y-Entity->SpatialComponent->Height/2;
			WireEntry->Rect.W = Entity->SpatialComponent->Width;
			WireEntry->Rect.H = Entity->SpatialComponent->Height;
		}

		if( Entity->Types & COMPONENT_TYPE_SPRITE_ANIMATION )
		{
			// Store only the sprite to be displayed for current frame
			entry_type_sprite* SpriteEntry = (entry_type_sprite*) PushNewHeader( RenderCommands, &PreviousEntry, RENDER_TYPE_SPRITE );
			SpriteEntry->Bitmap = Entity->SpriteAnimationComponent->Bitmap;
			SpriteEntry->Coordinates = Entity->SpriteAnimationComponent->ActiveSeries->ActiveFrame;
			
			SpriteEntry->M = M4Identity();
			m4 SpriteSize   = GetScaleMatrix( V4( Entity->SpriteAnimationComponent->Dimensions.W, Entity->SpriteAnimationComponent->Dimensions.H, 1, 0 ) );
			m4 SpriteOffset = GetTranslationMatrix( V4( Entity->SpriteAnimationComponent->Dimensions.X, Entity->SpriteAnimationComponent->Dimensions.Y, 0, 0 ) );
			if( Entity->Types & COMPONENT_TYPE_SPATIAL )
			{
				SpriteEntry->M = GetAsMatrix( Entity->SpatialComponent ) * SpriteOffset * SpriteSize;
			}
		}
	}
#if 0
	u32 Width = 40;
	u32 Height = 40;
	for(u32 i = 0; i < Height; ++i )
	{
		for(u32 j = 0; j < Width; ++j )
		{
			tile_contents Contents = GetTileContents( &World->TileMap, j, i, 0);
			if(Contents.Sprite)
			{
				Assert(Contents.Sprite->Bitmap);
				entry_type_floor_tile* SpriteEntry = (entry_type_floor_tile*) PushNewHeader( RenderCommands, &PreviousEntry, RENDER_TYPE_FLOOR_TILE );
				
				SpriteEntry->Bitmap = Contents.Sprite->Bitmap;

				r32 TileWidth = World->TileMap.TileWidthInMeters;
				r32 TileHeight = World->TileMap.TileHeightInMeters;
		
				// Todo(Jakob): Encoding the width and height of the tile in the scale part of the Affine matrix feels
				// a bit hacky. Is there some better way?
				m4 ScaleMatrix = GetScaleMatrix( V4(TileWidth, TileHeight, 1, 0) );
				m4 TranslationMatrix = GetTranslationMatrix( V4(TileWidth * j, TileHeight * i, 0, 1 ) );

				m4 Matrix = TranslationMatrix * ScaleMatrix;
				SpriteEntry->M = Matrix;

				SpriteEntry->Coordinates = &Contents.Sprite->TextureCoordinates;
			}
		}
	}
#endif

};
