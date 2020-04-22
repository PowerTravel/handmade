#include "render_push_buffer.h"
#include "component_camera.h"
#include "component_surface.h"
#include "component_light.h"
#include "component_spatial.h"
#include "component_collider.h"
#include "component_sprite_animation.h"
#include "gjk_epa_visualizer.h"
#include "component_mesh.h"
#include "epa_collision_data.h"

push_buffer_header* PushNewHeader(render_group* RenderGroup)
{
  //TIMED_FUNCTION();
  utils::push_buffer* Buffer = &RenderGroup->Buffer;
  RenderGroup->ElementCount++;

  push_buffer_header* NewEntryHeader = (push_buffer_header*) Buffer->GetMemory(sizeof(push_buffer_header));
  Assert( NewEntryHeader );
  NewEntryHeader->Next = 0;

  if( !RenderGroup->First )
  {
    RenderGroup->First = NewEntryHeader;
    RenderGroup->Last  = NewEntryHeader;
  }else{
    RenderGroup->Last->Next = NewEntryHeader;
    RenderGroup->Last = NewEntryHeader;
  }
  return NewEntryHeader;
}

internal inline r32
LinearTransform(const r32 OutputMin, const r32 OutputMax, const r32 InputMin, const r32 InputMax, const r32 Input)
{
  const r32 M = OutputMin;
  const r32 K = (InputMax - InputMin) / (OutputMax - OutputMin);
  const r32 Result = K * Input + M;
  return Result;
}

void DEBUGPushQuad(render_group* DebugRenderGroup, aabb2f Rect, v4 Color = V4(1,1,1,1))
{
  //TIMED_FUNCTION();
  component_surface* Surface = (component_surface*) DebugRenderGroup->Buffer.GetMemory(sizeof(component_surface));
  Surface->Material = (material*) DebugRenderGroup->Buffer.GetMemory(sizeof(material));
  Surface->Material->AmbientColor = Color;

  // Todo: Find a better API So you don't need this BS all the time
  push_buffer_header*  Header = (push_buffer_header*) PushNewHeader( DebugRenderGroup );
  Header->Type = render_buffer_entry_type::PRIMITIVE;
  Header->RenderState = RENDER_STATE_FILL;
  entry_type_primitive* Body = (entry_type_primitive*) DebugRenderGroup->Buffer.GetMemory(sizeof(entry_type_primitive));
  Body->PrimitiveType = primitive_type::QUAD;
  Body->Surface = Surface;
  Body->TM = M4Identity();
  r32 Width  = (Rect.P1.X-Rect.P0.X);
  r32 Height = (Rect.P1.Y-Rect.P0.Y);
  r32 X0  = Rect.P0.X + Width/2;
  r32 Y0 = Rect.P0.Y + Height/2;
  Body->M  = GetTranslationMatrix(V4(X0, Y0,0,1)) * GetScaleMatrix(V4(Width, Height,1,0));
}

