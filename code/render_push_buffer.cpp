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
      render_buffer* Buffer = (render_buffer*) RenderCommands->RenderMemory.GetMemory(sizeof(render_buffer));
      Buffer->VAO  = &Entity->MeshComponent->VAO;
      Buffer->VBO  = &Entity->MeshComponent->VBO;
      Buffer->Fill = *Buffer->VAO==0;
      Buffer->nvi  = Entity->MeshComponent->Indeces.Count;
      Buffer->nv   = Entity->MeshComponent->Data->nv;
      Buffer->nvn  = Entity->MeshComponent->Data->nvn;
      Buffer->nvt  = Entity->MeshComponent->Data->nvt;
      Buffer->v    = Entity->MeshComponent->Data->v;
      Buffer->vn   = Entity->MeshComponent->Data->vn;
      Buffer->vt   = Entity->MeshComponent->Data->vt;
      Buffer->vi   = Entity->MeshComponent->Indeces.vi;
      Buffer->ti   = Entity->MeshComponent->Indeces.ti;
      Buffer->ni   = Entity->MeshComponent->Indeces.ni;

      push_buffer_header* Header = (push_buffer_header*) PushNewHeader( RenderCommands, &PreviousEntry );
      Header->Type = render_buffer_entry_type::INDEXED_BUFFER;
      Header->RenderState = RENDER_STATE_CULL_BACK | RENDER_STATE_FILL;

      entry_type_indexed_buffer* Body = (entry_type_indexed_buffer*) RenderCommands->RenderMemory.GetMemory(sizeof(entry_type_indexed_buffer));
      Body->Buffer   = Buffer;
      Body->DataType = DATA_TYPE_TRIANGLE;
      Body->Surface  = Entity->SurfaceComponent;
      Body->M        = Entity->SpatialComponent->ModelMatrix;
      Body->NM       = Transpose(RigidInverse(Body->M));
      Body->ElementStart  = 0;
      Body->ElementLength = Buffer->nvi;
    }

    if( Entity->Types & COMPONENT_TYPE_SPRITE_ANIMATION )
    {
      component_surface* WhiteSurface = (component_surface*) RenderCommands->RenderMemory.GetMemory(sizeof(component_surface));
      WhiteSurface->Material = (material*) RenderCommands->RenderMemory.GetMemory(sizeof(material));
      SetMaterial(WhiteSurface->Material, MATERIAL_WHITE);

      push_buffer_header* Header = (push_buffer_header*) PushNewHeader( RenderCommands, &PreviousEntry );
      Header->Type = render_buffer_entry_type::PRIMITIVE;
      Header->RenderState = RENDER_STATE_FILL;

      entry_type_primitive* Body = (entry_type_primitive*) RenderCommands->RenderMemory.GetMemory(sizeof(entry_type_primitive));
      Body->Surface = WhiteSurface;
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
      render_buffer* Buffer = (render_buffer*) RenderCommands->RenderMemory.GetMemory(sizeof(render_buffer));
      Buffer->VAO  = &Entity->ColliderComponent->Mesh->VAO;
      Buffer->VBO  = &Entity->ColliderComponent->Mesh->VBO;
      Buffer->Fill =  *Buffer->VAO==0;
      Buffer->nvi  =  Entity->ColliderComponent->Mesh->nvi;
      Buffer->nv   =  Entity->ColliderComponent->Mesh->nv;
      Buffer->v    =  Entity->ColliderComponent->Mesh->v;
      Buffer->vi   =  Entity->ColliderComponent->Mesh->vi;

      component_surface* JadeSurface = (component_surface*) RenderCommands->RenderMemory.GetMemory(sizeof(component_surface));
      JadeSurface->Material = (material*) RenderCommands->RenderMemory.GetMemory(sizeof(material));
      SetMaterial(JadeSurface->Material, MATERIAL_JADE);

      component_surface* GreenSurface = (component_surface*) RenderCommands->RenderMemory.GetMemory(sizeof(component_surface));
      GreenSurface->Material = (material*) RenderCommands->RenderMemory.GetMemory(sizeof(material));
      SetMaterial(GreenSurface->Material, MATERIAL_GREEN);

      {
        push_buffer_header* Header = (push_buffer_header*) PushNewHeader( RenderCommands, &PreviousEntry );
        Header->Type = render_buffer_entry_type::INDEXED_BUFFER;
        Header->RenderState = RENDER_STATE_WIREFRAME;

        entry_type_indexed_buffer* Body = (entry_type_indexed_buffer*) RenderCommands->RenderMemory.GetMemory(sizeof(entry_type_indexed_buffer));
        Body->Buffer = Buffer;
        Body->DataType = DATA_TYPE_TRIANGLE;
        Body->Surface = JadeSurface;
        Body->M = Entity->SpatialComponent->ModelMatrix;
        Body->NM = Transpose(RigidInverse(Body->M));

        Body->ElementStart  = 0;
        Body->ElementLength = Buffer->nvi;
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
        Body->Surface = GreenSurface;
      }
    }

    if( Entity->Types & COMPONENT_TYPE_GJK_EPA_VISUALIZER  )
    {
      component_gjk_epa_visualizer* Vis = Entity->GjkEpaVisualizerComponent;
      if(Vis->Playback && Vis->IndexCount>0)
      {

        render_buffer* Buffer = (render_buffer*) RenderCommands->RenderMemory.GetMemory(sizeof(render_buffer));
        Buffer->VAO  = &Vis->VAO;
        Buffer->VBO  = &Vis->VBO;
        Buffer->Fill = Vis->UpdateVBO;
        Buffer->nvi  = Vis->IndexCount;
        Buffer->vi   = Vis->Indeces;
        Buffer->nv   = Vis->VertexCount;
        Buffer->v    = Vis->Vertices;

        Buffer->ni   = Vis->NormalIndeces;
        Buffer->nvn  = Vis->NormalCount;
        Buffer->vn   = Vis->Normals;

        component_surface* BlueRubberSurface = (component_surface*) RenderCommands->RenderMemory.GetMemory(sizeof(component_surface));
        BlueRubberSurface->Material = (material*) RenderCommands->RenderMemory.GetMemory(sizeof(material));
        SetMaterial(BlueRubberSurface->Material, MATERIAL_BLUE_RUBBER);

        component_surface* GreenRubberSurface = (component_surface*) RenderCommands->RenderMemory.GetMemory(sizeof(component_surface));
        GreenRubberSurface->Material = (material*) RenderCommands->RenderMemory.GetMemory(sizeof(material));
        SetMaterial(GreenRubberSurface->Material, MATERIAL_GREEN_RUBBER);

        component_surface* RedRubberSurface = (component_surface*) RenderCommands->RenderMemory.GetMemory(sizeof(component_surface));
        RedRubberSurface->Material = (material*) RenderCommands->RenderMemory.GetMemory(sizeof(material));
        SetMaterial(RedRubberSurface->Material, MATERIAL_RED_RUBBER);

        component_surface* WhiteSurface = (component_surface*) RenderCommands->RenderMemory.GetMemory(sizeof(component_surface));
        WhiteSurface->Material = (material*) RenderCommands->RenderMemory.GetMemory(sizeof(material));
        SetMaterial(WhiteSurface->Material, MATERIAL_WHITE);

        component_surface* RedSurface = (component_surface*) RenderCommands->RenderMemory.GetMemory(sizeof(component_surface));
        RedSurface->Material = (material*) RenderCommands->RenderMemory.GetMemory(sizeof(material));
        SetMaterial(RedSurface->Material, MATERIAL_RED);

        component_surface* GreenSurface = (component_surface*) RenderCommands->RenderMemory.GetMemory(sizeof(component_surface));
        GreenSurface->Material = (material*) RenderCommands->RenderMemory.GetMemory(sizeof(material));
        SetMaterial(GreenSurface->Material, MATERIAL_GREEN);

        // Show Origin
        {
          push_buffer_header*  Header = (push_buffer_header*) PushNewHeader( RenderCommands, &PreviousEntry );
          Header->Type = render_buffer_entry_type::PRIMITIVE;
          Header->RenderState = RENDER_STATE_FILL;

          entry_type_primitive* Body = (entry_type_primitive*) RenderCommands->RenderMemory.GetMemory(sizeof(entry_type_primitive));
          Body->Surface = WhiteSurface;
          Body->M = GetTranslationMatrix( V4(0,0,0,1)) * GetScaleMatrix( V4(0.1,0.1,0.1,1));
          Body->TM = M4Identity();
          Body->PrimitiveType = primitive_type::VOXEL;
        }

        // Show CSO
        {
          u32* vi = Vis->Indeces;
          v3* v   = Vis->Vertices;

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
            Body->Surface = BlueRubberSurface;
            Body->PrimitiveType = primitive_type::VOXEL;
          }
        }

        // Show GJK
        if( Vis->ActiveSimplexFrame <(s32) Vis->SimplexCount )
        {
          simplex_index* SI = &Vis->Simplex[Vis->ActiveSimplexFrame];

          // GJK WireFrame
          {
            push_buffer_header* Header = (push_buffer_header*) PushNewHeader( RenderCommands, &PreviousEntry );
            Header->RenderState = RENDER_STATE_WIREFRAME;
            Header->Type = render_buffer_entry_type::INDEXED_BUFFER;
            entry_type_indexed_buffer* Body = (entry_type_indexed_buffer*) RenderCommands->RenderMemory.GetMemory(sizeof(entry_type_indexed_buffer));
            Body->Buffer = Buffer;
            Body->Surface = GreenRubberSurface;
            Body->M  = M4Identity();
            Body->NM = M4Identity();

            Body->ElementStart  = SI->Offset;
            Body->ElementLength = SI->Length;

            switch(SI->Length)
            {
              case 1: Body->DataType = DATA_TYPE_LINE;      break;
              case 2: Body->DataType = DATA_TYPE_LINE;      break;
              case 3: Body->DataType = DATA_TYPE_TRIANGLE;  break;
              case 12: Body->DataType = DATA_TYPE_TRIANGLE; break;
              default: INVALID_CODE_PATH; break;
            }
          }

          // GJK CSO Points
          {
            // Todo: This is inefficient, better option would be either instancing
            // http://www.opengl-tutorial.org/intermediate-tutorials/billboards-particles/particles-instancing/
            // Or fill every quad into one big buffer and draw them with one draw call
            u32* vi = Entity->GjkEpaVisualizerComponent->Indeces;
            v3* v   = Entity->GjkEpaVisualizerComponent->Vertices;
            for(u32 i = SI->Offset; i < SI->Offset + SI->Length; ++i)
            {
              push_buffer_header*  Header = (push_buffer_header*) PushNewHeader( RenderCommands, &PreviousEntry );
              Header->Type = render_buffer_entry_type::PRIMITIVE;
              Header->RenderState = RENDER_STATE_FILL;
              entry_type_primitive* Body = (entry_type_primitive*) RenderCommands->RenderMemory.GetMemory(sizeof(entry_type_primitive));
              v3 Vertex = v[vi[i]];
              Body->M = GetTranslationMatrix(V4(Vertex,1)) * GetScaleMatrix(V4(0.1,0.1,0.1,1));
              Body->TM = M4Identity();
              Body->Surface = GreenRubberSurface;
              Body->PrimitiveType = primitive_type::VOXEL;
            }
          }

          // GJK Closset Point
          {
            push_buffer_header* Header = (push_buffer_header*) PushNewHeader( RenderCommands, &PreviousEntry );
            Header->Type = render_buffer_entry_type::PRIMITIVE;
            Header->RenderState = RENDER_STATE_FILL;

            entry_type_primitive* Body = (entry_type_primitive*) RenderCommands->RenderMemory.GetMemory(sizeof(entry_type_primitive));
            Body->Surface = RedRubberSurface;
            Body->M = GetTranslationMatrix( V4(SI->ClosestPoint,1)) * GetScaleMatrix( V4(0.09,0.12,0.09,1));
            Body->TM = M4Identity();
            Body->PrimitiveType = primitive_type::VOXEL;
          }

        }else if(Vis->ActiveSimplexFrame == (s32)Vis->SimplexCount )
        {
          // Show EPA
          epa_index* EPA = &Vis->EPA[Vis->ActiveEPAFrame];

          {
            push_buffer_header* Header = (push_buffer_header*) PushNewHeader( RenderCommands, &PreviousEntry );
            Header->RenderState = EPA->FillMesh ? RENDER_STATE_FILL : RENDER_STATE_WIREFRAME;
            Header->Type = render_buffer_entry_type::INDEXED_BUFFER;

            entry_type_indexed_buffer* Body = (entry_type_indexed_buffer*) RenderCommands->RenderMemory.GetMemory(sizeof(entry_type_indexed_buffer));
            Body->Buffer = Buffer;
            Body->Surface = BlueRubberSurface;
            Body->M  = M4Identity();
            Body->NM = M4Identity();
            Body->ElementStart  = EPA->MeshOffset;
            Body->ElementLength = EPA->MeshLength;
            Body->DataType = DATA_TYPE_TRIANGLE;
          }
          if(!EPA->FillMesh)
          {
            push_buffer_header* Header = (push_buffer_header*) PushNewHeader( RenderCommands, &PreviousEntry );
            Header->RenderState = RENDER_STATE_FILL;
            Header->Type = render_buffer_entry_type::INDEXED_BUFFER;

            entry_type_indexed_buffer* Body = (entry_type_indexed_buffer*) RenderCommands->RenderMemory.GetMemory(sizeof(entry_type_indexed_buffer));
            Body->Buffer   = Buffer;
            Body->Surface = GreenRubberSurface;
            Body->M  = M4Identity();
            Body->NM = M4Identity();

            Body->ElementStart  = EPA->ClosestFace;
            Body->ElementLength = 3;
            Body->DataType = DATA_TYPE_TRIANGLE;
          }
          {
            push_buffer_header*  Header = (push_buffer_header*) PushNewHeader( RenderCommands, &PreviousEntry );
            Header->Type = render_buffer_entry_type::PRIMITIVE;
            Header->RenderState = RENDER_STATE_FILL;

            entry_type_primitive* Body = (entry_type_primitive*) RenderCommands->RenderMemory.GetMemory(sizeof(entry_type_primitive));
            Body->Surface = RedSurface;
            Body->M = GetTranslationMatrix( V4(EPA->ClosestPointOnFace,1)) * GetScaleMatrix( V4(0.11,0.11,0.11,1));
            Body->TM = M4Identity();
            Body->PrimitiveType = primitive_type::VOXEL;
          }
          {
            push_buffer_header*  Header = (push_buffer_header*) PushNewHeader( RenderCommands, &PreviousEntry );
            Header->Type = render_buffer_entry_type::PRIMITIVE;
            Header->RenderState = RENDER_STATE_FILL;

            entry_type_primitive* Body = (entry_type_primitive*) RenderCommands->RenderMemory.GetMemory(sizeof(entry_type_primitive));
            Body->Surface = GreenSurface;
            Body->M = GetTranslationMatrix( V4(EPA->SupportPoint,1)) * GetScaleMatrix( V4(0.11,0.11,0.11,1));
            Body->TM = M4Identity();
            Body->PrimitiveType = primitive_type::VOXEL;
          }
        }
      }
    }
  }
}