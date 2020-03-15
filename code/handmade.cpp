#include "handmade.h"

#include "random.h"

#include "aabb.cpp"
#include "handmade_tile.cpp"
#include "obj_loader.cpp"
#include "tiles_spritesheet.hpp"
#include "render_push_buffer.cpp"
#include "dynamic_aabb_tree.cpp"
#include "gjk_narrow_phase.cpp"
#include "epa_collision_data.cpp"
#include "entity_components.cpp"
#include "system_controller.cpp"
#include "system_sprite_animation.cpp"
#include "system_spatial.cpp"
#include "system_camera.cpp"

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


void CreateEpaVisualizerTestScene(thread_context* Thread, game_memory* Memory, game_render_commands* RenderCommands,  game_input* Input )
{
  game_state* GameState = Memory->GameState;
  memory_arena* AssetArena = &GameState->AssetArena;
  memory_arena* TemporaryArena = &GameState->TemporaryArena;

  GameState->World = AllocateWorld(20);
  world* World = GameState->World;
  World->TransientArena = TemporaryArena;

  GameState->World->Assets = (game_assets*) PushStruct(AssetArena, game_assets);
  game_assets* Assets = GameState->World->Assets;


  entity* Light = NewEntity( World );
  NewComponents( World, Light, COMPONENT_TYPE_LIGHT | COMPONENT_TYPE_SPATIAL );
  Light->LightComponent->Color = V4(5,5,5,1);
  Put( V3(0,2,4), 0, V3(0,1,0), Light->SpatialComponent );

  obj_loaded_file* cube = ReadOBJFile( Thread, GameState,
         Memory->PlatformAPI.DEBUGPlatformReadEntireFile,
         Memory->PlatformAPI.DEBUGPlatformFreeFileMemory,
         "..\\handmade\\data\\cube\\cube.obj");

  entity* CubeA = CreateEntityFromOBJGroup( World, &cube->Objects[0], cube->MeshData );
  SetMaterial(CubeA->SurfaceComponent->Material, MATERIAL_RED_RUBBER);
  Put( V3( 3,0,0), 0, V3(0,0,0), CubeA->SpatialComponent );
  Scale( V3(2, 2, 2),  CubeA->SpatialComponent );

  entity* CubeB = CreateEntityFromOBJGroup( World, &cube->Objects[0], cube->MeshData );
  SetMaterial(CubeB->SurfaceComponent->Material, MATERIAL_BLUE_RUBBER);
  Put( V3( 3,0,1), Pi32/4, V3(0,0,1), CubeB->SpatialComponent );
  Scale( V3(1, 1, 1),  CubeB->SpatialComponent );

  NewComponents( World, CubeA, COMPONENT_TYPE_GJK_EPA_VISUALIZER | COMPONENT_TYPE_CONTROLLER );

  CubeA->GjkEpaVisualizerComponent->MaxIndexCount = 1024;
  CubeA->GjkEpaVisualizerComponent->Indeces = (u32*) PushArray(AssetArena, CubeA->GjkEpaVisualizerComponent->MaxIndexCount, u32);

  CubeA->GjkEpaVisualizerComponent->MaxVertexCount = 1024;
  CubeA->GjkEpaVisualizerComponent->Vertices = (v3*) PushArray(AssetArena, CubeA->GjkEpaVisualizerComponent->MaxVertexCount, v3);

  CubeA->GjkEpaVisualizerComponent->NormalMaxIndexCount = 1024;
  CubeA->GjkEpaVisualizerComponent->NormalIndeces = (u32*) PushArray(AssetArena, CubeA->GjkEpaVisualizerComponent->NormalMaxIndexCount, u32);

  CubeA->GjkEpaVisualizerComponent->MaxNormalCount = 1024;
  CubeA->GjkEpaVisualizerComponent->Normals = (v3*) PushArray(AssetArena, CubeA->GjkEpaVisualizerComponent->MaxNormalCount, v3);

  CubeA->GjkEpaVisualizerComponent->MaxSimplexCount = 16;
  CubeA->GjkEpaVisualizerComponent->Simplex = (simplex_index*) PushArray(AssetArena, CubeA->GjkEpaVisualizerComponent->MaxSimplexCount, simplex_index);

  CubeA->GjkEpaVisualizerComponent->MaxEPACount = 32;
  CubeA->GjkEpaVisualizerComponent->EPA = (epa_index*) PushArray(AssetArena, CubeA->GjkEpaVisualizerComponent->MaxEPACount, epa_index);;

  CubeA->ControllerComponent->Controller = GetController(Input, 1);
  CubeA->ControllerComponent->ControllerMappingFunction = EpaGjkVisualizerController;

  entity* ControllableCamera = NewEntity( World );
  NewComponents( World, ControllableCamera, COMPONENT_TYPE_CONTROLLER | COMPONENT_TYPE_CAMERA);

  r32 AspectRatio = (r32)RenderCommands->Width / (r32) RenderCommands->Height;
  r32 FieldOfView =  90;
  SetCameraComponent(ControllableCamera->CameraComponent, FieldOfView, AspectRatio );
  LookAt(ControllableCamera->CameraComponent, 3*V3(0,3,8), V3(0,3,0));
  ControllableCamera->ControllerComponent->Controller = GetController(Input, 1);
  ControllableCamera->ControllerComponent->ControllerMappingFunction = FlyingCameraController;
}

