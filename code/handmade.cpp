
#include "handmade.h"

internal void
GameOutputSound(game_sound_output_buffer* SoundBuffer, int ToneHz)
{


	int16 ToneVolume = 2000;
	int SamplesPerPeriod = SoundBuffer->SamplesPerSecond/ToneHz;

	real32 twopi = (real32) (2.0*Pi32);

	int16 *SampleOut = SoundBuffer->Samples;
	for(int SampleIndex = 0; 
		SampleIndex < SoundBuffer->SampleCount; 
		++SampleIndex)
	{

#if 0
		real32 SineValue = sinf(GameState->tSine);
		int16 SampleValue = (int16)(SineValue * ToneVolume);

		GameState->tSine += twopi/(real32)SamplesPerPeriod;	
		if(GameState->tSine > twopi)
		{
			GameState->tSine -= twopi;
		}
#else	
		int16 SampleValue = 0;
#endif	
		*SampleOut++ = SampleValue; // Left Speker
		*SampleOut++ = SampleValue; // Rright speaker
	}
}

int32 DEBUGRoundReal32ToInt32(real32 Real32)
{
	int32 Result = (int32) (Real32+0.5);
	return Result; 
}

int32 DEBUGTruncateReal32ToInt32(real32 Real32)
{
	int32 Result = (int32) Real32;
	return Result;
}

#include "math.h"
int32 FloorReal32ToInt32(real32 Real32)
{
	int32 Result = (int32) floorf(Real32);
	return Result;
}

internal void
DrawRectangle(game_offscreen_buffer* Buffer, real32 RealMinX, real32 RealMinY, real32 Width, real32 Height, 
				real32 R, real32 G, real32 B)
{
	int32 MinX = DEBUGRoundReal32ToInt32(RealMinX);
	int32 MinY = DEBUGRoundReal32ToInt32(RealMinY);
	int32 MaxX = DEBUGRoundReal32ToInt32(RealMinX+Width);
	int32 MaxY = DEBUGRoundReal32ToInt32(RealMinY+Height);

	int32 iR =(int32) R*255;
	int32 iG =(int32) G*255;
	int32 iB =(int32) B*255;

	if(MinX < 0)
	{
		MinX = 0;
	}
	if(MinY < 0)
	{
		MinY = 0;
	}
	if(MaxX > Buffer->Width)
	{
		MaxX =  Buffer->Width;
	}
	if(MaxY > Buffer->Height)
	{
		MaxY = Buffer->Height;
	}

	uint8* Row = ( (uint8*) Buffer->Memory  + MinX * Buffer->BytesPerPixel + 
											  MinY * Buffer->Pitch);
	for(int Y = MinY; Y<MaxY; ++Y)
	{
		uint32* Pixel = (uint32*)Row;
		for(int X = MinX; X<MaxX; ++X)
		{
			*Pixel++ = ((DEBUGRoundReal32ToInt32(R*255.f) << 16) |
					   (DEBUGRoundReal32ToInt32(G*255.f) << 8) 	|
					   (DEBUGRoundReal32ToInt32(B*255.f) << 0));
		}
		Row += Buffer->Pitch;
	}
}

int32 Get2DArrayIdx(int32 Col, int32 Row, int32 Stride )
{
	int32 Result = Row*Stride + Col;
	return Result;
}

canonical_position 
GetCanonicalPosition(world* World, raw_position* RawPos)
{
	canonical_position Result = {};
	Result.TileMapX = RawPos->TileMapX;
	Result.TileMapY = RawPos->TileMapY;


	Result.TileX = FloorReal32ToInt32(RawPos->TileMapPosX/(real32)World->TileWidth);
	Result.TileY = FloorReal32ToInt32(RawPos->TileMapPosY/(real32)World->TileHeight);

	Result.RelTileX = RawPos->TileMapPosX-Result.TileX*World->TileWidth;
	Result.RelTileY = RawPos->TileMapPosY-Result.TileY*World->TileHeight;

	if(RawPos->TileMapPosX < 0)
	{
		Result.TileX = World->TileMapWidth+Result.TileX;
		Result.TileMapX--;
	}
	if(RawPos->TileMapPosY < 0)
	{

		Result.TileY = World->TileMapHeight+Result.TileY;
		Result.TileMapY--;
	}
	if(RawPos->TileMapPosX >= World->TileMapWidth*World->TileWidth)
	{
		Result.TileX = World->TileMapWidth - Result.TileX;
		Result.TileMapX++;
	}
	if(RawPos->TileMapPosY >= World->TileMapHeight*World->TileHeight)
	{
		Result.TileY = World->TileMapHeight - Result.TileY;
		Result.TileMapY++;
	}
	return Result;
}

