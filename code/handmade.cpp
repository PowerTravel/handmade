
#include "handmade.h"
#include "random.h"

#include "handmade_tile.cpp"
#include "entity_components.cpp"
#include "obj_loader.cpp"

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

	obj_loaded_file* CubeObj = ReadOBJFile(Thread, GameState,
			   	 Memory->PlatformAPI.DEBUGPlatformReadEntireFile,
				 Memory->PlatformAPI.DEBUGPlatformFreeFileMemory,
				 "..\\handmade\\data\\cube\\cube.obj" );

	// Create Coordinate System
	entity* CubeX = CreateEntityFromOBJGroup( World, &CubeObj->Objects[0], CubeObj->MeshData);
	SetMaterial(CubeX->SurfaceComponent->Material, MATERIAL_RED_RUBBER );
	CubeX->SurfaceComponent->Material->DiffuseMap = 0;
	Translate(V3(1.2,0,0), CubeX->SpatialComponent );
	Scale(V3(1,0.2,0.2), CubeX->SpatialComponent);

	entity* CubeY = CreateEntityFromOBJGroup( World, &CubeObj->Objects[0], CubeObj->MeshData);
	SetMaterial(CubeY->SurfaceComponent->Material, MATERIAL_GREEN_RUBBER );
	CubeY->SurfaceComponent->Material->DiffuseMap = 0;
	Translate(V3(0,1.2,0), CubeY->SpatialComponent );
	Scale( V3(0.2,1,0.2), CubeY->SpatialComponent);

	entity* CubeZ = CreateEntityFromOBJGroup( World, &CubeObj->Objects[0], CubeObj->MeshData);
	SetMaterial(CubeZ->SurfaceComponent->Material, MATERIAL_BLUE_RUBBER  );
	CubeZ->SurfaceComponent->Material->DiffuseMap = 0;
	Translate(V3(0,0,1.2), CubeZ->SpatialComponent );
	Scale( V3(0.2,0.2,1), CubeZ->SpatialComponent);

	entity* Floor = CreateEntityFromOBJGroup( World, &CubeObj->Objects[0], CubeObj->MeshData);
	Translate(V3(0,-0.2,0), Floor->SpatialComponent );
	Scale( V3(5,0.2,5), Floor->SpatialComponent);

	entity* DynamicBox = CreateEntityFromOBJGroup( World, &CubeObj->Objects[0], CubeObj->MeshData);
	Translate(V3(0,3,0), DynamicBox->SpatialComponent );
	NewComponents( World, DynamicBox, COMPONENT_TYPE_DYNAMICS );
	component_dynamics* BoxDynamics = DynamicBox->DynamicsComponent;
	BoxDynamics->Velocity = V3(0,3,0);
	BoxDynamics->Mass     = 1;
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



