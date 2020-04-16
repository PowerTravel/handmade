/*
  TODO: AssetSystem.
    - Easy API to upload render assets to opengl
        * Per Render Asset: Must know if it's loaded into GPU and what it's handle is.
    - Easy API to add new assets
    - Just ask interface, I want a Cube with blue surface.
    - Render States. I want to render the cube filled with wire-mesh
  TODO: Multi Threading

*/

#include "handmade.h"

#include "random.h"
#include "tiles_spritesheet.h"

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

#include "debug.h"

internal stb_font_map STBBakeFont(memory_arena* Memory)
{
  const s32 Ranges[] =
  {
      0x20, 0x80,       // BasicLatin
      0xA1, 0x100,      // ExtendedLatin
      0x16A0, 0x1700,   // Runic
      0x600, 0x700,     // Arabic
      0x370, 0x400,     // Greek
      0x2200, 0x2300    // Mathematical
  };

  thread_context Thread;
  debug_read_file_result TTFFile = Platform.DEBUGPlatformReadEntireFile(&Thread, "C:\\Windows\\Fonts\\courbd.ttf");

  Assert(TTFFile.Contents);
  stb_font_map Result = {};
  Result.StartChar = Ranges[0];
  Result.NumChars = Ranges[1] - Ranges[0];
  Result.FontHeightPx = 24.f;
  Result.CharData = PushArray(Memory, Result.NumChars, stbtt_bakedchar);

  stbtt_GetScaledFontVMetrics((u8*) TTFFile.Contents, stbtt_GetFontOffsetForIndex((u8*) TTFFile.Contents, 0),
   Result.FontHeightPx, &Result.Ascent, &Result.Descent, &Result.LineGap);

  Result.BitMap.BPP = 8;
  Result.BitMap.Width = 1028;
  Result.BitMap.Height = 1028;
  Result.BitMap.Pixels = (void*) PushArray(Memory, Result.BitMap.Width * Result.BitMap.Height, u8);
  s32 ret = stbtt_BakeFontBitmap((u8*) TTFFile.Contents, stbtt_GetFontOffsetForIndex((u8*) TTFFile.Contents, 0),
                       Result.FontHeightPx,
                       (u8*)Result.BitMap.Pixels, Result.BitMap.Width, Result.BitMap.Height,
                       Result.StartChar, Result.NumChars,
                       Result.CharData);
  Assert(ret>0);

  Platform.DEBUGPlatformFreeFileMemory(&Thread, TTFFile.Contents);

  return Result;
}

void AllocateGlobaklGjkEpaVisualizer(memory_arena* AssetArena)
{
  GlobalGjkEpaVisualizer.MaxIndexCount = 1024;
  GlobalGjkEpaVisualizer.Indeces = (u32*) PushArray(AssetArena, GlobalGjkEpaVisualizer.MaxIndexCount, u32);
  GlobalGjkEpaVisualizer.MaxVertexCount = 1024;
  GlobalGjkEpaVisualizer.Vertices = (v3*) PushArray(AssetArena, GlobalGjkEpaVisualizer.MaxVertexCount, v3);
  GlobalGjkEpaVisualizer.NormalMaxIndexCount = 1024;
  GlobalGjkEpaVisualizer.NormalIndeces = (u32*) PushArray(AssetArena, GlobalGjkEpaVisualizer.NormalMaxIndexCount, u32);
  GlobalGjkEpaVisualizer.MaxNormalCount = 1024;
  GlobalGjkEpaVisualizer.Normals = (v3*) PushArray(AssetArena, GlobalGjkEpaVisualizer.MaxNormalCount, v3);
  GlobalGjkEpaVisualizer.MaxSimplexCount = 16;
  GlobalGjkEpaVisualizer.Simplex = (simplex_index*) PushArray(AssetArena, GlobalGjkEpaVisualizer.MaxSimplexCount, simplex_index);
  GlobalGjkEpaVisualizer.MaxEPACount = 32;
  GlobalGjkEpaVisualizer.EPA = (epa_index*) PushArray(AssetArena, GlobalGjkEpaVisualizer.MaxEPACount, epa_index);
}

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

