#ifndef HANDMADE_HPP
#define HANDMADE_HPP

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


void RenderWeirdGradient(game_offscreen_buffer* Buffer, int XOffset, int YOffset);


// FourThings: timing, controller/keyboard input, bitmap buffer to use, soundbuffer to use
void GameUpdateAndRender(game_offscreen_buffer* Buffer, int BlueOffset, int GreenOffset);


#endif
