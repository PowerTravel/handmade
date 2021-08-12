/*

  Random stuff

    Record video with ffmpg: ffmpg -f gdigrab -r 30 -i title="handmade" -y kekbur.gif

  TODO:
  BUGS:
    - Make things work with arbitrary screensize (we have desired aspect ratio hardcoded evertwhere, (Add as global variable?))
    - Fix game-loop-memory allocation bug.
      Pressing L activates game-looping. When the loop reloads win32-memory-allocation fails. No idea why, It seems
      to have been like this for a long long time. Before debug-system.
    - Collision detection breaks when we have several boxes
    - Keyboard Pushed/Released doesn't reliably trigger.
    - Having alot of boxes causes the debug collation to grow to big.
      Either filter out functions that are too numerous or make the block-thing grow dynamically.
      Making it super big also breaks rendering as the render buffer grows suspiciously big.
      We throw away frame-blocks in the profiler that are smaller than one pixel, so it should not grow too big
  Rendering:
    - Use TriangleStrips
    - Nice Lines (https://blog.mapbox.com/drawing-antialiased-lines-with-opengl-8766f34192dc)
    - Proper Diffuse Textures, BumpMapping
    - MipMapping
    - Depth Peeling and Anti Ailising
    - Environment skybox
    - Global Illumination
      - Investigate raytracing
      - Shadows
  Assets:
    - Separately built Asset file with multithreaded stream-loading. (Maybe using ASSIMP)
    - Add a tag-system. I want to get a random asset with a specific tag.
  Debug:
    - Add Memory usage window
    - Continue on caseys Stream, he gets rid of the __COUNT__ macro gimmic which we also use at some point. (ep 193)
  User Interface
    - Item selection w mouse
    - Convert the debug-function function list into a native scrollable list window
  Physics:
    - Persistent AABB-tree
    - Ray intersection test in AABB-tree
    - Make gjk-epa sequential impulse work
      - Make a super simple box - plane narrow-phase detection algorithm to help verify the sequential impulse algorithm.
    - Make a 2D Collision system (Broad phase, narrow phase, collision resolution) (for fun)
    - Make a point - point / point-line / point-plane / line-line / line-plane / plane-plane Collision detection
    - Try algorithms other than sequential impulse.
  General:
    - A good looking demo scene
    - Scene serialization
    - Come up with a "demo-game" to implement using your engine.
    - Editor interface
  Sound:
    - Add loading and playback of sound file
  Intrinsics:
    Round( r32 Real32 )
    Roof( r32 Real32 )
    Floor( r32 Real32 )
*/

#include "handmade.h"

#include "assets.h"
#include "tiles_spritesheet.h"

#if HANDMADE_INTERNAL
game_memory* DebugGlobalMemory = 0;
#endif

#include "math/aabb.cpp"
#include "handmade_tile.cpp"
#include "obj_loader.cpp"
#include "render_push_buffer.cpp"
#include "dynamic_aabb_tree.cpp"
#include "gjk_narrow_phase.cpp"
#include "epa_collision_data.cpp"
#include "halfedge_mesh.cpp"

#include "entity_components.cpp"
#include "system_controller.cpp"
#include "system_sprite_animation.cpp"
#include "system_collider.cpp"
#include "system_spatial.cpp"
//#include "Spatial2.cpp"
#include "system_camera.cpp"
#include "assets.cpp"
#include "asset_loading.cpp"
#include "menu_interface.cpp"

#include "debug.h"

internal void
GameOutputSound(game_sound_output_buffer* SoundBuffer, int ToneHz)
{

  local_persist r32 tSine = 0.f;

  s16 ToneVolume = 2000;
  int SamplesPerPeriod = SoundBuffer->SamplesPerSecond / ToneHz;

  r32 twopi = (r32)(2.0*Pi32);

  s16 *SampleOut = SoundBuffer->Samples;
  for (int SampleIndex = 0;
  SampleIndex < SoundBuffer->SampleCount;
    ++SampleIndex)
  {

#if 0
    r32 SineValue = Sin(tSine);
    s16 SampleValue = (s16)(SineValue * ToneVolume);

    tSine += twopi / (r32)SamplesPerPeriod;
    if (tSine > twopi)
    {
      tSine -= twopi;
    }
#else
    s16 SampleValue = 0;
#endif
    *SampleOut++ = SampleValue; // Left Speker
    *SampleOut++ = SampleValue; // Rright speaker
  }
}

