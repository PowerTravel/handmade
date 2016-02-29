
#include "handmade.hpp"

internal void
GameOutputSound(game_sound_output_buffer* SoundBuffer, int ToneHz)
{
	// TODO: tSin drifts after a while.

	local_persist real32 tSine;
	int16 ToneVolume = 2000;
	int SamplesPerPeriod = SoundBuffer->SamplesPerSecond/ ToneHz;


	real32 twopi = (real32) (2.0*Pi32);

	int16 *SampleOut = SoundBuffer->Samples;
	for(int SampleIndex = 0; 
		SampleIndex < SoundBuffer->SampleCount; 
		++SampleIndex)
	{
		real32 SineValue = sinf(tSine);
		int16 SampleValue = (int16)(SineValue * ToneVolume);
		*SampleOut++ = SampleValue; // Left Speker
		*SampleOut++ = SampleValue; // Rright speaker
		tSine += twopi/(real32)SamplesPerPeriod;
	}

	if(tSine > twopi)
	{
		tSine -= twopi;
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
			uint8 Red = 0;
	
			/*
						  8b 8b 8b 8b = 36bit
				Memmory   BB GG RR xx
				Registry  xx RR GG BB
			*/

			*Pixel++ = ( (Red << 16 )  | ( Green << 8) | Blue);
		}

		Row += Buffer->Pitch;
	}
}


internal void 
GameUpdateAndRender(game_memory* Memory,
					game_offscreen_buffer* Buffer, 
					game_sound_output_buffer* SoundBuffer,
					game_input* Input)
{
	Assert( (&Input->Controllers[0].Terminator - &Input->Controllers[0].Button[0]) == 
			 (ArrayCount(Input->Controllers[0].Button)) );
	Assert(sizeof(game_state) <= (Memory->PermanentStorageSize) );
	game_state *GameState = (game_state* ) Memory->PermanentStorage;

	if(!Memory->IsInitialized)
	{
		char* Filename = __FILE__;

		// NOTE: Temporary solution for file reading
		debug_read_file_result BitmapMemory = DEBUGPlatformReadEntireFile(Filename);
		if(BitmapMemory.Contents)
		{
			DEBUGPlatformWriteEntireFile("w:/handmade/data/test.out",BitmapMemory.ContentSize,BitmapMemory.Contents);
			DEBUGFreeFileMemory(BitmapMemory.Contents);	
		}
		
		GameState->ToneHz = 261;
		
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
	}

	//TODO: Allow sample offset here for more robust platform options
	GameOutputSound(SoundBuffer, GameState->ToneHz);

	RenderWeirdGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset);
}



