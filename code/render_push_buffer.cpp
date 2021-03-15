#include "render_push_buffer.h"
#include "component_camera.h"
#include "entity_components.h"
#include "epa_collision_data.h"

push_buffer_header* PushNewHeader(render_group* RenderGroup, render_buffer_entry_type Type, u32 RenderState, u32 SortKey = 0)
{
  RenderGroup->ElementCount++;
  push_buffer_header* NewEntryHeader = PushStruct(&RenderGroup->Arena, push_buffer_header);
  NewEntryHeader->SortKey = SortKey;
  NewEntryHeader->Next = 0;

  if(!RenderGroup->First)
  {
    RenderGroup->First = NewEntryHeader;
    RenderGroup->Last  = NewEntryHeader;
  }else{
    RenderGroup->Last->Next = NewEntryHeader;
    RenderGroup->Last = NewEntryHeader;
  }
  NewEntryHeader->Type = Type;
  NewEntryHeader->RenderState = RenderState;
  RenderGroup->BufferCounts[(u32) Type]++;

  return NewEntryHeader;
}

void PushNewRenderLevel(render_group* RenderGroup)
{
  push_buffer_header* Header = PushNewHeader(RenderGroup, render_buffer_entry_type::NEW_LEVEL, RENDER_STATE_NONE);
}

void PushTexturedOverlayQuad(rect2f QuadRect, rect2f TextureRect, bitmap_handle Handle)
{
  render_group* RenderGroup = GlobalGameState->RenderCommands->OverlayGroup;
  push_buffer_header* Header = PushNewHeader(RenderGroup, render_buffer_entry_type::OVERLAY_TEXTURED_QUAD, RENDER_STATE_FILL);
  entry_type_overlay_textured_quad* Body = PushStruct(&RenderGroup->Arena, entry_type_overlay_textured_quad);
  Body->TextureRect = TextureRect;
  QuadRect.X += QuadRect.W/2.f;
  QuadRect.Y += QuadRect.H/2.f;
  Body->QuadRect = QuadRect;
  Body->Handle = Handle;
}

void PushOverlayQuad(rect2f QuadRect, v4 Color)
{
  render_group* RenderGroup = GlobalGameState->RenderCommands->OverlayGroup;
  push_buffer_header* Header = PushNewHeader(RenderGroup, render_buffer_entry_type::OVERLAY_QUAD, RENDER_STATE_FILL);
  entry_type_overlay_quad* Body = PushStruct(&RenderGroup->Arena, entry_type_overlay_quad);
  Body->Colour = Color;
  Body->QuadRect = QuadRect;
}

void PushOverlayText(rect2f QuadRect, rect2f TextureRect, v4 Color, bitmap_handle BitmapHandle)
{
  render_group* RenderGroup = GlobalGameState->RenderCommands->OverlayGroup;
  push_buffer_header* Header = PushNewHeader( RenderGroup, render_buffer_entry_type::TEXT, RENDER_STATE_FILL);
  entry_type_text* Body = PushStruct(&RenderGroup->Arena, entry_type_text);
  Body->BitmapHandle = BitmapHandle;
  Body->QuadRect = QuadRect;
  Body->UVRect = TextureRect;
  Body->Colour = Color;
}


r32 GetTextLineHeightSize(u32 FontSize)
{
  stb_font_map* FontMap = GetFontMap(GlobalGameState->AssetManager, FontSize);
  game_window_size WindowSize = GameGetWindowSize();
  r32 Result = FontMap->FontHeightPx/ (r32) WindowSize.HeightPx;
  return Result;
}

r32 GetTextWidth(const c8* String, u32 FontSize)
{
  stb_font_map* FontMap = GetFontMap(GlobalGameState->AssetManager, FontSize);
  
  game_window_size WindowSize = GameGetWindowSize();
  const r32 PixelSize = 1.f / WindowSize.HeightPx;

  r32 Width = 0;
  while (*String != '\0')
  {
    stbtt_bakedchar* CH = &FontMap->CharData[*String-0x20];
    Width += CH->xadvance;
    ++String;
  }
  r32 Result = PixelSize*Width;
  return Result;  
}