void CalculateInertialTensor(component_spatial* S, component_collider* C, component_dynamics* D)
{
  collider_mesh Mesh = GetColliderMesh(GlobalGameState->AssetManager, C->Object);

  // For now all objects are just cubes
  v3 Dim = C->AABB.P1 - C->AABB.P0;
  r32 Ix = (1/12.f) * D->Mass * (Dim.Y*Dim.Y + Dim.Z*Dim.Z);
  r32 Iy = (1/12.f) * D->Mass * (Dim.X*Dim.X + Dim.Z*Dim.Z);
  r32 Iz = (1/12.f) * D->Mass * (Dim.X*Dim.X + Dim.Y*Dim.Y);

  D->I =  M3(Ix, 0, 0,
           0, Iy, 0,
           0, 0, Iz);

  D->I_inv = M3(1/Ix,0,0,
                 0,1/Iy,0,
                 0,0,1/Iz);
}

world* CreateWorld( u32 MaxNrManifolds )
{
  world* World = PushStruct(GlobalGameState->PersistentArena, world);
  World->Arena = GlobalGameState->PersistentArena;
  World->ContactManifolds = CreateWorldContactChunk(World->Arena, MaxNrManifolds);
  return World;
}

u32 CreateCube(entity_manager* EM, game_asset_manager* AssetManager, v3 Position, r32 RotAngle, v3 RotAxis, r32 Scale, v3 LinearVelocity, v3 AngularVelocity, r32 Mass)
{

  bitmap_handle BitmapHandle;
  object_handle ObjectHandle;
  material_handle MaterialHandle;
  GetHandle( AssetManager, "cube1",   &ObjectHandle );
  GetHandle( AssetManager, "cube",    &MaterialHandle );
  GetHandle( AssetManager, "cube_kd", &BitmapHandle );

  u32 CubeEntity = NewEntity( EM );
  NewComponents( EM, CubeEntity,
                COMPONENT_FLAG_RENDER   |
                COMPONENT_FLAG_SPATIAL  |
                COMPONENT_FLAG_COLLIDER |
                COMPONENT_FLAG_DYNAMICS);

  component_render* CubeRender = GetRenderComponent(CubeEntity);
  CubeRender->Object = ObjectHandle;
  CubeRender->Bitmap = BitmapHandle;
  CubeRender->Material = MaterialHandle;

  component_spatial* CubeSpatial = GetSpatialComponent(CubeEntity);
  CubeSpatial->Position = Position;
  CubeSpatial->Rotation = RotateQuaternion( RotAngle, RotAxis );
  CubeSpatial->Scale = V3(Scale, Scale, Scale);
  UpdateModelMatrix(CubeSpatial);

  component_collider* CubeCollider = GetColliderComponent(CubeEntity);
  CubeCollider->Object = ObjectHandle;
  CubeCollider->AABB = GetMeshAABB(AssetManager, ObjectHandle);

  component_dynamics* CubeDynamics = GetDynamicsComponent(CubeEntity);
  CubeDynamics->LinearVelocity  = LinearVelocity;
  CubeDynamics->AngularVelocity = AngularVelocity;
  CubeDynamics->Mass = Mass;

  CalculateInertialTensor(CubeSpatial, CubeCollider, CubeDynamics);

  return CubeEntity;
}

