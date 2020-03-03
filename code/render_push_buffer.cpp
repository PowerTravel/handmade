#include "render_push_buffer.h"

#include "component_camera.h"
#include "component_surface.h"
#include "component_light.h"
#include "component_collider.h"

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

  // Then push all the lights and camera
  for(u32 Index = 0; Index <  World->NrEntities; ++Index )
  {
    entity* Entity = &World->Entities[Index];

    if( Entity->Types & COMPONENT_TYPE_CAMERA )
    {
      PushBuffer->ProjectionMatrix = Entity->CameraComponent->P;
      PushBuffer->ViewMatrix       = Entity->CameraComponent->V;
      break;
    }

    if( (Entity->Types & COMPONENT_TYPE_LIGHT) &&
      (Entity->Types & COMPONENT_TYPE_SPATIAL) )
    {
      push_buffer_header* Header = PushNewHeader( RenderCommands, &PreviousEntry );
      Header->Type = render_buffer_entry_type::LIGHT;

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
      Header->Type = render_buffer_entry_type::INDEXED_BUFFER;
      Header->RenderState = RENDER_STATE_CULL_BACK | RENDER_STATE_FILL;
      entry_type_indexed_buffer* Body = (entry_type_indexed_buffer*) RenderCommands->RenderMemory.GetMemory(sizeof(entry_type_indexed_buffer));
      Body->BufferType = BUFFER_TYPE_TRIANGLE;
      Body->VAO = &Entity->MeshComponent->VAO;
      Body->VBO = &Entity->MeshComponent->VBO;
      Body->FillVBO = Entity->ColliderComponent->Mesh->VAO==0;
      Body->nvi = Entity->MeshComponent->Indeces.Count;
      Body->nv  = Entity->MeshComponent->Data->nv;
      Body->nvn = Entity->MeshComponent->Data->nvn;
      Body->nvt = Entity->MeshComponent->Data->nvt;
      Body->v   = Entity->MeshComponent->Data->v;
      Body->vn  = Entity->MeshComponent->Data->vn;
      Body->vt  = Entity->MeshComponent->Data->vt;
      Body->vi  = Entity->MeshComponent->Indeces.vi;
      Body->ti  = Entity->MeshComponent->Indeces.ti;
      Body->ni  = Entity->MeshComponent->Indeces.ni;
      Body->Surface = Entity->SurfaceComponent;
      Body->M       = Entity->SpatialComponent->ModelMatrix;
      Body->NM      = Transpose(RigidInverse(Body->M));

      Body->ElementStart  = 0;
      Body->ElementLength = Body->nvi;
    }

    if( Entity->Types & COMPONENT_TYPE_SPRITE_ANIMATION )
    {
      push_buffer_header* Header = (push_buffer_header*) PushNewHeader( RenderCommands, &PreviousEntry );
      Header->Type = render_buffer_entry_type::PRIMITIVE;
      Header->RenderState = RENDER_STATE_FILL;

      entry_type_primitive* Body = (entry_type_primitive*) RenderCommands->RenderMemory.GetMemory(sizeof(entry_type_primitive));
      Body->Surface = (component_surface*) RenderCommands->RenderMemory.GetMemory(sizeof(component_surface));
      Body->Surface->Material = (material*) RenderCommands->RenderMemory.GetMemory(sizeof(material));
      SetMaterial(Body->Surface->Material, MATERIAL_WHITE);
      Body->Surface->Material->DiffuseMap = Entity->SpriteAnimationComponent->Bitmap;
      Body->PrimitiveType = primitive_type::QUAD;
      Body->M  = M4Identity();
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
        Body->M = Entity->SpatialComponent->ModelMatrix;
      }
    }

    // Here we wanna do a wire frame and collision points
    if( Entity->Types & COMPONENT_TYPE_COLLIDER  )
    {
      {
        push_buffer_header* Header = (push_buffer_header*) PushNewHeader( RenderCommands, &PreviousEntry );
        Header->Type = render_buffer_entry_type::INDEXED_BUFFER;
        Header->RenderState = RENDER_STATE_WIREFRAME;
        entry_type_indexed_buffer* Body = (entry_type_indexed_buffer*) RenderCommands->RenderMemory.GetMemory(sizeof(entry_type_indexed_buffer));
        Body->VAO = &Entity->ColliderComponent->Mesh->VAO;
        Body->VBO = &Entity->ColliderComponent->Mesh->VBO;
        Body->FillVBO = Entity->ColliderComponent->Mesh->VAO==0;
        Body->BufferType = BUFFER_TYPE_TRIANGLE;
        Body->nvi =  Entity->ColliderComponent->Mesh->nvi;
        Body->nv  =  Entity->ColliderComponent->Mesh->nv;
        Body->v   =  Entity->ColliderComponent->Mesh->v;
        Body->vi  =  Entity->ColliderComponent->Mesh->vi;
        Body->Surface = (component_surface*) RenderCommands->RenderMemory.GetMemory(sizeof(component_surface));
        Body->Surface->Material = (material*) RenderCommands->RenderMemory.GetMemory(sizeof(material));
        SetMaterial(Body->Surface->Material, MATERIAL_JADE);
        Body->M = Entity->SpatialComponent->ModelMatrix;
        Body->NM = Transpose(RigidInverse(Body->M));

        Body->ElementStart  = 0;
        Body->ElementLength = Body->nvi;
      }
      if(Entity->ColliderComponent->IsColliding)
      {
        push_buffer_header* Header = (push_buffer_header*) PushNewHeader( RenderCommands, &PreviousEntry );
        Header->Type = render_buffer_entry_type::PRIMITIVE;
        Header->RenderState = RENDER_STATE_POINTS;
        entry_type_primitive* Body = (entry_type_primitive*) RenderCommands->RenderMemory.GetMemory(sizeof(entry_type_primitive));
        Body->M = Entity->SpatialComponent->ModelMatrix * GetTranslationMatrix( V4(Entity->ColliderComponent->CollisionPoint,1));
        Body->TM = M4Identity();
        Body->PrimitiveType = primitive_type::POINT;
      }
    }

    if( Entity->Types & COMPONENT_TYPE_GJK_EPA_VISUALIZER  )
    {
      component_gjk_epa_visualizer* Vis = Entity->GjkEpaVisualizerComponent;
      if(Vis->Playback)
      {
        {
          component_surface* Surface = (component_surface*) RenderCommands->RenderMemory.GetMemory(sizeof(component_surface));
          Surface->Material = (material*) RenderCommands->RenderMemory.GetMemory(sizeof(material));
          SetMaterial(Surface->Material, MATERIAL_BLUE_RUBBER);

          u32* vi = Entity->GjkEpaVisualizerComponent->Indeces;
          v3* v   = Entity->GjkEpaVisualizerComponent->Vertices;

          // Todo: This is inefficient, better option would be either instancing
          // http://www.opengl-tutorial.org/intermediate-tutorials/billboards-particles/particles-instancing/
          // Or fill every quad into one big buffer and draw them with one draw call
          for(u32 i = Vis->CSOMeshOffset; i < Vis->CSOMeshOffset + Vis->CSOMeshLength; ++i)
          {
            push_buffer_header*  Header = (push_buffer_header*) PushNewHeader( RenderCommands, &PreviousEntry );
            Header->Type = render_buffer_entry_type::PRIMITIVE;
            Header->RenderState = RENDER_STATE_FILL;
            entry_type_primitive* Body = (entry_type_primitive*) RenderCommands->RenderMemory.GetMemory(sizeof(entry_type_primitive));
            v3 Vertex = v[vi[i]];
            Body->M = GetTranslationMatrix(V4(Vertex,1)) * GetScaleMatrix(V4(0.05,0.05,0.05,1));
            Body->TM = M4Identity();
            Body->Surface = Surface;
            Body->PrimitiveType = primitive_type::VOXEL;
          }
        }

        {
          push_buffer_header* Header = (push_buffer_header*) PushNewHeader( RenderCommands, &PreviousEntry );
          Header->RenderState = RENDER_STATE_WIREFRAME;
          Header->Type = render_buffer_entry_type::INDEXED_BUFFER;
          entry_type_indexed_buffer* Body = (entry_type_indexed_buffer*) RenderCommands->RenderMemory.GetMemory(sizeof(entry_type_indexed_buffer));
          Body->VAO = &Entity->GjkEpaVisualizerComponent->VAO;
          Body->VBO = &Entity->GjkEpaVisualizerComponent->VBO;
          Body->FillVBO = Entity->GjkEpaVisualizerComponent->UpdateVBO;
          Body->nvi = Entity->GjkEpaVisualizerComponent->IndexCount;
          Body->vi  = Entity->GjkEpaVisualizerComponent->Indeces;
          Body->nv  = Entity->GjkEpaVisualizerComponent->VertexCount;
          Body->v   = Entity->GjkEpaVisualizerComponent->Vertices;

          Body->Surface = (component_surface*) RenderCommands->RenderMemory.GetMemory(sizeof(component_surface));
          Body->Surface->Material = (material*) RenderCommands->RenderMemory.GetMemory(sizeof(material));
          SetMaterial(Body->Surface->Material, MATERIAL_GREEN_RUBBER);
          Body->M  = M4Identity();
          Body->NM = M4Identity();

          simplex_index* SI = &Vis->Simplex[Vis->ActiveSimplexFrame];
          Body->ElementStart  = SI->Offset;
          Body->ElementLength = SI->Length;

          switch(SI->Length)
          {
            case 1: Body->BufferType = BUFFER_TYPE_LINE;     break;
            case 2: Body->BufferType = BUFFER_TYPE_LINE;     break;
            case 3: Body->BufferType = BUFFER_TYPE_TRIANGLE; break;
            case 4: Body->BufferType = BUFFER_TYPE_TRIANGLE; break;
          }
        }

        {
          component_surface* Surface = (component_surface*) RenderCommands->RenderMemory.GetMemory(sizeof(component_surface));
          Surface->Material = (material*) RenderCommands->RenderMemory.GetMemory(sizeof(material));
          SetMaterial(Surface->Material, MATERIAL_GREEN_RUBBER);

          u32 nvi = Entity->GjkEpaVisualizerComponent->IndexCount;
          u32* vi = Entity->GjkEpaVisualizerComponent->Indeces;
          v3* v   = Entity->GjkEpaVisualizerComponent->Vertices;

          // Todo: This is inefficient, better option would be either instancing
          // http://www.opengl-tutorial.org/intermediate-tutorials/billboards-particles/particles-instancing/
          // Or fill every quad into one big buffer and draw them with one draw call
          simplex_index* SI = &Vis->Simplex[Vis->ActiveSimplexFrame];
          for(u32 i = SI->Offset; i < SI->Offset + SI->Length; ++i)
          {
            push_buffer_header*  Header = (push_buffer_header*) PushNewHeader( RenderCommands, &PreviousEntry );
            Header->Type = render_buffer_entry_type::PRIMITIVE;
            Header->RenderState = RENDER_STATE_FILL;
            entry_type_primitive* Body = (entry_type_primitive*) RenderCommands->RenderMemory.GetMemory(sizeof(entry_type_primitive));
            v3 Vertex = v[vi[i]];
            Body->M = GetTranslationMatrix(V4(Vertex,1)) * GetScaleMatrix(V4(0.1,0.1,0.1,1));
            Body->TM = M4Identity();
            Body->Surface = Surface;
            Body->PrimitiveType = primitive_type::VOXEL;
          }
        }

        {
          push_buffer_header* Header = (push_buffer_header*) PushNewHeader( RenderCommands, &PreviousEntry );
          Header->Type = render_buffer_entry_type::PRIMITIVE;
          Header->RenderState = RENDER_STATE_FILL;
          entry_type_primitive* Body = (entry_type_primitive*) RenderCommands->RenderMemory.GetMemory(sizeof(entry_type_primitive));

          Body->Surface = (component_surface*) RenderCommands->RenderMemory.GetMemory(sizeof(component_surface));
          Body->Surface->Material = (material*) RenderCommands->RenderMemory.GetMemory(sizeof(material));
          SetMaterial(Body->Surface->Material, MATERIAL_RED_RUBBER);

          simplex_index* SI = &Vis->Simplex[Vis->ActiveSimplexFrame];
          Body->M = GetTranslationMatrix( V4(SI->ClosestPoint,1)) * GetScaleMatrix( V4(0.1,0.1,0.1,1));
          Body->TM = M4Identity();
          Body->PrimitiveType = primitive_type::VOXEL;
        }

        {
          push_buffer_header*  Header = (push_buffer_header*) PushNewHeader( RenderCommands, &PreviousEntry );
          Header->Type = render_buffer_entry_type::PRIMITIVE;
          Header->RenderState = RENDER_STATE_FILL;
          entry_type_primitive* Body = (entry_type_primitive*) RenderCommands->RenderMemory.GetMemory(sizeof(entry_type_primitive));

          Body->Surface = (component_surface*) RenderCommands->RenderMemory.GetMemory(sizeof(component_surface));
          Body->Surface->Material = (material*) RenderCommands->RenderMemory.GetMemory(sizeof(material));
          SetMaterial(Body->Surface->Material, MATERIAL_WHITE);

          Body->M = GetTranslationMatrix( V4(0,0,0,1)) * GetScaleMatrix( V4(0.1,0.1,0.1,1));
          Body->TM = M4Identity();
          Body->PrimitiveType = primitive_type::VOXEL;
        }
      }
    }
  }
#if 0
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
        Header->Type = render_buffer_entry_type::TILE;
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
#endif
}