rect2f GetTextSize(r32 x, r32 y, const c8* String, u32 FontSize)
{
  stb_font_map* FontMap = GetFontMap(GlobalGameState->AssetManager, FontSize);
  
  game_window_size WindowSize = GameGetWindowSize();
  const r32 ScreenScaleFactor = 1.f / WindowSize.HeightPx;

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

void PushTextAt(r32 CanPosX, r32 CanPosY, const c8* String, u32 FontSize, v4 Color)
{
  game_window_size WindowSize = GameGetWindowSize();
  r32 PixelPosX = Floor(CanPosX*WindowSize.HeightPx);
  r32 PixelPosY = Floor(CanPosY*WindowSize.HeightPx);
  game_asset_manager* AssetManager =  GlobalGameState->AssetManager;
  stb_font_map* FontMap = GetFontMap(GlobalGameState->AssetManager, FontSize);

  bitmap* BitMap = GetAsset(AssetManager, FontMap->BitmapHandle);
  stbtt_aligned_quad Quad  = {};

  const r32 ScreenScaleFactor = 1.f / WindowSize.HeightPx;

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
      PushOverlayText(GlyphOffset, TextureRect,Color, FontMap->BitmapHandle);
    }
    PixelPosX += CH->xadvance;
    ++String;
  }
}

render_group* InitiateRenderGroup()
{
  render_group* Result = BootstrapPushStruct(render_group, Arena);
  Result->PushBufferMemory = BeginTemporaryMemory(&Result->Arena);

  ResetRenderGroup(Result);
  return Result;
}

internal inline void
PushLine(render_group* RenderGroup, v3 Start, v3 End, v3 CameraPosition, r32 LineThickness, c8* MaterialName)
{
  v3 xAxis = End-Start;
  v3 cameraDir = CameraPosition - Start;

  m4 RotMat = GetRotationMatrix_X_ZHint(xAxis, cameraDir);

  r32 Length = Norm(End-Start);

  m4 ScaleMat = M4Identity();
  ScaleMat.E[0] = Length;
  ScaleMat.E[5] = LineThickness;

  v3 MidPoint = (Start + End) / 2;
  m4 TransMat = M4Identity();
  TransMat.E[3]  = MidPoint.X;
  TransMat.E[7]  = MidPoint.Y;
  TransMat.E[11] = MidPoint.Z;

  push_buffer_header* Header = PushNewHeader( RenderGroup, render_buffer_entry_type::RENDER_ASSET, RENDER_STATE_CULL_BACK | RENDER_STATE_FILL );
  entry_type_render_asset* Body = PushStruct(&RenderGroup->Arena, entry_type_render_asset);

  GetHandle(GlobalGameState->AssetManager, "quad", &Body->Object);
  GetHandle(GlobalGameState->AssetManager, MaterialName, &Body->Material);
  GetHandle(GlobalGameState->AssetManager, "null", &Body->Bitmap);
  Body->M = TransMat*RotMat*ScaleMat;
  Body->NM = Transpose(RigidInverse(Body->M));
  Body->TM = M4Identity();
}

internal inline void
PushBoxFrame(render_group* RenderGroup, m4 M, aabb3f AABB, v3 CameraPosition, r32 LineThickness, c8* MaterialName)
{
  v3 P[8] = {};
  GetAABBVertices(&AABB, P);
  for(u32 i = 0; i < ArrayCount(P); ++i)
  {
    P[i] = V3(M*V4(P[i],1));
  }

  // Negative Z
  PushLine(RenderGroup, P[0], P[1], CameraPosition, LineThickness, MaterialName);
  PushLine(RenderGroup, P[1], P[2], CameraPosition, LineThickness, MaterialName);
  PushLine(RenderGroup, P[2], P[3], CameraPosition, LineThickness, MaterialName);
  PushLine(RenderGroup, P[3], P[0], CameraPosition, LineThickness, MaterialName);
  // Positive Z
  PushLine(RenderGroup, P[4], P[5], CameraPosition, LineThickness, MaterialName);
  PushLine(RenderGroup, P[5], P[6], CameraPosition, LineThickness, MaterialName);
  PushLine(RenderGroup, P[6], P[7], CameraPosition, LineThickness, MaterialName);
  PushLine(RenderGroup, P[7], P[4], CameraPosition, LineThickness, MaterialName);
  // Joining Lines
  PushLine(RenderGroup, P[0], P[4], CameraPosition, LineThickness, MaterialName);
  PushLine(RenderGroup, P[1], P[5], CameraPosition, LineThickness, MaterialName);
  PushLine(RenderGroup, P[2], P[6], CameraPosition, LineThickness, MaterialName);
  PushLine(RenderGroup, P[3], P[7], CameraPosition, LineThickness, MaterialName);

}

