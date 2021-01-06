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
  r32 HitCount;
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
#define MAX_BLOCKS_PER_FRAME 4096
#define MAX_THREAD_COUNT 16
#define MAX_DEBUG_FRAME_COUNT 60
struct debug_block
{
  debug_record* Record;
  u32 ThreadIndex;
  u64 BeginClock;
  u64 EndClock;
  debug_event OpeningEvent;
  debug_block* Parent;
  debug_block* FirstChild;
  debug_block* Next;
};

struct debug_thread
{
  u32 ID;
  u32 LaneIndex;
  debug_block* FirstBlock;
  debug_block* OpenBlock;
  debug_block* ClosedBlock;
  debug_block* SelectedBlock;
};

struct debug_frame
{
  u64 BeginClock;
  u64 EndClock;
  r32 WallSecondsElapsed;

  u32 FrameBarLaneCount;

  debug_block Blocks[MAX_BLOCKS_PER_FRAME];
  debug_block* FirstFreeBlock;

  debug_thread Threads[MAX_THREAD_COUNT];

};

struct debug_state
{
  b32 Initialized;

  b32 Paused;

  memory_arena Arena;
  temporary_memory CollateTemp;

  b32 Compiling;
  debug_executing_process Compiler;

  b32 ConfigMultiThreaded;
  b32 ConfigCollisionPoints;
  b32 ConfigCollider;
  b32 ConfigAABBTree;

  // This is a rolling buffer that holds all data for all frames
  u32 CurrentFrameIndex;
  debug_frame* SelectedFrame;
  
  b32 ThreadSelected;
  u32 SelectedThreadIndex;
  debug_frame Frames[MAX_DEBUG_FRAME_COUNT];

  debug_statistics Statistics[MAX_DEBUG_RECORD_COUNT*MAX_DEBUG_TRANSLATION_UNITS];
};

inline void DebugRewriteConfigFile();
void PushDebugOverlay(game_input* GameInput);

