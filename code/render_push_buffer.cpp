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

    if( Entity->Types & COMPONENT_TYPE_SPRITE_ANIMATION )
    {
      push_buffer_header* Header = (push_buffer_header*) PushNewHeader( RenderCommands, &PreviousEntry );
      Header->Type = render_type::TILE;

      entry_type_sprite* Body = (entry_type_sprite*) RenderCommands->RenderMemory.GetMemory(sizeof(entry_type_sprite));

      // Store only the sprite to be displayed for current frame
      Body->Bitmap = Entity->SpriteAnimationComponent->Bitmap;
      Body->M = M4Identity();
      Body->TM = Entity->SpriteAnimationComponent->ActiveSeries->Get();

      if(Entity->SpriteAnimationComponent->InvertX)
      {
        v4 Dim = Body->TM * ( V4(1,1,0,1) - V4(0,0,0,1));

        m4 ScaleMat = GetScaleMatrix( V4(-1,1,1,0) );

        m4 tto = M4(1,0,0,-Body->TM.E[3] - Dim.X/2,
                    0,1,0,-Body->TM.E[7] - Dim.Y/2,
                    0,0,1,0,
                    0,0,0,1);

        m4 bfo = M4(1,0,0, Body->TM.E[3] + Dim.X/2,
                    0,1,0, Body->TM.E[7] + Dim.Y/2,
                    0,0,1,0,
                    0,0,0,1);

        Body->TM =  bfo * ScaleMat * tto * Body->TM;
      }
      if( Entity->Types & COMPONENT_TYPE_SPATIAL )
      {
        Body->M = Entity->SpatialComponent->ModelMatrix;// * SpriteOffset * SpriteSize;
      }
    }

    if( Entity->Types & COMPONENT_TYPE_COLLIDER  )
    {
      {
        push_buffer_header* Header = (push_buffer_header*) PushNewHeader( RenderCommands, &PreviousEntry );
        Header->Type = render_type::WIREBOX;

        entry_type_wirebox* Body = (entry_type_wirebox*) RenderCommands->RenderMemory.GetMemory(sizeof(entry_type_wirebox));
        Body->M = Entity->SpatialComponent->ModelMatrix;
        Body->Mesh = Entity->ColliderComponent->Mesh;
      }
      if(Entity->ColliderComponent->IsColliding)
      {
        push_buffer_header* Header = (push_buffer_header*) PushNewHeader( RenderCommands, &PreviousEntry );
        Header->Type = render_type::POINT;

        entry_type_point* Body = (entry_type_point*) RenderCommands->RenderMemory.GetMemory(sizeof(entry_type_point));
        Body->M = Entity->SpatialComponent->ModelMatrix * GetTranslationMatrix( V4(Entity->ColliderComponent->CollisionPoint,1));
      }
    }

  }

  // Get Position from Entity->CameraComponent->V
  u32 Width = 12;
  u32 Height = 12;

  sprite_sheet Sprite = World->Assets->TileMapSpriteSheet;
  for(u32 i = 0; i < Height; ++i )
  {
    for(u32 j = 0; j < Width; ++j )
    {
      tile_contents Contents = GetTileContents( &World->TileMap, j, i, 0);
      if(Contents.Type == TILE_TYPE_FLOOR)
      {
        push_buffer_header* Header = (push_buffer_header*) PushNewHeader( RenderCommands, &PreviousEntry );
        Header->Type = render_type::TILE;
        entry_type_sprite* Body = (entry_type_map_tile*) RenderCommands->RenderMemory.GetMemory(sizeof(entry_type_map_tile));

        Body->Bitmap = Contents.Bitmap;

        r32 TileWidth  = World->TileMap.TileWidthInMeters;
        r32 TileHeight = World->TileMap.TileHeightInMeters;

        m4 ScaleMatrix = GetScaleMatrix( V4(TileWidth, TileHeight, 1, 0) );
        m4 TranslationMatrix = GetTranslationMatrix( V4(TileWidth * j, TileHeight * i, 0, 1 ) );

        m4 Matrix = TranslationMatrix * ScaleMatrix;
        Body->M = Matrix;

        Body->TM = Contents.TM;
      }
    }
  }
}