void FillRenderPushBuffer(world* World)
{
  render_group* RenderGroup = GlobalGameState->RenderCommands->RenderGroup;
  render_group* LightGroup = GlobalGameState->RenderCommands->LightsGroup;

  TIMED_FUNCTION();
  game_asset_manager* AssetManager = GlobalGameState->AssetManager;
  ResetRenderGroup(RenderGroup);
  entity_manager* EM = GlobalGameState->EntityManager;
  game_asset_manager* AM = GlobalGameState->AssetManager;

  v3 CameraPosition = {};
  {
    ScopedTransaction(EM);
    component_result* ComponentList = GetComponentsOfType(EM, COMPONENT_FLAG_CAMERA);
    while(Next(EM, ComponentList))
    {
      component_camera* Camera = (component_camera*) GetComponent(EM, ComponentList, COMPONENT_FLAG_CAMERA);
      RenderGroup->ProjectionMatrix = Camera->P;
      RenderGroup->ViewMatrix       = Camera->V;
      CameraPosition = V3(Column(RigidInverse(Camera->V),3));
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

      push_buffer_header* Header = PushNewHeader( LightGroup, render_buffer_entry_type::LIGHT, 0 );
      entry_type_light* Body = PushStruct(&LightGroup->Arena, entry_type_light);
      Body->Color  = Light->Color;
      Body->M      = Spatial->ModelMatrix;
    }
  }

  {
    ScopedTransaction(EM);
    component_result* ComponentList = GetComponentsOfType(EM, COMPONENT_FLAG_RENDER | COMPONENT_FLAG_SPATIAL);
    while(Next(EM, ComponentList))
    {
      component_spatial* Spatial = (component_spatial*) GetComponent(EM, ComponentList, COMPONENT_FLAG_SPATIAL );
      component_render* Render = (component_render*) GetComponent(EM, ComponentList, COMPONENT_FLAG_RENDER );

      push_buffer_header* Header = PushNewHeader( RenderGroup, render_buffer_entry_type::RENDER_ASSET, RENDER_STATE_CULL_BACK | RENDER_STATE_FILL);
      entry_type_render_asset* Body = PushStruct(&RenderGroup->Arena, entry_type_render_asset);
      Body->Object = Render->Object;
      Body->Bitmap = Render->Bitmap;
      Body->Material = Render->Material;
      Body->M  = Spatial->ModelMatrix;
      Body->NM = Transpose(RigidInverse(Body->M));
      Body->TM = M4Identity();

      component_sprite_animation* SpriteAnimation = (component_sprite_animation*) GetComponent(EM, ComponentList, COMPONENT_FLAG_SPRITE_ANIMATION );
      if(SpriteAnimation)
      {
        Body->TM = SpriteAnimation->ActiveSeries->Get();
      }
    }
  }

  { 
    if( World->CastedRay.Hit)
    {
      PushLine(RenderGroup, World->CastedRay.RayOrigin, World->CastedRay.Intersection, CameraPosition, 0.2, "green");
      PushLine(RenderGroup, World->CastedRay.Intersection, World->CastedRay.Intersection + World->CastedRay.HitNormal * 100.f , CameraPosition, 0.2, "yellow_plastic");
    }else{
      v3 O = World->CastedRay.RayOrigin;
      v3 E = World->CastedRay.RayOrigin + 100.f * World->CastedRay.RayDirection;
      PushLine(RenderGroup, O, E, CameraPosition, 0.1, "red");
    }
  }

  {
    ScopedTransaction(EM);
    component_result* ComponentList = GetComponentsOfType(EM, COMPONENT_FLAG_DYNAMICS);
    while(Next(EM, ComponentList))
    {
      component_spatial* VelSpatial = (component_spatial*) GetComponent(EM, ComponentList, COMPONENT_FLAG_SPATIAL);
      component_dynamics* VelDynamics = (component_dynamics*) GetComponent(EM, ComponentList, COMPONENT_FLAG_DYNAMICS);
      {
        v4 Q = GetRotation( V3(1,0,0), VelDynamics->LinearVelocity);
        const m4 ObjRotation = GetRotationMatrix(Q);
        r32 Speed = Norm(VelDynamics->LinearVelocity);
        
        m4 M  = GetScaleMatrix(V4(Speed, 0.05, 0.05, 1));
        Translate(V4(Speed/2,0,0,1), M);
        M = ObjRotation * M;
        Translate(V4(VelSpatial->Position) + V4(0,2,0,1), M);

        push_buffer_header* VelocityHeader = PushNewHeader( RenderGroup, render_buffer_entry_type::RENDER_ASSET, RENDER_STATE_FILL | RENDER_STATE_CULL_BACK);
        entry_type_render_asset* VelocityBody = PushStruct(&RenderGroup->Arena, entry_type_render_asset);
        GetHandle(AM,"voxel", &VelocityBody->Object);
        GetHandle(AM, "red", &VelocityBody->Material);

        VelocityBody->M = M;
        VelocityBody->NM = Transpose(RigidInverse(VelocityBody->M));
      }

      {
        r32 Speed = Norm(VelDynamics->AngularVelocity);
        v4 Q = GetRotation( V3(1,0,0), VelDynamics->AngularVelocity);
        #if USE_ANGULAR_VEL_OBJECT_SPACE
        const m4 ObjRotation =  GetRotationMatrix(QuaternionMultiplication(Q,VelSpatial->Rotation));
        #else
        const m4 ObjRotation = GetRotationMatrix(Q);
        #endif

        m4 M  = GetScaleMatrix(V4(Speed, 0.05, 0.05, 1));
        Translate(V4(Speed/2,0,0,1), M);
        M = ObjRotation * M;
        Translate(V4(VelSpatial->Position) + V4(0,2,0,1), M);

        push_buffer_header* WorldArrowHeader = PushNewHeader( RenderGroup, render_buffer_entry_type::RENDER_ASSET, RENDER_STATE_FILL | RENDER_STATE_CULL_BACK);
        entry_type_render_asset* WoldArrowBody = PushStruct(&RenderGroup->Arena, entry_type_render_asset);
        GetHandle(AM,"voxel", &WoldArrowBody->Object);
        GetHandle(AM, "blue", &WoldArrowBody->Material);
        WoldArrowBody->M = M;
        WoldArrowBody->NM = Transpose(RigidInverse(WoldArrowBody->M));
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
      m4 M = Spatial->ModelMatrix;
      aabb3f AABB = Collider->AABB;
      r32 LineThickness = 0.03;

      PushBoxFrame(RenderGroup, M, AABB, CameraPosition, LineThickness, "jade");
    }
  }
#endif

#if SHOW_COLLISION_POINTS
  contact_manifold* Manifold = World->ContactManifolds->FirstManifold;
  while(Manifold)
  {
    //TODO: Turn manifold into an entity
    contact_data* Contact = Manifold->Contacts.First();
    while(Contact)
    {
      {
        push_buffer_header* Header = PushNewHeader( RenderGroup, render_buffer_entry_type::RENDER_ASSET, RENDER_STATE_FILL | RENDER_STATE_CULL_BACK );


        entry_type_render_asset* Body = PushStruct(&RenderGroup->Arena, entry_type_render_asset);
        GetHandle(AM,"voxel", &Body->Object);
        GetHandle(AM, "blue", &Body->Material);
        component_spatial* Spatial = (component_spatial*) GetComponent(EM, Manifold->EntityIDA, COMPONENT_FLAG_SPATIAL);
        const m4 Rotation = GetRotationMatrix(Spatial->Rotation);
        const m4 Translation = GetTranslationMatrix(Spatial->Position);
        const m4 Scale = GetScaleMatrix(V4(0.1,0.1,0.1,1));
        Body->M = GetTranslationMatrix( V4(Contact->A_ContactWorldSpace,1))*Scale;
        Body->NM = Transpose(RigidInverse(Body->M));
      }
      {
        push_buffer_header* Header = PushNewHeader( RenderGroup, render_buffer_entry_type::RENDER_ASSET, RENDER_STATE_FILL | RENDER_STATE_CULL_BACK );

        entry_type_render_asset* Body = PushStruct(&RenderGroup->Arena, entry_type_render_asset);
        GetHandle(AM, "voxel", &Body->Object);
        GetHandle(AM, "white", &Body->Material);

        component_spatial* Spatial = (component_spatial*) GetComponent(EM, Manifold->EntityIDB, COMPONENT_FLAG_SPATIAL);
        const m4 Rotation = GetRotationMatrix(Spatial->Rotation);
        const m4 Translation = GetTranslationMatrix(Spatial->Position);
        const m4 Scale = GetScaleMatrix(V4(0.1,0.1,0.1,1));
        Body->M = GetTranslationMatrix( V4(Contact->B_ContactWorldSpace,1))*Scale;
        Body->NM = Transpose(RigidInverse(Body->M));
      }
      Contact = Manifold->Contacts.Next(Contact);
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
    PushBoxFrame(RenderGroup, M4Identity(), *AABBTree++, CameraPosition, LineThickness, "ruby");
  }
#endif

}