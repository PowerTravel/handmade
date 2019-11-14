#include "render_push_buffer.h"

//#define GetRenderCommandsTail( RenderCommands ) ( RenderCommands->PushBuffer + RenderCommands->PushBufferSize )

push_buffer_header* PushNewHeader(game_render_commands* RenderCommands, push_buffer_header** PreviousEntry)
{
	Assert(PreviousEntry);

	utils::push_buffer* RenderMemory = &RenderCommands->RenderMemory;
	RenderCommands->RenderMemoryElementCount++;

	push_buffer_header* NewEntryHeader = (push_buffer_header*) RenderMemory->GetMemory(sizeof(push_buffer_header));
	Assert( NewEntryHeader );
	NewEntryHeader->Next = 0;
	
	if( ! *PreviousEntry )
	{
		render_push_buffer* PushBuffer = (render_push_buffer*) RenderMemory->GetBase();
		PushBuffer->First = NewEntryHeader;
		*PreviousEntry = PushBuffer->First;
	}else{
		(*PreviousEntry)->Next = NewEntryHeader;
		*PreviousEntry = NewEntryHeader;
	}
	return NewEntryHeader;
}

void FillRenderPushBuffer( world* World, game_render_commands* RenderCommands )
{
	Assert(RenderCommands);
	Assert(RenderCommands->RenderMemory.GetBase());

	RenderCommands->RenderMemory.Clear();
	RenderCommands->RenderMemoryElementCount = 0;
	
	render_push_buffer* PushBuffer = (render_push_buffer*) RenderCommands->RenderMemory.GetMemory(sizeof(render_push_buffer));
 	PushBuffer->First = 0;
	
	// TODO: Make a proper Asset library so we can extract for example a texture or mesh from a hash id
	//       Instead of storing pointers everywhere.
 	PushBuffer->Assets = World->Assets;

	push_buffer_header* PreviousEntry = 0;
	// TODO: Make a proper entity library so we can extracl for example ALL Lights efficiently
	//       So we don't have to loop over ALL entitis several times

	// First push camera
	for(u32 Index = 0; Index <  World->NrEntities; ++Index )
	{
		entity* Entity = &World->Entities[Index];

		if( Entity->Types & COMPONENT_TYPE_CAMERA )
		{
			PushBuffer->ProjectionMatrix = Entity->CameraComponent->P;
			PushBuffer->ViewMatrix       = Entity->CameraComponent->V;
			break;
		}
	}

	// Then push all the lights
	for(u32 Index = 0; Index <  World->NrEntities; ++Index )
	{
		entity* Entity = &World->Entities[Index];

		if( (Entity->Types & COMPONENT_TYPE_LIGHT) &&
		 	(Entity->Types & COMPONENT_TYPE_SPATIAL) )
		{
			push_buffer_header* Header = PushNewHeader( RenderCommands, &PreviousEntry );
			Header->Type = render_type::LIGHT;

			entry_type_light* Body = (entry_type_light*) RenderCommands->RenderMemory.GetMemory(sizeof(entry_type_light));
			Body->Color  = Entity->LightComponent->Color;
			Body->M      = Entity->SpatialComponent->ModelMatrix;
		}
	}

	for(u32 Index = 0; Index <  World->NrEntities; ++Index )
	{
		entity* Entity = &World->Entities[Index];

		if( (Entity->Types & COMPONENT_TYPE_MESH ) &&
		 	(Entity->Types & COMPONENT_TYPE_SPATIAL) )
		{
			push_buffer_header* Header = (push_buffer_header*) PushNewHeader( RenderCommands, &PreviousEntry );
			Header->Type = render_type::MESH;
			entry_type_mesh* Body = (entry_type_mesh*) RenderCommands->RenderMemory.GetMemory(sizeof(entry_type_mesh));
			Body->Mesh    = Entity->MeshComponent;
			Body->Surface = Entity->SurfaceComponent;
			Body->M  = Entity->SpatialComponent->ModelMatrix;
			Body->NM = Transpose(RigidInverse(Body->M));
		}

		#if 0
		if(Entity->Types & COMPONENT_TYPE_SPATIAL )
		{
			// TODO: Do Wirebox Rendering
		}
		#endif

		if( Entity->Types & COMPONENT_TYPE_SPRITE_ANIMATION )
		{
			push_buffer_header* Header = (push_buffer_header*) PushNewHeader( RenderCommands, &PreviousEntry );
			Header->Type = render_type::SPRITE;

			entry_type_sprite* Body = (entry_type_sprite*) RenderCommands->RenderMemory.GetMemory(sizeof(entry_type_sprite));

			// Store only the sprite to be displayed for current frame
			Body->Bitmap = Entity->SpriteAnimationComponent->Bitmap;
			Body->Coordinates = Entity->SpriteAnimationComponent->ActiveSeries->ActiveFrame;
			
			Body->M = M4Identity();
			m4 SpriteSize   = GetScaleMatrix( V4( Entity->SpriteAnimationComponent->Dimensions.W, Entity->SpriteAnimationComponent->Dimensions.H, 1, 0 ) );
			m4 SpriteOffset = GetTranslationMatrix( V4( Entity->SpriteAnimationComponent->Dimensions.X, Entity->SpriteAnimationComponent->Dimensions.Y, 0, 0 ) );
			if( Entity->Types & COMPONENT_TYPE_SPATIAL )
			{
				Body->M = Entity->SpatialComponent->ModelMatrix * SpriteOffset * SpriteSize;
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
