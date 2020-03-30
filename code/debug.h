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

struct debug_frame_snapshot
{
  debug_statistics HitCountStat;
  debug_statistics CycleCountStat;
  u32 HitCount;
  u64 CycleCount;
};

struct debug_frame_timestamp
{
  char* Name;
  r32 Seconds;
};

struct debug_frame_end_info
{
  r32 TotalSeconds;
  u32 TimestampCount;
  debug_frame_timestamp Timestamps[64];
};

#define SNAPSHOT_COUNT 128
struct debug_counter_state
{
  char* FileName;
  char* FunctionName;
  u32 LineNumber;
  debug_frame_snapshot Snapshots[SNAPSHOT_COUNT];
};

struct debug_state
{
  memory_arena Memory;
  u32 SnapShotIndex;
  u32 CounterStateCount;
  debug_counter_state CounterStates[512];
};


global_variable render_group* GlobalDebugRenderGroup;