void CreateCollisionTestScene(game_state* GameState, game_input* Input)
{
  world* World = GameState->World;
  game_asset_manager* AssetManager = GlobalGameState->AssetManager;
  entity_manager* EM = GlobalGameState->EntityManager;

#if 0
  for (int i = 0; i < 100; ++i)
  {
    u32 TestEntity = NewEntity( EM );
    NewComponents( EM, TestEntity, COMPONENT_FLAG_CAMERA  | COMPONENT_FLAG_CONTROLLER );
    if( (i % 5 ) == 0 || ((i % 5 ) == 1) )
    {
      NewComponents( EM, TestEntity, COMPONENT_FLAG_SPATIAL | COMPONENT_FLAG_COLLIDER );
    }else{
      NewComponents( EM, TestEntity, COMPONENT_FLAG_DYNAMICS | COMPONENT_FLAG_RENDER | COMPONENT_FLAG_SPRITE_ANIMATION);
    }
  }

  {
    BeginScopedEntityManagerMemory();
    component_result* Result = GetComponentsOfType(EM, COMPONENT_FLAG_COLLIDER | COMPONENT_FLAG_CAMERA);
    while(Next(EM, Result))
    {
      component_collider* Collider = GetColliderComponent(Result);
      component_camera* Camera = GetCameraComponent(Result);
      component_sprite_animation* Anim = GetSpriteAnimationComponent(Result);
      Assert(Collider);
      Assert(Camera);
      Assert(!Anim);
    }
  }
  exit(0);
#endif

  u32 ControllableCamera = NewEntity( EM );
  NewComponents( EM, ControllableCamera, COMPONENT_FLAG_CONTROLLER | COMPONENT_FLAG_SPATIAL | COMPONENT_FLAG_CAMERA);
  component_camera* Camera = GetCameraComponent(ControllableCamera);
  game_window_size WindowSize = GameGetWindowSize();
  r32 AspectRatio = WindowSize.WidthPx / WindowSize.HeightPx;
  r32 FieldOfView =  90;
  SetCameraComponent(Camera, FieldOfView, AspectRatio );
  LookAt(Camera, 1*V3(0,3,8), V3(0,3,0));
  GameState->World->ActiveCamera = Camera;

  component_controller* Controller = GetControllerComponent(ControllableCamera);
  Controller->Controller = GetController(Input, 0);
  Controller->Keyboard = &Input->Keyboard;
  Controller->Type = ControllerType_FlyingCamera;

  u32 LightEntity = NewEntity( EM );
  NewComponents( EM, LightEntity, COMPONENT_FLAG_LIGHT | COMPONENT_FLAG_SPATIAL | COMPONENT_FLAG_RENDER);
  component_light* Light = GetLightComponent(LightEntity);
  component_spatial* LightSpatial = GetSpatialComponent(LightEntity);
  component_render* LightRender = GetRenderComponent(LightEntity);

  Light->Color = 12.7*V4(1,1,1,1);

  LightSpatial->Position = V3(1,1,1);
  LightSpatial->Scale    = V3( 0.3, 0.3, 0.3);
  UpdateModelMatrix(LightSpatial);

  GetHandle(AssetManager, "voxel", &LightRender->Object);
  GetHandle(AssetManager, "red_plastic", &LightRender->Material);
  GetHandle(AssetManager, "null",  &LightRender->Bitmap);

  #if 0

#define state6
#if defined(state1)
  r32 ySpace = 1.1;
  r32 xzSpace = 1.1;
  s32 iarr[] = {-0,1};
  s32 jarr[] = {-1,14};
  s32 karr[] = {-0,1};
#elif defined(state2)
  r32 ySpace = 1;
  r32 xzSpace = 1;
  s32 iarr[] = {-2,2};
  s32 jarr[] = {-1,5};
  s32 karr[] = {-2,2};
#elif defined(state3)
  r32 ySpace = 1;
  r32 xzSpace = 1.2;
  s32 iarr[] = {-2,2};
  s32 jarr[] = {-0,3};
  s32 karr[] = {-2,2};
#elif defined(state4)
  r32 ySpace = 1;
  r32 xzSpace = 1.2;
  s32 iarr[] = {-0,1};
  s32 jarr[] = {-0,1};
  s32 karr[] = {-0,1};
#elif defined (state5)
  r32 ySpace = 20;
  r32 xzSpace = 1.2;
  s32 iarr[] = {-2,1};
  s32 jarr[] = {-0,2};
  s32 karr[] = {-2,1};
#elif defined (state6)
  r32 ySpace = 6;
  r32 xzSpace = 1.2;
  s32 iarr[] = {-0,1};
  s32 jarr[] = {-0,1};
  s32 karr[] = {-0,1};
#else
  // BUG: If qubes are sliiightly rotated the contact-generation spazzes out (contact normals are zero)
  //      which kind of makes sense since they are tightly packed to begin with
  //      However: Would like the gjk-algorithm to be a bit more stable
  //      The real bug however is, given cubes being perfectly aligned, this setup spazzes out
  //      if we run time as normal, but if we time-step or wait 0.1 sec before initiating spatial-system
  //      it works
  r32 ySpace = 1;
  r32 xzSpace = 1;
  s32 iarr[] = {-1,3};
  s32 jarr[] = {-1,3};
  s32 karr[] = {-1,3};
#endif

  bitmap_handle BitmapHandle;
  object_handle ObjectHandle;
  material_handle MaterialHandle;
  GetHandle( AssetManager, "cube1",   &ObjectHandle );
  GetHandle( AssetManager, "cube",    &MaterialHandle );
  GetHandle( AssetManager, "cube_kd", &BitmapHandle );
  r32 mass = 10;
  for (s32 i = iarr[0]; i < iarr[1]; ++i)
  {
    for (s32 j = jarr[0]; j < jarr[1]; ++j)
    {
      for (s32 k = karr[0]; k < karr[1]; ++k)
      {

        u32 CubeEntity = NewEntity( EM );
        NewComponents( EM, CubeEntity,
          COMPONENT_FLAG_RENDER   |
          COMPONENT_FLAG_SPATIAL  |
          COMPONENT_FLAG_COLLIDER |
          COMPONENT_FLAG_DYNAMICS);

        component_render* CubeRender = GetRenderComponent(CubeEntity);
        CubeRender->Object = ObjectHandle;
        CubeRender->Bitmap = BitmapHandle;
        CubeRender->Material = MaterialHandle;

        component_spatial* CubeSpatial = GetSpatialComponent(CubeEntity);
        CubeSpatial->Position =  V3(xzSpace*i, ySpace*(j+1), xzSpace*k);
        CubeSpatial->Rotation = RotateQuaternion( 0, V3(0,0,0) );
        CubeSpatial->Scale = V3(1, 1, 1);
        UpdateModelMatrix(CubeSpatial);

        component_collider* CubeCollider = GetColliderComponent(CubeEntity);
        CubeCollider->Object = ObjectHandle;
        CubeCollider->AABB = GetMeshAABB(AssetManager, ObjectHandle);

        component_dynamics* CubeDynamics = GetDynamicsComponent(CubeEntity);
        CubeDynamics->LinearVelocity  = V3(0,0,0);
        CubeDynamics->AngularVelocity = V3(0,0,0);
        CubeDynamics->Mass = mass;

        CalculateInertialTensor(CubeSpatial, CubeCollider, CubeDynamics);

      }
    }
  }
#else

  u32 Cube1 = CreateCube(EM, AssetManager, V3(0,0,0), 0.f, V3(0,1,0), 1.f,
                           V3(3,0,0), V3(0,0,0), 10);
  u32 Cube2 = CreateCube(EM, AssetManager, V3(-2,2,0), 0.f, V3(0,1,0), 1.f,
                           V3(0,0,0), V3(0,0,0), 10);

  World->Joint = CreateJointConstraint(Cube1,
                                         ToLocal(GetSpatialComponent(Cube1), V3(0,2,0)),
                                       Cube2,
                                         ToLocal(GetSpatialComponent(Cube2), V3(0,2,0)),
                                         V3(0,0,1)); // Global Rotation Angle

  #endif



  u32 FloorEntity = NewEntity( EM );
  NewComponents( EM, FloorEntity,
    COMPONENT_FLAG_RENDER   |
    COMPONENT_FLAG_SPATIAL  |
    COMPONENT_FLAG_COLLIDER);

  component_render* FloorRender = GetRenderComponent(FloorEntity);
  GetHandle( AssetManager, "voxel",   &FloorRender->Object);
  GetHandle( AssetManager, "red_plastic", &FloorRender->Material);
  GetHandle( AssetManager, "null", &FloorRender->Bitmap );

  component_spatial* FloorSpatial = GetSpatialComponent(FloorEntity);
  FloorSpatial->Position = V3( 0,-2, 0);
  FloorSpatial->Rotation = V4( 0,0, 0, 1);
  FloorSpatial->Scale = V3( 118, 1, 118);
  UpdateModelMatrix(FloorSpatial);

  component_collider* FloorCollider = GetColliderComponent(FloorEntity);
  FloorCollider->Object = FloorRender->Object;
  FloorCollider->AABB = GetMeshAABB( AssetManager, FloorRender->Object);

  PushBitmapData(AssetManager, "energy_plot", 512, 512, 32, 0, false);

  bitmap_handle Plot;
  GetHandle(GlobalGameState->AssetManager, "energy_plot", &Plot);
  bitmap* Bitmap = GetAsset(GlobalGameState->AssetManager,Plot);
  u32* Pixels = (u32*) Bitmap->Pixels;
  u32* EndPixel = Pixels + Bitmap->Width*Bitmap->Height;
  u8 Blue = 0;
  u8 Green = 0;
  u8 Red = 0;
  u8 Alpha = 255;
  while(Pixels < EndPixel)
  {
    u32 PixelData = (Blue << 0) | (Green << 8) | (Red << 16) | Alpha << 24;
    *Pixels++ = PixelData;
  }
#if 0
  u32 Teapot = NewEntity( EM );
  NewComponents( EM, Teapot,
    COMPONENT_FLAG_RENDER   |
    COMPONENT_FLAG_SPATIAL  |
    COMPONENT_FLAG_COLLIDER |
    COMPONENT_FLAG_DYNAMICS);


  component_render* TeapotRender = GetRenderComponent(Teapot);
  GetHandle( AssetManager, "teapot",   &TeapotRender->Object);
  GetHandle( AssetManager, "gold", &TeapotRender->Material);
  GetHandle( AssetManager, "checker_board", &TeapotRender->Bitmap );

  component_spatial* TeapotSpatial = GetSpatialComponent(Teapot);
  TeapotSpatial->Position = V3( -2,1,2);
  TeapotSpatial->Scale = V3( 0.1, 0.1, 0.1);
  TeapotSpatial->Rotation = RotateQuaternion( 0.5, V3(0.2,1,0) );
  UpdateModelMatrix(TeapotSpatial);


  component_collider* TeapotCollider = GetColliderComponent(Teapot);
  TeapotCollider->Object = TeapotRender->Object;
  TeapotCollider->AABB = GetMeshAABB(AssetManager, TeapotRender->Object);

  component_dynamics* TeapotDynamics = GetDynamicsComponent(Teapot);
  TeapotDynamics->LinearVelocity  = V3(0,0,0);
  TeapotDynamics->AngularVelocity = V3(0,0,0);
  TeapotDynamics->Mass = 20;
#endif

  u32 SpriteAnimationEntity = NewEntity( EM );
  NewComponents( EM, SpriteAnimationEntity,
    COMPONENT_FLAG_SPRITE_ANIMATION |
    COMPONENT_FLAG_RENDER           |
    COMPONENT_FLAG_SPATIAL);

  component_render* AnimationRender = GetRenderComponent(SpriteAnimationEntity);
  GetHandle( AssetManager, "quad",   &AnimationRender->Object);
  GetHandle( AssetManager, "red_rubber", &AnimationRender->Material);
  GetHandle( AssetManager, "hero_sprite_sheet", &AnimationRender->Bitmap );


  component_spatial* SpriteSpatial = GetSpatialComponent(SpriteAnimationEntity);
  SpriteSpatial->Position = V3( 0,  8, -8);
  SpriteSpatial->Rotation = RotateQuaternion( 0, V3(0,1,0) );
  SpriteSpatial->Scale    = V3( 18, 18, 1);
  UpdateModelMatrix(SpriteSpatial);

  component_sprite_animation* SpriteAnimation = GetSpriteAnimationComponent(SpriteAnimationEntity);

  hash_map<bitmap_coordinate> HeroCoordinates = LoadAdventurerSpriteSheetCoordinates( GameState->TransientArena );

  bitmap* HeroSpriteSheet = GetAsset(GameState->AssetManager, AnimationRender->Bitmap);
  SpriteAnimation->Animation = hash_map< list<m4> >(&GameState->AssetManager->AssetArena,6);

  list<m4> Idle1(&GameState->AssetManager->AssetArena);
  Idle1.First();
  Idle1.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("idle1_01")));
  Idle1.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("idle1_02")));
  Idle1.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("idle1_03")));
  Idle1.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("idle1_04")));
  SpriteAnimation->Animation.Insert("idle1",  Idle1);

  list<m4> Idle2(&GameState->AssetManager->AssetArena);
  Idle2.First();
  Idle2.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("idle2_01")));
  Idle2.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("idle2_02")));
  Idle2.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("idle2_03")));
  Idle2.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("idle2_04")));
  SpriteAnimation->Animation.Insert("idle2",  Idle2);

  list<m4> Run(&GameState->AssetManager->AssetArena);
  Run.First();
  Run.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("run_01")));
  Run.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("run_02")));
  Run.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("run_03")));
  Run.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("run_04")));
  Run.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("run_05")));
  Run.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("run_06")));
  SpriteAnimation->Animation.Insert("run",  Run);

  list<m4> Jump(&GameState->AssetManager->AssetArena);
  Jump.First();
  Jump.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("jump_01")));
  Jump.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("jump_02")));
  Jump.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("jump_03")));
  SpriteAnimation->Animation.Insert("jump",  Jump);

  list<m4> Fall(&GameState->AssetManager->AssetArena);
  Fall.First();
  Fall.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("fall_01")));
  Fall.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("fall_02")));
  SpriteAnimation->Animation.Insert("fall",  Fall);

  SpriteAnimation->ActiveSeries = SpriteAnimation->Animation.Get("idle1");
}

