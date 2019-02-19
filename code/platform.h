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

#ifndef HANDMADE_PLATFORM_H
#define HANDMADE_PLATFORM_H

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

#include <stddef.h> // size_t exists in this header on some platforms

#include "vector_math.h"

struct thread_context
{
	s32 placeholder;
};

#if HANDMADE_INTERNAL

struct debug_read_file_result{
	u32 ContentSize;
	void* Contents;
};

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name( thread_context* Thread, void* Memory )
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY( debug_platfrom_free_file_memory );

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name( thread_context* Thread, char* Filename )
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE( debug_platform_read_entire_file );

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) b32 name( thread_context* Thread, char* Filename, u32 MemorySize, void* Memory )
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE( debug_platform_write_entire_file );

#endif // HANDMADE_INTERNAL

inline u32 
SafeTruncateUInt64( u64 Value )
{
	Assert(Value <= 0xFFFFFFFF);
	u32 Result = (u32)Value;
	return Result;  
}

inline r32 
SafeTruncateReal32( u32 Value )
{
	Assert(Value <= 0xFFFFFFFF);
	Assert(Value >= 0xFFFFFFFF);
	r32 Result = (r32) Value;
	return Result;
}


/*
  NOTE: Services that the game provides to the platform layer.
*/

struct game_render_commands
{
	s32 Width;
	s32 Height;

	u32 MaxPushBufferSize;
	u32 PushBufferSize;
	u8* PushBuffer;

	u32 PushBufferElementCount;
};

#define RenderCommandStruct (MaxPushBufferSize, PushBuffer, Width, Height) {Width,Height, MaxPushBufferSize, 0, (u8*) PushBuffer, 0 , MaxPushBufferSize };

struct game_sound_output_buffer
{
	s32 SamplesPerSecond;
	s32 SampleCount;
	s32 channels;
	s16* Samples;
};

struct game_button_state
{
	s32 HalfTransitionCount;
	b32 EndedDown;
};

struct game_controller_input
{	
	b32 IsAnalog;
	b32 IsConnected;

	union
	{
		r32 Averages[6];
		struct
		{
			r32 LeftStickAverageX;
			r32 LeftStickAverageY;
			r32 RightStickAverageX;
			r32 RightStickAverageY;
			r32 LeftTriggerAverage;
			r32 RightTriggerAverage;
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
	r32 dt;
	b32 ExecutableReloaded;

	game_button_state MouseButton[5];
	s32 MouseX, MouseY, MouseZ;

	// Todo: handle keyboard like this? Use raw input?
	//game_button_state KeyboardButton[104];

