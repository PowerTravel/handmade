#include "render_push_buffer.h"
#include "component_camera.h"
#include "entity_components.h"
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

void DEBUGPushQuad(render_group* RenderGroup, rect2f QuadRect, rect2f TextureRect, v4 Color, u32 TextureIndex)
{
  const m4 TextureTranslate = GetTranslationMatrix(V4(TextureRect.X,TextureRect.Y,0,1));
  const m4 TextureScale     = GetScaleMatrix(V4(TextureRect.W, TextureRect.H,1,0));
  const m4 QuadTranslate    = GetTranslationMatrix(V4(QuadRect.X, QuadRect.Y,0,1));
  const m4 QuadScale        = GetScaleMatrix(V4(QuadRect.W, QuadRect.H,1,0));

  push_buffer_header* Header = PushNewHeader( RenderGroup );
  Header->Type = render_buffer_entry_type::OVERLAY_QUAD;
  Header->RenderState = RENDER_STATE_FILL;

  entry_type_overlay_quad* Body = PushStruct(&RenderGroup->Arena, entry_type_overlay_quad);

  Body->ObjectIndex = GetAssetIndex(RenderGroup->AssetManager, asset_type::OBJECT, "quad");
  Body->Colour = Color;
  Body->TextureIndex = TextureIndex;
  Body->M  = QuadTranslate * QuadScale;
  Body->TM = TextureTranslate * TextureScale;
}

void DEBUGPushText(render_group* RenderGroup, rect2f QuadRect, rect2f TextureRect, v4 Color)
{
  push_buffer_header* Header = PushNewHeader( RenderGroup );
  Header->Type = render_buffer_entry_type::TEXT;
  Header->RenderState = RENDER_STATE_FILL;
  
  entry_type_text* Body = PushStruct(&RenderGroup->Arena, entry_type_text);
  Body->QuadRect  = QuadRect;
  Body->UVRect  = TextureRect;
  Body->Colour = Color;
}

rect2f DEBUGTextSize(r32 x, r32 y, render_group* RenderGroup, c8* String)
{
  stb_font_map* FontMap = &GlobalGameState->AssetManager->FontMap;

  const r32 ScreenScaleFactor = 1.f / RenderGroup->ScreenHeight;

  r32 Width = 0;
  while (*String != '\0')
  {
    stbtt_bakedchar* CH = &FontMap->CharData[*String-0x20];
    Width += CH->xadvance;
    ++String;
  }
  rect2f Result = {};
  Result.X = x;
  Result.Y = y+ScreenScaleFactor*FontMap->Descent;
  Result.H = ScreenScaleFactor*FontMap->FontHeightPx;
  Result.W = ScreenScaleFactor*Width;
  return Result;
}


// The thing about STB is that their bit map has its origin in the top-left
// Our standard is that the origin is in the bottom right
inline internal rect2f
GetSTBBitMapTextureCoords(stbtt_bakedchar* CH, r32 WidthScale, r32 HeightScale)
{
  // This transforms from stbtt coordinates to Texture Coordinates
  // stbtt Bitmap Coordinates:                      Screen Canonical Coodinates
  // [0,0](top left):[PixelX, PixelY](bot right) -> [0,0](bot left):[1,1](top right)
  const r32 s0 = CH->x0 * WidthScale;
  const r32 s1 = CH->x1 * WidthScale;
  const r32 Width  = s1-s0;

  const r32 t0 = CH->y1 * HeightScale;
  const r32 t1 = CH->y0 * HeightScale;
  const r32 Height = t1-t0;

  rect2f Result = Rect2f(s0, t0, Width, Height);
  return Result;
}

// The thing about STB is that their bit map has its origin in the top-left
// Our standard is that the origin is in the bottom right
// The output is the translation needed to get a Quad properly centered with the
// base-point at x0, y0
inline internal rect2f
GetSTBGlyphRect(r32 xPosPx, r32 yPosPx, stbtt_bakedchar* CH )
{
  const r32 GlyphWidth   =  (r32)(CH->x1 - CH->x0); // Width of the symbol
  const r32 GlyphHeight  =  (r32)(CH->y1 - CH->y0); // Height of the symbol
  const r32 GlyphOffsetX =  CH->xoff;               // Distance from Left to BasepointX
  const r32 GlyphOffsetY = -CH->yoff;               // Distance from Top to BasepointY

  const r32 X = Floor(xPosPx + 0.5f) + GlyphWidth*0.5f + GlyphOffsetX;
  const r32 Y = Floor(yPosPx + 0.5f) - GlyphHeight*0.5f + GlyphOffsetY;
  rect2f Result = Rect2f(X,Y, GlyphWidth, GlyphHeight);
  return Result;
}

