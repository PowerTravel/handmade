
#include "handmade.hpp"

internal void
GameOutputSound(game_sound_output_buffer* SoundBuffer, int ToneHz)
{
	// TODO: tSin drifts after a while.

	local_persist real32 tSine;
	int16 ToneVolume = 2000;
	int SamplesPerPeriod = SoundBuffer->SamplesPerSecond/ ToneHz;


	real32 twopi = 2.0f*Pi32;

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

			uint8 Blue = (X+XOffset);			
			uint8 Green = (Y+YOffset);
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
GameUpdateAndRender(game_offscreen_buffer* Buffer, 
					game_sound_output_buffer* SoundBuffer,
					game_input* Input)
{
	local_persist int BlueOffset = 0; 
	local_persist int GreenOffset = 0;
	local_persist int ToneHz = 261;

	game_controller_input* Input0 = &Input->Controllers[0]; 

	if(Input0->IsAnalog){
		//TODO use analog movement tuning
		BlueOffset += (int)(4.0f*Input0->EndX);
		ToneHz = 261 + (int)(128.0f*(Input0->EndY));
	}else{
		//TODO use keyboard movement tuning	
	}

	//Input.AButtonEndedDown;
	//Input.AButteonHalfTransitionCount;
	if(Input0->Down.EndedDown)
	{
		GreenOffset +=1;
	}
	

	//TODO: Allow sample offset here for more robust platform options
	GameOutputSound(SoundBuffer, ToneHz);

	RenderWeirdGradient(Buffer, BlueOffset, GreenOffset);
}



