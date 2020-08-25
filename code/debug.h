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
  struct menu_interface* MenuInterface;

  b32 ConfigMultiThreaded;
  b32 ConfigCollisionPoints;
  b32 ConfigCollider;
  b32 ConfigAABBTree;

  b32 UpdateConfig;
};

void PushDebugOverlay(game_input* GameInput);
global_variable render_group* GlobalDebugRenderGroup;


enum class container_type
{
  None,
  Root,
  Empty,
  Split,
  TabbedHeader,
  MenuHeader
};

enum class window_regions
{
  None,
  WholeBody,
  BodyOne,      // Left / Bot
  BodyTwo,      // Right / Top
  MiddleBorder, // Vertical / Horizontal
  LeftBorder,
  RightBorder,
  BotBorder,
  TopBorder,
  Header,
  BotLeftCorner,
  BotRightCorner,
  TopLeftCorner,
  TopRightCorner,
  WindowRegionCount
};

const char* ToString(window_regions Region)
{
  switch(Region)
  {
    case window_regions::None: return "None";
    case window_regions::WholeBody: return "WholeBody";
    case window_regions::BodyOne: return "BodyOne";
    case window_regions::BodyTwo: return "BodyTwo";
    case window_regions::MiddleBorder: return "MiddleBorder";
    case window_regions::LeftBorder: return "LeftBorder";
    case window_regions::RightBorder: return "RightBorder";
    case window_regions::BotBorder: return "BotBorder";
    case window_regions::TopBorder: return "TopBorder";
    case window_regions::Header: return "Header";
    case window_regions::BotLeftCorner: return "BotLeftCorner";
    case window_regions::BotRightCorner: return "BotRightCorner";
    case window_regions::TopLeftCorner: return "TopLeftCorner";
    case window_regions::TopRightCorner: return "TopRightCorner";
    case window_regions::WindowRegionCount: return "WindowRegionCount";
  }
  return "";
};

struct container_node;

void PushWindow( window_regions Type, container_node* Node );


struct node_region_pair
{
  container_node* Node;
  window_regions Region;
};


#define MENU_MOUSE_DOWN(name) void name( menu_interface* Interface, container_node* Node, window_regions HotRegion, void* Params)
typedef MENU_MOUSE_DOWN( menu_mouse_down );

#define MENU_MOUSE_UP(name) void name( menu_interface* Interface, container_node* Node, window_regions HotRegion, void* Params)
typedef MENU_MOUSE_UP( menu_mouse_up );

#define MENU_MOUSE_ENTER(name) void name( menu_interface* Interface, container_node* Node, window_regions HotRegion, void* Params)
typedef MENU_MOUSE_ENTER( menu_mouse_enter );

#define MENU_MOUSE_EXIT(name) void name( menu_interface* Interface, container_node* Node, window_regions HotRegion, void* Params)
typedef MENU_MOUSE_EXIT( menu_mouse_exit );

#define MENU_HANDLE_INPUT(name) void name( menu_interface* Interface, container_node* Node,  void* Params)
typedef MENU_HANDLE_INPUT( menu_handle_input );

#define MENU_GET_REGION_RECT(name) rect2f name( window_regions Type, container_node* Node )
typedef MENU_GET_REGION_RECT( menu_get_region_rect );

#define MENU_GET_MOUSE_OVER_REGION(name) window_regions name( container_node* Node, v2 MousePos)
typedef MENU_GET_MOUSE_OVER_REGION( menu_get_mouse_over_region );

#define MENU_DRAW(name) void name( menu_interface* Interface, container_node* Node)
typedef MENU_DRAW( menu_draw );


struct menu_functions
{
  menu_mouse_down* MouseDown;
  menu_mouse_up* MouseUp;
  menu_mouse_enter* MouseEnter;
  menu_mouse_exit* MouseExit;

  menu_handle_input* HandleInput;
  menu_get_region_rect* GetRegionRect;
  menu_get_mouse_over_region* GetMouseOverRegion;
  menu_draw*  Draw;
};


struct container_node
{
  container_type Type;

  // Tree Links (Menu Structure)
  container_node* Parent;
  container_node* FirstChild;
  container_node* NextSibling;

  // List Links (Memory)
  container_node* Next;
  container_node* Previous;
  
  rect2f Region;

  u32 ContainerSize;

  menu_functions Functions;
};

#define GetContainingNode( WindowPtr )  (container_node*) ( ((u8*)(WindowPtr)) - OffsetOf(container_node, WindowPointer))

menu_functions GetEmptyFunctions();
menu_functions GetRootMenuFunctions();
menu_functions MenuHeaderMenuFunctions();
menu_functions TabbedHeaderMenuFunctions();
menu_functions SplitMenuFunctions();

struct tabbed_header_window
{
  r32 HeaderSize;

  // Tabs
  u32 TabCount;
  container_node* Tabs[12];
  u32 SelectedTabOrdinal; // [1,3,4,5...], 0 means no tab selected

  b32 TabDrag;
  v2 TabMouseOffset;
};

struct menu_header_window
{
  r32 HeaderSize;
  
  // Window Merging
  container_node* NodeToMerge;
  rect2f MergeZone[5];
  u32 HotMergeZone;

  // Window Dragging
  container_node* RootWindow;
  v2 DraggingStart;
  b32 WindowDrag;
};

struct split_window
{
  r32 BorderSize;
  r32 MinSize;
  r32 SplitFraction;

  b32 BorderDrag;
  r32 DraggingStart;

  b32 VerticalSplit; // Opposite of HorizontalSplit
};

struct empty_window
{
  v4 Color;
};

struct root_window
{
  r32 BorderSize;
  r32 MinSize;

  b32 LeftBorderDrag;
  b32 RightBorderDrag;
  b32 BotBorderDrag;
  b32 TopBorderDrag;

  rect2f WindowDraggingStart;
};

struct menu_tree
{
  u32 NodeCount;
  u32 Depth;
  container_node* Root;
};

struct menu_interface
{
  u32 RootContainerCount;
  menu_tree RootContainers[32];

  menu_tree HotWindow;

  window_regions HotRegion;
  container_node* HotSubWindow;

  u32 ActiveMemory;
  u32 MaxMemSize;
  u8* MemoryBase;
  u8* Memory;
  
  container_node Sentinel;

  v2 MousePos;
  binary_signal_state MouseLeftButton;
  v2 MouseLeftButtonPush;
  v2 MouseLeftButtonRelese;

  r32 BorderSize;
  r32 HeaderSize;
  r32 MinSize;
};


container_node* NewContainer(menu_interface* Interface, container_type Type);
void DeleteContainer(menu_interface* Interface, container_node* Node);
menu_tree* GetNewMenuTree(menu_interface* Interface);
void FreeMenuTree(menu_interface* Interface,  menu_tree* MenuToFree);
void DisconnectNode(container_node* Node);
void ConnectNode(container_node* Parent, container_node* NewNode);


void SetMouseInput(game_input* GameInput, menu_interface* Interface);

void InitializeMenuFunctionPointers(container_node* Root, memory_arena* Arena, u32 NodeCount);
void UpdateRegions(container_node* Root, rect2f RootRegion);
void ActOnInput(memory_arena* Arena, menu_interface* Interface);

window_regions CheckRegions(rect2f Region, u32 RegionCount, window_regions* RegionArray, r32 BorderSize, r32 HeaderSize, r32 BorderFrac);