void AllocateWorld( u32 NrMaxEntities, game_state* GameState, u32 NumManifolds = 4 )
{
  GameState->World = (world*) PushStruct(&GameState->PersistentArena, world);
  GameState->World->AssetArena      = &GameState->AssetArena;
  GameState->World->PersistentArena = &GameState->PersistentArena;
  GameState->World->TransientArena  = &GameState->TransientArena;

  GameState->World->NrEntities = 0;
  GameState->World->NrMaxEntities = NrMaxEntities;
  GameState->World->Entities = (entity*) PushArray( &GameState->PersistentArena, GameState->World->NrMaxEntities, entity );

  GameState->World->MaxNrManifolds = NumManifolds*NrMaxEntities;
  GameState->World->Manifolds = (contact_manifold*)  PushSize( &GameState->PersistentArena, GameState->World->MaxNrManifolds*( sizeof(contact_manifold) ));
  GameState->World->FirstContactManifold = 0;
  GameState->World->Assets = PushStruct(GameState->World->AssetArena, game_assets);
}

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

  r32 AspectRatio = (r32)RenderCommands->Width / (r32) RenderCommands->Height;
  r32 FieldOfView =  90;
  SetCameraComponent(ControllableCamera->CameraComponent, FieldOfView, AspectRatio );
  LookAt(ControllableCamera->CameraComponent, 1*V3(0,3,8), V3(0,3,0));
  ControllableCamera->ControllerComponent->Controller = GetController(Input, 1);
  ControllableCamera->ControllerComponent->Type = ControllerType_FlyingCamera;
}

void CreateCollisionTestScene(thread_context* Thread, game_memory* Memory, game_render_commands* RenderCommands, game_input* Input)
{
  game_state* GameState        = Memory->GameState;
  memory_arena* AssetArena     = &GameState->AssetArena;
  memory_arena* TransientArena = &GameState->TransientArena;

  AllocateWorld(120, GameState, 32);
  world* World = GameState->World;
  game_assets* Assets = World->Assets;

  obj_loaded_file* cube = ReadOBJFile( Thread, GameState,
         Memory->PlatformAPI.DEBUGPlatformReadEntireFile,
         Memory->PlatformAPI.DEBUGPlatformFreeFileMemory,
         "..\\handmade\\data\\cube\\cube.obj");

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
#else
  r32 ySpace = 1;
  r32 xzSpace = 1.2;
  s32 iarr[] = {-2,2};
  s32 jarr[] = {-0,3};
  s32 karr[] = {-2,2};
#endif
  for (s32 i = iarr[0]; i < iarr[1]; ++i)
  {
    for (s32 j = jarr[0]; j < jarr[1]; ++j)
    {
      for (s32 k = karr[0]; k < karr[1]; ++k)
      {
        entity* cubeEntity = CreateEntityFromOBJGroup( World, &cube->Objects[0], cube->MeshData );
        NewComponents( World, cubeEntity, COMPONENT_TYPE_DYNAMICS );

        // Uncomment this and set j < 1 in the for loop to reproduce a bug where
        // GJK perodically does not find a collision.
        //Put( V3(2.1f*i, 2.f*j, 2.1f*k), (Pi32/4), V3(1,2,1), cubeEntity->SpatialComponent );
        cubeEntity->SpatialComponent->Position =  V3(xzSpace*i, ySpace*j, xzSpace*k);
        cubeEntity->SpatialComponent->Rotation = RotateQuaternion( 0, V3(0,0,0) );
        cubeEntity->SpatialComponent->Scale = V3(1, 1, 1);
        cubeEntity->DynamicsComponent->LinearVelocity  = V3(0,0,0);
        cubeEntity->DynamicsComponent->AngularVelocity = V3(0,0,0);
        cubeEntity->DynamicsComponent->Mass = 2;
      }
    }
  }

  entity* floor = CreateEntityFromOBJGroup( World, &cube->Objects[1], cube->MeshData );
  floor->SpatialComponent->Position = V3( 0,-2, 0);
  floor->SpatialComponent->Scale = V3( 18, 1, 18);

#if 1
  NewComponents( World, floor, COMPONENT_TYPE_CONTROLLER );
  floor->ControllerComponent->Controller = GetController(Input, 1);
  floor->ControllerComponent->Type = ControllerType_EpaGjkVisualizer;
#endif


  entity* ControllableCamera = NewEntity( World );
  NewComponents( World, ControllableCamera, COMPONENT_TYPE_CONTROLLER | COMPONENT_TYPE_CAMERA);

  r32 AspectRatio = (r32)RenderCommands->Width / (r32) RenderCommands->Height;
  r32 FieldOfView =  90;
  SetCameraComponent(ControllableCamera->CameraComponent, FieldOfView, AspectRatio );
  LookAt(ControllableCamera->CameraComponent, 1*V3(3,3,3), V3(0,0,0));

  // TODO Jakob: Any function pointers that gets assigned at game initialization
  //             will become invalid when game code is recompiled.
  //             Make it so that any permanent function pointers get reinitialized after
  //             gameCode is updated
  ControllableCamera->ControllerComponent->Controller = GetController(Input, 1);
  ControllableCamera->ControllerComponent->Type = ControllerType_FlyingCamera;
}

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

  r32 AspectRatio = (r32)RenderCommands->Width / (r32) RenderCommands->Height;
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