void DEBUGTextOutAtPx(r32 xPos, r32 yPos, render_group* DebugRenderGroup, c8* String)
{
  stb_font_map* FontMap = &DebugRenderGroup->Assets->STBFontMap;

  component_surface* BitMapFont = (component_surface*) DebugRenderGroup->Buffer.GetMemory(sizeof(component_surface));
  BitMapFont->Material = (material*) DebugRenderGroup->Buffer.GetMemory(sizeof(material));
  SetMaterial(BitMapFont->Material, MATERIAL_WHITE);
  BitMapFont->Material->DiffuseMap = &FontMap->BitMap;
  stbtt_aligned_quad Quad = {};

  const r32 M = -0.5f;
  const r32 Kx = 1.f / DebugRenderGroup->ScreenWidth;
  const r32 Ky = 1.f / DebugRenderGroup->ScreenHeight;

  const r32 Ks = 1.f / FontMap->BitMap.Width;
  const r32 Kt = 1.f / FontMap->BitMap.Height;

  while (*String != '\0')
  {
    stbtt_bakedchar* CH = &FontMap->CharData[*String-0x20];
    const r32 x0u = Kx * (Floor(xPos + 0.5f)) + M;
    const r32 x1u = Kx * (Floor(xPos + 0.5f) + (CH->x1 - CH->x0)) + M;
    const r32 y0u = Ky * (Floor(yPos + 0.5f)) + M;
    const r32 y1u = Ky * (Floor(yPos + 0.5f) + (CH->y1 - CH->y0)) + M;

    const r32 GlyphWidth  = (x1u - x0u);
    const r32 GlyphHeight = (y1u - y0u);

    const r32 xoff = Kx * CH->xoff + GlyphWidth/2.f;
    const r32 yoff = Ky * CH->yoff + GlyphHeight/2.f;

    xPos += CH->xadvance;

    if(*String != ' ')
    {
      const r32 s0 = CH->x0 * Ks;
      const r32 s1 = CH->x1 * Ks;
      const r32 t0 = CH->y0 * Kt;
      const r32 t1 = CH->y1 * Kt;

      // Todo: Find a better API for scale and translation that doesnt need a full 4x4 matrix multiplication
      const m4 TextureTranslate = GetTranslationMatrix(V4(s0, t1,0,1)) * GetScaleMatrix(V4(s1-s0, t0-t1,1,0));
      const m4 QuadTranslate    = GetTranslationMatrix(V4(x0u + xoff - 0.5f, y0u - yoff + 0.5f,0,1)) * GetScaleMatrix(V4(GlyphWidth, GlyphHeight,1,0));

      // Todo: Find a better API So you don't need this BS all the time
      push_buffer_header*  Header = (push_buffer_header*) PushNewHeader( DebugRenderGroup );
      Header->Type = render_buffer_entry_type::PRIMITIVE;
      Header->RenderState = RENDER_STATE_FILL;
      entry_type_primitive* Body = (entry_type_primitive*) DebugRenderGroup->Buffer.GetMemory(sizeof(entry_type_primitive));
      Body->PrimitiveType = primitive_type::QUAD;
      Body->Surface = BitMapFont;
      Body->TM = TextureTranslate;
      Body->M  = QuadTranslate;
    }

    ++String;
  }
}

// Todo: Decide on one coordinate system to work with!
void DEBUGTextOutAt(r32 xPos, r32 yPos, render_group* DebugRenderGroup, c8* String)
{
  r32 X = ((xPos+1))*GlobalDebugRenderGroup->ScreenWidth;
  r32 Y = ((yPos))*GlobalDebugRenderGroup->ScreenHeight;
  DEBUGTextOutAtPx(X, Y, DebugRenderGroup, String);
}

void DEBUGAddTextSTB(render_group* DebugRenderGroup, c8* String, r32 cornerOffset, r32 LineNumber)
{
  TIMED_FUNCTION();

  stb_font_map* FontMap = &DebugRenderGroup->Assets->STBFontMap;
  r32 xPos = cornerOffset;
  r32 yPos = DebugRenderGroup->ScreenHeight - (LineNumber) * FontMap->FontHeightPx-FontMap->FontHeightPx;

  DEBUGTextOutAtPx(xPos, yPos,  DebugRenderGroup, String);
}


