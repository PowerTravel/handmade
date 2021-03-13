#pragma once

#include "types.h"
#include "intrinsics.h"
#include "memory.h"
#include "string.h"

#include "containers/vector_list.h"

#pragma intrinsic(__rdtsc)

struct debug_state;

internal debug_state* DEBUGGetState();

enum class function_sorting
{
  None,
  Ascending,
  Descending
};

struct debug_record_entry
{
  u32 LineNumber;
  char BlockName[256];
  u32 CycleCount;
  r32 HitCount;
  r32 HCCount;
};

struct debug_statistics
{
  r32 HitCount;
  r32 Min;
  r32 Max;
  r32 Tot;
  debug_record_entry* Record;
};

// The information for the frame
#define MAX_BLOCKS_PER_FRAME 16384
#define MAX_THREAD_COUNT 16
#define MAX_DEBUG_FRAME_COUNT 60
#define MAX_DEBUG_FUNCTION_COUNT 256

struct debug_block
{
  debug_record_entry* Record;
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

  midx MaxBlockCount;
  debug_block* Blocks;
  debug_block* FirstFreeBlock;

  debug_thread Threads[MAX_THREAD_COUNT];

  vector_list<debug_statistics> Statistics;
};


struct debug_state
{
  b32 Initialized;

  b32 Paused;

  memory_arena Arena;
  temporary_memory StatisticsTemp;

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

  // Keeps a global record of all seen functions and their execution time and hit cout.
  vector_list<debug_record_entry> FunctionList;

  function_sorting LineSorted;
  function_sorting BlockNameSorted;
  function_sorting CycleCountSorted;
  function_sorting HitCountSorted;
  function_sorting CyclePerHitSorted;
  r32 ScrollPercentage;
};

inline void DebugRewriteConfigFile();
void PushDebugOverlay(game_input* GameInput);

