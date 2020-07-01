/*
  TODO:
  BUGS:
    - Fix game-loop-memory allocation bug.
      Pressing L activates game-looping. When the loop reloads win32-memory-allocation fails. No idea why, It seems
      to have been like this for a long long time. Before debug-system.
  Rendering:
    - Shadows
    (working on) Instance rendering, Want to group geometries together and do as few draw-calls as possible. (when possible, fonts, graphs for example, cubes too)
      - Transforms, Texture-coordinates, Colours (as much data as possible) should be sent in buffers, not as Uniforms.
    - Investigate raytracing
    - Use 3d Textures and let indexed rendering also specify the depth
  Assets:
    - Separately built Asset file with multithreaded stream-loading. (Maybe using ASSIMP)
    - Add a tag-system. I want to get a random asset with a specific tag. Predefined-Material for example.
    - Let asset system gather up all the assets (geoms, textures, etx) needed and send them to the gpu in one go at start.
      - What is to be rendered should be known at the start so we can let the asset manager produce all 'active' assets so the
        renderer can uppload them to the gpu.
      - Benefits include:
        * Can be extended later to stream textures.
        * More defined responsibilities.
        
  Debug:
    (working on) Continue w decent Debug-interface.
    - Time-Plots
    - Item selection w mouse
    - Continue on caseys Stream, he gets rid of the __COUNT__ macro gimmic which we also use at some point.
  Physics:
    - Persistent AABB-tree
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
*/

#include "handmade.h"

#include "assets.h"
#include "tiles_spritesheet.h"

#if HANDMADE_INTERNAL
game_memory* DebugGlobalMemory = 0;
#endif

game_state* GlobalGameState = 0;
platform_api Platform;

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
#include "system_spatial.cpp"
#include "system_camera.cpp"
#include "assets.cpp"
#include "asset_loading.cpp"

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

world* CreateWorld( u32 MaxNrManifolds )
{
  world* World = PushStruct(GlobalGameState->PersistentArena, world);
  World->Arena = GlobalGameState->PersistentArena;
  World->ContactManifolds = CreateWorldContactChunk(World->Arena, MaxNrManifolds);
  return World;
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
    ScopedTransaction(GlobalGameState->EntityManager);
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
  NewComponents( EM, ControllableCamera, COMPONENT_FLAG_CONTROLLER | COMPONENT_FLAG_CAMERA);
  component_camera* Camera = GetCameraComponent(ControllableCamera);
  r32 AspectRatio = GameState->ScreenWidthPixels / GameState->ScreenHeightPixels;
  r32 FieldOfView =  90;
  SetCameraComponent(Camera, FieldOfView, AspectRatio );
  LookAt(Camera, 1*V3(0,3,-8), V3(0,3,0));

  component_controller* Controller = GetControllerComponent(ControllableCamera);
  Controller->Controller = GetController(Input, 1);
  Controller->Type = ControllerType_FlyingCamera;

  u32 LightEntity = NewEntity( EM );
  NewComponents( EM, LightEntity, COMPONENT_FLAG_LIGHT | COMPONENT_FLAG_SPATIAL | COMPONENT_FLAG_RENDER);
  component_light* Light = GetLightComponent(LightEntity);
  component_spatial* LightSpatial = GetSpatialComponent(LightEntity);
  component_render* LightRender = GetRenderComponent(LightEntity);

  Light->Color = 0.6*V4(1,1,1,1);

  LightSpatial->Position = V3(1,1,1);
  LightSpatial->Scale    = V3( 0.3, 0.3, 0.3);

  LightRender->AssetHandle = GetAssetHandle(GameState->AssetManager);
  SetAsset( GameState->AssetManager, asset_type::OBJECT,   "voxel", LightRender->AssetHandle);
  SetAsset( GameState->AssetManager, asset_type::MATERIAL, "white", LightRender->AssetHandle);  // May have a texture
  SetAsset( GameState->AssetManager, asset_type::BITMAP,   "null",  LightRender->AssetHandle);  // Overrides texture


#define State4
#if defined(State1)
  // BUG: Bug-producing state. Rotation bleeds over into scaling
  r32 ySpace = 1;
  r32 xzSpace = 1;
  s32 iarr[] = {-0,1};
  s32 jarr[] = {-1,6};
  s32 karr[] = {-0,1};
#elif defined(State2)
  r32 ySpace = 1;
  r32 xzSpace = 1;
  s32 iarr[] = {-2,2};
  s32 jarr[] = {-1,5};
  s32 karr[] = {-2,2};
#elif defined(State3)
  r32 ySpace = 1;
  r32 xzSpace = 1.2;
  s32 iarr[] = {-2,2};
  s32 jarr[] = {-0,3};
  s32 karr[] = {-2,2};
#else
  r32 ySpace = 1;
  r32 xzSpace = 1.2;
  s32 iarr[] = {-0,1};
  s32 jarr[] = {-0,1};
  s32 karr[] = {-0,1};
#endif
  u32 RollingIndex = 0;
  u32 CubeAssetHandle = GetAssetHandle( GameState->AssetManager);
  SetAsset( AssetManager, asset_type::OBJECT, "cube1", CubeAssetHandle );
  SetAsset( AssetManager, asset_type::MATERIAL, "cube", CubeAssetHandle );
  SetAsset( AssetManager, asset_type::BITMAP, "cube_kd", CubeAssetHandle );
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

        GetRenderComponent(CubeEntity)->AssetHandle = CubeAssetHandle;
        component_spatial* CubeSpatial = GetSpatialComponent(CubeEntity);
        CubeSpatial->Position =  V3(xzSpace*i, ySpace*j, xzSpace*k);
        CubeSpatial->Rotation = RotateQuaternion( 0, V3(0,0,0) );
        CubeSpatial->Scale = V3(1, 1, 1);

        component_collider* CubeCollider = GetColliderComponent(CubeEntity);
        CubeCollider->AssetHandle = CubeAssetHandle;
        CubeCollider->AABB = GetMeshAABB(AssetManager, CubeAssetHandle);

        component_dynamics* CubeDynamics = GetDynamicsComponent(CubeEntity);
        CubeDynamics->LinearVelocity  = V3(0,0,0);
        CubeDynamics->AngularVelocity = V3(0,0,0);
        CubeDynamics->Mass = 1;
      }
    }
  }

  u32 FloorEntity = NewEntity( EM );
  NewComponents( EM, FloorEntity,
    COMPONENT_FLAG_RENDER   |
    COMPONENT_FLAG_SPATIAL  |
    COMPONENT_FLAG_COLLIDER);

  u32 FloorHandle = GetAssetHandle(AssetManager);
  SetAsset( AssetManager, asset_type::OBJECT, "voxel", FloorHandle);
  SetAsset( AssetManager, asset_type::MATERIAL, "checker_board", FloorHandle );
  SetAsset( AssetManager, asset_type::BITMAP, "checker_board", FloorHandle );

  GetRenderComponent(FloorEntity)->AssetHandle = FloorHandle;

  component_spatial* FloorSpatial = GetSpatialComponent(FloorEntity);
  FloorSpatial->Position = V3( 0,-2, 0);
  FloorSpatial->Scale = V3( 18, 1, 18);

  component_collider* FloorCollider = GetColliderComponent(FloorEntity);
  FloorCollider->AssetHandle = FloorHandle;
  FloorCollider->AABB = GetMeshAABB( AssetManager, FloorHandle);

