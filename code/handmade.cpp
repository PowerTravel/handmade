
#include "handmade.h"
#include "handmade_random.h"
#include "handmade_tile.cpp"

internal void
GameOutputSound(game_sound_output_buffer* SoundBuffer, int ToneHz)
{
	local_persist real32 tSine = 0.f;

	int16 ToneVolume = 2000;
	int SamplesPerPeriod = SoundBuffer->SamplesPerSecond/ToneHz;

	real32 twopi = (real32) (2.0*Pi32);

	int16 *SampleOut = SoundBuffer->Samples;
	for(int SampleIndex = 0; 
		SampleIndex < SoundBuffer->SampleCount; 
		++SampleIndex)
	{

#if 0
		real32 SineValue = Sin(tSine);
		int16 SampleValue = (int16)(SineValue * ToneVolume);

		tSine += twopi/(real32)SamplesPerPeriod;	
		if(tSine > twopi)
		{
			tSine -= twopi;
		}
#else	
		int16 SampleValue = 0;
#endif	
		*SampleOut++ = SampleValue; // Left Speker
		*SampleOut++ = SampleValue; // Rright speaker
	}
}


internal void
DrawRectangle(game_offscreen_buffer* Buffer, real32 RealMinX, real32 RealMinY, real32 Width, real32 Height, 
				real32 R, real32 G, real32 B)
{
	int32 MinX = RoundReal32ToInt32(RealMinX);
	int32 MinY = RoundReal32ToInt32(RealMinY);
	int32 MaxX = RoundReal32ToInt32(RealMinX+Width);
	int32 MaxY = RoundReal32ToInt32(RealMinY+Height);

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
			*Pixel++ = ((TruncateReal32ToInt32(R*255.f) << 16) |
					   (TruncateReal32ToInt32(G*255.f) << 8) 	|
					   (TruncateReal32ToInt32(B*255.f) << 0));
		}
		Row += Buffer->Pitch;
	}
}



// makes the packing compact
#pragma pack(push, 1)
struct bmp_header{

	uint16   FileType;     /* File type, always 4D42h ("BM") */
	uint32  FileSize;     /* Size of the file in bytes */
	uint16   Reserved1;    /* Always 0 */
	uint16   Reserved2;    /* Always 0 */
	uint32  BitmapOffset; /* Starting position of image data in bytes */

	uint32 HeaderSize;       /* Size of this header in bytes */
	int32  Width;           /* Image width in pixels */
	int32  Height;          /* Image height in pixels */
	uint16  Planes;          /* Number of color planes */
	uint16  BitsPerPixel;    /* Number of bits per pixel */
	uint32 Compression;     /* Compression methods used */
	uint32 SizeOfBitmap;    /* Size of bitmap in bytes */
	int32  HorzResolution;  /* Horizontal resolution in pixels per meter */
	int32  VertResolution;  /* Vertical resolution in pixels per meter */
	uint32 ColorsUsed;      /* Number of colors in the image */
	uint32 ColorsImportant; /* Minimum number of important colors */
	/* Fields added for Windows 4.x follow this line */

	uint32 RedMask;       /* Mask identifying bits of red component */
	uint32 GreenMask;     /* Mask identifying bits of green component */
	uint32 BlueMask;      /* Mask identifying bits of blue component */
	uint32 AlphaMask;     /* Mask identifying bits of alpha component */
	uint32 CSType;        /* Color space type */
	int32  RedX;          /* X coordinate of red endpoint */
	int32  RedY;          /* Y coordinate of red endpoint */
	int32  RedZ;          /* Z coordinate of red endpoint */
	int32  GreenX;        /* X coordinate of green endpoint */
	int32  GreenY;        /* Y coordinate of green endpoint */
	int32  GreenZ;        /* Z coordinate of green endpoint */
	int32  BlueX;         /* X coordinate of blue endpoint */
	int32  BlueY;         /* Y coordinate of blue endpoint */
	int32  BlueZ;         /* Z coordinate of blue endpoint */
	uint32 GammaRed;      /* Gamma red coordinate scale value */
	uint32 GammaGreen;    /* Gamma green coordinate scale value */
	uint32 GammaBlue;     /* Gamma blue coordinate scale value */
};
#pragma pack(pop)