void InitiateGame(game_memory* Memory, game_render_commands* RenderCommands, game_input* Input )
{
  if (!Memory->GameState)
  {
    GlobalGameState = BootstrapPushStruct(game_state, PersistentArena);
    GlobalGameState->TransientArena = PushStruct(GlobalGameState->PersistentArena, memory_arena);
    GlobalGameState->TransientTempMem = BeginTemporaryMemory(GlobalGameState->TransientArena);

    GlobalGameState->FunctionPool = PushStruct(GlobalGameState->PersistentArena, function_pool);

    GlobalGameState->RenderCommands = RenderCommands;

    GlobalGameState->AssetManager  = CreateAssetManager();
    GlobalGameState->EntityManager = CreateEntityManager();
    GlobalGameState->MenuInterface = CreateMenuInterface(GlobalGameState->PersistentArena, Megabytes(1));

    GlobalGameState->World = CreateWorld(2048);

    GlobalGameState->Input = Input;

    CreateCollisionTestScene(GlobalGameState, Input);

    Memory->GameState = GlobalGameState;

    RenderCommands->AssetManager = GlobalGameState->AssetManager;

    for (s32 ControllerIndex = 0;
    ControllerIndex < ArrayCount(Input->Controllers);
      ++ControllerIndex)
    {
      game_controller_input* Controller = GetController(Input, ControllerIndex);
      Controller->IsAnalog = true;
    }

    GlobalGameState->IsInitialized = true;


    Assert(!RenderCommands->LightsGroup);
    Assert(!RenderCommands->RenderGroup);
    Assert(!RenderCommands->OverlayGroup);

    RenderCommands->LightsGroup = InitiateRenderGroup();
    RenderCommands->RenderGroup = InitiateRenderGroup();
    RenderCommands->OverlayGroup = InitiateRenderGroup();
  }

  Assert(RenderCommands->LightsGroup);
  Assert(RenderCommands->RenderGroup);
  Assert(RenderCommands->OverlayGroup);
}

