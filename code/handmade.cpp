
#include "handmade.h"
#include "random.h"

#include "handmade_tile.cpp"
#include "entity_components.cpp"
#include "obj_loader.cpp"
#include "tiles_spritesheet.hpp"

#include "render_push_buffer.cpp"
#include "sprite_mapping.h"
#include "system_camera.cpp"
#include "system_spatial.cpp"
#include "system_sprite_animation.cpp"
#include "system_controller.cpp"


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

void Create3DScene(thread_context* Thread, game_memory* Memory, game_render_commands* RenderCommands,  game_input* Input )
{
	game_state* GameState = Memory->GameState;
	memory_arena* AssetArena = &GameState->AssetArena;

	GameState->World = AllocateWorld(10000);
	world* World = GameState->World;
	
	r32 AspectRatio = (r32)RenderCommands->Width / (r32) RenderCommands->Height;
	r32 FieldOfView =  90;
	entity* Camera = CreateCameraEntity(World, FieldOfView, AspectRatio);
	LookAt(Camera->CameraComponent, V3(2,2,2), V3(0,0,0));

	NewComponents( World, Camera, COMPONENT_TYPE_CONTROLLER );
	Camera->ControllerComponent->Controller = GetController(Input, 1);
	Camera->ControllerComponent->ControllerMappingFunction = FlyingCameraController;

	entity* Light = NewEntity( World );
	NewComponents( World, Light, COMPONENT_TYPE_LIGHT | COMPONENT_TYPE_SPATIAL );
	Light->LightComponent->Color = V4(1,1,1,1);
	Light->SpatialComponent->ModelMatrix = M4Identity();
	Translate( V3(3,3,3), Light->SpatialComponent );

	GameState->World->Assets = (game_assets*) PushStruct(AssetArena, game_assets);
	game_assets* Assets = GameState->World->Assets;


//	Assets->TileMapSpriteSheet.bitmap = LoadTGA( Thread, &GameState->AssetArena,
//				 Memory->PlatformAPI.DEBUGPlatformReadEntireFile,
//				 Memory->PlatformAPI.DEBUGPlatformFreeFileMemory,
//				 "..\\handmade\\data\\Platformer\\tiles_spritesheet.tga" );
//
//	static const bitmap_coordinate* coordinates = LoadImageCoordinates(&Assets->TileMapSpriteSheet.EntryCount);
//	Assets->TileMapSpriteSheet.coordinates = (bitmap_coordinate*)
//		PushCopy(AssetArena, Assets->TileMapSpriteSheet.EntryCount*sizeof(bitmap_coordinate), (void*) coordinates);
//
	bitmap* TileMap = LoadTGA( Thread, &GameState->AssetArena,
				 Memory->PlatformAPI.DEBUGPlatformReadEntireFile,
				 Memory->PlatformAPI.DEBUGPlatformFreeFileMemory,
				 "..\\handmade\\data\\Platformer\\TileMap.tga" );

	u32* TileMapPixels = (u32*) TileMap->Pixels;
	for (u32 X = 0; X < TileMap->Width; ++X)
	{
		for (u32 Y = 0; Y < TileMap->Height; ++Y)
		{
			// Check IF Alpha Value exists
			if((*TileMapPixels++ & 0xff) != 0)
			{
				tile_contents TileContents = {};
				TileContents.TileType = 1;
				SetTileContentsAbs(&GameState->AssetArena, &World->TileMap, X, Y, 0, TileContents );
			}
		}
	}

}

void initiateGame(thread_context* Thread, game_memory* Memory, game_render_commands* RenderCommands, game_input* Input )
{
	if (!Memory->GameState)
	{
		Memory->GameState = BootstrapPushStruct(game_state, AssetArena);
		
		//Create2DScene(Thread, Memory, RenderCommands, Input );
		Create3DScene(Thread, Memory, RenderCommands, Input );
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
//							  game_memory* Memory, 
//							  render_commands* RenderCommands, 
//							  game_input* Input )

platform_api Platform;

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
	Platform = Memory->PlatformAPI;
	initiateGame(Thread, Memory, RenderCommands, Input);

	game_state* GameState = Memory->GameState;
	world* World = GameState->World;

	ControllerSystemUpdate(GameState->World);
	SpatialSystemUpdate(GameState->World);
	CameraSystemUpdate(GameState->World);
	SpriteAnimationSystemUpdate(GameState->World);

	FillRenderPushBuffer( World, RenderCommands );

	CheckArena(&GameState->TemporaryArena);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
	GameOutputSound(SoundBuffer, 400);
}