internal void 
BlitBMP(game_offscreen_buffer* Buffer, real32 RealMinX, real32 RealMinY, loaded_bitmap BitMap)
{
	int32 MinX = RoundReal32ToInt32(RealMinX);
	int32 MinY = RoundReal32ToInt32(RealMinY);
	int32 MaxX = RoundReal32ToInt32(RealMinX+(real32)BitMap.Width);
	int32 MaxY = RoundReal32ToInt32(RealMinY+(real32)BitMap.Height);


	uint32 ClippingOffsetX = 0;
	uint32 ClippingOffsetY = 0;
	if(MinX < 0)
	{
		ClippingOffsetX = - MinX;
		MinX = 0;
	}
	if(MinY < 0)
	{
		ClippingOffsetY = - MinY;
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


	uint32 BytesPerPixel = 4;

	uint8* SourceRow = (uint8*) BitMap.Pixels + (BitMap.Width*ClippingOffsetY + ClippingOffsetX)*BytesPerPixel;
	uint8* DestinationRow = (uint8*) ( (uint8*) Buffer->Memory + MinY* Buffer->Pitch + 
											 MinX * Buffer->BytesPerPixel );


	uint32 BitmapPitch = BitMap.Width * BytesPerPixel;

	for(int Y = MinY; Y < MaxY; ++Y)
	{
		uint32* SourcePixel = (uint32*) SourceRow;
		uint32* DesinationPixel =  (uint32*) DestinationRow; 
	 	for(int X = MinX; X < MaxX; ++X)
		{


			/// Note(Jakob): This loop is SLOW!! Tanking the fps

			uint32 Source = *SourcePixel;

			real32 Alpha = (real32) ((Source & 0xFF000000)>>24)/255.f;

			real32 Red =  (real32) (((Source & 0x00FF0000)>>16))/255.f;
			real32 Green =(real32) (((Source & 0x0000FF00)>>8) )/255.f;
			real32 Blue = (real32) (((Source & 0x000000FF)>>0) )/255.f;

			uint32 DestPix = *DesinationPixel;
			real32 DRed =(real32)    (((DestPix & 0x00FF0000)>>16))/255.f;
			real32 DGreen = (real32) (((DestPix & 0x0000FF00)>>8))/255.f;
			real32 DBlue =(real32)   (((DestPix & 0x000000FF)>>0))/255.f;

			Red = (1-Alpha) *DRed + Alpha*Red;
			Green = (1-Alpha)* DGreen + Alpha*Green;
			Blue = (1-Alpha) *DBlue + Alpha*Blue;

			int32 R = (int) (Red*255+0.5);
			int32 G = (int) (Green*255+0.5);
			int32 B = (int) (Blue*255+0.5);

			Source = (R << 16) | (G << 8) | (B << 0);

			*DesinationPixel = Source;

			DesinationPixel++;
			SourcePixel++;
	
		}
		DestinationRow += Buffer->Pitch;
		SourceRow += BitmapPitch;
	} 
}


internal loaded_bitmap
DEBUGReadBMP( thread_context* Thread, memory_arena* Memory, 
			   debug_platform_read_entire_file* ReadEntireFile, char* FileName)
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

	loaded_bitmap Result = {};
	debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);

	if (ReadResult.ContentSize != 0)
	{
		bmp_header* Header = (bmp_header*)ReadResult.Contents;

		Assert(Header->BitsPerPixel == 32);
		Assert(Header->Compression == 3);

		Result.Pixels = PushArray(Memory, Header->Width*Header->Height, uint32);
		Result.Width = Header->Width;
		Result.Height = Header->Height;

		uint32* Target = (uint32*)Result.Pixels;
		uint32* Source = (uint32*)(((uint8*)ReadResult.Contents) + Header->FileSize - 4);

		bit_scan_result RedBitShift = FindLeastSignificantSetBit(Header->RedMask);
		bit_scan_result GreenBitShift = FindLeastSignificantSetBit(Header->GreenMask);
		bit_scan_result BlueBitShift = FindLeastSignificantSetBit(Header->BlueMask);
		bit_scan_result AlphaBitShift = FindLeastSignificantSetBit(Header->AlphaMask);

		Assert(RedBitShift.Found);
		Assert(GreenBitShift.Found);
		Assert(BlueBitShift.Found);

		for (int Y = 0; Y < Result.Height; ++Y)
		{
			for (int X = 0; X < Result.Width; ++X)
			{

				uint32 Pixel = *Source--;
	
				bit_scan_result BitShift = {};


				uint32 Red   = Pixel & Header->RedMask;     
				uint8 R = (uint8) (Red >> RedBitShift.Index);

				uint32 Green = Pixel & Header->GreenMask;
				uint8 G = (uint8) (Green >> GreenBitShift.Index);
			
				uint32 Blue  = Pixel & Header->BlueMask;
				uint8 B = (uint8) (Blue >> BlueBitShift.Index);
				
				uint8 A = 0xff;
				if (AlphaBitShift.Found)
				{
					uint32 Alpha = Pixel & Header->AlphaMask;
					A = (uint8)(Alpha >> AlphaBitShift.Index);
				}
				*Target++ = (A << 24) | (R << 16) | (G << 8) | (B << 0);

			}
		}		
	}

	return Result;
}