void CreateCollisionTestScene(thread_context* Thread, game_memory* Memory, game_render_commands* RenderCommands, game_input* Input)
{
  game_state* GameState        = Memory->GameState;
  memory_arena* AssetArena     = &GameState->AssetArena;
  memory_arena* TemporaryArena = &GameState->TemporaryArena;

  GameState->World = AllocateWorld(1000);
  world* World = GameState->World;
  World->TransientArena = TemporaryArena;

  GameState->World->Assets = (game_assets*) PushStruct(AssetArena, game_assets);
  game_assets* Assets = GameState->World->Assets;

  obj_loaded_file* cube = ReadOBJFile( Thread, GameState,
         Memory->PlatformAPI.DEBUGPlatformReadEntireFile,
         Memory->PlatformAPI.DEBUGPlatformFreeFileMemory,
         "..\\handmade\\data\\cube\\cube.obj");

  entity* Light = NewEntity( World );
  NewComponents( World, Light, COMPONENT_TYPE_LIGHT | COMPONENT_TYPE_SPATIAL );
  Light->LightComponent->Color = V4(3,3,3,1);
  Put( V3(10,10,10), 0, V3(0,1,0), Light->SpatialComponent );

  for (s32 i = -0; i < 1; ++i)
  {
    for (s32 j = 0; j < 1; ++j)
    {
      for (s32 k = -0; k < 1; ++k)
      {
        entity* cubeEntity = CreateEntityFromOBJGroup( World, &cube->Objects[0], cube->MeshData );
        NewComponents( World, cubeEntity, COMPONENT_TYPE_DYNAMICS );

        // Uncomment this and set j < 1 in the for loop to reproduce a bug where
        // GJK perodically does not find a collision.
        //Put( V3(2.1f*i, 2.f*j, 2.1f*k), (Pi32/4), V3(1,2,1), cubeEntity->SpatialComponent );
        Put( V3(1.f*i, 1.0f*j, 1.f*k), 0, V3(1,1,1), cubeEntity->SpatialComponent );
        cubeEntity->DynamicsComponent->LinearVelocity  = V3(0,0,0.1);
        cubeEntity->DynamicsComponent->AngularVelocity = V3(0,0,0);
        cubeEntity->DynamicsComponent->Mass = 1;
      }
    }
  }

  entity* floor = CreateEntityFromOBJGroup( World, &cube->Objects[1], cube->MeshData );

  Put( V3( 0,-1, 0), 0, V3(0,1,0), floor->SpatialComponent );
  Scale( V3( 18, 1, 18),  floor->SpatialComponent );

#if 1
  NewComponents( World, floor, COMPONENT_TYPE_GJK_EPA_VISUALIZER | COMPONENT_TYPE_CONTROLLER );
  floor->GjkEpaVisualizerComponent->MaxIndexCount = 1024;
  floor->GjkEpaVisualizerComponent->Indeces = (u32*) PushArray(AssetArena, floor->GjkEpaVisualizerComponent->MaxIndexCount, u32);

  floor->GjkEpaVisualizerComponent->MaxVertexCount = 1024;
  floor->GjkEpaVisualizerComponent->Vertices = (v3*) PushArray(AssetArena, floor->GjkEpaVisualizerComponent->MaxVertexCount, v3);

  floor->GjkEpaVisualizerComponent->NormalMaxIndexCount = 1024;
  floor->GjkEpaVisualizerComponent->NormalIndeces = (u32*) PushArray(AssetArena, floor->GjkEpaVisualizerComponent->NormalMaxIndexCount, u32);

  floor->GjkEpaVisualizerComponent->MaxNormalCount = 1024;
  floor->GjkEpaVisualizerComponent->Normals = (v3*) PushArray(AssetArena, floor->GjkEpaVisualizerComponent->MaxNormalCount, v3);

  floor->GjkEpaVisualizerComponent->MaxSimplexCount = 16;
  floor->GjkEpaVisualizerComponent->Simplex = (simplex_index*) PushArray(AssetArena, floor->GjkEpaVisualizerComponent->MaxSimplexCount, simplex_index);

  floor->GjkEpaVisualizerComponent->MaxEPACount = 32;
  floor->GjkEpaVisualizerComponent->EPA = (epa_index*) PushArray(AssetArena, floor->GjkEpaVisualizerComponent->MaxEPACount, epa_index);

  floor->ControllerComponent->Controller = GetController(Input, 1);
  floor->ControllerComponent->ControllerMappingFunction = EpaGjkVisualizerController;
#endif
  entity* ControllableCamera = NewEntity( World );
  NewComponents( World, ControllableCamera, COMPONENT_TYPE_CONTROLLER | COMPONENT_TYPE_CAMERA);

  r32 AspectRatio = (r32)RenderCommands->Width / (r32) RenderCommands->Height;
  r32 FieldOfView =  90;
  SetCameraComponent(ControllableCamera->CameraComponent, FieldOfView, AspectRatio );
  LookAt(ControllableCamera->CameraComponent, 1*V3(3,3,3), V3(0,0,0));

  ControllableCamera->ControllerComponent->Controller = GetController(Input, 1);
  ControllableCamera->ControllerComponent->ControllerMappingFunction = FlyingCameraController;
}

