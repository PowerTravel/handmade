#pragma once

enum class container_type
{
  None,
  Root,
  Border,
  Split,
  HBF, // Header-Body-Footer
  Grid,
};

enum container_attribute
{
  ATTRIBUTE_NONE = 0x0,
  ATTRIBUTE_DRAG = 0x1,
  ATTRIBUTE_MERGE = 0x2,
  ATTRIBUTE_MERGE_SLOT = 0x4,
  ATTRIBUTE_BUTTON = 0x8,
  ATTRIBUTE_COLOR = 0x10,
  ATTRIBUTE_TEXT = 0x20,
  ATTRIBUTE_SIZE = 0x40
};


const c8* ToString(container_type Type)
{
  switch(Type)
  {
    case container_type::None: return "None";
    case container_type::Root: return "Root";
    case container_type::Border: return "Border";
    case container_type::Split: return "Split";
    case container_type::HBF: return "HBF";
    case container_type::Grid: return "Grid";
  }
  return "";
};

const c8* ToString(u32 Type)
{
  switch(Type)
  {
    case ATTRIBUTE_DRAG: return "Drag";
    case ATTRIBUTE_MERGE: return "Merge";
    case ATTRIBUTE_MERGE_SLOT: return "Slot";
    case ATTRIBUTE_BUTTON: return "Button";
    case ATTRIBUTE_COLOR: return "Color";
    case ATTRIBUTE_TEXT: return "Text";
    case ATTRIBUTE_SIZE: return "Size";
  }
  return "";
};


struct container_node;
struct menu_interface;
struct menu_tree;

menu_interface* CreateMenuInterface();
void UpdateAndRenderMenuInterface(game_input* GameInput, menu_interface* Interface);
container_node* NewContainer(menu_interface* Interface, container_type Type = container_type::None);
void DeleteContainer(menu_interface* Interface, container_node* Node);
menu_tree* NewMenuTree(menu_interface* Interface);
void FreeMenuTree(menu_interface* Interface,  menu_tree* MenuToFree);
container_node* ConnectNode(container_node* Parent, container_node* NewNode);
void DisconnectNode(container_node* Node);

container_node* CreateBorderNode(menu_interface* Interface, b32 Vertical=false, r32 Position = 0.5f,  v4 Color =  V4(0,0,0.4,1));


#define MENU_UPDATE_CHILD_REGIONS(name) void name(container_node* Parent)
typedef MENU_UPDATE_CHILD_REGIONS( menu_get_region );

#define MENU_DRAW(name) void name( menu_interface* Interface, container_node* Node)
typedef MENU_DRAW( menu_draw );

struct menu_functions
{
  menu_get_region** UpdateChildRegions;
  menu_draw** Draw;
};

menu_functions GetMenuFunction(container_type Type);

struct memory_link
{
  u32 Size;
  memory_link* Next;
  memory_link* Previous;
};

struct menu_attribute_header
{
  container_attribute Type;
  menu_attribute_header* Next;
};


struct container_node
{
  container_type Type;
  u32 Attributes;
  menu_attribute_header* FirstAttribute;

  // Tree Links (Menu Structure)
  u32 Depth;
  container_node* Parent;
  container_node* FirstChild;
  container_node* NextSibling;
  container_node* PreviousSibling;

  rect2f Region;
  menu_functions Functions;


};

struct root_node
{
  r32 HeaderSize;
  r32 FooterSize;
};

struct hbf_node
{
  r32 HeaderSize;
  r32 FooterSize;
};

struct border_leaf
{
  b32 Vertical;
  r32 Position;
  r32 Thickness;
};

struct grid_node
{
  u32 Col;
  u32 Row;
  r32 TotalMarginX;
  r32 TotalMarginY;
};

struct merge_slot_attribute
{
  u32 HotMergeZone;
  rect2f MergeZone[5];
};

struct mergable_attribute
{
  merge_slot_attribute* Slot;
};


#define BUTTON_ATTRIBUTE_UPDATE(name) void name( struct  menu_interface* Interface, struct button_attribute* Attr, struct  container_node* Node)
typedef BUTTON_ATTRIBUTE_UPDATE( button_attribute_update );

struct button_attribute
{
  //binary_signal_state State;
  void* Data;
  button_attribute_update** Update;
};

struct color_attribute
{
  v4 Color;
};

struct text_attribute
{
  c8 Text[256];
  u32 FontSize;
  v4 Color;
};

