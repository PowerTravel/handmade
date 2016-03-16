
#ifndef WIN32_HANDMADE_HPP
#define WIN32_HANDMADE_HPP

#include "handmade.hpp"

struct win32_offscreen_buffer
{
    // NOTE(casey): Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

struct win32_window_dimension{
	int Width;
	int Height;
};

struct win32_sound_output
{

	LPDIRECTSOUNDBUFFER SecondaryBuffer;	

	// NOTE: Orthogonal Values
	int SamplesPerSecond;
	int ChannelCount;
	int BytesPerSample;
	int BufferSizeInSeconds;
	real32 TargetSecondsOfLatency;

	// Derived Values
	int BytesPerSampleTotal; // = ChannelCount * BytesPerSample;
	int BytesPerSecond;
	DWORD BufferSizeInBytes; // = BufferSizeInSeconds * SamplesPerSecond * BytesPerSampleTotal;
	
	uint32 RunningSampleIndex;
	DWORD BytesOfLatency;

};

struct win32_debug_time_marker{
	
	DWORD OutputPlayCursor;
	DWORD OutputWriteCursor;

	DWORD OutputLocation;
	DWORD OutputByteCount;

	DWORD FlipPlayCursor;
	DWORD FlipWriteCursor; 

	DWORD ExpectedFlipPlayCursor;
};

struct win32_game_code
{
	HMODULE GameCodeDLL;
	FILETIME LastDLLWriteTime;

	// IMPORTANT: Both functions can be 0; Must check before calling
	game_update_and_render* UpdateAndRender;
	game_get_sound_samples* GetSoundSamples;

	bool32 IsValid;
};

// Note:	Never use MAX_PATH in code that is user facing because it can be wrong
#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH
struct win32_state
{

	uint64 TotalSize;
	void* GameMemoryBlock;

	HANDLE InputRecordingHandle;
	int InputPlayingIndex;

	HANDLE InputPlaybackHandle;
	int InputRecordingIndex;

	char EXEFileName[WIN32_STATE_FILE_NAME_COUNT];
	char* OnePastLastSlashEXEFileName;
};


#endif