#include "function_pointer_pool.h"

void BeginFrame(game_memory* Memory, game_render_commands* RenderCommands, game_input* Input )
{
  Platform = Memory->PlatformAPI;

  InitiateGame(Memory, RenderCommands, Input);

#if HANDMADE_INTERNAL
  DebugGlobalMemory = Memory;
#endif

  Assert(Memory->GameState);
  Assert(Memory->GameState->AssetManager);

  GlobalGameState = Memory->GameState;

  EndTemporaryMemory(GlobalGameState->TransientTempMem);
  GlobalGameState->TransientTempMem = BeginTemporaryMemory(GlobalGameState->TransientArena);

  if(Input->ExecutableReloaded)
  {
    ReinitiatePool();
  }


  ResetRenderGroup(RenderCommands->LightsGroup);
  ResetRenderGroup(RenderCommands->RenderGroup);
  ResetRenderGroup(RenderCommands->OverlayGroup);



  game_window_size WindowSize = GameGetWindowSize();
  r32 AspectRatio = WindowSize.WidthPx/WindowSize.HeightPx;

  m4 ScreenToCubeScale =  M4( 2/AspectRatio, 0, 0, 0,
                                           0, 2, 0, 0,
                                           0, 0, 0, 0,
                                           0, 0, 0, 1);
  m4 ScreenToCubeTrans =  M4( 1, 0, 0, -1,
                              0, 1, 0, -1,
                              0, 0, 1,  0,
                              0, 0, 0,  1);

  RenderCommands->OverlayGroup->ProjectionMatrix = ScreenToCubeTrans*ScreenToCubeScale;
//  RenderCommands->MainRenderGroup->ScreenWidth  = (r32)RenderCommands->ScreenWidthPixels;
//  RenderCommands->MainRenderGroup->ScreenHeight = (r32)RenderCommands->ScreenHeightPixels;
//  RenderCommands->DebugRenderGroup->ScreenWidth  = (r32) RenderCommands->ScreenWidthPixels;
//  RenderCommands->DebugRenderGroup->ScreenHeight = (r32) RenderCommands->ScreenHeightPixels;

}