void FillRenderPushBuffer( world* World, game_render_commands* RenderCommands )
{
  Assert(RenderCommands);

  TIMED_FUNCTION();
  render_group* RenderGroup = &RenderCommands->MainRenderGroup;

  RenderGroup->Buffer.Clear();
  RenderGroup->ElementCount = 0;
  RenderGroup->First = 0;

  // TODO: Make a proper entity library so we can extracl for example ALL Lights efficiently
  //       So we don't have to loop over ALL entitis several times

  // Then push all the lights and camera
  for(u32 Index = 0; Index <  World->NrEntities; ++Index )
  {
    entity* Entity = &World->Entities[Index];

    if( Entity->Types & COMPONENT_TYPE_CAMERA )
    {
      RenderGroup->ProjectionMatrix = Entity->CameraComponent->P;
      RenderGroup->ViewMatrix       = Entity->CameraComponent->V;
      break;
    }

    if( (Entity->Types & COMPONENT_TYPE_LIGHT) &&
      (Entity->Types & COMPONENT_TYPE_SPATIAL) )
    {
      push_buffer_header* Header = PushNewHeader( RenderGroup );
      Header->Type = render_buffer_entry_type::LIGHT;

      entry_type_light* Body = (entry_type_light*) RenderGroup->Buffer.GetMemory(sizeof(entry_type_light));
      Body->Color  = Entity->LightComponent->Color;
      Body->M      = GetModelMatrix(Entity->SpatialComponent);
    }
  }

  for(u32 Index = 0; Index <  World->NrEntities; ++Index )
  {
    entity* Entity = &World->Entities[Index];

    if( (Entity->Types & COMPONENT_TYPE_MESH ) &&
        (Entity->Types & COMPONENT_TYPE_SPATIAL) )
    {
      render_buffer* Buffer = (render_buffer*) RenderGroup->Buffer.GetMemory(sizeof(render_buffer));
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

      push_buffer_header* Header = (push_buffer_header*) PushNewHeader( RenderGroup );
      Header->Type = render_buffer_entry_type::INDEXED_BUFFER;
      Header->RenderState = RENDER_STATE_CULL_BACK | RENDER_STATE_FILL;

      entry_type_indexed_buffer* Body = (entry_type_indexed_buffer*) RenderGroup->Buffer.GetMemory(sizeof(entry_type_indexed_buffer));
      Body->Buffer   = Buffer;
      Body->DataType = DATA_TYPE_TRIANGLE;
      Body->Surface  = Entity->SurfaceComponent;
      Body->M        = GetModelMatrix(Entity->SpatialComponent);
      Body->NM       = Transpose(RigidInverse(Body->M));
      Body->ElementStart  = 0;
      Body->ElementLength = Buffer->nvi;
    }

    if( Entity->Types & COMPONENT_TYPE_SPRITE_ANIMATION )
    {
      component_surface* WhiteSurface = (component_surface*) RenderGroup->Buffer.GetMemory(sizeof(component_surface));
      WhiteSurface->Material = (material*) RenderGroup->Buffer.GetMemory(sizeof(material));
      SetMaterial(WhiteSurface->Material, MATERIAL_WHITE);

      push_buffer_header* Header = (push_buffer_header*) PushNewHeader( RenderGroup );
      Header->Type = render_buffer_entry_type::PRIMITIVE;
      Header->RenderState = RENDER_STATE_FILL;

      entry_type_primitive* Body = (entry_type_primitive*) RenderGroup->Buffer.GetMemory(sizeof(entry_type_primitive));
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
        Body->M = GetModelMatrix(Entity->SpatialComponent);
      }
    }

    // Here we wanna do a wire frame and collision points
    if( Entity->Types & COMPONENT_TYPE_COLLIDER  )
    {
      render_buffer* Buffer = (render_buffer*) RenderGroup->Buffer.GetMemory(sizeof(render_buffer));
      Buffer->VAO  = &Entity->ColliderComponent->Mesh->VAO;
      Buffer->VBO  = &Entity->ColliderComponent->Mesh->VBO;
      Buffer->Fill =  *Buffer->VAO==0;
      Buffer->nvi  =  Entity->ColliderComponent->Mesh->nvi;
      Buffer->nv   =  Entity->ColliderComponent->Mesh->nv;
      Buffer->v    =  Entity->ColliderComponent->Mesh->v;
      Buffer->vi   =  Entity->ColliderComponent->Mesh->vi;

      component_surface* JadeSurface = (component_surface*) RenderGroup->Buffer.GetMemory(sizeof(component_surface));
      JadeSurface->Material = (material*) RenderGroup->Buffer.GetMemory(sizeof(material));
      SetMaterial(JadeSurface->Material, MATERIAL_JADE);

      component_surface* GreenSurface = (component_surface*) RenderGroup->Buffer.GetMemory(sizeof(component_surface));
      GreenSurface->Material = (material*) RenderGroup->Buffer.GetMemory(sizeof(material));
      SetMaterial(GreenSurface->Material, MATERIAL_GREEN);

      {
        push_buffer_header* Header = (push_buffer_header*) PushNewHeader( RenderGroup );
        Header->Type = render_buffer_entry_type::INDEXED_BUFFER;
        Header->RenderState = RENDER_STATE_WIREFRAME;

        entry_type_indexed_buffer* Body = (entry_type_indexed_buffer*) RenderGroup->Buffer.GetMemory(sizeof(entry_type_indexed_buffer));
        Body->Buffer = Buffer;
        Body->DataType = DATA_TYPE_TRIANGLE;
        Body->Surface = JadeSurface;
        Body->M = GetModelMatrix(Entity->SpatialComponent);
        Body->NM = Transpose(RigidInverse(Body->M));

        Body->ElementStart  = 0;
        Body->ElementLength = Buffer->nvi;
      }
    }

    if( GlobalVis )
    {
      gjk_epa_visualizer* Vis = GlobalVis;
      if(Vis->Playback && Vis->IndexCount>0)
      {

        render_buffer* Buffer = (render_buffer*) RenderGroup->Buffer.GetMemory(sizeof(render_buffer));
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

        component_surface* BlueRubberSurface = (component_surface*) RenderGroup->Buffer.GetMemory(sizeof(component_surface));
        BlueRubberSurface->Material = (material*) RenderGroup->Buffer.GetMemory(sizeof(material));
        SetMaterial(BlueRubberSurface->Material, MATERIAL_BLUE_RUBBER);

        component_surface* GreenRubberSurface = (component_surface*) RenderGroup->Buffer.GetMemory(sizeof(component_surface));
        GreenRubberSurface->Material = (material*) RenderGroup->Buffer.GetMemory(sizeof(material));
        SetMaterial(GreenRubberSurface->Material, MATERIAL_GREEN_RUBBER);

        component_surface* RedRubberSurface = (component_surface*) RenderGroup->Buffer.GetMemory(sizeof(component_surface));
        RedRubberSurface->Material = (material*) RenderGroup->Buffer.GetMemory(sizeof(material));
        SetMaterial(RedRubberSurface->Material, MATERIAL_RED_RUBBER);

        component_surface* WhiteSurface = (component_surface*) RenderGroup->Buffer.GetMemory(sizeof(component_surface));
        WhiteSurface->Material = (material*) RenderGroup->Buffer.GetMemory(sizeof(material));
        SetMaterial(WhiteSurface->Material, MATERIAL_WHITE);

        component_surface* RedSurface = (component_surface*) RenderGroup->Buffer.GetMemory(sizeof(component_surface));
        RedSurface->Material = (material*) RenderGroup->Buffer.GetMemory(sizeof(material));
        SetMaterial(RedSurface->Material, MATERIAL_RED);

        component_surface* GreenSurface = (component_surface*) RenderGroup->Buffer.GetMemory(sizeof(component_surface));
        GreenSurface->Material = (material*) RenderGroup->Buffer.GetMemory(sizeof(material));
        SetMaterial(GreenSurface->Material, MATERIAL_GREEN);

        // Show Origin
        {
          push_buffer_header*  Header = (push_buffer_header*) PushNewHeader( RenderGroup );
          Header->Type = render_buffer_entry_type::PRIMITIVE;
          Header->RenderState = RENDER_STATE_FILL;

          entry_type_primitive* Body = (entry_type_primitive*) RenderGroup->Buffer.GetMemory(sizeof(entry_type_primitive));
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
            push_buffer_header*  Header = (push_buffer_header*) PushNewHeader( RenderGroup );
            Header->Type = render_buffer_entry_type::PRIMITIVE;
            Header->RenderState = RENDER_STATE_FILL;

            entry_type_primitive* Body = (entry_type_primitive*) RenderGroup->Buffer.GetMemory(sizeof(entry_type_primitive));
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
            push_buffer_header* Header = (push_buffer_header*) PushNewHeader( RenderGroup );
            Header->RenderState = RENDER_STATE_WIREFRAME;
            Header->Type = render_buffer_entry_type::INDEXED_BUFFER;
            entry_type_indexed_buffer* Body = (entry_type_indexed_buffer*) RenderGroup->Buffer.GetMemory(sizeof(entry_type_indexed_buffer));
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
            u32* vi = GlobalVis->Indeces;
            v3* v   = GlobalVis->Vertices;
            for(u32 i = SI->Offset; i < SI->Offset + SI->Length; ++i)
            {
              push_buffer_header*  Header = (push_buffer_header*) PushNewHeader( RenderGroup );
              Header->Type = render_buffer_entry_type::PRIMITIVE;
              Header->RenderState = RENDER_STATE_FILL;
              entry_type_primitive* Body = (entry_type_primitive*) RenderGroup->Buffer.GetMemory(sizeof(entry_type_primitive));
              v3 Vertex = v[vi[i]];
              Body->M = GetTranslationMatrix(V4(Vertex,1)) * GetScaleMatrix(V4(0.1,0.1,0.1,1));
              Body->TM = M4Identity();
              Body->Surface = GreenRubberSurface;
              Body->PrimitiveType = primitive_type::VOXEL;
            }
          }

          // GJK Closset Point
          {
            push_buffer_header* Header = (push_buffer_header*) PushNewHeader( RenderGroup );
            Header->Type = render_buffer_entry_type::PRIMITIVE;
            Header->RenderState = RENDER_STATE_FILL;

            entry_type_primitive* Body = (entry_type_primitive*) RenderGroup->Buffer.GetMemory(sizeof(entry_type_primitive));
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
            push_buffer_header* Header = (push_buffer_header*) PushNewHeader( RenderGroup );
            Header->RenderState = EPA->FillMesh ? RENDER_STATE_FILL : RENDER_STATE_WIREFRAME;
            Header->Type = render_buffer_entry_type::INDEXED_BUFFER;

            entry_type_indexed_buffer* Body = (entry_type_indexed_buffer*) RenderGroup->Buffer.GetMemory(sizeof(entry_type_indexed_buffer));
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
            push_buffer_header* Header = (push_buffer_header*) PushNewHeader( RenderGroup );
            Header->RenderState = RENDER_STATE_FILL;
            Header->Type = render_buffer_entry_type::INDEXED_BUFFER;

            entry_type_indexed_buffer* Body = (entry_type_indexed_buffer*) RenderGroup->Buffer.GetMemory(sizeof(entry_type_indexed_buffer));
            Body->Buffer   = Buffer;
            Body->Surface = GreenRubberSurface;
            Body->M  = M4Identity();
            Body->NM = M4Identity();

            Body->ElementStart  = EPA->ClosestFace;
            Body->ElementLength = 3;
            Body->DataType = DATA_TYPE_TRIANGLE;
          }
          {
            push_buffer_header*  Header = (push_buffer_header*) PushNewHeader( RenderGroup );
            Header->Type = render_buffer_entry_type::PRIMITIVE;
            Header->RenderState = RENDER_STATE_FILL;

            entry_type_primitive* Body = (entry_type_primitive*) RenderGroup->Buffer.GetMemory(sizeof(entry_type_primitive));
            Body->Surface = RedSurface;
            Body->M = GetTranslationMatrix( V4(EPA->ClosestPointOnFace,1)) * GetScaleMatrix( V4(0.11,0.11,0.11,1));
            Body->TM = M4Identity();
            Body->PrimitiveType = primitive_type::VOXEL;
          }
          {
            push_buffer_header*  Header = (push_buffer_header*) PushNewHeader( RenderGroup );
            Header->Type = render_buffer_entry_type::PRIMITIVE;
            Header->RenderState = RENDER_STATE_FILL;

            entry_type_primitive* Body = (entry_type_primitive*) RenderGroup->Buffer.GetMemory(sizeof(entry_type_primitive));
            Body->Surface = GreenSurface;
            Body->M = GetTranslationMatrix( V4(EPA->SupportPoint,1)) * GetScaleMatrix( V4(0.11,0.11,0.11,1));
            Body->TM = M4Identity();
            Body->PrimitiveType = primitive_type::VOXEL;
          }
        }
      }
    }
  }

  component_surface* GreenSurface = (component_surface*) RenderGroup->Buffer.GetMemory(sizeof(component_surface));
  GreenSurface->Material = (material*) RenderGroup->Buffer.GetMemory(sizeof(material));
  SetMaterial(GreenSurface->Material, MATERIAL_GREEN);
  component_surface* RedSurface = (component_surface*) RenderGroup->Buffer.GetMemory(sizeof(component_surface));
  RedSurface->Material = (material*) RenderGroup->Buffer.GetMemory(sizeof(material));
  SetMaterial(RedSurface->Material, MATERIAL_RED);

  component_surface* BlueSurface = (component_surface*) RenderGroup->Buffer.GetMemory(sizeof(component_surface));
  BlueSurface->Material = (material*) RenderGroup->Buffer.GetMemory(sizeof(material));
  SetMaterial(BlueSurface->Material, MATERIAL_BLUE);
  component_surface* WhiteSurface = (component_surface*) RenderGroup->Buffer.GetMemory(sizeof(component_surface));
  WhiteSurface->Material = (material*) RenderGroup->Buffer.GetMemory(sizeof(material));
  SetMaterial(WhiteSurface->Material, MATERIAL_WHITE);

  contact_manifold* Manifold = World->FirstContactManifold;
  while(Manifold)
  {
    Assert(Manifold->ContactCount);
    entity* A = Manifold->A;
    entity* B = Manifold->B;
    for( u32 j = 0; j <Manifold->ContactCount; ++j)
    {
      #if 0
      {
        push_buffer_header* Header = (push_buffer_header*) PushNewHeader( RenderGroup );
        Header->Type = render_buffer_entry_type::PRIMITIVE;
        Header->RenderState = RENDER_STATE_POINTS;
        entry_type_primitive* Body = (entry_type_primitive*) RenderGroup->Buffer.GetMemory(sizeof(entry_type_primitive));
        Body->M = GetTranslationMatrix( V4(Manifold->Contacts[j].A_ContactWorldSpace,1));
        Body->TM = M4Identity();
        Body->PrimitiveType = primitive_type::POINT;
        Body->Surface = RedSurface;
      }
      {
        push_buffer_header* Header = (push_buffer_header*) PushNewHeader( RenderGroup );
        Header->Type = render_buffer_entry_type::PRIMITIVE;
        Header->RenderState = RENDER_STATE_POINTS;
        entry_type_primitive* Body = (entry_type_primitive*) RenderGroup->Buffer.GetMemory(sizeof(entry_type_primitive));
        Body->M = GetTranslationMatrix( V4(Manifold->Contacts[j].B_ContactWorldSpace,1));
        Body->TM = M4Identity();
        Body->PrimitiveType = primitive_type::POINT;
        Body->Surface = GreenSurface;
      }
      #endif
      {
        push_buffer_header* Header = (push_buffer_header*) PushNewHeader( RenderGroup );
        Header->Type = render_buffer_entry_type::PRIMITIVE;
        Header->RenderState = RENDER_STATE_POINTS;
        entry_type_primitive* Body = (entry_type_primitive*) RenderGroup->Buffer.GetMemory(sizeof(entry_type_primitive));
        Body->M = GetModelMatrix( A->SpatialComponent ) * GetTranslationMatrix(Manifold->Contacts[j].A_ContactModelSpace);
        //Body->M = GetTranslationMatrix( Manifold->Contacts[j].A_ContactWorldSpace);
        Body->TM = M4Identity();
        Body->PrimitiveType = primitive_type::POINT;
        Body->Surface = BlueSurface;
      }
      {
        push_buffer_header* Header = (push_buffer_header*) PushNewHeader( RenderGroup );
        Header->Type = render_buffer_entry_type::PRIMITIVE;
        Header->RenderState = RENDER_STATE_POINTS;
        entry_type_primitive* Body = (entry_type_primitive*) RenderGroup->Buffer.GetMemory(sizeof(entry_type_primitive));
        Body->M = GetModelMatrix( B->SpatialComponent ) * GetTranslationMatrix(Manifold->Contacts[j].B_ContactModelSpace);
        //Body->M = GetTranslationMatrix( Manifold->Contacts[j].B_ContactWorldSpace);
        Body->TM = M4Identity();
        Body->PrimitiveType = primitive_type::POINT;
        Body->Surface = WhiteSurface;
      }
    }
    Manifold = Manifold->Next;
  }

}