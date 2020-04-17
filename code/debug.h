#pragma once

#include "types.h"
#include "intrinsics.h"
#include "memory.h"
#include <stdio.h>
#pragma intrinsic(__rdtsc)

struct debug_statistics
{
  r32 Count;
  r32 Min;
  r32 Max;
  r32 Avg;
};

struct debug_frame_timestamp
{
  char* Name;
  r32 Seconds;
};

struct debug_frame_region
{
  debug_record Record;
  u32 LaneIndex;
  r32 MinT;
  r32 MaxT;
};

#define MAX_REGIONS_PER_FRAME 128
struct debug_frame
{
  u64 BeginClock;
  u64 EndClock;
  r32 WallSecondsElapsed;
  u32 RegionCount;
  debug_frame_region* Regions;
};

struct open_debug_block
{
  u32 FrameIndex;
  debug_event* OpeningEvent;
  open_debug_block* Parent;
  open_debug_block* NextFree;
};

struct debug_thread
{
  u32 ID;
  u32 LaneIndex;
  open_debug_block* Parent;
  open_debug_block* FirstOpenBlock;
  debug_thread* Next;
};

struct debug_state
{
  b32 Initialized;
  b32 Paused;

  memory_arena Arena;
  temporary_memory CollateTemp;

  u32 FrameCount;
  u32 FrameBarLaneCount;
  r32 FrameBarRange;

  debug_frame* Frames;
  debug_thread* FirstThread;
  open_debug_block* FirstFreeBlock;
};


void PushDebugOverlay(debug_state* DebugState, game_input* GameInput);

global_variable render_group* GlobalDebugRenderGroup;