bool32 closeEnough(real32 a, real32 b) 
{

	real32 diff = a - b;

	if(diff < 0)
	{
		diff = -diff;
	}

	return (diff < .001);

}

real32 betterGuess(real32 x, real32 g) 
{
   return ( (g + x/g) / 2.f );
}

real32 test(real32 x, real32 g) 
{
	if( closeEnough(x/g, g) )
	{
		return g;
	}else{
		return test(x, betterGuess(x, g) );
	}
}



void draw_mandelbrot_set(game_offscreen_buffer* Buffer)
{
	int32 YResolution = 525;
	int32 XResolution = 840;

	real32 ScreenRatio = ( (real32) XResolution ) / ( (real32) YResolution );

	real32 XOffset = ScreenRatio / 2.f;
	real32 YOffset = 1.f /2.f;


	uint32 ConvergeIterations = 100;
	real32 Divergance_Squared = 10;

	uint8* Row = (uint8*) Buffer->Memory;

	for(real32 Y = 0;  Y < Buffer->Height; ++Y)
	{
		uint32* Pixel = (uint32*) Row;
		for(real32 X = 0;  X < Buffer->Width; ++X)
		{
			real32 Re = 2.f * ( ( X / ( (real32) Buffer->Width ) ) - 0.5f);
			real32 Im = 2.f * ( ( Y / ( (real32) Buffer->Height ) ) - 0.5f);

			real32 Rec = -0.7f;
			real32 Imc = 0.3;

			uint32 Iterations = 0;
			real32 Distance_Squared = 0;
			real32 R = 0;
			real32 Color = 0;
			while( (Iterations < ConvergeIterations) )
			{
				real32 Re_n = Re*Re - Im*Im + Rec;
				real32 Im_n = 2*Re*Im + Imc;

				Distance_Squared = Re_n * Re_n + Im_n*Im_n;


				if(Distance_Squared > Divergance_Squared )
				{
					Color = (real32) Iterations/(real32)ConvergeIterations;
				}

				Re = Re_n;
				Im = Im_n;

				++Iterations;
			}
				*Pixel++ = ( (TruncateReal32ToInt32(Color*255.f) << 16) |
				      	   (  TruncateReal32ToInt32(Color*255.f) <<  8) |
				       	   (  TruncateReal32ToInt32(Color*255.f) <<  0)  );
		}
		Row += Buffer->Pitch;
	}
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

		GameState->PlayerPos.RelTile.X = 0;
		GameState->PlayerPos.RelTile.Y = 0;
		GameState->PlayerPos.RelTile.Z = 0;
		GameState->PlayerPos.AbsTileX = 1;
		GameState->PlayerPos.AbsTileY = 3;
		GameState->PlayerPos.AbsTileZ = 0;
		GameState->dv = V3(0.f, 0.f, 0.f);
		GameState->MovedUp = true;
		InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize- sizeof(game_state),
						(uint8*) Memory->PermanentStorage + sizeof(game_state) );


		GameState->Backdrop = DEBUGReadBMP(Thread, &GameState->WorldArena, 
											Memory->DEBUGPlatformReadEntireFile ,
										"..\\handmade\\data\\test\\test_background.bmp");
		GameState->Head = DEBUGReadBMP(Thread, &GameState->WorldArena, 
											Memory->DEBUGPlatformReadEntireFile ,
									"..\\handmade\\data\\test\\test_hero_front_head.bmp");
		GameState->Body = DEBUGReadBMP(Thread, &GameState->WorldArena, 
											Memory->DEBUGPlatformReadEntireFile ,
									"..\\handmade\\data\\test\\test_hero_front_torso.bmp");

		GameState->Map = DEBUGReadBMP(Thread, &GameState->WorldArena, 
											Memory->DEBUGPlatformReadEntireFile ,
									"..\\handmade\\data\\map.bmp");

		GameState->World = PushStruct(&GameState->WorldArena, world);
		world* World = GameState->World;
		World->TileMap = PushStruct(&GameState->WorldArena,tile_map);

		tile_map* TileMap = World->TileMap;
		real32 TileSideInMeters = 1.4f;
		InitializeTileMap(TileMap, TileSideInMeters);


		uint32* SourceRow = (uint32*) GameState->Map.Pixels;
		for(int32 RowIndex = GameState->Map.Height-1; RowIndex >= 0; --RowIndex)
		{
			uint32* Pixel = SourceRow; 
			for(int32 ColIndex = GameState->Map.Width-1; ColIndex >= 0; --ColIndex)
			{
				int32 TileValue = 0;
				uint32 White= 0xffffffff;

				if( *Pixel == White )
				{
					TileValue = 1;
				}else{
					TileValue = 2;
				}
				
				SetTileValueAbs(&GameState->WorldArena,World->TileMap, 
								ColIndex, RowIndex, 0, TileValue );

				Pixel++;
			}

			SourceRow += GameState->Map.Width;
		}
