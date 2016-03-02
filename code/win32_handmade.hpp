
#ifndef WIN32_HANDMADE_HPP
#define WIN32_HANDMADE_HPP

#include "handmade.hpp"
#include "handmade.cpp"

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
	// NOTE: Sound test
	int SamplesPerSecond;
	int ToneHz;
	uint32 RunningSampleIndex;
	int BytesPerSample;
	DWORD SecondaryBufferSize;
	DWORD SafteyBytes;
	int LatencySampleCount;
};

struct win32_debug_time_marker{
	
	DWORD OutputPlayCursor;
	DWORD OutputWriteCursor;

	DWORD OutputLocation;
	DWORD OutputByteCount;

	DWORD FlipPlayCursor;
	DWORD FlipWriteCursor; 
};

#endif