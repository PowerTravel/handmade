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
  u32 LaneIndex;
  r32 MinT;
  r32 MaxT;
  u16 ColorIndex;
  debug_record* Record;
};

#define MAX_REGIONS_PER_FRAME 4096
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
  u32 StartingFrameIndex;
  debug_record* Record;
  debug_event OpeningEvent;
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
  b32 Resumed;

  memory_arena Arena;
  temporary_memory CollateTemp;

  u32 CollationArrayIndex;
  debug_frame* CollationFrame;
  u32 FrameCount;
  u32 FrameBarLaneCount;
  r32 FrameBarRange;

  //render_group* RenderGroup;

  b32 Compiling;
  debug_executing_process Compiler;

  debug_frame* Frames;
  debug_thread* FirstThread;
  open_debug_block* FirstFreeBlock;
  debug_record* ScopeToRecord;

  r32 ChartVisible;
  r32 ChartLeft;
  r32 ChartBot;
  r32 ChartWidth;
  r32 ChartHeight;

  b32 MainMenu;

  u32 HotMenuItem;
  r32 RadialMenuX;
  r32 RadialMenuY;
  b32 ConfigMultiThreaded;
  b32 ConfigCollisionPoints;
  b32 ConfigCollider;

  b32 UpdateConfig;
};

void PushDebugOverlay(game_input* GameInput);
global_variable render_group* GlobalDebugRenderGroup;