bool32 IsWorldPointEmpty(world* World, raw_position* RawPos)
{
	bool32 Result = true;

	canonical_position CanPos = GetCanonicalPosition(World, RawPos);

	int32 TileMapIdx = Get2DArrayIdx(CanPos.TileMapX, CanPos.TileMapY, World->Width);
	int32 TileIdx = Get2DArrayIdx(CanPos.TileX, CanPos.TileY, World->TileMapWidth);
	if( World->TileMaps[TileMapIdx].Tiles[TileIdx] == 1)
	{
		Result = false;
	}
	return Result;
}

/* 
	Note: 
	extern "C" prevents the C++ compiler from renaming the functions which it does for function-overloading reasons (among other things) by forcing it to use C conventions which does not support overloading. Also called 'name mangling' or 'name decoration'. The actual function names are visible in the outputted .map file in the build directory
*/
extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
	Assert( (&Input->Controllers[0].Terminator - &Input->Controllers[0].Button[0]) == 
			 (ArrayCount(Input->Controllers[0].Button)) );
	Assert(sizeof(game_state) <= (Memory->PermanentStorageSize) );
	game_state *GameState = (game_state* ) Memory->PermanentStorage;

	if(!Memory->IsInitialized)
	{
		Memory->IsInitialized = true;

		GameState->PlayerPos.TileMapPosX = 300;
		GameState->PlayerPos.TileMapPosY = 300;
		GameState->PlayerPos.TileMapX = 0;
		GameState->PlayerPos.TileMapY = 0;
	}

