#ifndef HANDMADE_HPP
#define HANDMADE_HPP


/*
	NOTE: 
	 
	HANDMADE_INTERNAL
		0: build for public release
		1: build for developer only

	HANDMADE_SLOW
		0: no slow code allowed
		1: slow code welcome
*/


#if HANDMADE_SLOW
#define Assert(Expression) if(!(Expression)){ *(int *)0 = 0;}
#else
#define Assert(Expression)
#endif



#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terrabytes(Value) (Gigabytes(Value)*1024LL)

#define ArrayCount(Array) ( sizeof(Array)/sizeof((Array)[0]) )

inline uint32
SafeTruncateUInt64(uint64 Value)
{
	Assert(Value <= 0xFFFFFFFF);
	uint32 Result = (uint32)Value;
	return Result;  
}

/*

	NOTE: Services that the platformlayer provides to the game.
		  Implemented on Platform layer
*/

#if HANDMADE_INTERNAL
/*
	IMPORTANT: These NOT for doing anything in the shipping game.
			   They are blocking the write and dont protect against 
			   lost data. Savefiles become corrupted if this call fails 
			   mid wite. 
			   Also final api should be along the lines of
				uint64 FileSize = GetFileSize(FileName);
				void* BitmapMemory = ReserveMemory(Memory, FileSize);
				ReadEntireFileMemory(FileName, BitmapMemory);
*/
struct debug_read_file_result{
	uint32 ContentSize;
	void* Contents;
};
internal debug_read_file_result DEBUGPlatformReadEntireFile(char* Filename);
internal void DEBUGFreeFileMemory(void* Memory);	
internal bool32 DEBUGPlatformWriteEntireFile(char* Filename, uint32 MemorySize, void* Memory);
#endif


/*

	NOTE: Services that the game provides the platformlayer.
		  Implemented in game layer
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
	// TODO: insert Game Clock here
	game_controller_input Controllers[4];
};

struct game_memory
{
	bool32 IsInitialized;
	uint64 PermanentStorageSize;
	void* PermanentStorage; // Note: Required to be initialized to zero at startup
	uint64 TransientStorageSize;
	void* TransientStorage;
};

internal void
GameOutputSound(game_sound_output_buffer* SoundBuffer, int ToneHz);

void RenderWeirdGradient(game_offscreen_buffer* Buffer, int XOffset, int YOffset);


// FourThings: timing, controller/keyboard input, bitmap buffer to use, soundbuffer to use
internal void 
GameUpdateAndRender(	 game_memory* Memory,
						 game_offscreen_buffer* Buffer,
						 game_sound_output_buffer* SoundBuffer,
						 game_input* Input);

///
///
///
///
struct game_state{
	int ToneHz;
	int GreenOffset;
	int BlueOffset;
};
#endif
