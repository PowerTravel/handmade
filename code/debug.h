#pragma once

#include "types.h"
#include "intrinsics.h"
#include "memory.h"
#include <stdio.h>
#include "string.h"

#pragma intrinsic(__rdtsc)

struct debug_state;

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

struct binary_signal_state
{
  // if     Active and     Edge -> We just became active (rising edge)
  // if     Active and not Edge -> We have been active for more than one frame
  // if not Active and     Edge -> We just became inactive (falling edge)
  // if not Active and not Edge -> We have been inactive for more than one frame
  b32 Active;
  b32 Edge;
};

inline void Update(binary_signal_state* DigitalState, b32 Value )
{
  DigitalState->Edge   = (DigitalState->Active != Value); // <- Edge is true if state changed
  DigitalState->Active = Value;
}

struct radial_menu;

struct menu_item
{
  c8 Header[64];
  
  // Input State
  b32 MouseReleased;
  binary_signal_state MouseOverState;
  binary_signal_state MenuActivationState;

  // Internal State
  b32 Active;
  
  void (*Activate)(debug_state* DebugState, menu_item* Item);
};

menu_item MenuItem(c8* Name, b32 Active = false) 
{
  menu_item Result = {};
  Assert(str::StringLength( Name ) < ArrayCount( Result.Header ) );
  str::CopyStringsUnchecked( Name, Result.Header );
  Result.Active = Active;
  return Result;
}

struct radial_menu_region
{
  r32 AngleStart;
  r32 AngleEnd;
  r32 RadiusStart;
  r32 Radius;
};


#define AssertCanonical(Angle) Assert(( (Angle) >= 0 ) && ( (Angle) < Tau32))

// Angle -> [0,Tau]
inline r32 RecanonicalizeAngle(r32 Angle)
{
  r32 Result = Angle - Floor(Angle/Tau32) * Tau32;
  return Result;
}

inline r32 GetDeltaAngle(r32 AngleStart, r32 AngleEnd)
{
  AssertCanonical(AngleStart);
  AssertCanonical(AngleEnd);

  b32 Wrapping = (AngleStart > AngleEnd);
  r32 Result = AngleEnd - AngleStart + Wrapping*Tau32;
  return Result;
}

inline r32 GetParametarizedAngle(r32 AngleStart, r32 AngleEnd,  r32 Angle)
{
  AssertCanonical(AngleStart);
  AssertCanonical(AngleEnd);
  AssertCanonical(Angle);

  r32 Interval = GetDeltaAngle(AngleStart, AngleEnd);
  r32 Delta = GetDeltaAngle(AngleStart, Angle);
  r32 Result = Delta / Interval;

  return Result;
}

inline b32 IsInRegion(r32 AngleStart, r32 AngleEnd, r32 Angle)
{
  AssertCanonical(AngleStart);
  AssertCanonical(AngleEnd);
  AssertCanonical(Angle);
  b32 Wrapping = (AngleStart > AngleEnd);
  b32 BetweenStartAndEnd = (Angle >= AngleStart) && (Angle < AngleEnd);
  b32 BetweenStartAndTau = (Angle >= AngleStart) && (Angle < Tau32);
  b32 BetweenZeroAndEnd  = (Angle >= 0) && (Angle < AngleEnd);
  b32 Result = (!Wrapping && BetweenStartAndEnd) || (Wrapping && (BetweenStartAndTau || BetweenZeroAndEnd) );
  return Result;
}

radial_menu_region RadialMenuRegion(r32 AngleStart, r32 AngleEnd, r32 RadiusStart, r32 Radius)
{
  radial_menu_region Result = {};

  Result.AngleStart = RecanonicalizeAngle(AngleStart);
  Result.AngleEnd = RecanonicalizeAngle(AngleEnd);
  Result.RadiusStart = RadiusStart;
  Result.Radius = Radius;
  return Result;
}

enum class menu_type
{
  None,
  Radial,
  Box
};

struct radial_menu
{
  u32 MenuRegionCount;
  u32 HotRegionIndex;
  radial_menu_region* Regions;
  menu_item* MenuItems;

  r32 MouseRadius;
  r32 MouseAngle;

  r32 MenuX;
  r32 MenuY;
};

struct box_menu
{
  u32 MenuItemCount;
  menu_item* MenuItems;
  r32 x;
  r32 y;
  r32 w;
  r32 h;
};

struct active_menu
{
  menu_type Type;
  union
  {
    radial_menu* RadialMenu;
    box_menu* BoxMenu; 
  };
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

  b32 Compiling;
  debug_executing_process Compiler;

  debug_frame* Frames;
  debug_thread* FirstThread;
  open_debug_block* FirstFreeBlock;
  debug_record* ScopeToRecord;

  r32 ChartVisible;
  b32 MainMenu;

  midx MemorySize;
  u8* MemoryBase;
  u8* Memory;

  u32 RadialMenuEntries;
  radial_menu* RadialMenues;

  u32 BoxMenuEntries;
  box_menu* BoxMenues;

  active_menu ActiveMenu;
  struct main_window* MainWindow;

  b32 ConfigMultiThreaded;
  b32 ConfigCollisionPoints;
  b32 ConfigCollider;
  b32 ConfigAABBTree;

  b32 UpdateConfig;
};

void PushDebugOverlay(game_input* GameInput);
global_variable render_group* GlobalDebugRenderGroup;


struct region_node
{
  rect2f Region;
  u32 BranchCount;
  region_node* Branches;
  v4 Color;
};

enum class window_regions
{
  LeftBody,
  RightBody,
  LeftBorder,
  MiddleBorder,
  RightBorder,
  BotBorder,
  Header,
  TopBorder,
  BotLeftCorner,
  BotRightCorner,
  TopLeftCorner,
  TopRightCorner,
};

enum class split_direction
{
  None,
  Horizontal,
  Vertical
};

struct main_window
{
  //region_node Root;
  rect2f Region;

  r32 X,Y,W;

  r32 MinWidth;
  r32 MinHeight;

  r32 BorderSize;
  r32 HeaderSize;
  c8 Header[64];
  v4 Color;

  b32 VerticalSplit;
  r32 MiddleBorderPos; // [0,1]
  r32 DraggingStart;

  b32 WindowDrag;
  b32 LeftBorderDrag;
  b32 RightBorderDrag;
  b32 BotBorderDrag;
  b32 TopBorderDrag;
  b32 MiddleBorderDrag;

  rect2f WindowDraggingStart;

  v2 MousePos;

  binary_signal_state MouseLeftButton;
  v2 MouseLeftButtonPush;
  v2 MouseLeftButtonRelese;
};




void Draw(region_node* Root);
region_node* GetMouseOverRegion(region_node* Root, v2 MousePosition);

main_window* GetMenu(debug_state* DebugState);
void SetMenuInput(game_input* GameInput, debug_state* DebugState, main_window*  MenuRoot);
void ActOnInput(debug_state* DebugState, main_window*  MenuRoot);
void Draw(main_window* MenuRoot);
rect2f GetRegion(window_regions Type, main_window* MainWindow);