/*
  Note:
  extern "C" prevents the C++ compiler from renaming the functions which it does for function-overloading reasons
  (among other things) by forcing it to use C conventions which does not support overloading.
  Also called 'name mangling' or 'name decoration'. The actual function names are visible in the outputted .map file
  i the build directory.
*/


rect2f PlotToBitmap( bitmap* Bitmap, r32 ChartXMin, r32 ChartXMax, r32 ChartYMin, r32 ChartYMax, r32 XValue, r32 YValue, s32 LineSize, v4 Color)
{
  if(XValue < ChartXMin || XValue >= ChartXMax ||
     YValue < ChartYMin || YValue >= ChartYMax)
  {
    return {};
  }

  r32 XPercent = (XValue-ChartXMin) / (ChartXMax - ChartXMin);
  r32 YPercent = (YValue-ChartYMin) / (ChartYMax - ChartYMin);
  s32 PixelXValue = (s32) Round(XPercent*Bitmap->Width);
  s32 PixelYValue = (s32) Round(YPercent*Bitmap->Height);

  u8 Alpha = (u8) (Color.W * 255.f);
  u8 Blue = (u8) (Color.X * 255.f);
  u8 Green = (u8) (Color.Y * 255.f);
  u8 Red = (u8) (Color.Z * 255.f);

  u32 PixelData = (Blue << 0) | (Green << 8) | (Red << 16) | Alpha << 24;

  r32 X = (r32) Maximum(0, PixelXValue - LineSize);
  r32 Y = (r32) Maximum(0, PixelYValue - LineSize);
  r32 W = (r32) Minimum(PixelXValue + LineSize, 2*LineSize);
  r32 H = (r32) Minimum(PixelYValue + LineSize, 2*LineSize);

  W = (r32) Minimum(Bitmap->Width-X, W);
  H = (r32) Minimum(Bitmap->Height-Y, H);

  rect2f Result = Rect2f(X,Y,W,H);
  u32* Pixels = (u32*) Bitmap->Pixels;

  for( s32 YPixel = (s32) Y;
           YPixel < (s32)(Y+H);
         ++YPixel)
  {
    for( s32 XPixel = (s32)X;
             XPixel < (s32)(X+W);
           ++XPixel)
    {
      u32 PixelIndex = YPixel * Bitmap->Width + XPixel;
      u32* Pixel = Pixels + PixelIndex;
      *Pixel = 0;
    }
  }

  for( s32 YPixel = (s32) Y;
           YPixel < (s32)(Y+H);
         ++YPixel)
  {
    for( s32 XPixel = (s32)X;
             XPixel < (s32)(X+W);
           ++XPixel)
    {
      u32 PixelIndex = YPixel * Bitmap->Width + XPixel;
      u32* Pixel = Pixels + PixelIndex;
      *Pixel = PixelData;
    }
  }

  return Result;
}