	game_controller_input Controllers[5];
};

inline game_controller_input* GetController(game_input* Input, s32 ControllerIndex)
{
	Assert( ControllerIndex < ArrayCount( Input->Controllers ) );
	game_controller_input* Result  = &Input->Controllers[ControllerIndex];
	return Result;
}

struct platform_file_handle
{
    b32 NoErrors;
    void *Platform;
};
/*
struct platform_file_info
{
    platform_file_info *Next;
    u64 FileDate; // NOTE(casey): This is a 64-bit number that _means_ the date to the platform, but doesn't have to be understood by the app as any particular date.
    u64 FileSize;
    char *BaseName; // NOTE(casey): Doesn't include a path or an extension
    void *Platform;
};
struct platform_file_group
{
    u32 FileCount;
    platform_file_info *FirstFileInfo;
    void *Platform;
};
enum platform_file_type
{
    PlatformFileType_AssetFile,
    PlatformFileType_SavedGameFile,
    PlatformFileType_PNG,
    PlatformFileType_WAV,
    
    PlatformFileType_Count,
};
*/
enum platform_memory_block_flags
{
    PlatformMemory_NotRestored = 0x1,
    PlatformMemory_OverflowCheck = 0x2,
    PlatformMemory_UnderflowCheck = 0x4,
};

/*
 *	Platform Memory Block (PMB)
 *			 01234567
 *	Base -> |xxxxxxxx|   0	  
 *			|xxxxxxxx|   1
 *			|xxxxxxxx|   2	  
 *			...
 *			|xxxxxxxx|   Used-1
 *	Used->	|--------|   Used
 *			...	  
 *			|--------|   Size-2	  
 *			|--------|   Size-1
 *			|--------|   Size
 */
struct platform_memory_block
{
    u64 Flags;
    u64 Size;
    u8* Base;						// Pointer to the beginning of the memory block
    uintptr_t Used;						// Pointer to the end of the used data
    platform_memory_block *ArenaPrev;
};

enum platform_open_file_mode_flags
{
    OpenFile_Read = 0x1,
    OpenFile_Write = 0x2,
};

//#define PLATFORM_OPEN_FILE(name) platform_file_handle name(platform_file_group *FileGroup, platform_file_info *Info, u32 ModeFlags)
//typedef PLATFORM_OPEN_FILE(platform_open_file);
//
//#define PLATFORM_READ_DATA_FROM_FILE(name) void name(platform_file_handle *Handle, u64 Offset, u64 Size, void *Dest)
//typedef PLATFORM_READ_DATA_FROM_FILE(platform_read_data_from_file);
//
//#define PLATFORM_WRITE_DATA_TO_FILE(name) void name(platform_file_handle *Handle, u64 Offset, u64 Size, void *Source)
//typedef PLATFORM_WRITE_DATA_TO_FILE(platform_write_data_to_file);
//
//#define PLATFORM_FILE_ERROR(name) void name(platform_file_handle *Handle, char *Message)
//typedef PLATFORM_FILE_ERROR(platform_file_error);
//
//#define PLATFORM_CLOSE_FILE(name) void name(platform_file_handle *Handle)
//typedef PLATFORM_CLOSE_FILE(platform_close_file);

#define PlatformNoFileErrors(Handle) ((Handle)->NoErrors)

#define PLATFORM_ALLOCATE_MEMORY(name) platform_memory_block *name(memory_index aSize, u64 aFlags)
typedef PLATFORM_ALLOCATE_MEMORY(platform_allocate_memory);

#define PLATFORM_DEALLOCATE_MEMORY(name) void name(platform_memory_block *aBlock)
typedef PLATFORM_DEALLOCATE_MEMORY(platform_deallocate_memory);

struct platform_api
{
//    platform_open_file *OpenFile;
//    platform_read_data_from_file *ReadDataFromFile;
//    platform_write_data_to_file *WriteDataToFile;
//    platform_file_error *FileError;
//    platform_close_file *CloseFile;

    platform_allocate_memory* AllocateMemory;
    platform_deallocate_memory* DeallocateMemory;

#if HANDMADE_INTERNAL
//     TODO(casey): Get rid of these eventually, make them just go through
//     the OpenFile/ReadDataFromFile/WriteDataToFile/CloseFile API.
//     {
     	debug_platform_read_entire_file*  DEBUGPlatformReadEntireFile;
		debug_platfrom_free_file_memory*  DEBUGPlatformFreeFileMemory;
		debug_platform_write_entire_file* DEBUGPlatformWriteEntireFile;
//     }
    
//    debug_platform_execute_system_command *DEBUGExecuteSystemCommand;
//    debug_platform_get_process_state *DEBUGGetProcessState;
//    debug_platform_get_memory_stats *DEBUGGetMemoryStats;
#endif

};

extern platform_api Platform;

struct game_memory
{
	struct game_state* GameState;
	platform_api PlatformAPI;
};

#define GAME_UPDATE_AND_RENDER(name) void name(thread_context* Thread, game_memory* Memory, game_render_commands* RenderCommands, game_input* Input )
typedef GAME_UPDATE_AND_RENDER( game_update_and_render );


// Note: At the moment this has to be a very fast function it cannot be more than a millisecond or so.
// TODO: Reduce the pressure on this  function's preformance by measuring it  or asking about it, etc

#define GAME_GET_SOUND_SAMPLES(name) void name(thread_context* Thread, game_memory* Memory, game_sound_output_buffer* SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);


#endif // HANDMADE_PLATFORM_H