void DEBUGTextOutAt(r32 CanPosX, r32 CanPosY, render_group* RenderGroup, c8* String)
{
  r32 PixelPosX = CanPosX*RenderGroup->ScreenHeight;
  r32 PixelPosY = CanPosY*RenderGroup->ScreenHeight;
  game_asset_manager* AssetManager =  GlobalGameState->AssetManager;
  stb_font_map* FontMap = &AssetManager->FontMap;

  bitmap* BitMap = GetBitmapFromIndex(AssetManager, FontMap->BitmapIndex);
  stbtt_aligned_quad Quad  = {};

  const r32 ScreenScaleFactor = 1.f / RenderGroup->ScreenHeight;

  const r32 Ks = 1.f / BitMap->Width;
  const r32 Kt = 1.f / BitMap->Height;

  while (*String != '\0')
  {
    stbtt_bakedchar* CH = &FontMap->CharData[*String-0x20];
    if(*String != ' ')
    {
      rect2f TextureRect = GetSTBBitMapTextureCoords(CH, Ks, Kt);
      rect2f GlyphOffset = GetSTBGlyphRect(PixelPosX,PixelPosY,CH);
      GlyphOffset.X *= ScreenScaleFactor;
      GlyphOffset.Y *= ScreenScaleFactor;
      GlyphOffset.W *= ScreenScaleFactor;
      GlyphOffset.H *= ScreenScaleFactor;
      DEBUGPushText(RenderGroup, GlyphOffset, TextureRect, V4(1,1,1,1));
    }
    PixelPosX += CH->xadvance;
    ++String;
  }
}