/*
		uint32 RandomNumberIndex = 0;
		uint32 TilesPerWidth = 17;
		uint32 TilesPerHeight = 9;
		uint32 TilesPerDepth = 1;


		int32 ScreenX = 0;
		int32 ScreenY = 0;
		int32 ScreenZ = 0;

		int32 TileX = 0;
		int32 TileY = 0;
		int32 TileZ = 0;

		bool32 GoUp = true;

		bool32 FirstScreen = true;
		bool32 ZChanged = false;

		int32 AbsTileX = TileX;
		int32 AbsTileY = TileY;
		int32 AbsTileZ = TileZ;
		int32 AbsTileXP = TileX;
		int32 AbsTileYP = TileY;
		int32 AbsTileZP = TileZ;
		for( uint32 ScreenIndex = 0; ScreenIndex < 100; ++ScreenIndex)
		{
			Assert(RandomNumberIndex < ArrayCount(RandomNumberTable));
			uint32 RandomChoice = RandomNumberTable[RandomNumberIndex++] % 3;

			if(!FirstScreen)
			{
				if(RandomChoice == 0)
				{
					ScreenX +=1;
				}else if(RandomChoice == 1){
					ScreenY +=1;
				}else{
					if(GoUp)
					{
						ScreenZ++;
					}else{
						ScreenZ--;
					}
					GoUp = !GoUp;
					ZChanged = true;
				}
			}
		

			for(uint32 ScreenTileY  = 0; ScreenTileY < TilesPerHeight; ++ScreenTileY)
			{
				
				for(uint32 ScreenTileX  = 0; ScreenTileX < TilesPerWidth; ++ScreenTileX)
				{
					AbsTileX = TileX+ScreenX*TilesPerWidth+ScreenTileX;
					AbsTileY = TileY+ScreenY*TilesPerHeight+ScreenTileY;
					AbsTileZ = TileZ+ScreenZ;

					uint32 TileValue = 1;
					
					if( (ScreenTileX == 0) || (ScreenTileX == (TilesPerWidth - 1)) )
					{
						if( ScreenTileY == (  TilesPerHeight / 2)  )
						{
							TileValue = 1;
						}else{
							TileValue = 2;
						}
					}
					
					if( (ScreenTileY == 0) || (ScreenTileY == (TilesPerHeight - 1)) )
					{
						if(ScreenTileX == TilesPerWidth/2)
						{
							TileValue = 1;
						}else{
							TileValue = 2;
						}
					}

					// If Z changed we need to add a portal on the previous floor
					if( (ScreenTileX == TilesPerWidth/2) && (ScreenTileY == TilesPerHeight/2) )
					{
						if(ZChanged)
						{
							TileValue = 3;
							SetTileValueAbs(&GameState->WorldArena,World->TileMap, 
											 AbsTileXP, AbsTileYP, AbsTileZP, 3 );
							ZChanged = false;
						}
						
						AbsTileXP = AbsTileX;
						AbsTileYP = AbsTileY;
						AbsTileZP = AbsTileZ;
					}
					

					SetTileValueAbs(&GameState->WorldArena,World->TileMap, 
								AbsTileX, AbsTileY, AbsTileZ, TileValue );
				}
			}
		
			FirstScreen = false;
		}
	*/	
		Memory->IsInitialized = true;	
	}

	#if 1
	world* World = GameState->World;
	tile_map* TileMap = World->TileMap;


	real32 PlayerWidth = 0.75f*TileMap->TileSideInMeters;
	real32 PlayerHeight = TileMap->TileSideInMeters;
	real32 dt = Input->dt;

	real32 dvx = 0.f;
	real32 dvy = 0.f;

	real32 dax = 0.f;
	real32 day = 0.f;

	// Meters per second
	real32 speed;


	for(int32 ControllerIndex = 0; 
		ControllerIndex < ArrayCount(Input->Controllers); 
		++ControllerIndex)
	{
		
		game_controller_input* Controller = GetController(Input,ControllerIndex);

		if(Controller->IsAnalog )
		{
			// Controller
		}else{
			
			if(Controller->Start.EndedDown)
			{
				speed = 50;
			}else{
				speed = 9;
			}

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
				dvy += speed;
			}
			if(Controller->LeftStickDown.EndedDown)
			{
				dvy -= speed;
			}
			if(Controller->RightShoulder.EndedDown)
			{
				GameState->PlayerPos.RelTile.X = 0.f;
				GameState->PlayerPos.RelTile.Y = 0.f;	
				GameState->PlayerPos.RelTile.Z = 0.f;	
				GameState->PlayerPos.AbsTileX = 1;
				GameState->PlayerPos.AbsTileY = 3;
				GameState->PlayerPos.AbsTileZ = 0;
			}
		}	
	}

	v3 delta_pos = V3(dt * dvx, dt * dvy, 0);


	tile_map_position NewPosition = MoveNewTileMapPosition(TileMap, GameState->PlayerPos, delta_pos);

	tile_map_position NewPositionL = MoveNewTileMapPosition(TileMap, NewPosition,
									-PlayerWidth*0.5f, 0, 0);

	tile_map_position NewPositionR = MoveNewTileMapPosition(TileMap, NewPosition,
									PlayerWidth*0.5f, 0, 0);

	if( IsTileMapPointEmpty(TileMap, NewPositionR) &&
		IsTileMapPointEmpty(TileMap, NewPositionL) &&
		IsTileMapPointEmpty(TileMap, NewPosition))
	{		
		if( (GetTileValue(TileMap, NewPosition) == 3) && (!GameState->JustMoved) )
		{
			if(GameState->MovedUp)
			{
				NewPosition.AbsTileZ++;
			}else{
				NewPosition.AbsTileZ--;
			}
			GameState->MovedUp = !GameState->MovedUp;
			GameState->JustMoved = true;
		}

		if((GetTileValue(TileMap, NewPosition) != 3)){
			GameState->JustMoved = false;
		}

		GameState->PlayerPos  = NewPosition;
	}

	

	real32 TileSideInPixels =64.f;
	real32 PixelsPerMeter =TileSideInPixels/TileMap->TileSideInMeters;

	DrawRectangle(Buffer,  0.f, 0.f,(real32) Buffer->Width, (real32) Buffer->Height , 1.f,0.f,0.f);


	real32 CenterX = 0.5f * (real32)Buffer->Width;
	real32 CenterY = 0.5f * (real32)Buffer->Height;

	v2 PlayerOffset = {}; 
	PlayerOffset.X = GameState->PlayerPos.RelTile.X*PixelsPerMeter;
	PlayerOffset.Y = GameState->PlayerPos.RelTile.Y*PixelsPerMeter;

