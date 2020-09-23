#pragma once

enum class container_type
{
  None,
  Root,
  Color,
  Border,
  Header,
  Split,
  HBF, // Header-Body-Footer
};

enum container_attribute
{
  ATTRIBUTE_NONE = 0,
  ATTRIBUTE_DRAG = 1,
  ATTRIBUTE_MERGE = 2,
  ATTRIBUTE_MERGE_SLOT = 4,
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


struct container_node;
struct menu_interface;
struct menu_tree;

menu_interface* CreateMenuInterface();
void UpdateAndRenderMenuInterface(game_input* GameInput, menu_interface* Interface);
container_node* NewContainer(menu_interface* Interface, container_type Type,  u32 Attributes = 0);
void DeleteContainer(menu_interface* Interface, container_node* Node);
menu_tree* NewMenuTree(menu_interface* Interface);
void FreeMenuTree(menu_interface* Interface,  menu_tree* MenuToFree);
container_node* ConnectNode(container_node* Parent, container_node* NewNode);
void DisconnectNode(container_node* Node);

container_node* CreateBorderNode(menu_interface* Interface, b32 Vertical=false, r32 Position = 0.5f,  v4 Color =  V4(0,0,0.4,1));

#define GetContainerPayload( Type, Container )  ((Type*) (((u8*)Container) + sizeof(container_node)))

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

struct container_node
{
  container_type Type;
  u32 Attributes;
  // Tree Links (Menu Structure)
  u32 Depth;
  container_node* Parent;
  container_node* FirstChild;
  container_node* NextSibling;
  container_node* PreviousSibling;

  
  // List Links (Memory)
  u32 ContainerSize;
  u32 AttributeBaseOffset;
  container_node* Next;
  container_node* Previous;

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

struct mergable_attribute
{
  container_node* SrcNode = 0;
  container_node* DstNode = 0;
  u32 HotMergeZone;
  rect2f MergeZone[5];
};

struct draggable_attribute
{
  b32 Dragging;
  void* Data;
  void (*Update)( menu_interface* Interface, container_node* Node, draggable_attribute* Attr);
};

struct split_draggable_data
{
  container_node* SplitNode;
  border_leaf* Border;
};

struct empty_leaf
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
  container_node Sentinel;

  v2 MousePos;
  v2 PreviousMousePos;
  binary_signal_state MouseLeftButton;
  v2 MouseLeftButtonPush;
  v2 MouseLeftButtonRelese;

  r32 BorderSize;
  r32 HeaderSize;
  r32 MinSize;
};
