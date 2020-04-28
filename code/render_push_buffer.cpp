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
  RenderGroup->ElementCount++;

  push_buffer_header* NewEntryHeader = PushStruct(&RenderGroup->Arena, push_buffer_header);
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


internal v2
CanonicalToRender(v2 CanonicalPoint, r32 ScreenWidthPx, r32 ScreenHeightPx)
{
  // X: 0 -> -1
  // X: AspectRatio -> 1
  // y: 0 -> -1
  // y: 1 -> 1
  v2 Result = {};
  r32 AspectRatio = ScreenWidthPx / ScreenHeightPx;
  Result.X = 2*CanonicalPoint.X/AspectRatio - 1.f;
  Result.Y = 2*CanonicalPoint.Y - 1.f;
  return Result;
}

void DEBUGPushQuad(render_group* RenderGroup, aabb2f Rect, v4 Color = V4(1,1,1,1))
{
  //TIMED_FUNCTION();
  v2 P0rc = CanonicalToRender(Rect.P0, RenderGroup->ScreenWidth, RenderGroup->ScreenHeight);
  v2 P1rc = CanonicalToRender(Rect.P1, RenderGroup->ScreenWidth, RenderGroup->ScreenHeight);
  Rect = AABB2f(P0rc, P1rc);

  component_surface* Surface = PushStruct(&RenderGroup->Arena, component_surface);
  Surface->Material = PushStruct(&RenderGroup->Arena, material);
  Surface->Material->AmbientColor = Color;

  push_buffer_header*  Header = PushNewHeader( RenderGroup );
  Header->Type = render_buffer_entry_type::PRIMITIVE;
  Header->RenderState = RENDER_STATE_FILL;
  entry_type_primitive* Body = PushStruct(&RenderGroup->Arena, entry_type_primitive);
  Body->PrimitiveType = primitive_type::QUAD;
  Body->Surface = Surface;
  Body->TM = M4Identity();
  r32 Width  = (Rect.P1.X-Rect.P0.X);
  r32 Height = (Rect.P1.Y-Rect.P0.Y);
  r32 X0 = Rect.P0.X + Width/2;
  r32 Y0 = Rect.P0.Y + Height/2;
  Body->M  = GetTranslationMatrix(V4(X0, Y0,0,1)) * GetScaleMatrix(V4(Width, Height,1,0));
}

rect2f DEBUGTextSize(render_group* RenderGroup, c8* String)
{
  stb_font_map* FontMap = &RenderGroup->Assets->STBFontMap;
  rect2f Result = {};

  const r32 Kx = 2.f / RenderGroup->ScreenWidth;
  const r32 Ky = 2.f / RenderGroup->ScreenHeight;

  Result.X = 0;
  Result.W = 0;
  Result.Y = Ky*FontMap->Descent;
  Result.H = Ky*(FontMap->Ascent + FontMap->Descent);

  while (*String != '\0')
  {
    stbtt_bakedchar* CH = &FontMap->CharData[*String-0x20];
    Result.W += Kx*CH->xadvance;
    ++String;
  }

  return Result;
}

component_surface* PushNewSurface(render_group* RenderGroup, bitmap* BitMap, u32 ColorEnum)
{
  component_surface* Result = PushStruct(&RenderGroup->Arena, component_surface);
  Result->Material = PushStruct(&RenderGroup->Arena, material);
  SetMaterial(Result->Material, ColorEnum);
  Result->Material->DiffuseMap = BitMap;
  return Result;
}


void PushTexturedQuad(render_group* RenderGroup, m4 QuadTranslate, m4 TextureTranslate, component_surface* Surface )
{
  // Todo: Find a better API So you don't need this BS all the time
  push_buffer_header* Header = PushNewHeader( RenderGroup );
  Header->Type = render_buffer_entry_type::PRIMITIVE;
  Header->RenderState = RENDER_STATE_FILL;
  entry_type_primitive* Body = PushStruct(&RenderGroup->Arena, entry_type_primitive);
  Body->PrimitiveType = primitive_type::QUAD;
  Body->Surface = Surface;
  Body->TM = TextureTranslate;
  Body->M  = QuadTranslate;
}

