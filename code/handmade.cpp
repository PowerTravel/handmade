
#include "handmade.hpp"

internal void
GameOutputSound(game_state* GameState, game_sound_output_buffer* SoundBuffer)
{

	int16 ToneVolume = 2000;
	int SamplesPerPeriod = SoundBuffer->SamplesPerSecond/ GameState->ToneHz;


	real32 twopi = (real32) (2.0*Pi32);

	int16 *SampleOut = SoundBuffer->Samples;
	for(int SampleIndex = 0; 
		SampleIndex < SoundBuffer->SampleCount; 
		++SampleIndex)
	{
		real32 SineValue = sinf(GameState->tSine);
		int16 SampleValue = (int16)(SineValue * ToneVolume);
		*SampleOut++ = SampleValue; // Left Speker
		*SampleOut++ = SampleValue; // Rright speaker

		GameState->tSine += twopi/(real32)SamplesPerPeriod;	
		if(GameState->tSine > twopi)
		{
			GameState->tSine -= twopi;
		}
	}
}



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

internal void
RenderPlayer(game_offscreen_buffer* Buffer, int PlayerX, int PlayerY)
{
	uint8* EndOfBuffer = (uint8 *) Buffer->Memory + Buffer->Pitch*Buffer->Height;
	uint32 Color = 0xFFFFFFFF;	
	int Top = PlayerY;
	int Bottom = PlayerY+10;
	for(int X = PlayerX; X<PlayerX+10; ++X){
		if(( X>=0 ) && ( X<Buffer->Width ))
		{
			uint8* Pixel =  ((uint8*) Buffer->Memory + 
							X*Buffer->BytesPerPixel + 
							Top*Buffer->Pitch);
	
			for(int Y = Top; Y<Bottom; ++Y)
			{
				if( (Pixel + 4 >= Buffer->Memory) && (Pixel< EndOfBuffer))
				{
					*(uint32*) Pixel = Color;
					
				}
				Pixel += Buffer->Pitch;
			}
		}
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
		char* Filename = __FILE__;

		// NOTE: Temporary solution for file reading
		debug_read_file_result BitmapMemory = Memory->DEBUGPlatformReadEntireFile(Filename);
		if(BitmapMemory.Contents)
		{
			Memory->DEBUGPlatformWriteEntireFile("w:/handmade/data/test.out",BitmapMemory.ContentSize,BitmapMemory.Contents);
			Memory->DEBUGPlatformFreeFileMemory(BitmapMemory.Contents);	
		}
		
		GameState->ToneHz = 261;
		GameState->tSine = 0.0f;
		GameState->PlayerX=100;
		GameState->PlayerY=100;


		Memory->IsInitialized = true;

	}

	for(int ControllerIndex = 0; 
		ControllerIndex < ArrayCount(Input->Controllers); 
		++ControllerIndex)
	{
		
		game_controller_input* Controller = GetController(Input,ControllerIndex);

		if(Controller->IsAnalog ){
			//NOTE use analog movement tuning
			GameState->BlueOffset += (int)(4.0f*Controller->LeftStickAverageX);
			GameState->ToneHz = 261 + (int)(128.0f*Controller->LeftStickAverageY);


		}else{

			if(Controller->LeftStickLeft.EndedDown)
			{
				GameState->BlueOffset -= 4;
			}

			if(Controller->LeftStickRight.EndedDown)
			{
				GameState->BlueOffset += 4;
			}
		}	

		GameState->PlayerX += (int)(8.0f*Controller->LeftStickAverageX);
		GameState->PlayerY -= (int)(8.0f*Controller->LeftStickAverageY);
	}

	RenderWeirdGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset);

	RenderPlayer(Buffer, GameState->PlayerX, GameState->PlayerY);
}


extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
	game_state *GameState = (game_state* ) Memory->PermanentStorage;
	GameOutputSound(GameState, SoundBuffer);
}


