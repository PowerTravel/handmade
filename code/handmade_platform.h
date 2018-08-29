#ifndef HANDMAD_PLATFORM_H
#define HANDMAD_PLATFORM_H

//	NOTE: Stuff that gets refferenced in the platform layer as well as
//			the game layer

/*
	NOTE: 
	 
	HANDMADE_INTERNAL
		0: build for public release 
		1: build for developer only

	HANDMADE_SLOW
		0: no slow code allowed
		1: slow code welcome
*/


// Note(Jakob): Compilers

#ifndef COMPILER_MSVC
#define COMPILER_MSVC 0
#endif 

#ifndef COMPILER_LLVM
#define COMPILER_LLVM 0
#endif 

#if !COMPILER_MSVC && !COMPILER_LLVM

#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#else
// Todo(Jakob): More compilers
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#endif

#endif

#if COMPILER_MSVC
#include <intrin.h>
#endif

#include <stdint.h>
#include <stddef.h> // size_t exists in this header on some platforms

#define internal		 static
#define local_persist    static
#define global_variable  static

#define Pi32 3.14159265359

#ifndef UINT32_MAX
#define UINT32_MAX 0xffffffff
#endif

#if HANDMADE_SLOW
#define Assert(Expression) if(!(Expression)){ *(int *)0 = 0;}
#else
#define Assert(Expression)
#endif // HANDMADE_SLOW

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terrabytes(Value) (Gigabytes(Value)*1024LL)

#define ArrayCount(Array) ( sizeof(Array)/sizeof((Array)[0]) )


typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef int32 bool32;

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef size_t memory_index;

typedef float real32;
typedef double real64;

struct thread_context
{
	int placeholder;
};

#if HANDMADE_INTERNAL

struct debug_read_file_result{
	uint32 ContentSize;
	void* Contents;
};
#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context* Thread, void* Memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platfrom_free_file_memory);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(thread_context* Thread, char* Filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(thread_context* Thread, char* Filename, uint32 MemorySize, void* Memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

#endif // HANDMADE_INTERNAL

inline uint32 
SafeTruncateUInt64(uint64 Value)
{
	Assert(Value <= 0xFFFFFFFF);
	uint32 Result = (uint32)Value;
	return Result;  
}


/*
  NOTE: Services that the game provides to the platform layer.
*/
struct game_offscreen_buffer
{
	// Note: Pixels are always 32-bits wide: Memory Order BB GG RR XX
	void* Memory;
	// In Pixels
	int Width;
	int Height;
	int Pitch;
	
	int BytesPerPixel;
};

struct game_sound_output_buffer
{
	int SamplesPerSecond;
	int SampleCount;
	int channels;
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
	bool32 IsConnected;

	union
	{
		real32 Averages[6];
		struct
		{
			real32 LeftStickAverageX;
			real32 LeftStickAverageY;
			real32 RightStickAverageX;
			real32 RightStickAverageY;
			real32 LeftTriggerAverage;
			real32 RightTriggerAverage;
		};
	};

	union
	{
		game_button_state Button[24];
		struct
		{

			game_button_state DPadUp;
			game_button_state DPadDown;
			game_button_state DPadLeft;
			game_button_state DPadRight;

			game_button_state Start;
			game_button_state Select;

			game_button_state LeftShoulder;
			game_button_state RightShoulder;

			game_button_state LeftTrigger;
			game_button_state RightTrigger;

			game_button_state LeftStick;
			game_button_state LeftStickUp;
			game_button_state LeftStickDown;
			game_button_state LeftStickLeft;
			game_button_state LeftStickRight;

			game_button_state RightStick;
			game_button_state RightStickUp;
			game_button_state RightStickDown;
			game_button_state RightStickLeft;
			game_button_state RightStickRight;

			game_button_state A;
			game_button_state B;
			game_button_state X;
			game_button_state Y;


			// Note: Fake Button, All new buttons must be added above this one
			game_button_state Terminator;
		};
	};

};

struct game_input
{	
	// GameUpdateTime
	real32 dt;

	game_button_state MouseButton[5];
	int32 MouseX, MouseY, MouseZ;

	// Todo: handle keyboard like this? Use raw input?
	//game_button_state KeyboardButton[104];

	game_controller_input Controllers[5];
};

inline game_controller_input* GetController(game_input* Input, int ControllerIndex)
{
	Assert( ControllerIndex < ArrayCount( Input->Controllers ) );
	game_controller_input* Result  = &Input->Controllers[ControllerIndex];
	return Result;
}

struct game_memory
{
	bool32 IsInitialized;
	uint64 PermanentStorageSize;
	void* PermanentStorage; // Note: Required to be initialized to zero at startup
	uint64 TransientStorageSize;
	void* TransientStorage;

	debug_platform_read_entire_file* DEBUGPlatformReadEntireFile;
	debug_platfrom_free_file_memory* DEBUGPlatformFreeFileMemory;
	debug_platform_write_entire_file* DEBUGPlatformWriteEntireFile;
};

#define GAME_UPDATE_AND_RENDER(name) void name(thread_context* Thread, game_memory* Memory, game_offscreen_buffer* Buffer, game_input* Input )
typedef GAME_UPDATE_AND_RENDER( game_update_and_render );


// Note: At the moment this has to be a very fast function it cannot be more than a millisecond or so.
// TODO: Reduce the pressure on this  function's preformance by measuring it  or asking about it, etc

#define GAME_GET_SOUND_SAMPLES(name) void name(thread_context* Thread, game_memory* Memory, game_sound_output_buffer* SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

#endif // HANDMADE_PLATFORM_H