struct tabbed_button_data
{
  container_node* TabbedHBF;
  container_node* ItemHBF;
  container_node* ItemHeader;
  container_node* ItemBody;
  container_node* ItemFooter;
};

#define DRAGGABLE_ATTRIBUTE_UPDATE(name) void name( struct  menu_interface* Interface, struct  container_node* Node,  struct draggable_attribute* Attr)
typedef DRAGGABLE_ATTRIBUTE_UPDATE( draggable_attribute_update );

struct draggable_attribute
{
  container_node* Node;
  draggable_attribute_update** Update;
};

struct split_draggable_data
{
  container_node* SplitNode;
  border_leaf* Border;
};

enum class menu_region_alignment
{
  CENTER,
  LEFT,
  RIGHT,
  TOP,
  BOT
};

enum class menu_size_type
{
  RELATIVE,
  ABSOLUTE
};

struct container_size_t
{
  menu_size_type Type;
  r32 Value;
};

inline container_size_t
ContainerSizeT(menu_size_type T, r32 Val)
{
  container_size_t Result{};
  Result.Type = T;
  Result.Value = Val;
  return Result;
}

struct size_attribute
{
  container_size_t Width;
  container_size_t Height;
  container_size_t LeftOffset;
  container_size_t TopOffset;
  menu_region_alignment XAlignment;
  menu_region_alignment YAlignment;
};

struct menu_tree
{
  b32 Visible;

  u32 NodeCount;
  u32 Depth;
  container_node* Root;

  u32 HotLeafCount;
  container_node* HotLeafs[32];

  u32 ActiveLeafCount;
  container_node* ActiveLeafs[32];

  // TODO:
  // b32 ShouldDelete; <-- Implement later (Delete last in a cleanup phase)
};

struct menu_interface
{
  // TODO: Right now alot of the tree structure is dynamic, ie, gets freed and applied
  //       as the windows change.
  //       Others may want a handle to for example 'menu_tree' to see if it's visible 
  //       or other things... But 
  //         1: RootContainers being in an array where the elements get switched around
  //            makes any ptr garbrage when another window gets focus.
  //         Solution: Make into a list.
  //         2: Root Windows gets deleted when we merge and split windows.
  //         Solution: Not sure, 'Content' windows are permanent. Give them a special
  //         status and permanent handle?
  u32 RootContainerCount;
  menu_tree RootContainers[32];

  container_node* HotSubWindow;

  u32 ActiveMemory;
  u32 MaxMemSize;
  u8* MemoryBase;
  u8* Memory;
  memory_link Sentinel;

  v2 MousePos;
  v2 PreviousMousePos;
  binary_signal_state MouseLeftButton;
  binary_signal_state TAB;
  v2 MouseLeftButtonPush;
  v2 MouseLeftButtonRelese;

  r32 BorderSize;
  r32 HeaderSize;
  r32 MinSize;
};

inline u8* GetContainerPayload( container_node* Container )
{
  u8* Result = (u8*)(Container+1);
  return Result;
}

inline root_node* GetRootNode(container_node* Container)
{
  Assert(Container->Type == container_type::Root);
  root_node* Result = (root_node*) GetContainerPayload(Container);
  return Result;
}
inline border_leaf* GetBorderNode(container_node* Container)
{
  Assert(Container->Type == container_type::Border);
  border_leaf* Result = (border_leaf*) GetContainerPayload(Container);
  return Result;
}
inline hbf_node* GetHBFNode(container_node* Container)
{
  Assert(Container->Type == container_type::HBF);
  hbf_node* Result = (hbf_node*) GetContainerPayload(Container);
  return Result;
}
inline grid_node* GetGridNode(container_node* Container)
{
  Assert(Container->Type == container_type::Grid);
  grid_node* Result = (grid_node*) GetContainerPayload(Container);
  return Result;
}


void SetSplitDragAttribute(container_node* SplitNode, container_node* BorderNode);
container_node* ReallocateNode(menu_interface* Interface, container_node* SrcNode, u32 InputAttributes = 0);
void SplitWindowHeaderDrag( menu_interface* Interface, container_node* Node, draggable_attribute* Attr );
menu_tree* CreateNewRootContainer(menu_interface* Interface, container_node* BaseWindow, rect2f Region);
container_node* CreateSplitWindow( menu_interface* Interface, b32 Vertical, r32 BorderPos = 0.5);
container_node* CreateHBF(menu_interface* Interface, v4 HeaderColor, container_node* BodyNode);
container_node* SetSplitWindows( menu_interface* Interface, container_node* SplitNode, container_node* HBF1, container_node* HBF2);