void DEBUGTextOutAtPx(r32 xPos, r32 yPos, render_group* RenderGroup, c8* String)
{
  stb_font_map* FontMap = &RenderGroup->Assets->STBFontMap;
  component_surface* BitMapFont = PushNewSurface(RenderGroup, &FontMap->BitMap, MATERIAL_WHITE);
  stbtt_aligned_quad Quad = {};

  const r32 Kx = 1.f / RenderGroup->ScreenWidth;
  const r32 Ky = 1.f / RenderGroup->ScreenHeight;

  const r32 Ks = 1.f / FontMap->BitMap.Width;
  const r32 Kt = 1.f / FontMap->BitMap.Height;

  while (*String != '\0')
  {
    stbtt_bakedchar* CH = &FontMap->CharData[*String-0x20];

    v2 GlyphSize     = V2(Kx*(CH->x1 - CH->x0),  Ky*(CH->y1 - CH->y0));
    v2 GlyphPosition = V2(Kx*Floor(xPos + 0.5f), Ky*Floor(yPos + 0.5f));
    v2 GlyphOffset   = V2(Kx*CH->xoff + GlyphSize.X/2.f, -(Ky*CH->yoff + GlyphSize.Y/2.f));

    xPos += CH->xadvance;

    if(*String != ' ')
    {
      const r32 s0 = CH->x0 * Ks;
      const r32 s1 = CH->x1 * Ks;
      const r32 t0 = CH->y0 * Kt;
      const r32 t1 = CH->y1 * Kt;

      // Todo: Find a better API for scale and translation that doesnt need a full 4x4 matrix multiplication
      const m4 TextureTranslate = GetTranslationMatrix(V4(s0, t1,0,1)) * GetScaleMatrix(V4(s1-s0, t0-t1,1,0));
      v2 RenPos = CanonicalToRender(GlyphPosition, RenderGroup->ScreenWidth, RenderGroup->ScreenHeight);
      v4 GlyphTranslate = V4(RenPos.X + GlyphOffset.X, RenPos.Y + GlyphOffset.Y,0,1);
      v4 GlyphScale = V4(GlyphSize,1,0);

      const m4 QuadTranslate = GetTranslationMatrix(GlyphTranslate) * GetScaleMatrix(GlyphScale);

      PushTexturedQuad(RenderGroup, QuadTranslate, TextureTranslate, BitMapFont );

    }

    ++String;
  }
}

// Todo: Decide on one coordinate system to work with!
void DEBUGTextOutAt(r32 xPos, r32 yPos, render_group* RenderGroup, c8* String)
{
  //v2 P = CanonicalToRender(V2(xPos,yPos), RenderGroup->ScreenWidth, RenderGroup->ScreenHeight);
  r32 X = xPos*GlobalDebugRenderGroup->ScreenWidth;
  r32 Y = yPos*GlobalDebugRenderGroup->ScreenHeight;
  DEBUGTextOutAtPx(X, Y, RenderGroup, String);
}

void DEBUGAddTextSTB(render_group* RenderGroup, c8* String, r32 cornerOffset, r32 LineNumber)
{
  TIMED_FUNCTION();
  stb_font_map* FontMap = &RenderGroup->Assets->STBFontMap;
  r32 xPos = cornerOffset;
  r32 yPos = RenderGroup->ScreenHeight - (LineNumber) * FontMap->FontHeightPx-FontMap->FontHeightPx;
  DEBUGTextOutAtPx(xPos, yPos, RenderGroup, String);
}

void ResetRenderGroup(render_group* RenderGroup)
{
  EndTemporaryMemory(RenderGroup->PushBufferMemory);
  RenderGroup->PushBufferMemory = BeginTemporaryMemory(&RenderGroup->Arena);

  RenderGroup->ProjectionMatrix = M4Identity();
  RenderGroup->ViewMatrix = M4Identity();
  RenderGroup->ElementCount = 0;
  RenderGroup->First = 0;
  RenderGroup->Last = 0;
}

render_group* InitiateRenderGroup(world* World, r32 ScreenWidth, r32 ScreenHeight)
{
  render_group* Result = BootstrapPushStruct(render_group, Arena);
  Result->PushBufferMemory = BeginTemporaryMemory(&Result->Arena);
  Result->Assets = World->Assets;
  Result->ScreenWidth = ScreenWidth;
  Result->ScreenHeight = ScreenHeight;
  ResetRenderGroup(Result);

  Result->Initialized = true;

  return Result;
}