//	BlitBMP(Buffer, 0.f, 0.f, GameState->Backdrop);

 	for(int32 RelRow = -7; RelRow<7; RelRow++)
	{
		for(int32 RelCol = -11; RelCol<11; RelCol++)
		{
			uint32 Col = GameState->PlayerPos.AbsTileX + RelCol;
			uint32 Row = GameState->PlayerPos.AbsTileY - RelRow;

			uint32 TileValue = GetTileValue(TileMap, Col, Row, GameState->PlayerPos.AbsTileZ);
			real32 color = 0;


			real32 MinX = CenterX + ((real32) RelCol) * TileSideInPixels - PlayerOffset.X;
			real32 MinY = CenterY + ((real32) RelRow) * TileSideInPixels + PlayerOffset.Y;
			
			if(TileValue > 0)
			{
				if(TileValue == 1)
				{
					color = 0.5;
					DrawRectangle(Buffer, MinX, MinY, TileSideInPixels, TileSideInPixels, color, color, color);
				}else if(TileValue == 2){
					color =  0.3f;
					DrawRectangle(Buffer, MinX, MinY, TileSideInPixels, TileSideInPixels, color, color, color);
				}else if(TileValue == 3){
					color =  0.7f;
					DrawRectangle(Buffer, MinX, MinY, TileSideInPixels, TileSideInPixels, color, color, color);
				}
			}
		}

	}

	DrawRectangle(Buffer, CenterX - PlayerOffset.X , CenterY + PlayerOffset.Y,
				  TileSideInPixels, TileSideInPixels , 0.f,0.f,0.f);

	real32 PlayerLeft = CenterX + GameState->PlayerPos.AbsTileX*TileSideInPixels;
	real32 PlayerBottom = CenterY + GameState->PlayerPos.AbsTileY*TileSideInPixels;


	real32 PlayerWidthInPixels = PlayerWidth*PixelsPerMeter;
	real32 PlayerHeightInPixels = PlayerHeight*PixelsPerMeter;


	BlitBMP(Buffer, CenterX+(TileSideInPixels - 3.f*PlayerWidthInPixels)/2.f, CenterY - 1.2f*PlayerHeightInPixels,
							GameState->Head);

//	DrawRectangle(Buffer,  CenterX+(TileSideInPixels - PlayerWidthInPixels)/2.f, CenterY - 0.5f*PlayerHeightInPixels,
//						 PlayerWidthInPixels, PlayerHeightInPixels ,1.f,1.f,1.f);

	#else 
	draw_mandelbrot_set(Buffer);
 	#endif

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
