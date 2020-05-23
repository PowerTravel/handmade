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

#if HANDMADE_INTERNAL
game_memory* DebugGlobalMemory = 0;
#endif

game_state* GlobalGameState;
platform_api Platform;

#include "gjk_epa_visualizer.h"
gjk_epa_visualizer GlobalGjkEpaVisualizer = {};
gjk_epa_visualizer* GlobalVis;
global_variable b32 GlobalFireVic = false;


#include "math/aabb.cpp"
#include "handmade_tile.cpp"
#include "obj_loader.cpp"
#include "render_push_buffer.cpp"
#include "dynamic_aabb_tree.cpp"
#include "gjk_narrow_phase.cpp"
#if 1
#include "epa_collision_data.cpp"
#include "halfedge_mesh.cpp"
#else
#include "epa_collision_data_old.cpp"
#endif

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

#if 0
void CreateEpaVisualizerTestScene(thread_context* Thread, game_memory* Memory, game_render_commands* RenderCommands,  game_input* Input )
{
  game_state* GameState = Memory->GameState;
  memory_arena* AssetArena = &GameState->AssetArena;
  memory_arena* TransientArena = &GameState->TransientArena;

  AllocateWorld(20, GameState);
  world* World = GameState->World;
  game_assets* Assets = World->Assets;


  entity* Light = NewEntity( World );
  NewComponents( World, Light, COMPONENT_TYPE_LIGHT | COMPONENT_TYPE_SPATIAL );
  Light->LightComponent->Color = V4(5,5,5,1);
  Light->SpatialComponent->Position = V3(0,2,4);

  obj_loaded_file* cube = ReadOBJFile( Thread, GameState,
         Platform.DEBUGPlatformReadEntireFile,
         Platform.DEBUGPlatformFreeFileMemory,
         "..\\handmade\\data\\cube\\cube.obj");

  entity* CubeA = CreateEntityFromOBJGroup( World, &cube->Objects[0], cube->MeshData );
  SetMaterial(CubeA->SurfaceComponent->Material, MATERIAL_RED_RUBBER);
  CubeA->SpatialComponent->Position = V3( 3,0,0);
  CubeA->SpatialComponent->Scale = V3(2, 2, 2);

  entity* CubeB = CreateEntityFromOBJGroup( World, &cube->Objects[0], cube->MeshData );
  SetMaterial(CubeB->SurfaceComponent->Material, MATERIAL_BLUE_RUBBER);
  CubeB->SpatialComponent->Position = V3(3,0,1);
  CubeB->SpatialComponent->Rotation = RotateQuaternion( Pi32/4.f, V3(0,0,1) );
  CubeB->SpatialComponent->Scale = V3(2, 2, 2);

  NewComponents( World, CubeA, COMPONENT_TYPE_CONTROLLER );

  CubeA->ControllerComponent->Controller = GetController(Input, 1);
  CubeA->ControllerComponent->Type = ControllerType_EpaGjkVisualizer;

  entity* ControllableCamera = NewEntity( World );
  NewComponents( World, ControllableCamera, COMPONENT_TYPE_CONTROLLER | COMPONENT_TYPE_CAMERA);

  r32 AspectRatio = (r32)RenderCommands->ResolutionWidthPixels / (r32) RenderCommands->ResolutionHeightPixels;
  r32 FieldOfView =  90;
  SetCameraComponent(ControllableCamera->CameraComponent, FieldOfView, AspectRatio );
  LookAt(ControllableCamera->CameraComponent, 1*V3(0,3,8), V3(0,3,0));
  ControllableCamera->ControllerComponent->Controller = GetController(Input, 1);
  ControllableCamera->ControllerComponent->Type = ControllerType_FlyingCamera;
}
#endif