#if 0
void Create2DScene(thread_context* Thread, game_memory* Memory, game_render_commands* RenderCommands,  game_input* Input )
{
	game_state* GameState = Memory->GameState;
	memory_arena* AssetArena = &GameState->AssetArena;
	
	u32 Height = 3;
	u32 Width = 3;
	u32 NrMaxWorldEntities = Height*Width + 10;
	GameState->World = AllocateWorld(NrMaxWorldEntities);
	world* World = GameState->World;

	r32 TilesPerScreenWidth  = 16;
	r32 TilesPerScreenHeight = 9;


	r32 AspectRatio = (r32)RenderCommands->Width / (r32) RenderCommands->Height;
	r32 FieldOfView =  90;
	entity* Camera = CreateCameraEntity(World, FieldOfView, AspectRatio);
	LookAt(Camera->CameraComponent, V3(TilesPerScreenWidth/2, TilesPerScreenHeight/2, 1), V3(TilesPerScreenWidth/2, TilesPerScreenHeight/2, 0));
	SetOrthoProj( Camera->CameraComponent, -100, 100, TilesPerScreenWidth/2, -TilesPerScreenWidth/2, TilesPerScreenHeight/2, -TilesPerScreenHeight/2 );

	NewComponents( World, Camera, COMPONENT_TYPE_CONTROLLER );
	Camera->ControllerComponent->Controller = GetController(Input, 1);
	Camera->ControllerComponent->ControllerMappingFunction = CameraController;


	GameState->World->Assets = (game_assets*) PushStruct(AssetArena, game_assets);
	game_assets* Assets = GameState->World->Assets;

	

	bitmap* TileSet = LoadTGA( Thread, &GameState->AssetArena,
				 Memory->PlatformAPI.DEBUGPlatformReadEntireFile,
				 Memory->PlatformAPI.DEBUGPlatformFreeFileMemory,
				 "..\\handmade\\data\\Isometric\\sheet.tga" );

	Assert(!TileSet->Handle);

	s32 FloorWidthInPixels  = 16;
	s32 FloorHeightInPixels = 16;


	s32 FloorStartX = 6;
	s32 FloorStartY = 0;
	s32 NrFloorTilesX = 7;
	s32 NrFloorTilesY = 2;

	World->Assets->NrFloorTiles = NrFloorTilesX*NrFloorTilesY;
	World->Assets->FloorTiles   = PushArray( &GameState->AssetArena, World->Assets->NrFloorTiles, floor_tile_sprite );

	s32 SpriteIdx = 0;
	for( s32 RowIdx = FloorStartY; RowIdx <  (s32) (FloorStartY+NrFloorTilesY); ++RowIdx )
	{
		for( s32 ColumnIdx = FloorStartX; ColumnIdx < (s32) (FloorStartX + NrFloorTilesX); ++ColumnIdx )
		{
			floor_tile_sprite* Sprite = &World->Assets->FloorTiles[SpriteIdx++];
			Sprite->Bitmap = TileSet;
			Sprite->TextureCoordinates.X = (r32) ColumnIdx*((r32) FloorWidthInPixels) /(r32)TileSet->Width;
			Sprite->TextureCoordinates.Y = (r32) 1.f - (RowIdx+1) * ((r32) FloorHeightInPixels)/(r32)TileSet->Height;
			Sprite->TextureCoordinates.W = (r32) FloorWidthInPixels  / (r32) TileSet->Width;
			Sprite->TextureCoordinates.H = (r32) FloorHeightInPixels / (r32) TileSet->Height;
		}
	}

	World->Assets->NrEdgeTiles = 8;
	World->Assets->EdgeTiles = PushArray( &GameState->AssetArena, World->Assets->NrEdgeTiles, floor_tile_sprite );
	floor_tile_sprite* EdgeTiles = World->Assets->EdgeTiles;

	floor_tile_sprite* TopLeftDrop = &EdgeTiles[0];
	TopLeftDrop->TextureCoordinates.X =  3 * FloorWidthInPixels  / (r32) TileSet->Width;
	TopLeftDrop->TextureCoordinates.Y =  1 - FloorHeightInPixels / (r32) TileSet->Height;
	TopLeftDrop->TextureCoordinates.W =  FloorWidthInPixels      / (r32) TileSet->Width;
	TopLeftDrop->TextureCoordinates.H =  FloorHeightInPixels     / (r32) TileSet->Height;
	TopLeftDrop->Bitmap = TileSet;

	floor_tile_sprite* TopRightDrop = &EdgeTiles[1];
	TopRightDrop->TextureCoordinates.X =  4 * FloorWidthInPixels  / (r32) TileSet->Width;
	TopRightDrop->TextureCoordinates.Y =  1 - FloorHeightInPixels / (r32) TileSet->Height;
	TopRightDrop->TextureCoordinates.W =  FloorWidthInPixels      / (r32) TileSet->Width;
	TopRightDrop->TextureCoordinates.H =  FloorHeightInPixels     / (r32) TileSet->Height;
	TopRightDrop->Bitmap = TileSet;

	floor_tile_sprite* BottomLeftDrop = &EdgeTiles[2];
	BottomLeftDrop->TextureCoordinates.X =  3 * FloorWidthInPixels      / (r32) TileSet->Width;
	BottomLeftDrop->TextureCoordinates.Y =  1 - 2 * FloorHeightInPixels / (r32) TileSet->Height;
	BottomLeftDrop->TextureCoordinates.W =  FloorWidthInPixels          / (r32) TileSet->Width;
	BottomLeftDrop->TextureCoordinates.H =  FloorHeightInPixels         / (r32) TileSet->Height;
	BottomLeftDrop->Bitmap = TileSet;

	floor_tile_sprite* BottomRightDrop = &EdgeTiles[3];
	BottomRightDrop->TextureCoordinates.X =  4 * FloorWidthInPixels      / (r32) TileSet->Width;
	BottomRightDrop->TextureCoordinates.Y =  1 - 2 * FloorHeightInPixels / (r32) TileSet->Height;
	BottomRightDrop->TextureCoordinates.W =  FloorWidthInPixels          / (r32) TileSet->Width;
	BottomRightDrop->TextureCoordinates.H =  FloorHeightInPixels         / (r32) TileSet->Height;
	BottomRightDrop->Bitmap = TileSet;

	floor_tile_sprite* LeftDrop = &EdgeTiles[4];
	LeftDrop->TextureCoordinates.X =  2 * FloorWidthInPixels      / (r32) TileSet->Width;
	LeftDrop->TextureCoordinates.Y =  1 - 2 * FloorHeightInPixels / (r32) TileSet->Height;
	LeftDrop->TextureCoordinates.W =  FloorWidthInPixels          / (r32) TileSet->Width;
	LeftDrop->TextureCoordinates.H =  FloorHeightInPixels         / (r32) TileSet->Height;
	LeftDrop->Bitmap = TileSet;

	floor_tile_sprite* RightDrop = &EdgeTiles[5];
	RightDrop->TextureCoordinates.X =  0;
	RightDrop->TextureCoordinates.Y =  1 - 2 * FloorHeightInPixels / (r32) TileSet->Height;
	RightDrop->TextureCoordinates.W =  FloorWidthInPixels          / (r32) TileSet->Width;
	RightDrop->TextureCoordinates.H =  FloorHeightInPixels         / (r32) TileSet->Height;
	RightDrop->Bitmap = TileSet;

	floor_tile_sprite* TopDrop = &EdgeTiles[6];
	TopDrop->TextureCoordinates.X =  1 * FloorWidthInPixels      / (r32) TileSet->Width;
	TopDrop->TextureCoordinates.Y =  1 - 3 * FloorHeightInPixels / (r32) TileSet->Height;
	TopDrop->TextureCoordinates.W =  FloorWidthInPixels          / (r32) TileSet->Width;
	TopDrop->TextureCoordinates.H =  FloorHeightInPixels         / (r32) TileSet->Height;
	TopDrop->Bitmap = TileSet;

	floor_tile_sprite* BottomDrop = &EdgeTiles[7];
	BottomDrop->TextureCoordinates.X =  1 * FloorWidthInPixels      / (r32) TileSet->Width;
	BottomDrop->TextureCoordinates.Y =  1 - 1 * FloorHeightInPixels / (r32) TileSet->Height;
	BottomDrop->TextureCoordinates.W =  FloorWidthInPixels          / (r32) TileSet->Width;
	BottomDrop->TextureCoordinates.H =  FloorHeightInPixels         / (r32) TileSet->Height;
	BottomDrop->Bitmap = TileSet;


	r32 TilesPerRoomWidth = 16;
	r32 TilesPerRoomHeight = 9;

	for( u32 i = 0;  i < TilesPerRoomHeight; ++i)
	{
		for( u32 j = 0;  j < TilesPerRoomWidth; ++j)
		{
			tile_contents TileContents = {};

			TileContents.Type = TILE_TYPE_WALL;
			if( i == 0 && j == 0){
				TileContents.Sprite = BottomLeftDrop;
			}else if( i == 0 && (j+1) == TilesPerRoomWidth ){
				TileContents.Sprite = BottomRightDrop;
			}else if( (i+1) == TilesPerRoomHeight && j == 0 ){
				TileContents.Sprite = TopLeftDrop;
			}else if( (i+1) == TilesPerRoomHeight && (j+1) == TilesPerRoomWidth ){
				TileContents.Sprite = TopRightDrop;
			}else if( i == 0 ){
				TileContents.Sprite = BottomDrop;
			}else if( (i+1) == TilesPerRoomHeight ){
				TileContents.Sprite = TopDrop;
			}else if( j == 0 ){
				TileContents.Sprite = LeftDrop;
			}else if( (j+1) == TilesPerRoomWidth ){
				TileContents.Sprite = RightDrop;
			}else{
				if( (i == ( (u32) TilesPerRoomHeight/2)) && (j == ( (u32) TilesPerRoomWidth/2))  ){
					TileContents.Sprite =  TopDrop;
				}else{		
					r32 Random = GetRandomReal(i*j);
					u32 RandIDX = ((s32)(Random * 1000 )) % World->Assets->NrFloorTiles;
					TileContents.Sprite =  &Assets->FloorTiles[RandIDX];
					TileContents.Type = TILE_TYPE_FLOOR;
				}
			}

			SetTileContentsAbs(&GameState->AssetArena, &World->TileMap, j, i, 0, TileContents );	
		}
	}

	bitmap* PrinnySet = LoadTGA( Thread, &GameState->AssetArena,
				 Memory->PlatformAPI.DEBUGPlatformReadEntireFile,
				 Memory->PlatformAPI.DEBUGPlatformFreeFileMemory,
				 "..\\handmade\\data\\Isometric\\Prinny.tga" );

	entity* PrinnyHero = NewEntity( World );
	NewComponents( World, PrinnyHero, COMPONENT_TYPE_SPATIAL | COMPONENT_TYPE_SPRITE_ANIMATION | COMPONENT_TYPE_CONTROLLER );
	
	ExtractPrinny( AssetArena, PrinnySet, PrinnyHero->SpriteAnimationComponent);

	component_spatial* PrinnySpatial = PrinnyHero->SpatialComponent;

	// PrinnySpatial->Position = V3(1.5, 3, 0);
	// PrinnySpatial->Velocity = V3(0,0,0);
	// PrinnySpatial->ExternalForce = V3(0,0,0);
	// PrinnySpatial->RotationAngle = 0;
	// PrinnySpatial->RotationAxis = V3(0,0,1);
	// PrinnySpatial->Width  = 0.8;
	// PrinnySpatial->Height = 1.5;
	// PrinnySpatial->Width  = 1;
	// PrinnySpatial->Height = 2;
	// PrinnySpatial->Depth  = 0;

	PrinnyHero->ControllerComponent->Controller = GetController(Input, 1);
	PrinnyHero->ControllerComponent->ControllerMappingFunction = HeroController;

	Assets->PrinnySet = PrinnyHero->SpriteAnimationComponent;

/*
	loaded_obj_file* ObjFile = ReadOBJFile(Thread, GameState,
		Memory->PlatformAPI.DEBUGPlatformReadEntireFile,
		Memory->PlatformAPI.DEBUGPlatformFreeFileMemory,
		"..\\handmade\\data\\Cube.obj");

	CreateEntitiesFromOBJFile(GameState->World, ObjFile);

	obj_group* Grp = &ObjFile->Objects[0];

	for( u32 i = 0; i < Height; ++i )
	{
		for( u32 j = 0; j < Width; ++j )
		{
			entity* Square = CreateEntityFromOBJGroup(GameState->World, Grp, ObjFile->MeshData);
			Square->SpatialComponent->Scale = 0.2;
			r32 SquareWidth =  Square->SpatialComponent->Scale * Square->SpatialComponent->AABBDimensions.X;
			r32 SquareHeight = Square->SpatialComponent->Scale * Square->SpatialComponent->AABBDimensions.Y;
			Square->SpatialComponent->Position = V3(j*SquareWidth, i*SquareHeight, 0);
		}	
	}
*/
}

#endif