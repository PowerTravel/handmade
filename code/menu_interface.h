#pragma once

enum class container_type
{
  None,
  Root,
  Empty,
  Split,
  TabbedHeader,
  MenuHeader,
  ContainerList,
  Button,
  Profiler,
  Border,
  FrameBorder,
  Body,
  Header
};
const c8* ToString(container_type Type)
{
  switch(Type)
  {
    case container_type::None: return "None";
    case container_type::Root: return "Root";
    case container_type::Empty: return "Empty";
    case container_type::Split: return "Split";
    case container_type::TabbedHeader: return "TabbedHeader";
    case container_type::MenuHeader: return "MenuHeader";
    case container_type::ContainerList: return "ContainerList";
    case container_type::Button: return "Button";
    case container_type::Profiler: return "Profiler";
    case container_type::Border: return "Border";
    case container_type::FrameBorder: return "FrameBorder";
    case container_type::Body: return "Body";
    case container_type::Header: return "Header";
  }
  return "";
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

struct menu_interface;

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

#define MENU_GET_CHILD_REGION(name) rect2f name(container_node* Parent, container_node* Child)
typedef MENU_GET_CHILD_REGION( menu_get_region );

#define MENU_GET_REGION_RECT(name) rect2f name( window_regions Type, container_node* Node )
typedef MENU_GET_REGION_RECT( menu_get_region_rect );

#define MENU_GET_MOUSE_OVER_REGION(name) window_regions name( container_node* Node, v2 MousePos)
typedef MENU_GET_MOUSE_OVER_REGION( menu_get_mouse_over_region );

#define MENU_DRAW(name) void name( menu_interface* Interface, container_node* Node)
typedef MENU_DRAW( menu_draw );

#define GetContainerPayload( Type, Container )  ((Type*) (((u8*)Container) + sizeof(container_node)))

struct menu_functions
{
  menu_mouse_down* MouseDown;
  menu_mouse_up* MouseUp;
  menu_mouse_enter* MouseEnter;
  menu_mouse_exit* MouseExit;

  menu_get_region* GetChildRegion;

  menu_handle_input* HandleInput;
  menu_get_region_rect* GetRegionRect;
  menu_get_mouse_over_region* GetMouseOverRegion;
  menu_draw*  Draw;
};

menu_functions GetMenuFunction(container_type Type);


struct container_node
{
  container_type Type;
  u32 ContainerSize;

  u32 Depth;

  // Tree Links (Menu Structure)
  container_node* Parent;
  container_node* FirstChild;
  container_node* NextSibling;

  // List Links (Memory)
  container_node* Next;
  container_node* Previous;

  rect2f Region;

  menu_functions Functions;
};

enum class alignment
{
  Left,
  Right,
  Top,
  Bot,
  Middle,
};

#define GetContainingNode( WindowPtr )  (container_node*) ( ((u8*)(WindowPtr)) - OffsetOf(container_node, WindowPointer))

struct frame_border_leaf
{
  r32 Thickness;
  v4 Color;

  r32 DraggingStart;
  b32 Drag;
};

inline frame_border_leaf CreateFrameBorder(r32 Thickness = 0.01, v4 Color =  V4(0,0,0.4,1))
{
  frame_border_leaf Result = {};
  Result.Thickness = Thickness;
  Result.Color = Color;
  return Result;
};

struct border_leaf
{
  b32 Vertical;
  r32 Position;
  r32 Thickness;
  v4 Color;

  alignment Alignment;
  r32 DraggingStart;
  b32 Drag;
};

inline border_leaf CreateBorder(b32 Vertical, r32 Thickness = 0.01, r32 Position = 0.5f,  v4 Color =  V4(0,0,0.4,1),  alignment Alignment = alignment::Middle)
{
  border_leaf Result = {};
  Result.Vertical = Vertical;
  Result.Position = Position;
  Result.Alignment = Alignment;
  Result.Thickness = Thickness;
  Result.Color = Color;  
  return Result;
}

struct header_leaf
{
  r32 Thickness;
  v4 Color;

  v2 DraggingStart;
  b32 Drag;

  container_node* NodeToMerge = 0;
  u32 HotMergeZone;
  rect2f MergeZone[5];

  // Tabs
  u32 TabCount;
  container_node* Tabs[12];
  u32 SelectedTabOrdinal; // [1,3,4,5...], 0 means no tab selected
};

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
  r32 HeaderSize;
  r32 MinSize;

  b32 HeaderDrag;
  b32 LeftBorderDrag;
  b32 RightBorderDrag;
  b32 BotBorderDrag;
  b32 TopBorderDrag;

  rect2f DraggingStart;
  struct menu_tree* Menu;
};

struct container_list
{

};


struct profiling_window
{

};


struct menu_tree
{
  u32 NodeCount;
  u32 Depth;
  container_node* Root;
  //menu_tree* Next;


  u32 HotLeafCount;
  container_node* HotLeafs[32];

  u32 ActiveLeafCount;
  container_node* ActiveLeafs[32];
};

#define ACTIVATE_BUTTON(name) void (name)(struct menu_button* Button, debug_state* DebugState)
typedef ACTIVATE_BUTTON( activate_button );

struct menu_button
{
  v4 Color;
  c8 Text[32];

  b32 Active;
  b32 Hot;

  activate_button** Activate;
};

struct menu_interface
{
  u32 RootContainerCount;
  menu_tree RootContainers[32];
  menu_tree* TopMenu;

  window_regions HotRegion;
  container_node* HotSubWindow;

  u32 ActiveMemory;
  u32 MaxMemSize;
  u8* MemoryBase;
  u8* Memory;
  
  container_node Sentinel;

  v2 MousePos;
  v2 PreviousMousePos;
  binary_signal_state MouseLeftButton;
  v2 MouseLeftButtonPush;
  v2 MouseLeftButtonRelese;


  // TODO: Make concept of min-size more polished and consistent
  r32 BorderSize;
  r32 HeaderSize;
  r32 MinSize;

  // Button Function (Trying out some kind of function pool)
  activate_button* Activate[5];

};


menu_interface* CreateMenuInterface();
void UpdateAndRenderMenuInterface(game_input* GameInput, menu_interface* Interface);
container_node* NewContainer(menu_interface* Interface, container_type Type);
void DeleteContainer(menu_interface* Interface, container_node* Node);
menu_tree* GetNewMenuTree(menu_interface* Interface);
void FreeMenuTree(menu_interface* Interface,  menu_tree* MenuToFree);
void DisconnectNode(container_node* Node);
container_node* ConnectNode(container_node* Parent, container_node* NewNode);

//container_node* PushBorder(menu_interface* Interface, container_node* Parent, border_leaf Border);

void SetMouseInput(memory_arena* Arena, game_input* GameInput, menu_interface* Interface);

void SetInterfaceFunctionPointers(container_node* Root, memory_arena* Arena, u32 NodeCount);
void ActOnInput(memory_arena* Arena, menu_interface* Interface, menu_tree* Menu);

window_regions CheckRegions(rect2f Region, u32 RegionCount, window_regions* RegionArray, r32 BorderSize, r32 HeaderSize, r32 BorderFrac);