// Signature is
//void game_update_and_render (thread_context* Thread,
//                game_memory* Memory,
//                render_commands* RenderCommands,
//                game_input* Input )


extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  BeginFrame(Memory, RenderCommands, Input);

  TIMED_FUNCTION();

  world* World = GlobalGameState->World;
  World->dtForFrame = Input->dt;

  entity_manager* EM = GlobalGameState->EntityManager;
  ControllerSystemUpdate(World);
  //World->AdvanceOneFrame = true;
  if(World->AdvanceOneFrame)
  {
    ColliderSystemUpdate(World);
    SpatialSystemUpdate(World);
  }
  CameraSystemUpdate(World);
  SpriteAnimationSystemUpdate(World);
  if(World->AdvanceOneFrame)
  {
    World->GlobalTimeSec += Input->dt;
  }
#if 1
  bitmap_handle Plot;
  GetHandle(GlobalGameState->AssetManager, "energy_plot", &Plot);
  bitmap* PlotBitMap = GetAsset(GlobalGameState->AssetManager, Plot);


  BeginScopedEntityManagerMemory();
  component_result* ComponentList = GetComponentsOfType(EM, COMPONENT_FLAG_DYNAMICS);
  rect2f UpdatedRegion {};
  u32 PointSize = 1;

  r32 Ek = 0;
  r32 Er = 0;
  r32 Ep = 0;
  while(Next(EM, ComponentList))
  {
    component_spatial* Spatial = (component_spatial*) GetComponent(EM, ComponentList, COMPONENT_FLAG_SPATIAL);
    component_dynamics* Dynamics = (component_dynamics*) GetComponent(EM, ComponentList, COMPONENT_FLAG_DYNAMICS);

    Ek += Dynamics->Mass *(Dynamics->LinearVelocity * Dynamics->LinearVelocity) * 0.5f;
    Er += Dynamics->Mass *(Dynamics->AngularVelocity * Dynamics->AngularVelocity) * 0.5f;
    Ep += Dynamics->Mass * 9.82f * (Spatial->Position.Y+1);
  }

  v2 Cursor = V2(0,0);

  r32 Cycles = Floor(World->GlobalTimeSec/20.f);
  r32 Remainder = World->GlobalTimeSec - 20*Cycles;

  rect2f TmpRegion1 = PlotToBitmap( PlotBitMap, 0, 20, -500, 7000,  Remainder, Er, PointSize, HexCodeToColorV4(0xff0000));
  UpdatedRegion = MergeRect(UpdatedRegion, TmpRegion1);
  rect2f TmpRegion2 = PlotToBitmap( PlotBitMap, 0, 20, -500, 7000,  Remainder, Ek, PointSize, HexCodeToColorV4(0x00ff00));
  UpdatedRegion = MergeRect(UpdatedRegion, TmpRegion2);
  rect2f TmpRegion4 = PlotToBitmap( PlotBitMap, 0, 20, -500, 7000,  Remainder, Ep, PointSize, HexCodeToColorV4(0x0000ff));
  UpdatedRegion = MergeRect(UpdatedRegion, TmpRegion4);
  rect2f TmpRegion5 = PlotToBitmap( PlotBitMap, 0, 20, -500, 7000,  Remainder, Ep+Ek+Er, PointSize, HexCodeToColorV4(0xff00FF));
  UpdatedRegion = MergeRect(UpdatedRegion, TmpRegion5);

  Reupload(GlobalGameState->AssetManager, Plot, UpdatedRegion);
#endif


  FillRenderPushBuffer(World);

  if(Memory->DebugState)
  {
    PushDebugOverlay(Input);
  }
  UpdateAndRenderMenuInterface(Input, GlobalGameState->MenuInterface);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
  GameOutputSound(SoundBuffer, 400);
}

#include "debug.cpp"