void Create2DScene(thread_context* Thread, game_memory* Memory, game_render_commands* RenderCommands,  game_input* Input )
{
  game_state* GameState = Memory->GameState;
  memory_arena* AssetArena = &GameState->AssetArena;
  memory_arena* TemporaryArena = &GameState->TemporaryArena;

  GameState->World = AllocateWorld(100);
  world* World = GameState->World;
  World->TransientArena = TemporaryArena;

  entity* Light = NewEntity( World );
  NewComponents( World, Light, COMPONENT_TYPE_LIGHT | COMPONENT_TYPE_SPATIAL );
  Light->LightComponent->Color = V4(1,1,1,1);
  Light->SpatialComponent->ModelMatrix = M4Identity();
  Translate( V3(3,3,3), Light->SpatialComponent );

  GameState->World->Assets = (game_assets*) PushStruct(AssetArena, game_assets);
  game_assets* Assets = GameState->World->Assets;

  entity* Player = NewEntity( World );

  NewComponents( World, Player, COMPONENT_TYPE_CONTROLLER |
                                COMPONENT_TYPE_SPATIAL | COMPONENT_TYPE_COLLIDER | COMPONENT_TYPE_DYNAMICS |
                                COMPONENT_TYPE_SPRITE_ANIMATION |
                                COMPONENT_TYPE_CAMERA);

  r32 AspectRatio = (r32)RenderCommands->Width / (r32) RenderCommands->Height;
  r32 FieldOfView =  90;
  SetCameraComponent(Player->CameraComponent, FieldOfView, AspectRatio );
  LookAt(Player->CameraComponent, 3*V3(0,0,1), V3(0,0,0));

  Player->ControllerComponent->Controller = GetController(Input, 1);
  Player->ControllerComponent->ControllerMappingFunction = HeroController;

  Put( V3(0,3,0), 0, V3(0,1,0), Player->SpatialComponent );
  Player->ColliderComponent->AABB = AABB3f( V3(-0.5,-0.5,0), V3(0.5,0.5,0) );
  SetColliderMeshFromAABB(&World->PersistentArena, Player->ColliderComponent);

  Player->DynamicsComponent->LinearVelocity  = V3(0,0,0);
  Player->DynamicsComponent->AngularVelocity = V3(0,0,0);
  Player->DynamicsComponent->Mass = 1;

  component_sprite_animation* SpriteAnimation = Player->SpriteAnimationComponent;

  Assets->HeroSpriteSheet.bitmap = LoadTGA( Thread, &GameState->AssetArena,
         Memory->PlatformAPI.DEBUGPlatformReadEntireFile,
         Memory->PlatformAPI.DEBUGPlatformFreeFileMemory,
        "..\\handmade\\data\\Platformer\\Adventurer\\adventurer-Sheet.tga" );
  bitmap* HeroSpriteSheet = Assets->HeroSpriteSheet.bitmap;


  hash_map<bitmap_coordinate> HeroCoordinates = LoadAdventurerSpriteSheetCoordinates( TemporaryArena );
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

void InitiateGame(thread_context* Thread, game_memory* Memory, game_render_commands* RenderCommands, game_input* Input )
{
  if (!Memory->GameState)
  {
    Memory->GameState = BootstrapPushStruct(game_state, AssetArena);

    //Create2DScene(Thread, Memory, RenderCommands, Input );
    CreateCollisionTestScene(Thread, Memory, RenderCommands, Input );
    //CreateEpaVisualizerTestScene(Thread, Memory, RenderCommands, Input );
    for (s32 ControllerIndex = 0;
    ControllerIndex < ArrayCount(Input->Controllers);
      ++ControllerIndex)
    {
      game_controller_input* Controller = GetController(Input, ControllerIndex);
      Controller->IsAnalog = true;
    }

  }
}

/*
  Note:
  extern "C" prevents the C++ compiler from renaming the functions which it does for function-overloading reasons (among other things) by forcing it to use C conventions which does not support overloading. Also called 'name mangling' or 'name decoration'. The actual function names are visible in the outputted .map file in the build directory
*/
// Signature is
//void game_update_and_render (thread_context* Thread,
//                game_memory* Memory,
//                render_commands* RenderCommands,
//                game_input* Input )

s32 eqFun( const s32* A, const s32* B)
{
  return *A-*B;
}

platform_api Platform;

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
  Platform = Memory->PlatformAPI;
  InitiateGame(Thread, Memory, RenderCommands, Input);

  game_state* GameState = Memory->GameState;
  world* World = GameState->World;
  World->dtForFrame = Input->dt;
  World->GlobalTimeSec += Input->dt;

  ControllerSystemUpdate(GameState->World);
  SpatialSystemUpdate(GameState->World, &Memory->PlatformAPI);
  CameraSystemUpdate(GameState->World);
  SpriteAnimationSystemUpdate(GameState->World);

  FillRenderPushBuffer( World, RenderCommands );

  CheckArena(&GameState->TemporaryArena);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
  GameOutputSound(SoundBuffer, 400);
}
