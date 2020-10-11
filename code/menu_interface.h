#pragma once

enum class container_type
{
  None,
  Root,
  Color,
  Border,
  Split,
  HBF, // Header-Body-Footer
};

enum container_attribute
{
  ATTRIBUTE_NONE = 0x0,
  ATTRIBUTE_DRAG = 0x1,
  ATTRIBUTE_MERGE = 0x2,
  ATTRIBUTE_MERGE_SLOT = 0x4,
  ATTRIBUTE_TABS = 0x8,
};


const c8* ToString(container_type Type)
{
  switch(Type)
  {
    case container_type::None: return "None";
    case container_type::Root: return "Root";
    case container_type::Color: return "Color";
    case container_type::Border: return "Border";
    case container_type::Split: return "Split";
    case container_type::HBF: return "HBF";
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
    case ATTRIBUTE_TABS: return "Tabs";
  }
  return "";
};


struct container_node;
struct menu_interface;
struct menu_tree;

menu_interface* CreateMenuInterface();
void UpdateAndRenderMenuInterface(game_input* GameInput, menu_interface* Interface);
container_node* NewContainer(menu_interface* Interface, container_type Type);
void DeleteContainer(menu_interface* Interface, container_node* Node);
menu_tree* NewMenuTree(menu_interface* Interface);
void FreeMenuTree(menu_interface* Interface,  menu_tree* MenuToFree);
container_node* ConnectNode(container_node* Parent, container_node* NewNode);
void DisconnectNode(container_node* Node);

container_node* CreateBorderNode(menu_interface* Interface, b32 Vertical=false, r32 Position = 0.5f,  v4 Color =  V4(0,0,0.4,1));


#define MENU_HANDLE_INPUT(name) void name( menu_interface* Interface, container_node* Node)
typedef MENU_HANDLE_INPUT( menu_handle_input );

#define MENU_UPDATE_CHILD_REGIONS(name) void name(container_node* Parent)
typedef MENU_UPDATE_CHILD_REGIONS( menu_get_region );

#define MENU_DRAW(name) void name( menu_interface* Interface, container_node* Node)
typedef MENU_DRAW( menu_draw );

struct menu_functions
{
  menu_get_region* UpdateChildRegions;
  menu_handle_input* HandleInput;
  menu_draw*  Draw;
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

struct tabbed_node
{
  u32 TabCount;
  container_node* Tabs[12];
  u32 SelectedTabOrdinal; // [1,3,4,5...], 0 means no tab selected
};

struct border_leaf
{
  b32 Vertical;
  r32 Position;
  r32 Thickness;
  v4 Color;
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

struct tabbed_attribute
{
  u32 ActiveTabIndex;
  container_node* Tabs[32];
};

struct draggable_attribute
{
  void* Data;
  void (*Update)( menu_interface* Interface, container_node* Node, draggable_attribute* Attr);
};

struct split_draggable_data
{
  container_node* SplitNode;
  border_leaf* Border;
};

struct color_leaf
{
  v4 Color;
};

struct menu_tree
{
  u32 NodeCount;
  u32 Depth;
  container_node* Root;

  u32 HotLeafCount;
  container_node* HotLeafs[32];

  u32 ActiveLeafCount;
  container_node* ActiveLeafs[32];
};


struct menu_interface
{
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
inline color_leaf* GetColorNode(container_node* Container)
{
  Assert(Container->Type == container_type::Color);
  color_leaf* Result = (color_leaf*) GetContainerPayload(Container);
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


void SetSplitDragAttribute(container_node* SplitNode, container_node* BorderNode);
container_node* ReallocateNode(menu_interface* Interface, container_node* SrcNode, u32 InputAttributes = 0);
void SwapNode(container_node* Out, container_node* In);
void SplitWindowHeaderDrag( menu_interface* Interface, container_node* Node, draggable_attribute* Attr );