void CreateCollisionTestScene(game_state* GameState, game_input* Input)
{
  world* World = GameState->World;

  // TODO: Create a better way to ask for assets than giving known array-indeces
  //       Maybe Enums?
  const u32 CubeIndex0 = 1; // CubeIndex0 and CubeIndex1 have different texture indeces
  const u32 CubeIndex1 = 2;
  const u32 CubeTextureIndex = 30;

  entity* Light = NewEntity( World );
  NewComponents( World, Light, COMPONENT_TYPE_LIGHT | COMPONENT_TYPE_SPATIAL );
  Light->LightComponent->Color = V4(3,3,3,1);
  Light->SpatialComponent->Position = V3(10,10,10);

#define State3
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

        cubeEntity->RenderComponent->MeshHandle    = GetMeshAssetHandle(CubeIndex0);
        cubeEntity->RenderComponent->TextureHandle = GetTextureAssetHandle(RollingIndex++ % CubeTextureIndex);

        cubeEntity->ColliderComponent->MeshHandle = GetMeshAssetHandle(CubeIndex0);
        cubeEntity->ColliderComponent->AABB = GetMeshAABB(cubeEntity->ColliderComponent->MeshHandle);

        cubeEntity->SpatialComponent->Position =  V3(xzSpace*i, ySpace*j, xzSpace*k);
        cubeEntity->SpatialComponent->Rotation = RotateQuaternion( 0, V3(0,0,0) );
        cubeEntity->SpatialComponent->Scale = V3(1, 1, 1);

        cubeEntity->DynamicsComponent->LinearVelocity  = V3(0,0,0);
        cubeEntity->DynamicsComponent->AngularVelocity = V3(0,0,0);
        cubeEntity->DynamicsComponent->Mass = 2;
      }
    }
  }

  entity* floorEntity = NewEntity( World );
  NewComponents( World, floorEntity,
    COMPONENT_TYPE_RENDER   |
    COMPONENT_TYPE_SPATIAL  |
    COMPONENT_TYPE_COLLIDER);

  floorEntity->RenderComponent->MeshHandle    = GetMeshAssetHandle(CubeIndex1);
  floorEntity->RenderComponent->TextureHandle = GetTextureAssetHandle(CubeTextureIndex);

  floorEntity->ColliderComponent->MeshHandle = GetMeshAssetHandle(CubeIndex1);
  floorEntity->ColliderComponent->AABB = GetMeshAABB(floorEntity->ColliderComponent->MeshHandle);

  floorEntity->SpatialComponent->Position = V3( 0,-2, 0);
  floorEntity->SpatialComponent->Scale = V3( 18, 1, 18);

#if 0
  NewComponents( World, floor, COMPONENT_TYPE_CONTROLLER );
  floor->ControllerComponent->Controller = GetController(Input, 1);
  floor->ControllerComponent->Type = ControllerType_EpaGjkVisualizer;
#endif

  entity* ControllableCamera = NewEntity( World );
  NewComponents( World, ControllableCamera, COMPONENT_TYPE_CONTROLLER | COMPONENT_TYPE_CAMERA);

  r32 AspectRatio = GameState->ScreenWidthPixels / GameState->ScreenHeightPixels;
  r32 FieldOfView =  90;
  SetCameraComponent(ControllableCamera->CameraComponent, FieldOfView, AspectRatio );
  LookAt(ControllableCamera->CameraComponent, 1*V3(3,3,3), V3(0,0,0));

  ControllableCamera->ControllerComponent->Controller = GetController(Input, 1);
  ControllableCamera->ControllerComponent->Type = ControllerType_FlyingCamera;
}

