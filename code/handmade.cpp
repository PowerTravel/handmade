#include <stdio.h>

#include "handmade.h"
#include "render.cpp"
#include "camera_system.cpp"
#include "render_system.cpp"
#include "obj_loader.cpp"

internal void
GameOutputSound(game_sound_output_buffer* SoundBuffer, int ToneHz)
{

	local_persist r32 tSine = 0.f;

	s16 ToneVolume = 2000;
	int SamplesPerPeriod = SoundBuffer->SamplesPerSecond/ToneHz;

	r32 twopi = (r32) (2.0*Pi32);

	s16 *SampleOut = SoundBuffer->Samples;
	for(int SampleIndex = 0; 
		SampleIndex < SoundBuffer->SampleCount; 
		++SampleIndex)
	{

#if 0
		r32 SineValue = Sin(tSine);
		s16 SampleValue = (s16)(SineValue * ToneVolume);

		tSine += twopi/(r32)SamplesPerPeriod;	
		if(tSine > twopi)
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

#if 1
internal bitmap
DEBUGReadBMP( thread_context* Thread, game_state* aGameState, 
			   debug_platform_read_entire_file* ReadEntireFile,
			   debug_platfrom_free_file_memory* FreeEntireFile,
			   char* FileName)
{
	// Note(Jakob). BMP is stored in little Endian, 
	// Little endian means that the least significant digit is stored first
	// So if we read a 32 bit int in 8 bit cunks it would come out as this:
	// 32bit: 0xAABBCCDD ->  
	// 8bit chunks:
	// 0xDD, 0xCC, 0xBB, 0xAA

	// Todo(Jakob): Add hotspots to bitmaps. A hotspot is the alignpoint in pixels. 
	//				For a mouse pointer sprite this would for example be on the tip of
	//				the pointer. Its used to poroperly display the bitmap.

	bitmap Result = {};
	debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);

	if (ReadResult.ContentSize != 0)
	{
		void* imageContentsTmp = ReadResult.Contents;
		memory_arena* Arena = &aGameState->AssetArena;
		//ReadResult.Contents = PushCopy(Arena, ReadResult.ContentSize, imageContentsTmp);


		bmp_header* Header = (bmp_header*)ReadResult.Contents;//ReadResult.Contents;

		Assert(Header->BitsPerPixel == 32);
		Assert(Header->Compression == 3);

		Result.Pixels = PushArray(Arena, Header->Width*Header->Height, u32);
		Result.Width = Header->Width;
		Result.Height = Header->Height;

		u32* Target = (u32*)Result.Pixels;
		u32* Source = (u32*)(((u8*)ReadResult.Contents) + Header->FileSize - 4);

		bit_scan_result RedBitShift =   FindLeastSignificantSetBit(Header->RedMask);
		bit_scan_result GreenBitShift = FindLeastSignificantSetBit(Header->GreenMask);
		bit_scan_result BlueBitShift =  FindLeastSignificantSetBit(Header->BlueMask);
		bit_scan_result AlphaBitShift = FindLeastSignificantSetBit(Header->AlphaMask);

		Assert(RedBitShift.Found);
		Assert(GreenBitShift.Found);
		Assert(BlueBitShift.Found);	

		for (u32 Y = 0; Y < Result.Height; ++Y)
		{
			for (u32 X = 0; X < Result.Width; ++X)
			{
				u32 Pixel = *Source--;
	
				bit_scan_result BitShift = {};


				u32 Red   = Pixel & Header->RedMask;     
				u8 R = (u8) (Red >> RedBitShift.Index);

				u32 Green = Pixel & Header->GreenMask;
				u8 G = (u8) (Green >> GreenBitShift.Index);
			
				u32 Blue  = Pixel & Header->BlueMask;
				u8 B = (u8) (Blue >> BlueBitShift.Index);
				
				u8 A = 0xff;
				if (AlphaBitShift.Found)
				{
					u32 Alpha = Pixel & Header->AlphaMask;
					A = (u8)(Alpha >> AlphaBitShift.Index);
				}
				*Target++ = (A << 24) | (R << 16) | (G << 8) | (B << 0);

			}
		}		

		FreeEntireFile(Thread, imageContentsTmp);
	}

	return Result;
}

void CreateMuroScene(thread_context* Thread, game_memory* Memory, game_input* Input, game_offscreen_buffer* Buffer)
{
		game_state* GameState = Memory->GameState;
		
		u32 NrMaxWorldEntities = 256;
		GameState->World = AllocateWorld( NrMaxWorldEntities );

		entity* Camera = NewEntity( GameState->World );
		NewComponents( GameState->World, Camera, COMPONENT_TYPE_CAMERA |  COMPONENT_TYPE_CONTROLLER );
		CreateCameraComponent(Camera->CameraComponent, 60, -0.1, -100, (r32) Buffer->Width, (r32) Buffer->Height );
		LookAt( Camera->CameraComponent, 100* Normalize( V3(0,0,1) ), V3(0,0,0));
		Camera->ControllerComponent->Controller = GetController( Input, 1 );
		
		entity* Light = NewEntity( GameState->World );
		NewComponents( GameState->World, Light, COMPONENT_TYPE_LIGHT );
		component_light* L = Light->LightComponent;
		L->Position = V4(5,3,3,1);
		L->Color 	= V4(0.8,0.8,0.8,1);

		Light = NewEntity( GameState->World );
		NewComponents( GameState->World, Light, COMPONENT_TYPE_LIGHT );
		L = Light->LightComponent;
		L->Position = V4(5,-3,3,1);
		L->Color 	= V4(0.2,0.2,0.2,1);

		loaded_obj_file* ObjFile = ReadOBJFile(Thread, GameState, 
							Memory->PlatformAPI.DEBUGPlatformReadEntireFile,
							Memory->PlatformAPI.DEBUGPlatformFreeFileMemory,	
								"C:\\Users\\Mother Shabobo\\Desktop\\handmade\\data\\geometry\\muro\\muro.obj");

		SetMeshAndMaterialComponentFromObjFile( GameState->World, ObjFile );

}

void CreateCubeScene(thread_context* Thread, game_memory* Memory, game_input* Input, game_offscreen_buffer* Buffer)
{
		game_state* GameState = Memory->GameState;

		u32 NrMaxWorldEntities = 256;
		GameState->World = AllocateWorld( NrMaxWorldEntities );

		entity* Camera = NewEntity( GameState->World );
		NewComponents( GameState->World, Camera, COMPONENT_TYPE_CAMERA |  COMPONENT_TYPE_CONTROLLER );
		CreateCameraComponent(Camera->CameraComponent, 60, -0.1, -100, (r32) Buffer->Width, (r32) Buffer->Height );
		LookAt( Camera->CameraComponent, 10* Normalize( V3(0,0,1) ), V3(0,0,0));
		Camera->ControllerComponent->Controller = GetController( Input, 1 );
		
		r32 PixelsPerUnitDistance = 16;

		entity* Light = NewEntity( GameState->World );
		NewComponents( GameState->World, Light, COMPONENT_TYPE_LIGHT );
		component_light* L = Light->LightComponent;
		L->Position = V4(5,3,3,1);
		L->Color 	= V4(1,1,1,1);

		loaded_obj_file* ObjFile = ReadOBJFile(Thread, GameState, 
							Memory->PlatformAPI.DEBUGPlatformReadEntireFile,
							Memory->PlatformAPI.DEBUGPlatformFreeFileMemory,	
								"..\\handmade\\data\\geometry\\square.obj");

		bitmap* FloorTiles = LoadTGA( Thread, &GameState->AssetArena, 
								Memory->PlatformAPI.DEBUGPlatformReadEntireFile,
								Memory->PlatformAPI.DEBUGPlatformFreeFileMemory,
				  "C:\\Users\\Mother Shabobo\\Desktop\\handmade\\data\\Floor\\FloorTile.tga");


		s32 Width = 16;
		r32 Side = 1;
		u32 RandNrIdx = 0;
		r32 Height = 4;
		for( s32 i = 0; i <= Width; ++i )
		{
			r32 RandomNumber = GetRandomNumber(RandNrIdx++) * 100;
			u32 ModelIndex = ( ( (s32)RandomNumber ) % ObjFile->ObjectCount);
			obj_group* Grp = &ObjFile->Objects[ ModelIndex ];
			entity* WallTile = SetMeshAndMaterialComponentFromObjGroup( GameState->World, Grp,  ObjFile->MeshData );
			Scale(     V4( Side, Side, Side, 1 ), WallTile->RenderMeshComponent->T );
			Translate( V4( (r32) (-Width/2 + i)*Side, -Height*Side, 0, 1 ), WallTile->RenderMeshComponent->T );

			RandomNumber = GetRandomNumber(RandNrIdx++) * 100;
			ModelIndex = ( ( (s32)RandomNumber ) % ObjFile->ObjectCount);
			Grp = &ObjFile->Objects[ ModelIndex ];
			WallTile = SetMeshAndMaterialComponentFromObjGroup( GameState->World, Grp, ObjFile->MeshData );
			Scale(     V4( Side, Side, Side, 1 ), WallTile->RenderMeshComponent->T );
			Translate( V4( (r32) (-Width/2 + i)*Side, Height*Side, 0, 1 ), WallTile->RenderMeshComponent->T );
		}

		for( s32 i = 0; i <= 2*Height; ++i )
		{
			r32 RandomNumber = GetRandomNumber(RandNrIdx++) * 100;
			u32 ModelIndex = ( ( (s32)RandomNumber ) % ObjFile->ObjectCount);
			obj_group* Grp = &ObjFile->Objects[ ModelIndex ];
			entity* WallTile = SetMeshAndMaterialComponentFromObjGroup( GameState->World, Grp,  ObjFile->MeshData );
			Scale(     V4( Side, Side, Side, 1 ), WallTile->RenderMeshComponent->T );
			Translate( V4( (r32) (-Width/2 + i)*Side, -Height*Side, 0, 1 ), WallTile->RenderMeshComponent->T );

			RandomNumber = GetRandomNumber(RandNrIdx++) * 100;
			ModelIndex = ( ( (s32)RandomNumber ) % ObjFile->ObjectCount);
			Grp = &ObjFile->Objects[ ModelIndex ];
			WallTile = SetMeshAndMaterialComponentFromObjGroup( GameState->World, Grp, ObjFile->MeshData );
			Scale(     V4( Side, Side, Side, 1 ), WallTile->RenderMeshComponent->T );
			Translate( V4( (r32) (-Width/2 + i)*Side, Height*Side, 0, 1 ), WallTile->RenderMeshComponent->T );
		}

		//obj_group* Grp = &ObjFile->Objects[1];
		//SetMeshAndMaterialComponentFromObjGroup(GameState->World, Grp, ObjFile->MeshData );
		//SetMeshAndMaterialComponentFromObjFile( GameState->World, ObjFile );

}

void initiateGame(thread_context* Thread, game_memory* Memory, game_input* Input, game_offscreen_buffer* Buffer)
{
	if( ! Memory->GameState )
	{
		Memory->GameState = BootstrapPushStruct(game_state, AssetArena);

		game_state* GameState = Memory->GameState;

//		GameState->testBMP = *LoadTGA( Thread, &GameState->AssetArena, 
//								Memory->PlatformAPI.DEBUGPlatformReadEntireFile,
//								Memory->PlatformAPI.DEBUGPlatformFreeFileMemory,
//				 "C:\\Users\\Mother Shabobo\\Desktop\\handmade\\data\\blue3.tga");

		memory_arena* AssetArena = &GameState->AssetArena;
		GameState->DepthBuffer = {};
		GameState->DepthBuffer.Width = Buffer->Width;
		GameState->DepthBuffer.Height = Buffer->Height;
		GameState->DepthBuffer.Buffer = PushArray(AssetArena, Buffer->Width*Buffer->Height, r32 );

		//CreateCubeScene(Thread, Memory, Input, Buffer);
	    CreateMuroScene(Thread, Memory, Input, Buffer);

		for(s32 ControllerIndex = 0; 
			ControllerIndex < ArrayCount(Input->Controllers); 
			++ControllerIndex)
		{
			game_controller_input* Controller = GetController(Input,ControllerIndex);
			Controller->IsAnalog = true;
		}

	}
}

#endif
/* 
	Note: 
	extern "C" prevents the C++ compiler from renaming the functions which it does for function-overloading reasons (among other things) by forcing it to use C conventions which does not support overloading. Also called 'name mangling' or 'name decoration'. The actual function names are visible in the outputted .map file in the build directory
*/
// Signature is
//void game_update_and_render (thread_context* Thread, 
//							  game_memory* Memory, 
//							  game_offscreen_buffer* Buffer, 
//							  game_input* Input )

platform_api Platform;

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{

#if 1
	local_persist r32 t = 0;

	Platform = Memory->PlatformAPI;
	initiateGame( Thread,  Memory, Input, Buffer );

	game_state* GameState = Memory->GameState;

	render_push_buffer PushBuffer = {};
	InitiatePushBuffer(&PushBuffer, Buffer, &GameState->DepthBuffer, &GameState->TemporaryArena);

	// Clear Screen
	DrawRectangle(Buffer, 0,0, (r32) Buffer->Width,   (r32) Buffer->Height, 1,1,1);
	DrawRectangle(Buffer, 1,1, (r32) Buffer->Width-2, (r32) Buffer->Height-2, 0.3,0.3,0.3);

	CameraSystemUpdate(GameState->World);
	RenderSystemUpdate(GameState->World, &PushBuffer);

	DrawTriangles(&PushBuffer);

	ClearPushBuffer(&PushBuffer);

	BlitBMP( Buffer, 20,20, GameState->testBMP );

	t += Pi32/60;
	if(t >= 200*Pi32)
	{
		t -=200*Pi32;
	}
	CheckArena(&Memory->GameState->TemporaryArena);

	#endif
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
	GameOutputSound(SoundBuffer, 400);
}