void DEBUGAddTextSTB(c8* String, r32 LineNumber)
{
  TIMED_FUNCTION();
  render_group* RenderGroup = GlobalDebugRenderGroup;
  stb_font_map* FontMap = &GlobalGameState->AssetManager->FontMap;
  r32 CanPosX = 1/100.f;
  r32 CanPosY = 1 - ((LineNumber+1) * FontMap->Ascent - LineNumber*FontMap->Descent)/RenderGroup->ScreenHeight;
  DEBUGTextOutAt(CanPosX, CanPosY, RenderGroup, String);
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

render_group* InitiateRenderGroup(game_state* GameState, r32 ScreenWidth, r32 ScreenHeight)
{
  render_group* Result = BootstrapPushStruct(render_group, Arena);
  Result->PushBufferMemory = BeginTemporaryMemory(&Result->Arena);
  Result->AssetManager = GameState->AssetManager;
  Result->ScreenWidth = ScreenWidth;
  Result->ScreenHeight = ScreenHeight;
  ResetRenderGroup(Result);

  Result->Initialized = true;

  return Result;
}


internal inline void
PushLine(render_group* RenderGroup, v3 Start, v3 End, r32 LineThickness, u32 MaterialIndex)
{
  push_buffer_header* Header = PushNewHeader( RenderGroup );
  Header->Type = render_buffer_entry_type::LINE;
  Header->RenderState = RENDER_STATE_CULL_BACK | RENDER_STATE_FILL;;
  entry_type_line* Body = PushStruct(&RenderGroup->Arena, entry_type_line);
  Body->Start = Start;
  Body->End = End;
  Body->LineThickness = LineThickness;
  // TODO: Add setting like this to make the line be drawn with the same width on screen
  //       no matter how far away it is from the camera.
  // Body->ConstantWidth = false;
  Body->MaterialIndex = MaterialIndex;
}

internal inline void
PushBoxFrame(render_group* RenderGroup, m4 M, aabb3f AABB, r32 LineThickness, u32 MaterialIndex)
{
  v3 P[8] = {};;
  GetAABBVertices(&AABB, P);
  for(u32 i = 0; i < ArrayCount(P); ++i)
  {
    P[i] = V3(M*V4(P[i],1));
  }

  // Negative Z
  PushLine(RenderGroup, P[0], P[1], LineThickness, MaterialIndex);
  PushLine(RenderGroup, P[1], P[2], LineThickness, MaterialIndex);
  PushLine(RenderGroup, P[2], P[3], LineThickness, MaterialIndex);
  PushLine(RenderGroup, P[3], P[0], LineThickness, MaterialIndex);
  // Positive Z
  PushLine(RenderGroup, P[4], P[5], LineThickness, MaterialIndex);
  PushLine(RenderGroup, P[5], P[6], LineThickness, MaterialIndex);
  PushLine(RenderGroup, P[6], P[7], LineThickness, MaterialIndex);
  PushLine(RenderGroup, P[7], P[4], LineThickness, MaterialIndex);

  // Joining Lines
  PushLine(RenderGroup, P[0], P[4], LineThickness, MaterialIndex);
  PushLine(RenderGroup, P[1], P[5], LineThickness, MaterialIndex);
  PushLine(RenderGroup, P[2], P[6], LineThickness, MaterialIndex);
  PushLine(RenderGroup, P[3], P[7], LineThickness, MaterialIndex);

}

void FillRenderPushBuffer(world* World, render_group* RenderGroup )
{
  TIMED_FUNCTION();
  game_asset_manager* AssetManager = GlobalGameState->AssetManager;
  ResetRenderGroup(RenderGroup);
  entity_manager* EM = GlobalGameState->EntityManager;
  game_asset_manager* AM = GlobalGameState->AssetManager;

  {
    ScopedTransaction(EM);
    component_result* ComponentList = GetComponentsOfType(EM, COMPONENT_FLAG_CAMERA);
    while(Next(EM, ComponentList))
    {
      component_camera* Camera = (component_camera*) GetComponent(EM, ComponentList, COMPONENT_FLAG_CAMERA);
      RenderGroup->ProjectionMatrix = Camera->P;
      RenderGroup->ViewMatrix       = Camera->V;
    }
  }

  {
    ScopedTransaction(EM);
    component_result* ComponentList = GetComponentsOfType(EM, COMPONENT_FLAG_LIGHT | COMPONENT_FLAG_SPATIAL);
    while(Next(EM, ComponentList))
    {
      component_light* Light = (component_light*) GetComponent(EM, ComponentList, COMPONENT_FLAG_LIGHT);
      component_spatial* Spatial = (component_spatial*) GetComponent(EM, ComponentList, COMPONENT_FLAG_SPATIAL);
      Assert(Spatial);

      push_buffer_header* Header = PushNewHeader( RenderGroup );
      Header->Type = render_buffer_entry_type::LIGHT;

      entry_type_light* Body = PushStruct(&RenderGroup->Arena, entry_type_light);
      Body->Color  = Light->Color;
      Body->M      = GetModelMatrix(Spatial);
    }
  }

  {
    ScopedTransaction(EM);
    component_result* ComponentList = GetComponentsOfType(EM, COMPONENT_FLAG_RENDER | COMPONENT_FLAG_SPATIAL);
    while(Next(EM, ComponentList))
    {
      component_spatial* Spatial = (component_spatial*) GetComponent(EM, ComponentList, COMPONENT_FLAG_SPATIAL );
      component_render* Render = (component_render*) GetComponent(EM, ComponentList, COMPONENT_FLAG_RENDER );

      push_buffer_header* Header = PushNewHeader( RenderGroup );
      Header->Type = render_buffer_entry_type::RENDER_ASSET;
      Header->RenderState = RENDER_STATE_CULL_BACK | RENDER_STATE_FILL;
      entry_type_render_asset* Body = PushStruct(&RenderGroup->Arena, entry_type_render_asset);
      Body->AssetHandle = Render->AssetHandle;
      Body->M  = GetModelMatrix(Spatial);
      Body->NM = Transpose(RigidInverse(Body->M));
      Body->TM = M4Identity();

      component_sprite_animation* SpriteAnimation = (component_sprite_animation*) GetComponent(EM, ComponentList, COMPONENT_FLAG_SPRITE_ANIMATION );
      if(SpriteAnimation)
      {
        Body->TM = SpriteAnimation->ActiveSeries->Get();
      }
    }
  }

#if SHOW_COLLIDER
  {
    ScopedTransaction(EM);
    component_result* ComponentList = GetComponentsOfType(EM, COMPONENT_FLAG_COLLIDER);
    while(Next(EM, ComponentList))
    {
      component_spatial* Spatial = (component_spatial*) GetComponent(EM, ComponentList, COMPONENT_FLAG_SPATIAL);
      component_collider* Collider = (component_collider*) GetComponent(EM, ComponentList, COMPONENT_FLAG_COLLIDER);
      m4 M = GetModelMatrix(Spatial);
      aabb3f AABB = Collider->AABB;
      r32 LineThickness = 0.03;
      u32 MaterialIndex = GetAssetIndex(AM, asset_type::MATERIAL, "jade");
      PushBoxFrame(RenderGroup, M, AABB, LineThickness, MaterialIndex);
    }
  }
#endif

#if SHOW_COLLISION_POINTS
  contact_manifold* Manifold = World->ContactManifolds->FirstManifold;
  while(Manifold)
  {
    //TODO: Turn manifold into an entity
    Assert(Manifold->ContactCount);
    for( u32 j = 0; j <Manifold->ContactCount; ++j)
    {
      {
        push_buffer_header* Header = PushNewHeader( RenderGroup );
        Header->Type = render_buffer_entry_type::RENDER_ASSET;
        Header->RenderState = RENDER_STATE_FILL;

        u32 TempHandle = GetTemporaryAssetHandle(AM);
        SetAsset(AM, asset_type::OBJECT, "voxel", TempHandle);
        SetAsset(AM, asset_type::MATERIAL, "blue", TempHandle);

        entry_type_render_asset* Body = PushStruct(&RenderGroup->Arena, entry_type_render_asset);
        Body->AssetHandle = TempHandle;
        component_spatial* Spatial = (component_spatial*) GetComponent(EM, Manifold->EntityIDA, COMPONENT_FLAG_SPATIAL);
        const m4 Rotation = GetRotationMatrix(Spatial->Rotation);
        const m4 Translation = GetTranslationMatrix(Spatial->Position);
        const m4 Scale = GetScaleMatrix(V4(0.1,0.1,0.1,1));
        #if 1
        Body->M = GetTranslationMatrix( V4(Manifold->Contacts[j].A_ContactWorldSpace,1))*Scale;
        #else
        // This is not working for some reason
        Body->M  = Translation*GetTranslationMatrix(Manifold->Contacts[j].A_ContactModelSpace)*Rotation*Scale;
        #endif
        Body->NM = Transpose(RigidInverse(Body->M));
      }
      {
        push_buffer_header* Header = PushNewHeader( RenderGroup );
        Header->Type = render_buffer_entry_type::RENDER_ASSET;
        Header->RenderState = RENDER_STATE_FILL;

        u32 TempHandle = GetTemporaryAssetHandle(AM);
        SetAsset(AM, asset_type::OBJECT, "voxel", TempHandle);
        SetAsset(AM, asset_type::MATERIAL, "white", TempHandle);

        entry_type_render_asset* Body = PushStruct(&RenderGroup->Arena, entry_type_render_asset);
        Body->AssetHandle = TempHandle;

        component_spatial* Spatial = (component_spatial*) GetComponent(EM, Manifold->EntityIDB, COMPONENT_FLAG_SPATIAL);
        const m4 Rotation = GetRotationMatrix(Spatial->Rotation);
        const m4 Translation = GetTranslationMatrix(Spatial->Position);
        const m4 Scale = GetScaleMatrix(V4(0.1,0.1,0.1,1));
        #if 1
        Body->M = GetTranslationMatrix( V4(Manifold->Contacts[j].B_ContactWorldSpace,1))*Scale;
        #else
        Body->M  =  Translation*GetTranslationMatrix(Manifold->Contacts[j].B_ContactModelSpace)*Rotation*Scale;
        #endif
        Body->NM = Transpose(RigidInverse(Body->M));
      }
    }
    Manifold = Manifold->Next;
  }
#endif

#if SHOW_AABB_TREE
  aabb3f* AABBTree;
  u32 Count = GetAABBList(&World->BroadPhaseTree, &AABBTree);
  for(u32 Idx = 0; Idx < Count; ++Idx)
  {
    r32 LineThickness = 0.03;
    u32 MaterialIndex = GetAssetIndex(AM, asset_type::MATERIAL, "jade");
    PushBoxFrame(RenderGroup, M4Identity(), *AABBTree++, LineThickness, MaterialIndex);
  }
#endif

}