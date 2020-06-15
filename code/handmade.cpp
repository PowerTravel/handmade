/*
  TODO: AssetSystem.
    - Easy API to upload render assets to opengl
        * Per Render Asset: Must know if it's loaded into GPU and what it's handle is.
    - Easy API to add new assets
    - Just ask interface, I want a Cube with blue surface.
    - Render States. I want to render the cube filled with wire-mesh
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

world* AllocateWorld( u32 NrMaxEntities, u32 NumManifolds = 4 )
{
  world* World = PushStruct(GlobalGameState->PersistentArena, world);
  World->Arena = GlobalGameState->PersistentArena;

  World->NrEntities = 0;
  World->NrMaxEntities = NrMaxEntities;
  World->Entities = (entity*) PushArray( World->Arena, World->NrMaxEntities, entity );

  World->MaxNrManifolds = NumManifolds*NrMaxEntities;
  World->Manifolds = (contact_manifold*)  PushSize( World->Arena, World->MaxNrManifolds*( sizeof(contact_manifold) ));
  World->FirstContactManifold = 0;
  return World;
}

void CreateCollisionTestScene(game_state* GameState, game_input* Input)
{
  world* World = GameState->World;
  game_asset_manager* AssetManager = GlobalGameState->AssetManager;
  entity* ControllableCamera = NewEntity( World );
  NewComponents( World, ControllableCamera, COMPONENT_TYPE_CONTROLLER | COMPONENT_TYPE_CAMERA);

  r32 AspectRatio = GameState->ScreenWidthPixels / GameState->ScreenHeightPixels;
  r32 FieldOfView =  90;
  SetCameraComponent(ControllableCamera->CameraComponent, FieldOfView, AspectRatio );
  LookAt(ControllableCamera->CameraComponent, 1*V3(0,3,-8), V3(0,3,0));

  ControllableCamera->ControllerComponent->Controller = GetController(Input, 1);
  ControllableCamera->ControllerComponent->Type = ControllerType_FlyingCamera;
 
  entity* Light = NewEntity( World );
  NewComponents( World, Light, COMPONENT_TYPE_LIGHT | COMPONENT_TYPE_SPATIAL | COMPONENT_TYPE_RENDER);

  Light->LightComponent->Color = 0.6*V4(1,1,1,1);

  Light->SpatialComponent->Position = V3(1,1,1);
  Light->SpatialComponent->Scale    = V3( 0.3, 0.3, 0.3);

  Light->RenderComponent->AssetHandle = GetAssetHandle(GameState->AssetManager);
  SetAsset( GameState->AssetManager, asset_type::OBJECT,   "voxel", Light->RenderComponent->AssetHandle);
  SetAsset( GameState->AssetManager, asset_type::MATERIAL, "white", Light->RenderComponent->AssetHandle);  // May have a texture
  SetAsset( GameState->AssetManager, asset_type::BITMAP,   "null",  Light->RenderComponent->AssetHandle);  // Overrides texture


#define State4
#if defined(State1)
  // Bug - producing state. Gets a broken mesh somewhere
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
  u32 cubeAssetHandle = GetAssetHandle( GameState->AssetManager);
  SetAsset( AssetManager, asset_type::OBJECT, "cube1", cubeAssetHandle );
  SetAsset( AssetManager, asset_type::MATERIAL, "cube", cubeAssetHandle );
  SetAsset( AssetManager, asset_type::BITMAP, "cube_kd", cubeAssetHandle );
  for (s32 i = iarr[0]; i < iarr[1]; ++i)
  {
    for (s32 j = jarr[0]; j < jarr[1]; ++j)
    {
      for (s32 k = karr[0]; k < karr[1]; ++k)
      {
        entity* cubeEntity = NewEntity( World );
        NewComponents( World, cubeEntity,
          COMPONENT_TYPE_RENDER   |
          COMPONENT_TYPE_SPATIAL  |
          COMPONENT_TYPE_COLLIDER |
          COMPONENT_TYPE_DYNAMICS);

        cubeEntity->RenderComponent->AssetHandle = cubeAssetHandle;

        cubeEntity->ColliderComponent->AssetHandle = cubeAssetHandle;
        cubeEntity->ColliderComponent->AABB = GetMeshAABB(AssetManager, cubeAssetHandle);

        cubeEntity->SpatialComponent->Position =  V3(xzSpace*i, ySpace*j, xzSpace*k);
        cubeEntity->SpatialComponent->Rotation = RotateQuaternion( 0, V3(0,0,0) );
        cubeEntity->SpatialComponent->Scale = V3(1, 1, 1);

        cubeEntity->DynamicsComponent->LinearVelocity  = V3(0,0,0);
        cubeEntity->DynamicsComponent->AngularVelocity = V3(0,0,0);
        cubeEntity->DynamicsComponent->Mass = 2;
      }
    }
  }

  entity* FloorEntity = NewEntity( World );
  NewComponents( World, FloorEntity,
    COMPONENT_TYPE_RENDER   |
    COMPONENT_TYPE_SPATIAL  |
    COMPONENT_TYPE_COLLIDER);

  u32 FloorHandle = GetAssetHandle(AssetManager);
  FloorEntity->RenderComponent->AssetHandle = FloorHandle;
  SetAsset( AssetManager, asset_type::OBJECT, "cube1", FloorHandle);
  SetAsset( AssetManager, asset_type::MATERIAL, "cube", FloorHandle );
  SetAsset( AssetManager, asset_type::BITMAP, "cube_kd", FloorHandle );

  FloorEntity->ColliderComponent->AssetHandle = FloorHandle;
  FloorEntity->ColliderComponent->AABB = GetMeshAABB( AssetManager, FloorHandle);

  FloorEntity->SpatialComponent->Position = V3( 0,-2, 0);
  FloorEntity->SpatialComponent->Scale = V3( 18, 1, 18);

  entity* SpriteAnimationEntity = NewEntity( World );
  NewComponents( World, SpriteAnimationEntity,
    COMPONENT_TYPE_SPRITE_ANIMATION |
    COMPONENT_TYPE_RENDER           |
    COMPONENT_TYPE_SPATIAL);

  u32 HeroSpriteHandle = GetAssetHandle(AssetManager);
  SetAsset( AssetManager, asset_type::OBJECT, "quad", HeroSpriteHandle);
  SetAsset( AssetManager, asset_type::BITMAP, "hero_sprite_sheet", HeroSpriteHandle);
  SetAsset( AssetManager, asset_type::MATERIAL, "cube", HeroSpriteHandle );

  SpriteAnimationEntity->RenderComponent->AssetHandle = HeroSpriteHandle;

  SpriteAnimationEntity->SpatialComponent->Position = V3( -0,  8, 8);
  SpriteAnimationEntity->SpatialComponent->Rotation = RotateQuaternion( Pi32, V3(0,1,0) );
  SpriteAnimationEntity->SpatialComponent->Scale    = V3( 18, 18, 1);

  component_sprite_animation* SpriteAnimation = SpriteAnimationEntity->SpriteAnimationComponent;

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
    GlobalGameState->AssetManager = CreateAssetManager();
    GlobalGameState->TransientArena = PushStruct(GlobalGameState->PersistentArena, memory_arena);
    GlobalGameState->IsInitialized = true;

    GlobalGameState->TransientTempMem = BeginTemporaryMemory(GlobalGameState->TransientArena);

    LoadAssets(GlobalGameState->AssetManager);
    GlobalGameState->World = AllocateWorld(120, 32);

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

internal inline aabb2f
GetTimeBar(r32 TimeValue, r32 MaxTimeValue, r32 BarMaxLength, r32 BarWidth)
{
  r32 TimeFraction = TimeValue/MaxTimeValue;
  v2 P0 = V2(-1,1-BarWidth);
  v2 P1 = V2(-1 + TimeFraction * BarMaxLength, 1);
  aabb2f Result = AABB2f(P0,P1);
  return Result;
}

#include "debug.cpp"
