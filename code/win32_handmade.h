#pragma once

#include <stdio.h>
#include <windows.h>
#include <xinput.h>
#include <dsound.h>

#include "intrinsics.h"
#include "platform.h"
#include "debug.h"
#include "types.h"
#include "string.h"

struct win32_offscreen_buffer
{
    // NOTE(casey): Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
    BITMAPINFO Info;
    void *Memory;
    s32 Width;
    s32 Height;
    s32 Pitch;
    s32 BytesPerPixel;
};

struct win32_window_dimension{
  s32 Width;
  s32 Height;
};

struct win32_sound_output
{

  LPDIRECTSOUNDBUFFER SecondaryBuffer;

  // NOTE: Orthogonal Values
  s32 SamplesPerSecond;
  s32 ChannelCount;
  s32 BytesPerSample;
  s32 BufferSizeInSeconds;
  s32 TargetSecondsOfLatency;

  // Derived Values
  s32 BytesPerSampleTotal; // = ChannelCount * BytesPerSample;
  s32 BytesPerSecond;
  DWORD BufferSizeInBytes; // = BufferSizeInSeconds * SamplesPerSecond * BytesPerSampleTotal;
  DWORD BytesOfLatency;

  // Variable
  u32 RunningSampleIndex;
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
  debug_frame_end*        DEBUGGameFrameEnd;

  b32 IsValid;
};

enum win32_memory_block_flag
{
    Win32Mem_AllocatedDuringLooping = 0x1,
    Win32Mem_FreedDuringLooping = 0x2,
};
struct win32_memory_block
{
    platform_memory_block Block;
    win32_memory_block* Prev;
    win32_memory_block* Next;
    u64 LoopingFlags;
};

struct win32_saved_memory_block
{
    u64 BasePointer;
    u64 Size;
};

// Note:  Never use MAX_PATH in code that is user facing because it can be wrong
#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH
struct win32_replay_buffer
{
  HANDLE FileHandle;
  HANDLE MemoryMap;
  char FileName[WIN32_STATE_FILE_NAME_COUNT];
  void* MemoryBlock;
};

struct win32_state
{
  ticket_mutex MemoryMutex;
  win32_memory_block MemorySentinel;

  u64 TotalSize;
  //TODO, give support for more than 1 replaybuffer
  //    It works fine to add as many ass you want, however
  //    selecting specific buffers to record to is not implemented.
  //    Also slow startup for some reason with many buffers.
  //    See ReplayBuffer loop initialization in win32_handmade.cpp
  win32_replay_buffer ReplayBuffer[1];

  HANDLE RecordingHandle;
  s32 PlayingIndex;

  HANDLE PlaybackHandle;
  s32 RecordingIndex;

  char EXEFileName[WIN32_STATE_FILE_NAME_COUNT];
  char* OnePastLastSlashEXEFileName;
};
