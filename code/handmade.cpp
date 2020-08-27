/* 
  TODO:
  BUGS:
    - Make things work with arbitrary screensize (we have desired aspect ratio hardcoded evertwhere, (Add as global variable?))
    - Fix game-loop-memory allocation bug.
      Pressing L activates game-looping. When the loop reloads win32-memory-allocation fails. No idea why, It seems
      to have been like this for a long long time. Before debug-system.
    - Collision detection breaks when we have several boxes
  Rendering:
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
    * Continue w decent Debug-interface.
    - A global Function-Pointer-Pool that is automatically reinitialized upon recompilation.
    - Time-Plots
    - Item selection w mouse
    - Continue on caseys Stream, he gets rid of the __COUNT__ macro gimmic which we also use at some point. (ep 193)
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
  NewComponents( EM, ControllableCamera, COMPONENT_FLAG_CONTROLLER | COMPONENT_FLAG_SPATIAL | COMPONENT_FLAG_CAMERA);
  component_camera* Camera = GetCameraComponent(ControllableCamera);
  game_window_size WindowSize = GameGetWindowSize();
  r32 AspectRatio = WindowSize.WidthPx / WindowSize.HeightPx;
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

  Light->Color = 12.7*V4(1,1,1,1);

  LightSpatial->Position = V3(1,1,1);
  LightSpatial->Scale    = V3( 0.3, 0.3, 0.3);

  GetHandle(AssetManager, "voxel", &LightRender->Object);
  GetHandle(AssetManager, "white", &LightRender->Material);
  GetHandle(AssetManager, "null",  &LightRender->Bitmap);


#define State1
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


  bitmap_handle BitmapHandle;
  object_handle ObjectHandle;
  material_handle MaterialHandle;
  GetHandle( AssetManager, "cube1",   &ObjectHandle );
  GetHandle( AssetManager, "cube",    &MaterialHandle );
  GetHandle( AssetManager, "cube_kd", &BitmapHandle );
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
        CubeSpatial->Position =  V3(xzSpace*i, ySpace*j, xzSpace*k);
        CubeSpatial->Rotation = RotateQuaternion( 0, V3(0,0,0) );
        CubeSpatial->Scale = V3(1, 1, 1);

        component_collider* CubeCollider = GetColliderComponent(CubeEntity);
        CubeCollider->Object = ObjectHandle;
        CubeCollider->AABB = GetMeshAABB(AssetManager, ObjectHandle);

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

  component_render* FloorRender = GetRenderComponent(FloorEntity);
  GetHandle( AssetManager, "voxel",   &FloorRender->Object);
  GetHandle( AssetManager, "checker_board", &FloorRender->Material);
  GetHandle( AssetManager, "checker_board", &FloorRender->Bitmap );

  component_spatial* FloorSpatial = GetSpatialComponent(FloorEntity);
  FloorSpatial->Position = V3( 0,-2, 0);
  FloorSpatial->Scale = V3( 18, 1, 18);

  component_collider* FloorCollider = GetColliderComponent(FloorEntity);
  FloorCollider->Object = FloorRender->Object;
  FloorCollider->AABB = GetMeshAABB( AssetManager, FloorRender->Object);

#if 1
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

  component_collider* TeapotCollider = GetColliderComponent(Teapot);
  TeapotCollider->Object = TeapotRender->Object;
  TeapotCollider->AABB = GetMeshAABB(AssetManager, TeapotRender->Object);

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

  component_render* AnimationRender = GetRenderComponent(SpriteAnimationEntity);
  GetHandle( AssetManager, "quad",   &AnimationRender->Object);
  GetHandle( AssetManager, "white", &AnimationRender->Material);
  GetHandle( AssetManager, "hero_sprite_sheet", &AnimationRender->Bitmap );


  component_spatial* SpriteSpatial = GetSpatialComponent(SpriteAnimationEntity);
  SpriteSpatial->Position = V3( -0,  8, 8);
  SpriteSpatial->Rotation = RotateQuaternion( Pi32, V3(0,1,0) );
  SpriteSpatial->Scale    = V3( 18, 18, 1);

  component_sprite_animation* SpriteAnimation = GetSpriteAnimationComponent(SpriteAnimationEntity);

  hash_map<bitmap_coordinate> HeroCoordinates = LoadAdventurerSpriteSheetCoordinates( GameState->TransientArena );

  bitmap* HeroSpriteSheet =  GetAsset(GameState->AssetManager, AnimationRender->Bitmap);
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

    GlobalGameState->RenderCommands = RenderCommands;

    GlobalGameState->AssetManager = CreateAssetManager();
    GlobalGameState->EntityManager = CreateEntityManager();

    GlobalGameState->World = CreateWorld(2048);

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
  }

  if(!RenderCommands->MainRenderGroup)
  {
    RenderCommands->MainRenderGroup = InitiateRenderGroup((r32)RenderCommands->ScreenWidthPixels, (r32)RenderCommands->ScreenHeightPixels);
  }

  if(!RenderCommands->DebugRenderGroup)
  {
    // TODO: Right now DebugRenderGroup just solves the problem of drawing the overlay on top of everything else with a special shader.
    //       This should be supported by just one RenderGroup with sorting capabilities and shaderswitching.
    //       Maybe DebugRenderGroup is something we want to move away from and consolidate into the DebugState.
    //       Is there a problem with the debug system piping it's drawing through the GameRenderingPipeline?
    //       I mean it already sort of does since it all gets drawn in RenderGroupToOutput.
    RenderCommands->DebugRenderGroup = InitiateRenderGroup((r32)RenderCommands->ScreenWidthPixels, (r32)RenderCommands->ScreenHeightPixels);
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

//  RenderCommands->MainRenderGroup->ScreenWidth  = (r32)RenderCommands->ScreenWidthPixels;
//  RenderCommands->MainRenderGroup->ScreenHeight = (r32)RenderCommands->ScreenHeightPixels;
//
//  RenderCommands->DebugRenderGroup->ScreenWidth  = (r32) RenderCommands->ScreenWidthPixels;
//  RenderCommands->DebugRenderGroup->ScreenHeight = (r32) RenderCommands->ScreenHeightPixels;
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