#if 1
  u32 Teapot = NewEntity( EM );
  NewComponents( EM, Teapot,
    COMPONENT_FLAG_RENDER   |
    COMPONENT_FLAG_SPATIAL  |
    COMPONENT_FLAG_COLLIDER |
    COMPONENT_FLAG_DYNAMICS);

  u32 TeapotHandle = GetAssetHandle(AssetManager);
  SetAsset( AssetManager, asset_type::OBJECT, "teapot", TeapotHandle);
  SetAsset( AssetManager, asset_type::MATERIAL, "gold", TeapotHandle );
  SetAsset( AssetManager, asset_type::BITMAP, "checker_board", TeapotHandle );
  GetRenderComponent(Teapot)->AssetHandle = TeapotHandle;


  component_spatial* TeapotSpatial = GetSpatialComponent(Teapot);
  TeapotSpatial->Position = V3( -2,1,2);
  TeapotSpatial->Scale = V3( 0.1, 0.1, 0.1);
  TeapotSpatial->Rotation = RotateQuaternion( 0.5, V3(0.2,1,0) );

  component_collider* TeapotCollider = GetColliderComponent(Teapot);
  TeapotCollider->AssetHandle = TeapotHandle;
  TeapotCollider->AABB = GetMeshAABB(AssetManager, TeapotHandle);

  component_dynamics* TeapotDynamics = GetDynamicsComponent(Teapot);
  TeapotDynamics->LinearVelocity  = V3(0,0,0);
  TeapotDynamics->AngularVelocity = V3(0,0,0);
  TeapotDynamics->Mass = 2;
