
#include "handmade.hpp"

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
GameUpdateAndRender(game_offscreen_buffer* Buffer, int BlueOffset, int GreenOffset)
{
	//TODO: Allow sample offset here for more robust platform options
//	GameOutPutSound(SampleCountToOutPut, SoundBuffer);
	RenderWeirdGradient(Buffer, BlueOffset, GreenOffset);
}