#if 0 

// Leftover code for a radial menu.
// It's good code, so lets keep it for later.

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


struct radial_menu_region
{
  r32 AngleStart;
  r32 AngleEnd;
  r32 RadiusStart;
  r32 Radius;
};

radial_menu_region RadialMenuRegion(r32 AngleStart, r32 AngleEnd, r32 RadiusStart, r32 Radius)
{
  radial_menu_region Result = {};

  Result.AngleStart = RecanonicalizeAngle(AngleStart);
  Result.AngleEnd = RecanonicalizeAngle(AngleEnd);
  Result.RadiusStart = RadiusStart;
  Result.Radius = Radius;
  return Result;
}

struct radial_menu
{
  u32 MenuRegionCount;
  u32 HotRegionIndex;
  radial_menu_region* Regions;

  r32 MouseRadius;
  r32 MouseAngle;

  r32 MenuX;
  r32 MenuY;
};


void DEBUGDrawDottedLine(v2 Start, v2 End, v4 Color = V4(1,1,1,1), r32 NrDots = 100.f)
{
  for (r32 i = 0; i < NrDots; ++i)
  {
    v2 Pos = Start + (End-Start) * (i/NrDots);
    DEBUGPushQuad(Rect2f(Pos.X-0.005f,Pos.Y-0.005f, 0.01, 0.01), Color);
  }
}

void DEBUGDrawDottedCircularSection(v2 Origin, r32 AngleStart, r32 AngleEnd, r32 Radius, v4 Color = V4(1,1,1,1), r32 NrDots = 100.f)
{
  r32 AngleInterval = GetDeltaAngle(AngleStart, AngleEnd);
  for (r32 i = 0; i < NrDots; ++i)
  {
    r32 Angle = AngleStart + i * AngleInterval / NrDots;
    v2 Pos = V2(Origin.X + Radius * Cos(Angle),
                Origin.Y + Radius * Sin(Angle));
    DEBUGPushQuad(Rect2f(Pos.X-0.005f,Pos.Y-0.005f, 0.01, 0.01), Color);
  }
}

void DEBUGDrawRadialRegion( r32 OriginX, r32 OriginY, r32 MouseX, r32 MouseY, r32 AngleStart, r32 AngleEnd, r32 RadiusStart, r32 RadiusEnd )
{
  AssertCanonical(AngleStart);
  AssertCanonical(AngleEnd);

  // Draw Debug Regions
  v4 Color = V4(1,0,0,1);

  if(RadiusEnd < RadiusStart) RadiusEnd+=(Tau32-RadiusStart);

  v2 Origin = V2(OriginX, OriginY);
  v2 Mouse = V2(MouseX, MouseY) - Origin;
  r32 MouseAngle = ATan2(Mouse.Y, Mouse.X);
  MouseAngle = MouseAngle < 0 ? MouseAngle+Tau32 : MouseAngle;
  r32 MouseRadius = Norm(Mouse);

  v2 MouseLine  = V2(Origin.X +  MouseRadius * Cos(MouseAngle),
                     Origin.Y +  MouseRadius * Sin(MouseAngle));
  v2 StartLine  = V2(Origin.X +  RadiusEnd * Cos(AngleStart+0.1f),
                     Origin.Y +  RadiusEnd * Sin(AngleStart+0.1f));
  v2 StopLine   = V2(Origin.X +  RadiusEnd * Cos(AngleEnd-0.1f),
                     Origin.Y +  RadiusEnd * Sin(AngleEnd-0.1f));

  DEBUGDrawDottedLine(Origin, StartLine,  V4(0,1,0,1));
  DEBUGDrawDottedLine(Origin, StopLine,   V4(1,0,0,1));
  DEBUGDrawDottedLine(Origin, MouseLine,  V4(0,0,1,1));

  DEBUGDrawDottedCircularSection(Origin, AngleStart+0.1f, AngleEnd-0.1f, RadiusStart, Color);
  DEBUGDrawDottedCircularSection(Origin, AngleStart+0.1f, AngleEnd-0.1f, RadiusEnd,   Color);
  DEBUGDrawDottedCircularSection(Origin, 0, MouseAngle, MouseRadius, Color);
}

#endif