void InitiateGame(thread_context* Thread, game_memory* Memory, game_render_commands* RenderCommands, game_input* Input )
{
  if (!Memory->GameState)
  {
    Memory->GameState = BootstrapPushStruct(game_state, PersistentArena);

    u32 RenderMemorySize = Megabytes(128);
    u32 TempMemorySize   = Megabytes(16);
    RenderCommands->TemporaryMemory = utils::push_buffer((u8*) PushSize(&Memory->GameState->PersistentArena, TempMemorySize), TempMemorySize);

    //Create2DScene(Thread, Memory, RenderCommands, Input );
    CreateCollisionTestScene(Thread, Memory, RenderCommands, Input );
    //CreateEpaVisualizerTestScene(Thread, Memory, RenderCommands, Input );

    RenderCommands->MainRenderGroup.ElementCount = 0;
    RenderCommands->MainRenderGroup.Buffer = utils::push_buffer((u8*) PushSize(&Memory->GameState->PersistentArena, RenderMemorySize), RenderMemorySize);
    RenderCommands->MainRenderGroup.Assets = Memory->GameState->World->Assets;
    RenderCommands->MainRenderGroup.ScreenWidth  = (r32) RenderCommands->Width;
    RenderCommands->MainRenderGroup.ScreenHeight = (r32) RenderCommands->Height;
    RenderCommands->MainRenderGroup.ProjectionMatrix = M4Identity();
    RenderCommands->MainRenderGroup.ViewMatrix = M4Identity();

    AllocateGlobaklGjkEpaVisualizer(&Memory->GameState->AssetArena);

    RenderCommands->DebugRenderGroup.ElementCount = 0;
    RenderCommands->DebugRenderGroup.Buffer = utils::push_buffer((u8*) PushSize(&Memory->GameState->PersistentArena, RenderMemorySize), RenderMemorySize);
    RenderCommands->DebugRenderGroup.Assets = Memory->GameState->World->Assets;
    RenderCommands->DebugRenderGroup.ScreenWidth  =  (r32) RenderCommands->Width;
    RenderCommands->DebugRenderGroup.ScreenHeight =  (r32) RenderCommands->Height;
    RenderCommands->DebugRenderGroup.ProjectionMatrix = M4Identity();
    RenderCommands->DebugRenderGroup.ViewMatrix = M4Identity();

    Memory->GameState->World->Assets->STBFontMap = STBBakeFont(&Memory->GameState->AssetArena);

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
  GlobalDebugRenderGroup = &RenderCommands->DebugRenderGroup;
  GlobalVis = &GlobalGjkEpaVisualizer;
  TIMED_FUNCTION();
  Platform = Memory->PlatformAPI;
  InitiateGame(Thread, Memory, RenderCommands, Input);
  game_state* GameState = Memory->GameState;
  world* World = GameState->World;
  World->dtForFrame = Input->dt;
  World->GlobalTimeSec += Input->dt;

  ControllerSystemUpdate(GameState->World);
  SpatialSystemUpdate(GameState->World);
  CameraSystemUpdate(GameState->World);
  SpriteAnimationSystemUpdate(GameState->World);
  FillRenderPushBuffer( World, RenderCommands );
  CheckArena(&GameState->TransientArena);
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

void BeginDebugStatistics(debug_statistics* Statistic)
{
  Statistic->Count = 0;
  Statistic->Min =  R32Max;
  Statistic->Max = -R32Max;
  Statistic->Avg = 0;
}

void EndDebugStatistics(debug_statistics* Statistic)
{
  if(Statistic->Count != 0)
  {
    Statistic->Avg /= Statistic->Count;
  }else{
    Statistic->Min = 0;
    Statistic->Max = 0;
  }
}

void AccumulateStatistic(debug_statistics* Statistic, r32 Value)
{
  if(Statistic->Min > Value)
  {
    Statistic->Min = Value;
  }
  if(Statistic->Max < Value)
  {
    Statistic->Max = Value;
  }
  Statistic->Avg += Value;
  ++Statistic->Count;
}

#include "debug.cpp"