#define TILE_WIDTH 60
#define TILE_HEIGHT 60
#define TILE_MAP_WIDTH 16
#define TILE_MAP_HEIGHT 9
#define WORLD_WIDTH 2
#define WORLD_HEIGHT 2


	uint32 TileMap00[TILE_MAP_HEIGHT][TILE_MAP_WIDTH] = 
						   {{1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1, 1},
							{1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0, 1},
							{1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0, 1},
							{1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0, 0},

							{1,1,1,0, 0,0,0,0, 0,0,0,0, 0,1,0, 0},
							{1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0, 1},
							{1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0, 1},
							{1,0,0,0, 0,0,0,0, 0,1,1,0, 0,0,0, 1},
							
							{1,1,1,1, 1,1,1,0, 0,1,1,1, 1,1,1, 1}};
	
	uint32 TileMap01[TILE_MAP_HEIGHT][TILE_MAP_WIDTH] = 
	   					   {{1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1, 1},
							{1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0, 1},
							{1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0, 1},
							{0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0, 1},

							{0,1,1,0, 0,0,0,0, 0,0,0,0, 0,1,0, 1},
							{1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0, 1},
							{1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0, 1},
							{1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0, 1},
							
							{1,1,1,1, 1,1,1,0, 0,1,1,1, 1,1,1, 1}};
	
	uint32 TileMap10[TILE_MAP_HEIGHT][TILE_MAP_WIDTH] = { 
							{1,1,1,1, 1,1,1,0, 0,1,1,1, 1,1,1, 1},
							{1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0, 1},
							{1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0, 1},
							{1,0,0,0, 0,0,0,0, 0,0,0,1, 0,0,0, 0},

							{1,1,1,0, 0,0,0,0, 0,0,0,1, 0,1,0, 0},
							{1,0,0,0, 0,0,0,0, 0,0,0,1, 0,0,0, 1},
							{1,0,0,0, 0,0,0,0, 0,0,0,1, 0,0,0, 1},
							{1,0,0,0, 0,0,0,0, 1,1,1,1, 0,0,0, 1},
							
							{1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1, 1}};

	uint32 TileMap11[TILE_MAP_HEIGHT][TILE_MAP_WIDTH] = { 
							{1,1,1,1, 1,1,1,0, 0,1,1,1, 1,1,1, 1},
							{1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0, 1},
							{1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0, 1},
							{0,0,0,0, 0,0,0,1, 1,0,0,0, 0,0,0, 1},

							{0,1,1,0, 0,0,0,1, 1,0,0,0, 0,1,0, 1},
							{1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0, 1},
							{1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0, 1},
							{1,0,0,0, 0,0,0,0, 1,1,1,0, 0,0,0, 1},
							
							{1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1, 1}};

	tile_map TileMap[WORLD_WIDTH][WORLD_HEIGHT] = {};
	TileMap[0][0].Tiles = (uint32*) TileMap00;
	TileMap[0][1].Tiles = (uint32*) TileMap01;
	TileMap[1][0].Tiles = (uint32*) TileMap10;
	TileMap[1][1].Tiles = (uint32*) TileMap11;

	world World = {};
	World.TileWidth = TILE_WIDTH;
	World.TileHeight = TILE_HEIGHT;
	World.TileMapWidth = TILE_MAP_WIDTH;
	World.TileMapHeight = TILE_MAP_HEIGHT;
	World.Width = WORLD_WIDTH;
	World.Height = WORLD_HEIGHT;
	World.TileMaps = (tile_map*) &TileMap;


	real32 PlayerWidth = 0.75*World.TileWidth;
	real32 PlayerHeight = World.TileHeight;


	real32 dt = Input->dt;

	real32 dvx = 0.f;
	real32 dvy = 0.f;

	// Pixels per second
	real32 speed = 200;


	for(int ControllerIndex = 0; 
		ControllerIndex < ArrayCount(Input->Controllers); 
		++ControllerIndex)
	{
		
		game_controller_input* Controller = GetController(Input,ControllerIndex);

		if(Controller->IsAnalog ){
			// Controller
		}else{
			
			if(Controller->LeftStickLeft.EndedDown)
			{
				dvx -= speed;
			}
			if(Controller->LeftStickRight.EndedDown)
			{
				dvx += speed;
			}
			if(Controller->LeftStickUp.EndedDown)
			{
				dvy -= speed;
			}
			if(Controller->LeftStickDown.EndedDown)
			{
				dvy += speed;
			}
			if(Controller->RightShoulder.EndedDown)
			{
				GameState->PlayerPos.TileMapPosX = 200.f;
				GameState->PlayerPos.TileMapPosY = 200.f;	
			}
		}	
	}

	raw_position NewPosition = GameState->PlayerPos;
	raw_position NewPositionL = NewPosition;
	raw_position NewPositionR = NewPosition;


	NewPosition.TileMapPosX = NewPosition.TileMapPosX + dt * dvx;
	NewPosition.TileMapPosY = NewPosition.TileMapPosY + dt * dvy;

	NewPositionL.TileMapPosX = NewPosition.TileMapPosX-PlayerWidth*0.5;
	NewPositionR.TileMapPosX = NewPosition.TileMapPosX+PlayerWidth*0.5;

	// WARNING: A bug when 1 side point registers a collision but the middle point doesnt.
	//			Probably not final movement code so il deffer tracking it down.
	if( IsWorldPointEmpty(&World, &NewPositionR) &&
		IsWorldPointEmpty(&World, &NewPositionL) &&
		IsWorldPointEmpty(&World, &NewPosition))
	{		
		canonical_position NewCan = GetCanonicalPosition(&World, &NewPosition);
		GameState->PlayerPos.TileMapPosX = World.TileWidth * NewCan.TileX + NewCan.RelTileX;
		GameState->PlayerPos.TileMapPosY = World.TileHeight * NewCan.TileY + NewCan.RelTileY;
		GameState->PlayerPos.TileMapX = NewCan.TileMapX;
		GameState->PlayerPos.TileMapY = NewCan.TileMapY; 
	}else{
		int a  =32;

	}

	real32 XPos = GameState->PlayerPos.TileMapPosX;
	real32 YPos = GameState->PlayerPos.TileMapPosY;

	DrawRectangle(Buffer, 0.0,0.0,(real32) Buffer->Width, (real32) Buffer->Height, 1.f,0.f,1.f);
	for(int Col = 0; Col<World.TileMapWidth; Col++)
	{
		for(int Row = 0; Row<World.TileMapHeight; Row++)
		{
			tile_map CurrentMap = World.TileMaps[GameState->PlayerPos.TileMapY*World.Width+GameState->PlayerPos.TileMapX];
			if(CurrentMap.Tiles[World.TileMapWidth*Row+Col] == 1)
			{
				DrawRectangle(Buffer, (real32) World.TileWidth*Col , (real32) World.TileHeight*Row, World.TileWidth, World.TileHeight, 0.3f, 0.3f, 0.3f);
			}else{
				DrawRectangle(Buffer, (real32) World.TileWidth*Col , (real32) World.TileHeight*Row, World.TileWidth, World.TileHeight, 0.4f, 0.4f, 0.4f);
			}
		}
	}
	DrawRectangle(Buffer,  XPos-0.5*PlayerWidth, YPos-PlayerHeight, PlayerWidth, PlayerHeight , 1.f,1.f,1.f);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
	game_state *GameState = (game_state* ) Memory->PermanentStorage;
	GameOutputSound(SoundBuffer, 400);
}





#if 0
internal void  
RenderWeirdGradient(game_offscreen_buffer* Buffer, int XOffset, int YOffset)
{	
	uint8* Row = (uint8*)Buffer->Memory;

	for(int Y= 0; Y< Buffer->Height; ++Y)
	{
		uint32* Pixel = (uint32*)Row;
		for(int X= 0; X < Buffer->Width; ++X)
		{  

			uint8 Blue = (uint8)(X+XOffset);			
			uint8 Green = (uint8)(Y+YOffset);

	
			/*
						  8b 8b 8b 8b = 36bit
				Memmory   BB GG RR xx
				Registry  xx RR GG BB
			*/

			*Pixel++ = ( ( Green << 8) | Blue);
		}

		Row += Buffer->Pitch;
	}
}
#endif
