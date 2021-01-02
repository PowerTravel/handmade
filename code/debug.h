#pragma once

#include "types.h"
#include "intrinsics.h"
#include "memory.h"
#include "string.h"

#pragma intrinsic(__rdtsc)

struct debug_state;

internal debug_state* DEBUGGetState();

struct debug_statistics
{
  r32 Count;
  r32 Min;
  r32 Max;
  r32 Avg;
  debug_record* Record;
};

struct debug_frame_region
{
  u32 LaneIndex;
  r32 MinT;
  r32 MaxT;
  u16 ColorIndex;
  debug_record* Record;
};


// The information for the frame
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
  open_debug_block* Parent;    // The open block preceding this one (in order to properly count have nested functions)
  open_debug_block* NextFree;  // The next free block in 
};

struct debug_thread
{
  u32 ID;
  u32 LaneIndex;
  open_debug_block* FirstOpenBlock; // This contains the deepest open block.
                                    // Remember, blocks are like russian nesting dolls:
                                    // we know that any 'DebugEvent_EndBlock' will close the current FirstOpenBlock for that thread.
  debug_thread* Next;
};


struct debug_state
{
  b32 Initialized;

  b32 Paused;

  memory_arena Arena;
  temporary_memory CollateTemp;

  u32 CollationArrayIndex;
  debug_frame* CollationFrame;
  u32 FrameCount;
  u32 FrameBarLaneCount;
  r32 FrameBarRange;

  b32 Compiling;
  debug_executing_process Compiler;

  debug_frame* Frames;
  debug_thread* FirstThread;
  open_debug_block* FirstFreeBlock;
  debug_record* ScopeToRecord;

  midx MemorySize;
  u8* MemoryBase;
  u8* Memory;

  b32 ConfigMultiThreaded;
  b32 ConfigCollisionPoints;
  b32 ConfigCollider;
  b32 ConfigAABBTree;

  u32* StatisticsCounts;
  debug_statistics** Statistics;
};

inline void DebugRewriteConfigFile();
void PushDebugOverlay(game_input* GameInput);