void FillRenderPushBuffer( world* World, render_group* RenderGroup )
{
  TIMED_FUNCTION();

  ResetRenderGroup(RenderGroup);

  // TODO: Make a proper entity library so we can extracl for example ALL Lights efficiently
  //       So we don't have to loop over ALL entitis several times
  for(u32 Index = 0; Index <  World->NrEntities; ++Index )
  {
    entity* Entity = &World->Entities[Index];

    if( Entity->Types & COMPONENT_TYPE_CAMERA )
    {
      RenderGroup->ProjectionMatrix = Entity->CameraComponent->P;
      RenderGroup->ViewMatrix       = Entity->CameraComponent->V;
      break;
    }

    if( Entity->LightComponent && Entity->SpatialComponent )
    {
      push_buffer_header* Header = PushNewHeader( RenderGroup );
      Header->Type = render_buffer_entry_type::LIGHT;

      entry_type_light* Body = PushStruct(&RenderGroup->Arena, entry_type_light);
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
      // Todo: Move this to its own asset system. I have no idea what that should look like
      //       All i Know is that I would like to do something like:
      //       Body->MeshID   = Entity->MeshComponent->ID;
      //       And have the opengl-render code be able to access the Asset library and load
      //       the mesh into the GUP via the ID

      // This also gives each mesh it's own BVO even though they may use the same mesh.
      // (Same mesh gets sent to the gpu multiple times)
      render_buffer* Buffer = PushStruct(&RenderGroup->Arena, render_buffer);
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

      push_buffer_header* Header = PushNewHeader( RenderGroup );
      Header->Type = render_buffer_entry_type::INDEXED_BUFFER;
      Header->RenderState = RENDER_STATE_CULL_BACK | RENDER_STATE_FILL;

      entry_type_indexed_buffer* Body = PushStruct(&RenderGroup->Arena, entry_type_indexed_buffer);
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
      component_surface* WhiteSurface = PushStruct(&RenderGroup->Arena, component_surface);
      WhiteSurface->Material = PushStruct(&RenderGroup->Arena, material);
      SetMaterial(WhiteSurface->Material, MATERIAL_WHITE);

      push_buffer_header* Header = PushNewHeader( RenderGroup );
      Header->Type = render_buffer_entry_type::PRIMITIVE;
      Header->RenderState = RENDER_STATE_FILL;

      entry_type_primitive* Body = PushStruct(&RenderGroup->Arena, entry_type_primitive);
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
      render_buffer* Buffer = PushStruct(&RenderGroup->Arena, render_buffer);
      Buffer->VAO  = &Entity->ColliderComponent->Mesh->VAO;
      Buffer->VBO  = &Entity->ColliderComponent->Mesh->VBO;
      Buffer->Fill =  *Buffer->VAO==0;
      Buffer->nvi  =  Entity->ColliderComponent->Mesh->nvi;
      Buffer->nv   =  Entity->ColliderComponent->Mesh->nv;
      Buffer->v    =  Entity->ColliderComponent->Mesh->v;
      Buffer->vi   =  Entity->ColliderComponent->Mesh->vi;

      component_surface* JadeSurface = PushStruct(&RenderGroup->Arena, component_surface);
      JadeSurface->Material = PushStruct(&RenderGroup->Arena, material);
      SetMaterial(JadeSurface->Material, MATERIAL_JADE);

      component_surface* GreenSurface = PushStruct(&RenderGroup->Arena, component_surface);
      GreenSurface->Material = PushStruct(&RenderGroup->Arena, material);
      SetMaterial(GreenSurface->Material, MATERIAL_GREEN);

      {
        push_buffer_header* Header = PushNewHeader( RenderGroup );
        Header->Type = render_buffer_entry_type::INDEXED_BUFFER;
        Header->RenderState = RENDER_STATE_WIREFRAME;

        entry_type_indexed_buffer* Body = PushStruct(&RenderGroup->Arena, entry_type_indexed_buffer);
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

        render_buffer* Buffer = PushStruct(&RenderGroup->Arena, render_buffer);
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

        component_surface* BlueRubberSurface = PushStruct(&RenderGroup->Arena, component_surface);
        BlueRubberSurface->Material = PushStruct(&RenderGroup->Arena, material);
        SetMaterial(BlueRubberSurface->Material, MATERIAL_BLUE_RUBBER);

        component_surface* GreenRubberSurface = PushStruct(&RenderGroup->Arena, component_surface);
        GreenRubberSurface->Material = PushStruct(&RenderGroup->Arena, material);
        SetMaterial(GreenRubberSurface->Material, MATERIAL_GREEN_RUBBER);

        component_surface* RedRubberSurface = PushStruct(&RenderGroup->Arena, component_surface);
        RedRubberSurface->Material = PushStruct(&RenderGroup->Arena, material);
        SetMaterial(RedRubberSurface->Material, MATERIAL_RED_RUBBER);

        component_surface* WhiteSurface = PushStruct(&RenderGroup->Arena, component_surface);
        WhiteSurface->Material = PushStruct(&RenderGroup->Arena, material);
        SetMaterial(WhiteSurface->Material, MATERIAL_WHITE);

        component_surface* RedSurface = PushStruct(&RenderGroup->Arena, component_surface);
        RedSurface->Material = PushStruct(&RenderGroup->Arena, material);
        SetMaterial(RedSurface->Material, MATERIAL_RED);

        component_surface* GreenSurface = PushStruct(&RenderGroup->Arena, component_surface);
        GreenSurface->Material = PushStruct(&RenderGroup->Arena, material);
        SetMaterial(GreenSurface->Material, MATERIAL_GREEN);

        // Show Origin
        {
          push_buffer_header* Header = PushNewHeader( RenderGroup );
          Header->Type = render_buffer_entry_type::PRIMITIVE;
          Header->RenderState = RENDER_STATE_FILL;

          entry_type_primitive* Body = PushStruct(&RenderGroup->Arena, entry_type_primitive);
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

            push_buffer_header* Header = PushNewHeader( RenderGroup );
            Header->Type = render_buffer_entry_type::PRIMITIVE;
            Header->RenderState = RENDER_STATE_FILL;

            entry_type_primitive* Body = PushStruct(&RenderGroup->Arena, entry_type_primitive);
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
            push_buffer_header* Header = PushNewHeader( RenderGroup );
            Header->RenderState = RENDER_STATE_WIREFRAME;
            Header->Type = render_buffer_entry_type::INDEXED_BUFFER;
            entry_type_indexed_buffer* Body = PushStruct(&RenderGroup->Arena, entry_type_indexed_buffer);
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
              push_buffer_header* Header = PushNewHeader( RenderGroup );
              Header->Type = render_buffer_entry_type::PRIMITIVE;
              Header->RenderState = RENDER_STATE_FILL;
              entry_type_primitive* Body = PushStruct(&RenderGroup->Arena, entry_type_primitive);
              v3 Vertex = v[vi[i]];
              Body->M = GetTranslationMatrix(V4(Vertex,1)) * GetScaleMatrix(V4(0.1,0.1,0.1,1));
              Body->TM = M4Identity();
              Body->Surface = GreenRubberSurface;
              Body->PrimitiveType = primitive_type::VOXEL;
            }
          }

          // GJK Closset Point
          {
            push_buffer_header* Header = PushNewHeader( RenderGroup );
            Header->Type = render_buffer_entry_type::PRIMITIVE;
            Header->RenderState = RENDER_STATE_FILL;

            entry_type_primitive* Body = PushStruct(&RenderGroup->Arena, entry_type_primitive);
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
            push_buffer_header* Header = PushNewHeader( RenderGroup );
            Header->RenderState = EPA->FillMesh ? RENDER_STATE_FILL : RENDER_STATE_WIREFRAME;
            Header->Type = render_buffer_entry_type::INDEXED_BUFFER;

            entry_type_indexed_buffer* Body = PushStruct(&RenderGroup->Arena, entry_type_indexed_buffer);
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
            push_buffer_header* Header = PushNewHeader( RenderGroup );
            Header->RenderState = RENDER_STATE_FILL;
            Header->Type = render_buffer_entry_type::INDEXED_BUFFER;

            entry_type_indexed_buffer* Body = PushStruct(&RenderGroup->Arena, entry_type_indexed_buffer);
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

            entry_type_primitive* Body = PushStruct(&RenderGroup->Arena, entry_type_primitive);
            Body->Surface = RedSurface;
            Body->M = GetTranslationMatrix( V4(EPA->ClosestPointOnFace,1)) * GetScaleMatrix( V4(0.11,0.11,0.11,1));
            Body->TM = M4Identity();
            Body->PrimitiveType = primitive_type::VOXEL;
          }
          {
            push_buffer_header*  Header = (push_buffer_header*) PushNewHeader( RenderGroup );
            Header->Type = render_buffer_entry_type::PRIMITIVE;
            Header->RenderState = RENDER_STATE_FILL;

            entry_type_primitive* Body = PushStruct(&RenderGroup->Arena, entry_type_primitive);
            Body->Surface = GreenSurface;
            Body->M = GetTranslationMatrix( V4(EPA->SupportPoint,1)) * GetScaleMatrix( V4(0.11,0.11,0.11,1));
            Body->TM = M4Identity();
            Body->PrimitiveType = primitive_type::VOXEL;
          }
        }
      }
    }
  }

  component_surface* GreenSurface = PushStruct(&RenderGroup->Arena, component_surface);
  GreenSurface->Material = PushStruct(&RenderGroup->Arena, material);
  SetMaterial(GreenSurface->Material, MATERIAL_GREEN);
  component_surface* RedSurface = PushStruct(&RenderGroup->Arena, component_surface);
  RedSurface->Material = PushStruct(&RenderGroup->Arena, material);
  SetMaterial(RedSurface->Material, MATERIAL_RED);

  component_surface* BlueSurface = PushStruct(&RenderGroup->Arena, component_surface);
  BlueSurface->Material = PushStruct(&RenderGroup->Arena, material);
  SetMaterial(BlueSurface->Material, MATERIAL_BLUE);
  component_surface* WhiteSurface = PushStruct(&RenderGroup->Arena, component_surface);
  WhiteSurface->Material = PushStruct(&RenderGroup->Arena, material);
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
        push_buffer_header* Header = PushNewHeader( RenderGroup );
        Header->Type = render_buffer_entry_type::PRIMITIVE;
        Header->RenderState = RENDER_STATE_POINTS;
        entry_type_primitive* Body = PushStruct(&RenderGroup->Arena, entry_type_primitive);
        Body->M = GetTranslationMatrix( V4(Manifold->Contacts[j].A_ContactWorldSpace,1));
        Body->TM = M4Identity();
        Body->PrimitiveType = primitive_type::POINT;
        Body->Surface = RedSurface;
      }
      {
        push_buffer_header* Header = PushNewHeader( RenderGroup );
        Header->Type = render_buffer_entry_type::PRIMITIVE;
        Header->RenderState = RENDER_STATE_POINTS;
        entry_type_primitive* Body = PushStruct(&RenderGroup->Arena, entry_type_primitive);
        Body->M = GetTranslationMatrix( V4(Manifold->Contacts[j].B_ContactWorldSpace,1));
        Body->TM = M4Identity();
        Body->PrimitiveType = primitive_type::POINT;
        Body->Surface = GreenSurface;
      }
      #endif
      {
        push_buffer_header* Header = PushNewHeader( RenderGroup );
        Header->Type = render_buffer_entry_type::PRIMITIVE;
        Header->RenderState = RENDER_STATE_POINTS;
        entry_type_primitive* Body = PushStruct(&RenderGroup->Arena, entry_type_primitive);
        Body->M = GetModelMatrix( A->SpatialComponent ) * GetTranslationMatrix(Manifold->Contacts[j].A_ContactModelSpace);
        //Body->M = GetTranslationMatrix( Manifold->Contacts[j].A_ContactWorldSpace);
        Body->TM = M4Identity();
        Body->PrimitiveType = primitive_type::POINT;
        Body->Surface = BlueSurface;
      }
      {
        push_buffer_header* Header = PushNewHeader( RenderGroup );
        Header->Type = render_buffer_entry_type::PRIMITIVE;
        Header->RenderState = RENDER_STATE_POINTS;
        entry_type_primitive* Body = PushStruct(&RenderGroup->Arena, entry_type_primitive);
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