#endif

  u32 SpriteAnimationEntity = NewEntity( EM );
  NewComponents( EM, SpriteAnimationEntity,
    COMPONENT_FLAG_SPRITE_ANIMATION |
    COMPONENT_FLAG_RENDER           |
    COMPONENT_FLAG_SPATIAL);

  u32 HeroSpriteHandle = GetAssetHandle(AssetManager);
  SetAsset( AssetManager, asset_type::OBJECT, "quad", HeroSpriteHandle);
  SetAsset( AssetManager, asset_type::BITMAP, "hero_sprite_sheet", HeroSpriteHandle);
  SetAsset( AssetManager, asset_type::MATERIAL, "cube", HeroSpriteHandle );

  GetRenderComponent(SpriteAnimationEntity)->AssetHandle = HeroSpriteHandle;

  component_spatial* SpriteSpatial = GetSpatialComponent(SpriteAnimationEntity);
  SpriteSpatial->Position = V3( -0,  8, 8);
  SpriteSpatial->Rotation = RotateQuaternion( Pi32, V3(0,1,0) );
  SpriteSpatial->Scale    = V3( 18, 18, 1);

  component_sprite_animation* SpriteAnimation = GetSpriteAnimationComponent(SpriteAnimationEntity);

  hash_map<bitmap_coordinate> HeroCoordinates = LoadAdventurerSpriteSheetCoordinates( GameState->TransientArena );

  bitmap* HeroSpriteSheet =  GetBitmap(GameState->AssetManager, HeroSpriteHandle);
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

    GlobalGameState->AssetManager = CreateAssetManager();
    GlobalGameState->EntityManager = CreateEntityManager();

    GlobalGameState->World = CreateWorld(2048);

    GlobalGameState->ScreenWidthPixels  = (r32)RenderCommands->ResolutionWidthPixels;
    GlobalGameState->ScreenHeightPixels = (r32)RenderCommands->ResolutionHeightPixels;

    CreateCollisionTestScene(GlobalGameState, Input);

    Memory->GameState = GlobalGameState;

    for (s32 ControllerIndex = 0;
    ControllerIndex < ArrayCount(Input->Controllers);
      ++ControllerIndex)
    {
      game_controller_input* Controller = GetController(Input, ControllerIndex);
      Controller->IsAnalog = true;
    }

    GlobalGameState->IsInitialized = true;
  }

  if(!RenderCommands->MainRenderGroup)
  {
    RenderCommands->MainRenderGroup = InitiateRenderGroup(Memory->GameState, (r32)RenderCommands->ScreenWidthPixels, (r32)RenderCommands->ScreenHeightPixels);
  }

  if(!RenderCommands->DebugRenderGroup)
  {
    // TODO: Right now DebugRenderGroup just solves the problem of drawing the overlay on top of everything else with a special shader.
    //       This should be supported by just one RenderGroup with sorting capabilities and shaderswitching.
    //       Maybe DebugRenderGroup is something we want to move away from and consolidate into the DebugState.
    //       Is there a problem with the debug system piping it's drawing through the GameRenderingPipeline?
    //       I mean it already sort of does since it all gets drawn in RenderGroupToOutput.
    RenderCommands->DebugRenderGroup = InitiateRenderGroup(Memory->GameState, (r32)RenderCommands->ScreenWidthPixels, (r32)RenderCommands->ScreenHeightPixels);
  }
}

void BeginFrame(game_memory* Memory, game_render_commands* RenderCommands, game_input* Input )
{
  Platform = Memory->PlatformAPI;

  InitiateGame(Memory, RenderCommands, Input);

#if HANDMADE_INTERNAL
  GlobalDebugRenderGroup =  RenderCommands->DebugRenderGroup;
  DebugGlobalMemory = Memory;
#endif

  Assert(Memory->GameState);
  Assert(Memory->GameState->AssetManager);

  GlobalGameState = Memory->GameState;

  EndTemporaryMemory(GlobalGameState->TransientTempMem);
  GlobalGameState->TransientTempMem = BeginTemporaryMemory(GlobalGameState->TransientArena);

  RenderCommands->MainRenderGroup->ScreenWidth  = (r32)RenderCommands->ScreenWidthPixels;
  RenderCommands->MainRenderGroup->ScreenHeight = (r32)RenderCommands->ScreenHeightPixels;

  RenderCommands->DebugRenderGroup->ScreenWidth  = (r32) RenderCommands->ScreenWidthPixels;
  RenderCommands->DebugRenderGroup->ScreenHeight = (r32) RenderCommands->ScreenHeightPixels;

  ResetAssetManagerTemporaryInstances(GlobalGameState->AssetManager);
};

/*
  Note:
  extern "C" prevents the C++ compiler from renaming the functions which it does for function-overloading reasons
  (among other things) by forcing it to use C conventions which does not support overloading.
  Also called 'name mangling' or 'name decoration'. The actual function names are visible in the outputted .map file
  i the build directory.
*/


// Signature is
//void game_update_and_render (thread_context* Thread,
//                game_memory* Memory,
//                render_commands* RenderCommands,
//                game_input* Input )
extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  TIMED_FUNCTION();

  BeginFrame(Memory, RenderCommands, Input);

  world* World = GlobalGameState->World;
  World->dtForFrame = Input->dt;
  World->GlobalTimeSec += Input->dt;

  ControllerSystemUpdate(World);
  SpatialSystemUpdate(World);
  CameraSystemUpdate(World);
  SpriteAnimationSystemUpdate(World);
  FillRenderPushBuffer(World, RenderCommands->MainRenderGroup );

  if(Memory->DebugState)
  {
    PushDebugOverlay(Input);
  }
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
  GameOutputSound(SoundBuffer, 400);
}

#include "debug.cpp"
