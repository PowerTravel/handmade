
#include "handmade.h"
#include "random.h"

#include "handmade_tile.cpp"
#include "entity_components.cpp"
#include "camera_system.cpp"
#include "obj_loader.cpp"

#include "render.cpp"

#include "render_push_buffer.cpp"
#include "sprite_mapping.h"
#include "system_spatial.cpp"
#include "system_sprite_animation.cpp"
#include "system_controller.cpp"

//#include "unit_tests.cpp"

// makes the packing compact
#pragma pack(push, 1)
struct bmp_header
{
	u16 FileType;     /* File type, always 4D42h ("BM") */
	u32 FileSize;     /* Size of the file in bytes */
	u16 Reserved1;    /* Always 0 */
	u16 Reserved2;    /* Always 0 */
	u32 BitmapOffset; /* Starting position of image data in bytes */

	u32 HeaderSize;       /* Size of this header in bytes */
	s32 Width;           /* Image width in pixels */
	s32 Height;          /* Image height in pixels */
	u16 Planes;          /* Number of color planes */
	u16 BitsPerPixel;    /* Number of bits per pixel */
	u32 Compression;     /* Compression methods used */
	u32 SizeOfBitmap;    /* Size of bitmap in bytes */
	s32 HorzResolution;  /* Horizontal resolution in pixels per meter */
	s32 VertResolution;  /* Vertical resolution in pixels per meter */
	u32 ColorsUsed;      /* Number of colors in the image */
	u32 ColorsImportant; /* Minimum number of important colors */
	/* Fields added for Windows 4.x follow this line */

	u32 RedMask;       /* Mask identifying bits of red component */
	u32 GreenMask;     /* Mask identifying bits of green component */
	u32 BlueMask;      /* Mask identifying bits of blue component */
	u32 AlphaMask;     /* Mask identifying bits of alpha component */
	u32 CSType;        /* Color space type */
	s32 RedX;          /* X coordinate of red endpoint */
	s32 RedY;          /* Y coordinate of red endpoint */
	s32 RedZ;          /* Z coordinate of red endpoint */
	s32 GreenX;        /* X coordinate of green endpoint */
	s32 GreenY;        /* Y coordinate of green endpoint */
	s32 GreenZ;        /* Z coordinate of green endpoint */
	s32 BlueX;         /* X coordinate of blue endpoint */
	s32 BlueY;         /* Y coordinate of blue endpoint */
	s32 BlueZ;         /* Z coordinate of blue endpoint */
	u32 GammaRed;      /* Gamma red coordinate scale value */
	u32 GammaGreen;    /* Gamma green coordinate scale value */
	u32 GammaBlue;     /* Gamma blue coordinate scale value */
};
#pragma pack(pop)


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

#if 1
internal bitmap
DEBUGReadBMP(thread_context* Thread, game_state* aGameState,
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

		bit_scan_result RedBitShift = FindLeastSignificantSetBit(Header->RedMask);
		bit_scan_result GreenBitShift = FindLeastSignificantSetBit(Header->GreenMask);
		bit_scan_result BlueBitShift = FindLeastSignificantSetBit(Header->BlueMask);
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


				u32 Red = Pixel & Header->RedMask;
				u8 R = (u8)(Red >> RedBitShift.Index);

				u32 Green = Pixel & Header->GreenMask;
				u8 G = (u8)(Green >> GreenBitShift.Index);

				u32 Blue = Pixel & Header->BlueMask;
				u8 B = (u8)(Blue >> BlueBitShift.Index);

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
void Create3DScene(thread_context* Thread, game_memory* Memory, game_render_commands* RenderCommands,  game_input* Input )
{
	game_state* GameState = Memory->GameState;
	memory_arena* AssetArena = &GameState->AssetArena;

	GameState->World = AllocateWorld(256);
	world* World = GameState->World;
	entity* Camera = NewEntity( World );


	NewComponents( World, Camera, COMPONENT_TYPE_CAMERA | COMPONENT_TYPE_CONTROLLER );
	r32 AspectRatio = (r32)RenderCommands->Width / (r32) RenderCommands->Height;
	r32 FieldOfView =  90;
	CreateCameraComponent(Camera->CameraComponent, FieldOfView, AspectRatio );
	LookAt(Camera->CameraComponent, V3(0,0,1), V3(0,1,0));

	Camera->ControllerComponent->Controller = GetController(Input, 1);
	Camera->ControllerComponent->ControllerMappingFunction = CameraController;

	GameState->World->Assets = (game_assets*) PushStruct(AssetArena, game_assets);
	game_assets* Assets = GameState->World->Assets;

#if 0
	loaded_obj_file* square = ReadOBJFile(Thread, GameState,
			   	 Memory->PlatformAPI.DEBUGPlatformReadEntireFile,
				 Memory->PlatformAPI.DEBUGPlatformFreeFileMemory,
				 "..\\handmade\\data\\cube.obj" );
#else
	loaded_obj_file* square = ReadOBJFile(Thread, GameState,
		   	 Memory->PlatformAPI.DEBUGPlatformReadEntireFile,
			 Memory->PlatformAPI.DEBUGPlatformFreeFileMemory,
			 "..\\handmade\\data\\square.obj" );
#endif
	CreateEntitiesFromOBJFile( World, square );

	for(u32 Index = 0; Index < World->NrEntities; ++Index)
	{
		entity* Entity = &World->Entities[Index];
		if( Entity->Types & COMPONENT_TYPE_MESH )
		{
//			Memory->PlatformAPI.ToGPU();
		}
 	}

}

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

	entity* Camera = NewEntity( World );
	NewComponents( World, Camera, COMPONENT_TYPE_CAMERA | COMPONENT_TYPE_CONTROLLER );
	r32 AspectRatio = (r32)RenderCommands->Width / (r32) RenderCommands->Height;
	r32 FieldOfView =  90;
	CreateCameraComponent(Camera->CameraComponent, FieldOfView, AspectRatio );
	LookAt(Camera->CameraComponent, V3(TilesPerScreenWidth/2, TilesPerScreenHeight/2, 1), V3(TilesPerScreenWidth/2, TilesPerScreenHeight/2, 0));
	SetOrthoProj( Camera->CameraComponent, -100, 100, TilesPerScreenWidth/2, -TilesPerScreenWidth/2, TilesPerScreenHeight/2, -TilesPerScreenHeight/2 );


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

	PrinnySpatial->Position = V3(1.5, 3, 0);
	PrinnySpatial->Velocity = V3(0,0,0);
	PrinnySpatial->ExternalForce = V3(0,0,0);
	PrinnySpatial->RotationAngle = 0;
	PrinnySpatial->RotationAxis = V3(0,0,1);
	PrinnySpatial->Width  = 0.8;
	PrinnySpatial->Height = 1.5;
	PrinnySpatial->Width  = 1;
	PrinnySpatial->Height = 2;
	PrinnySpatial->Depth  = 0;

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

#endif
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