#if 0
void Create2DScene(thread_context* Thread, game_memory* Memory, game_render_commands* RenderCommands,  game_input* Input )
{
  game_state* GameState = Memory->GameState;
  memory_arena* AssetArena = &GameState->AssetArena;
  memory_arena* TransientArena = &GameState->TransientArena;

  AllocateWorld(100, GameState);
  InitializeTileMap( &GameState->World->TileMap );
  world* World = GameState->World;
  game_assets* Assets = World->Assets;

  entity* Light = NewEntity( World );
  NewComponents( World, Light, COMPONENT_TYPE_LIGHT | COMPONENT_TYPE_SPATIAL );
  Light->LightComponent->Color = V4(1,1,1,1);
  Light->SpatialComponent->Position =  V3(3,3,3);

  entity* Player = NewEntity( World );

  NewComponents( World, Player, COMPONENT_TYPE_CONTROLLER |
                                COMPONENT_TYPE_SPATIAL | COMPONENT_TYPE_COLLIDER | COMPONENT_TYPE_DYNAMICS |
                                COMPONENT_TYPE_SPRITE_ANIMATION |
                                COMPONENT_TYPE_CAMERA);

  r32 AspectRatio = (r32)RenderCommands->ResolutionWidthPixels / (r32) RenderCommands->ResolutionHeightPixels;
  r32 FieldOfView =  90;
  SetCameraComponent(Player->CameraComponent, FieldOfView, AspectRatio );
  LookAt(Player->CameraComponent, 3*V3(0,0,1), V3(0,0,0));

  Player->ControllerComponent->Controller = GetController(Input, 1);
  Player->ControllerComponent->Type = ControllerType_Hero;

  Player->SpatialComponent->Position = V3(0,3,0);
  Player->ColliderComponent->AABB = AABB3f( V3(-0.5,-0.5,0), V3(0.5,0.5,0) );
  SetColliderMeshFromAABB(World->PersistentArena, Player->ColliderComponent);

  Player->DynamicsComponent->LinearVelocity  = V3(0,0,0);
  Player->DynamicsComponent->AngularVelocity = V3(0,0,0);
  Player->DynamicsComponent->Mass = 1;

  component_sprite_animation* SpriteAnimation = Player->SpriteAnimationComponent;

  Assets->HeroSpriteSheet.bitmap = LoadTGA( Thread, &GameState->AssetArena,
         Memory->PlatformAPI.DEBUGPlatformReadEntireFile,
         Memory->PlatformAPI.DEBUGPlatformFreeFileMemory,
        "..\\handmade\\data\\Platformer\\Adventurer\\adventurer-Sheet.tga" );
  bitmap* HeroSpriteSheet = Assets->HeroSpriteSheet.bitmap;

  hash_map<bitmap_coordinate> HeroCoordinates = LoadAdventurerSpriteSheetCoordinates( TransientArena );
  SpriteAnimation->Bitmap = Assets->HeroSpriteSheet.bitmap;
  SpriteAnimation->Animation = hash_map< list<m4> >(AssetArena,6);

  list<m4> Idle1(AssetArena);
  Idle1.First();
  Idle1.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("idle1_01")));
  Idle1.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("idle1_02")));
  Idle1.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("idle1_03")));
  Idle1.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("idle1_04")));
  SpriteAnimation->Animation.Insert("idle1",  Idle1);

  list<m4> Idle2(AssetArena);
  Idle2.First();
  Idle2.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("idle2_01")));
  Idle2.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("idle2_02")));
  Idle2.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("idle2_03")));
  Idle2.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("idle2_04")));
  SpriteAnimation->Animation.Insert("idle2",  Idle2);

  list<m4> Run(AssetArena);
  Run.First();
  Run.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("run_01")));
  Run.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("run_02")));
  Run.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("run_03")));
  Run.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("run_04")));
  Run.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("run_05")));
  Run.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("run_06")));
  SpriteAnimation->Animation.Insert("run",  Run);

  list<m4> Jump(AssetArena);
  Jump.First();
  Jump.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("jump_01")));
  Jump.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("jump_02")));
  Jump.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("jump_03")));
  SpriteAnimation->Animation.Insert("jump",  Jump);

  list<m4> Fall(AssetArena);
  Fall.First();
  Fall.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("fall_01")));
  Fall.InsertAfter(GetSpriteSheetTranslationMatrix(HeroSpriteSheet, HeroCoordinates.Get("fall_02")));
  SpriteAnimation->Animation.Insert("fall",  Fall);

  SpriteAnimation->ActiveSeries = SpriteAnimation->Animation.Get("idle1");


  Assets->TileMapSpriteSheet.bitmap = LoadTGA( Thread, &GameState->AssetArena,
         Memory->PlatformAPI.DEBUGPlatformReadEntireFile,
         Memory->PlatformAPI.DEBUGPlatformFreeFileMemory,
        "..\\handmade\\data\\Platformer\\tga\\tiles_spritesheet.tga" );

  bitmap* TileMap = LoadTGA( Thread, &GameState->AssetArena,
         Memory->PlatformAPI.DEBUGPlatformReadEntireFile,
         Memory->PlatformAPI.DEBUGPlatformFreeFileMemory,
         "..\\handmade\\data\\Platformer\\tga\\TileMap.tga" );

  hash_map<bitmap_coordinate> SpriteCoordinates = LoadTileSpriteSheetCoordinates(AssetArena);
  bitmap* SpriteSheet = Assets->TileMapSpriteSheet.bitmap;
  for (u32 X = 0; X < TileMap->Width; ++X)
  {
    for (u32 Y = 0; Y < TileMap->Height; ++Y)
    {
      // Check IF Alpha Value exists
      u8 A,R,G,B;
      u32  CurrentPixel = 0;
      if(!GetPixelValue(TileMap, X, Y, &CurrentPixel)) break;

      SplitPixelIntoARGBComponents(CurrentPixel,&A,&R,&G,&B);

      if(!A) break;

      tile_contents TileContents = {};
      TileContents.Bitmap = SpriteSheet;
      if( G == 0xff )
      {
        b32 ToLeft  = false;
        b32 ToRight = false;
        TileContents.Type = TILE_TYPE_FLOOR;

        u32 LeftPixelValue = 0;
        if( GetPixelValue(TileMap, X+1, Y, &LeftPixelValue) )
        {
          SplitPixelIntoARGBComponents(LeftPixelValue,&A,&R,&G,&B);
          if( G == 0xff )
          {
            ToLeft = true;
          }
        }

        u32 RightPixelValue = 0;
        if( GetPixelValue(TileMap, X-1, Y, &RightPixelValue) )
        {
          SplitPixelIntoARGBComponents(RightPixelValue,&A,&R,&G,&B);
          if( G == 0xff )
          {
            ToRight = true;
          }
        }

        if(ToLeft && ToRight)
        {
          TileContents.TM = GetSpriteSheetTranslationMatrix(SpriteSheet, SpriteCoordinates.Get("grassMid"));
        }else if(ToLeft && !ToRight)
        {
          TileContents.TM = GetSpriteSheetTranslationMatrix(SpriteSheet, SpriteCoordinates.Get("grassLeft"));
        }else if(!ToLeft && ToRight)
        {
          TileContents.TM = GetSpriteSheetTranslationMatrix(SpriteSheet, SpriteCoordinates.Get("grassRight"));
        }else if(!ToLeft && !ToRight)
        {
          TileContents.TM = GetSpriteSheetTranslationMatrix(SpriteSheet, SpriteCoordinates.Get("grass"));
        }
        SetTileContentsAbs(&GameState->AssetArena, &World->TileMap, X, Y, 0, TileContents );
      }
    }
  }

}
#endif


void InitiateGame(game_memory* Memory, game_render_commands* RenderCommands, game_input* Input )
{
  if (!Memory->GameState)
  {
    GlobalGameState = BootstrapPushStruct(game_state, PersistentArena);
    Memory->GameState = GlobalGameState;
    Memory->GameState->TransientArena = PushStruct(Memory->GameState->PersistentArena, memory_arena);
    Memory->GameState->IsInitialized = true;

    Memory->GameState->TransientTempMem = BeginTemporaryMemory(Memory->GameState->TransientArena);

    Memory->GameState->AssetManager = CreateAssetManager();
    Memory->GameState->World = AllocateWorld(120, 32);

    Memory->GameState->ScreenWidthPixels = (r32)RenderCommands->ResolutionWidthPixels;
    Memory->GameState->ScreenHeightPixels = (r32)RenderCommands->ResolutionHeightPixels;

    CreateCollisionTestScene(Memory->GameState, Input);    

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
