#ifndef HANDMADE_HPP
#define HANDMADE_HPP


#define ArrayCount(Array) ( sizeof(Array)/sizeof((Array)[0]) )

/*

	TODO: Services that the platformlayer provides to the game.

*/


/*

	NOTE: Services that the game provides the platformlayer.

*/
struct game_offscreen_buffer
{
	// Note: Pixels are always 32-bits wide: Memory Order BB GG RR XX
	void* Memory;
	int Width;
	int Height;
	int Pitch;
};

struct game_sound_output_buffer
{
	int SamplesPerSecond;
	int SampleCount;
	int16* Samples;
};

struct game_button_state
{
	int HalfTransitionCount;
	bool32 EndedDown;
};

struct game_controller_input
{	
	bool32 IsAnalog;
	real32 StartX;
	real32 StartY;
	real32 MinX;
	real32 MinY;
	real32 MaxX;
	real32 MaxY;
	real32 EndX;
	real32 EndY;
	
	union
	{
		game_button_state Button[6];
		struct
		{
			game_button_state Up;
			game_button_state Down;
			game_button_state Left;
			game_button_state Right;
			game_button_state LeftShoulder;
			game_button_state RightShoulder;
		};
	};

};

struct game_input
{
	game_controller_input Controllers[4];
};

internal void
GameOutputSound(game_sound_output_buffer* SoundBuffer, int ToneHz);

void RenderWeirdGradient(game_offscreen_buffer* Buffer, int XOffset, int YOffset);


// FourThings: timing, controller/keyboard input, bitmap buffer to use, soundbuffer to use
void GameUpdateAndRender(game_offscreen_buffer* Buffer,
						 game_sound_output_buffer* SoundBuffer,
						 game_input* Input);


#endif
