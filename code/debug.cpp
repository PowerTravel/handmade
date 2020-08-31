#include "debug.h"

u8* DEBUGPushSize_(debug_state* DebugState, u32 Size)
{
  u8* Result = DebugState->Memory;
  DebugState->Memory += Size;
  Assert( (DebugState->Memory ) < (DebugState->MemoryBase + DebugState->MemorySize)  );
  return Result;
}

#define DEBUGPushStruct(DebugState, type) (type*) DEBUGPushSize_(DebugState, sizeof(type))
#define DEBUGPushArray(DebugState, count, type) (type*) DEBUGPushSize_(DebugState, count*sizeof(type))
#define DEBUGPushCopy(DebugState, Size, Src) utils::Copy(Size, (void*) (Src), (void*) DEBUGPushSize_(DebugState, Size))

internal void RefreshCollation();
internal void RestartCollation();
internal inline void DebugRewriteConfigFile();

internal void
TogglePause(debug_state* DebugState, menu_item* Item)
{
  if(!DebugState->Paused)
  {
    DebugState->Paused = true;
    DebugState->Resumed = false;
  }else{
    if(DebugState->Resumed)
    {
      RefreshCollation();
      DebugState->Resumed = false;
    }

    if(DebugState->Paused)
    {
      DebugState->Resumed = true;
    }

    DebugState->Paused = false;
  }

  Item->Active = DebugState->Paused;
}


void SetRadialMenuFunctionPointers(debug_state* DebugState)
{
  radial_menu* MainMenu = &DebugState->RadialMenues[0];
  Assert(MainMenu->MenuRegionCount == 3);

  // "Show Collation"
  MainMenu->MenuItems[0].Activate = [](debug_state* DebugState, menu_item* Item)
  {
    DebugState->ChartVisible = !DebugState->ChartVisible;
    Item->Active = (b32) DebugState->ChartVisible;
  };

  // "Pause Collation"
  MainMenu->MenuItems[1].Activate = TogglePause;

  // "Options"
  MainMenu->MenuItems[2].Activate = [](debug_state* DebugState, menu_item* Item)
  {
    active_menu ActiveMenu = {};
    ActiveMenu.Type = menu_type::Box;
    ActiveMenu.BoxMenu = &DebugState->BoxMenues[0];
    DebugState->ActiveMenu = ActiveMenu;

  };
}

menu_functions GetMenuFunction(container_type Type)
{
  switch(Type)
  {
    case container_type::Root: return GetRootMenuFunctions();
    case container_type::Empty: return GetEmptyFunctions();
    case container_type::Split: return SplitMenuFunctions();
    case container_type::TabbedHeader: return TabbedHeaderMenuFunctions();
    case container_type::MenuHeader: return MenuHeaderMenuFunctions();
    case container_type::ContainerList: return ContainerListFunctions();
    case container_type::Button: return GetButtonFunctions();
    case container_type::Profiler: return GetProfilerFunctions();
    default: Assert(0);
  }
  return {};
}


u32 GetContainerSize(container_type Type)
{
  u32 Result = sizeof(container_node);
  switch(Type)
  {
    case container_type::Root:          {Result += sizeof(root_window);}break;
    case container_type::Empty:         {Result += sizeof(empty_window);}break;
    case container_type::Split:         {Result += sizeof(split_window);}break;
    case container_type::TabbedHeader:  {Result += sizeof(tabbed_header_window);}break;
    case container_type::MenuHeader:    {Result += sizeof(menu_header_window);}break;
    case container_type::ContainerList: {Result += sizeof(container_list);}break;
    case container_type::Button:        {Result += sizeof(menu_button);}break;
    case container_type::Profiler:      {Result += sizeof(profiling_window);}break;
    default: Assert(0);
  }
  return Result;
}

container_node* NewContainer(menu_interface* Interface, container_type Type)
{
  u32 ContainerSize = GetContainerSize(Type);
  Interface->ActiveMemory += ContainerSize;

  container_node* Result = 0;
  {
    u32 RegionUsed = (u32)(Interface->Memory - Interface->MemoryBase);
    u32 TotSize = (u32) Interface->MaxMemSize;
    r32 Percentage = RegionUsed / (r32) TotSize;
    u32 ActiveMemory = Interface->ActiveMemory;
    r32 Fragmentation = ActiveMemory/(r32)RegionUsed;
    #if 0
    Platform.DEBUGPrint("--==<< Pre Memory >>==--\n");
    Platform.DEBUGPrint(" - Tot Mem Used   : %2.3f  (%d/%d)\n", Percentage, RegionUsed, TotSize );
    Platform.DEBUGPrint(" - Fragmentation  : %2.3f  (%d/%d)\n", Fragmentation, ActiveMemory, RegionUsed );
    #endif
  }
  
  u32 RegionUsed = (u32)(Interface->Memory - Interface->MemoryBase);
  r32 MemoryFragmentation = Interface->ActiveMemory/(r32)RegionUsed;
  b32 MemoryTooFragmented = MemoryFragmentation < 0.8;
  if( MemoryTooFragmented || RegionUsed == Interface->MaxMemSize )
  {
    u32 Slot = 0;
    u32 SlotSpace = 0;
    u32 SlotSize = 0;

    container_node* CurrentNode = Interface->Sentinel.Next;
    container_node* NextNode = CurrentNode->Next;
    while( CurrentNode->Next != &Interface->Sentinel)
    {
      midx Base = (midx) CurrentNode + CurrentNode->ContainerSize;
      midx NextNodeAddress    = (midx)  CurrentNode->Next;
      Assert(Base <= NextNodeAddress);

      midx OpenSpace = NextNodeAddress - Base;

      if(OpenSpace >= ContainerSize)
      {
        Result = (container_node*) Base;
        ListInsertAfter(CurrentNode, Result);
        SlotSpace = (u32) Slot;
        SlotSize = (u32) OpenSpace;
        break;
      }

      Slot++;
      CurrentNode =  CurrentNode->Next;
    }

   
    {
      u32 SlotCount = 0;
      container_node* CurrentNode2 = Interface->Sentinel.Next;
      container_node* NextNode2 = CurrentNode->Next;
     
      while( CurrentNode2->Next != &Interface->Sentinel)
      {
        SlotCount++;
        CurrentNode2 = CurrentNode2->Next;
      }  
      #if 0
      Platform.DEBUGPrint("--==<< Middle Inset >>==--\n");
      Platform.DEBUGPrint(" - Slot: [%d,%d]\n", SlotSpace, SlotCount);
      Platform.DEBUGPrint(" - Size: [%d,%d]\n", ContainerSize, SlotSize);
      #endif
    }
    
    
  }
  
  if(!Result)
  {
    Assert(RegionUsed+ContainerSize < Interface->MaxMemSize);
    #if 0
    Platform.DEBUGPrint("--==<< Post Inset >>==--\n");
    Platform.DEBUGPrint(" - Memory Left  : %d\n",Interface->MaxMemSize - (u32)RegionUsed + ContainerSize);
    Platform.DEBUGPrint(" - ContainerSize: %d\n\n", ContainerSize);
    #endif
    Result = (container_node*) Interface->Memory;
    Interface->Memory += ContainerSize;  
    container_node* Sentinel = &Interface->Sentinel;
    ListInsertBefore( Sentinel, Result );
  }
  
  Result->Type = Type;
  Result->ContainerSize = ContainerSize;
  Result->Functions = GetMenuFunction(Type);
  {
    u32 RegionUsed2 = (u32)(Interface->Memory - Interface->MemoryBase);
    u32 TotSize = (u32) Interface->MaxMemSize;
    r32 Percentage = RegionUsed2 / (r32) TotSize;
    u32 ActiveMemory = Interface->ActiveMemory;
    r32 Fragmentation = ActiveMemory/(r32)RegionUsed2;
    #if 0
    Platform.DEBUGPrint("--==<< Post Memory >>==--\n");
    Platform.DEBUGPrint(" - Tot Mem Used   : %2.3f  (%d/%d)\n", Percentage, RegionUsed, TotSize );
    Platform.DEBUGPrint(" - Fragmentation  : %2.3f  (%d/%d)\n", Fragmentation, ActiveMemory, RegionUsed );
    #endif
  }
  return Result;
}
void DeleteContainer( menu_interface* Interface, container_node* Node)
{
  Node->Previous->Next = Node->Next;
  Node->Next->Previous = Node->Previous;
  Interface->ActiveMemory -= Node->ContainerSize;
  
  // Note: We should in theory not have to zerosize the deleted containers, 
  //       But if we don't we sometimes crash, so look out for that when refactoring.
  //       This is probably masking some bug.
  utils::ZeroSize(Node->ContainerSize, (void*)Node);
}

// Preorder breadth first.
void SetInterfaceFunctionPointers(container_node* RootWindow, memory_arena* Arena, u32 NodeCount)
{
  u32 StackElementSize = sizeof(container_node*);
  u32 StackByteSize = NodeCount * StackElementSize;

  u32 StackCount = 0;
  container_node** ContainerStack = PushArray(Arena, NodeCount, container_node*);

  // Push Root
  ContainerStack[StackCount++] = RootWindow;

  while(StackCount>0)
  {
    // Pop new parent from Stack
    container_node* Parent = ContainerStack[--StackCount];
    ContainerStack[StackCount] = 0;

    Parent->Functions = GetMenuFunction(Parent->Type);
    if(Parent->Type == container_type::TabbedHeader)
    {
      tabbed_header_window* TabbedHeader = GetContainerPayload(tabbed_header_window, Parent);
      for (u32 TabIndex = 0; TabIndex < TabbedHeader->TabCount; ++TabIndex)
      {
        TabbedHeader->Tabs[TabIndex]->Functions = GetMenuFunction(TabbedHeader->Tabs[TabIndex]->Type);
      }
    }

    container_node* Child = Parent->FirstChild;
    while(Child)
    {
      ContainerStack[StackCount++] = Child;

      Child = Child->NextSibling;
    }
  }
}


//  PostOrder (Left, Right, Root),  Depth first.
u32_pair UpdateSubTreeDepthAndCount( u32 ParentDepth, container_node* SubTreeRoot )
{
  u32 TotalDepth = 0;
  u32 CurrentDepth = ParentDepth;
  u32 NodeCount = 0;

  // Make SubTreeRoot look like an actual root node
  container_node* SubTreeParent = SubTreeRoot->Parent;
  container_node* SubTreeSibling = SubTreeRoot->NextSibling;

  SubTreeRoot->Parent = 0;
  SubTreeRoot->NextSibling = 0;

  container_node* CurrentNode = SubTreeRoot;

  while(CurrentNode != SubTreeRoot->Parent)
  {
    // Set the depth of the current Node
    CurrentNode->Depth = CurrentDepth++;
    ++NodeCount;

    // Step all the way down (setting depth as you go along)
    while(CurrentNode->FirstChild)
    {
      CurrentNode = CurrentNode->FirstChild;
      CurrentNode->Depth = CurrentDepth++;
      ++NodeCount;
    }

    // The depth is now set until the leaf.
    TotalDepth = Maximum(CurrentDepth, TotalDepth);

    // Step up until you find another sibling or we reach root
    while(!CurrentNode->NextSibling && CurrentNode->Parent)
    {
      CurrentNode = CurrentNode->Parent;  
      CurrentDepth--;
      Assert(CurrentDepth >= 0)
    }

    // Either we found another sibling and we can traverse that part of the tree
    //  or we are at root and root has no siblings and we are done.
    CurrentNode = CurrentNode->NextSibling;
  }

  // Restore the Root
  SubTreeRoot->Parent = SubTreeParent;
  SubTreeRoot->NextSibling = SubTreeSibling;

  u32_pair Result = {};
  Result.a = NodeCount;
  Result.b = TotalDepth;
  
  return Result;
}


void TreeSensus( menu_tree* Menu )
{
  u32_pair Pair =  UpdateSubTreeDepthAndCount( 0, Menu->Root );

  Menu->NodeCount = Pair.a;
  Menu->Depth = Pair.b;
 // Platform.DEBUGPrint("Tree Sensus:  Depth: %d, Count: %d\n", Pair.b, Pair.a);
}


// Preorder breadth first.
void UpdateRegions( memory_arena* Arena, u32 NodeCount, container_node* Container )
{
  u32 StackElementSize = sizeof(container_node*);
  u32 StackByteSize = NodeCount * StackElementSize;

  u32 StackCount = 0;
  container_node** ContainerStack = PushArray(Arena, NodeCount, container_node*);

  // Push Root
  ContainerStack[StackCount++] = Container;

  while(StackCount>0)
  {
    // Pop new parent from Stack
    container_node* Parent = ContainerStack[--StackCount];
    ContainerStack[StackCount] = 0;

    // Update the region of all children and push them to the stack
    container_node* Child = Parent->FirstChild;
    u32 ChildIndex = 0;
    window_regions RegionType = window_regions::WholeBody;
    while(Child)
    {
      if(Parent->Type == container_type::Split)
      {
        RegionType = ChildIndex == 0 ? window_regions::BodyOne : window_regions::BodyTwo;  
      }

      if(Parent->Functions.GetChildRegion)
      {
        Child->Region = Parent->Functions.GetChildRegion(Parent, Child);
      }else{
        Child->Region = Parent->Functions.GetRegionRect(RegionType, Parent);  
      }
      
      
      ContainerStack[StackCount++] = Child;

      Child = Child->NextSibling;
      ChildIndex++;
    }
  }
}

// Preorder breadth first.
void DrawMenu( memory_arena* Arena, menu_interface* Interface, u32 NodeCount, container_node* Container )
{
  u32 StackElementSize = sizeof(container_node*);
  u32 StackByteSize = NodeCount * StackElementSize;

  u32 StackCount = 0;
  container_node** ContainerStack = PushArray(Arena, NodeCount, container_node*);

  // Push Root
  ContainerStack[StackCount++] = Container;

  while(StackCount>0)
  {
    // Pop new parent from Stack
    container_node* Parent = ContainerStack[--StackCount];
    ContainerStack[StackCount] = 0;

    Parent->Functions.Draw(Interface, Parent);
    // Update the region of all children and push them to the stack
    container_node* Child = Parent->FirstChild;
    while(Child)
    {
      ContainerStack[StackCount++] = Child;
      Child = Child->NextSibling;
    }
  }
}


node_region_pair GetRegion(memory_arena* Arena, u32 NodeCount, container_node* Container, v2 MousePos)
{
  node_region_pair Result = {};
  u32 StackElementSize = sizeof(container_node*);
  u32 StackByteSize = NodeCount * StackElementSize;

  u32 StackCount = 0;
  container_node** ContainerStack = PushArray(Arena, NodeCount, container_node*);

  // Push Root
  ContainerStack[StackCount++] = Container;

  while(StackCount>0)
  {
    // Pop new parent from Stack
    container_node* Parent = ContainerStack[--StackCount];
    ContainerStack[StackCount] = 0;

    window_regions Region = Parent->Functions.GetMouseOverRegion(Parent, MousePos);

    if(Region == window_regions::None)
    {
      Result.Node = 0;
      Result.Region = window_regions::None;
      return Result;
    }

    rect2f RegionRect = Parent->Functions.GetRegionRect(Region, Parent);

    // Check if mouse is inside the child region and push those to the stack.
    container_node* Child = Parent->FirstChild;
    while(Child)
    {
      if(Intersects(Child->Region, MousePos))
      {
        ContainerStack[StackCount++] = Child;
      }
      Child = Child->NextSibling;
    }

    if(StackCount == 0)
    {
      Result.Node = Parent;
      Result.Region = Region;
      return Result;
    }
  }

  return Result;
}


menu_tree* GetNewMenuTree(menu_interface* Interface)
{
  menu_tree* Result = &Interface->RootContainers[Interface->RootContainerCount++];
  return Result;
}

void FreeMenuTree(menu_interface* Interface,  menu_tree* MenuToFree)
{
  container_node* Root = MenuToFree->Root;

  // Remove the menu from RootContainers
  u32 WindowIndex = 0;
  while(WindowIndex < Interface->RootContainerCount)
  {
    menu_tree* MenuTree = &Interface->RootContainers[WindowIndex];
    if( MenuTree->Root == Root )
    {
      break;
    }
    ++WindowIndex;
  }

  Assert(WindowIndex != Interface->RootContainerCount);

  while(WindowIndex < Interface->RootContainerCount-1)
  {
    Interface->RootContainers[WindowIndex] = Interface->RootContainers[WindowIndex+1];
    WindowIndex++;
  }

  Interface->RootContainers[Interface->RootContainerCount-1] = {};
  Interface->RootContainerCount--;


  // Free the nodes;
  // 1: Go to the bottom
  // 2: Step up Once
  // 3: Delete FirstChild 
  // 4: Set NextSibling as FirstChild
  // 5: Repeat from 1
  container_node* Node = Root->FirstChild;
  while(Node)
  {
    while(Node->FirstChild)
    {
      Node = Node->FirstChild;
    }

    Node = Node->Parent;
    if(Node)
    {
      container_node* NodeToDelete = Node->FirstChild;
      Node->FirstChild = NodeToDelete->NextSibling;
      DeleteContainer(Interface, NodeToDelete);
    }
  }
  DeleteContainer(Interface, Root);
}

void DisconnectNode(container_node* Node)
{
  container_node* Parent = Node->Parent;

  if(Parent)
  {
    Assert(Parent->FirstChild);
    if(Parent->FirstChild == Node)
    {
      Parent->FirstChild = Node->NextSibling; 
    }else{
      container_node* Child = Parent->FirstChild;
      while(Child->NextSibling)
      {
        if(Child->NextSibling == Node)
        {
          Child->NextSibling = Node->NextSibling;
          break;
        }
      }  
    }
  } 

  Node->NextSibling = 0;
  Node->Parent = 0;
}



void ConnectNode(container_node* Parent, container_node* NewNode)
{
  NewNode->Parent = Parent;

  if( Parent )
  {
    container_node** Child = &Parent->FirstChild;
    while(*Child)
    {
      Child = &((*Child)->NextSibling);
    }
    *Child = NewNode;
  }
}

ACTIVATE_BUTTON( toggle_colliders )
{
  DebugState->ConfigCollider = !DebugState->ConfigCollider;
  Button->Active = DebugState->ConfigCollider;
}

ACTIVATE_BUTTON( toggle_multi_thread )
{
  DebugState->ConfigMultiThreaded = !DebugState->ConfigMultiThreaded;
  Button->Active = (b32) DebugState->ConfigMultiThreaded;
}

ACTIVATE_BUTTON( toggle_aabb_tree )
{
  DebugState->ConfigAABBTree = !DebugState->ConfigAABBTree;
  Button->Active = DebugState->ConfigAABBTree;
}

ACTIVATE_BUTTON( toggle_collision_points )
{
  DebugState->ConfigCollisionPoints = !DebugState->ConfigCollisionPoints;
  Button->Active = DebugState->ConfigCollisionPoints;
}

ACTIVATE_BUTTON( recompile )
{
  DebugRewriteConfigFile();
}

void SetMenuButtonFunctions(menu_interface* Interface)
{
  Interface->Activate[0] = toggle_colliders;
  Interface->Activate[1] = toggle_multi_thread;
  Interface->Activate[2] = toggle_aabb_tree;
  Interface->Activate[3] = toggle_collision_points;
  Interface->Activate[4] = recompile;
}


void UpdateFunctionPointers(debug_state* DebugState)
{
  if(!DebugState->UpdateFunctionPointers)
  {
    SetRadialMenuFunctionPointers(DebugState);

    SetMenuButtonFunctions(DebugState->MenuInterface);
    for (s32 WindowIndex = 0;
             WindowIndex >= 0;
           --WindowIndex)
    {
      menu_tree* MenuTree = &DebugState->MenuInterface->RootContainers[WindowIndex];
      SetInterfaceFunctionPointers(MenuTree->Root, &DebugState->Arena, MenuTree->NodeCount);
    }  
  }
}

internal debug_state*
DEBUGGetState()
{
  debug_state* DebugState = DebugGlobalMemory->DebugState;
  if(!DebugState)
  {
    DebugGlobalMemory->DebugState = BootstrapPushStruct(debug_state, Arena);
    DebugState = DebugGlobalMemory->DebugState;
  }

  if(!DebugState->Initialized)
  {
    // Permanent Memory
    DebugState->MemorySize = (midx) Megabytes(1);
    DebugState->MemoryBase = PushArray(&DebugState->Arena, DebugState->MemorySize, u8);
    DebugState->Memory = DebugState->MemoryBase;

    DebugState->MenuInterface = PushStruct(&DebugState->Arena, menu_interface);
    menu_interface* Interface = DebugState->MenuInterface;
    Interface->ActiveMemory = 0;
    Interface->MaxMemSize = Megabytes(1);
    Interface->MemoryBase = (u8*) PushSize(&DebugState->Arena, Interface->MaxMemSize);
    Interface->Memory = Interface->MemoryBase;
    Interface->BorderSize = 0.007;
    Interface->HeaderSize = 0.02;
    Interface->MinSize = 0.2f;

    // Transient Memory Begin
    DebugState->CollateTemp = BeginTemporaryMemory(&DebugState->Arena);
    DebugState->Initialized = true;
    DebugState->Paused = false;
    DebugState->Resumed = false;
    DebugState->ScopeToRecord = 0;

    // Config state
    DebugState->ConfigMultiThreaded = MULTI_THREADED;
    DebugState->ConfigCollisionPoints = SHOW_COLLISION_POINTS;
    DebugState->ConfigCollider = SHOW_COLLIDER;
    DebugState->ConfigAABBTree = SHOW_AABB_TREE;

    // Allocate Menu Space
    DebugState->ActiveMenu = {};

    DebugState->RadialMenuEntries =  1;
    DebugState->RadialMenues = DEBUGPushArray(DebugState, DebugState->RadialMenuEntries, radial_menu);

    menu_item MainMenuItems[] =
    {
      MenuItem("Show Collation"),
      MenuItem("Pause Collation"),
      MenuItem("Options")
    };

    radial_menu* MainMenu = &DebugState->RadialMenues[0];
    MainMenu->MenuRegionCount = ArrayCount(MainMenuItems);
    MainMenu->MenuItems = (menu_item*) DEBUGPushCopy(DebugState, sizeof(MainMenuItems), MainMenuItems);
    MainMenu->Regions = DEBUGPushArray(DebugState, MainMenu->MenuRegionCount, radial_menu_region);


    DebugState->BoxMenuEntries =  1;
    DebugState->BoxMenues = DEBUGPushArray(DebugState, DebugState->RadialMenuEntries, box_menu);
    menu_item OptionMenuItems[] =
    {
      MenuItem("Multi Threaded", DebugState->ConfigMultiThreaded),
      MenuItem("Collision Points", DebugState->ConfigCollisionPoints),
      MenuItem("Colliders", DebugState->ConfigCollider),
      MenuItem("AABBTree", DebugState->ConfigAABBTree),
    };

    box_menu* OptionsMenu = &DebugState->BoxMenues[0];
    OptionsMenu->MenuItemCount = ArrayCount(OptionMenuItems);
    OptionsMenu->MenuItems = (menu_item*) DEBUGPushCopy(DebugState, sizeof(OptionMenuItems), OptionMenuItems);



    container_node* Sentinel = &(Interface->Sentinel);
    ListInitiate(Sentinel);

    {

      /// Options Menu

      // "Toggle Colliders"
      container_node* ListEntry0 = NewContainer(Interface, container_type::Button);
      menu_button* Button0 = GetContainerPayload(menu_button, ListEntry0);
      str::CopyStringsUnchecked("Colliders", Button0->Text);
      Button0->Color = V4(1,0,0,1);
      Button0->Active = DebugState->ConfigCollider;
      Button0->Activate = &Interface->Activate[0];

      // "Toggle Multi Threaded"
      container_node*  ListEntry1 = NewContainer(Interface, container_type::Button);
      menu_button* Button1 = GetContainerPayload(menu_button, ListEntry1);
      str::CopyStringsUnchecked("Multi Threaded", Button1->Text);
      Button1->Color = V4(0,1,0,1);
      Button1->Active = DebugState->ConfigMultiThreaded;
      Button1->Activate = &Interface->Activate[1];

      // "Toggle AABBTree"
      container_node*  ListEntry2 = NewContainer(Interface, container_type::Button);
      menu_button* Button2 = GetContainerPayload(menu_button, ListEntry2);
      str::CopyStringsUnchecked("AABB Tree", Button2->Text);
      Button2->Color = V4(0,0,1,1);
      Button2->Active = DebugState->ConfigAABBTree;
      Button2->Activate = &Interface->Activate[2];

      // "Collision Points"
      container_node*  ListEntry3 = NewContainer(Interface, container_type::Button);
      menu_button* Button3 = GetContainerPayload(menu_button, ListEntry3);
      str::CopyStringsUnchecked("Collision Points", Button3->Text);
      Button3->Color = V4(0,1,1,1);
      Button3->Active = DebugState->ConfigCollisionPoints;
      Button3->Activate = &Interface->Activate[3];

      // "Collision Points"
      container_node*  ListEntry4 = NewContainer(Interface, container_type::Button);
      menu_button* Button4 = GetContainerPayload(menu_button, ListEntry4);
      str::CopyStringsUnchecked("Recompile", Button4->Text);
      Button4->Color = V4(0,0,0,1);
      Button4->Active = false;
      Button4->Activate = &Interface->Activate[4];

      container_node* ContainerList = NewContainer(Interface, container_type::ContainerList);
      container_list* List = GetContainerPayload(container_list, ContainerList);
      ConnectNode(ContainerList, ListEntry0);
      ConnectNode(ContainerList, ListEntry1);
      ConnectNode(ContainerList, ListEntry2);
      ConnectNode(ContainerList, ListEntry3);
      ConnectNode(ContainerList, ListEntry4);


      /// Profiling Menu
      container_node* ProfilingContainer = NewContainer(Interface, container_type::Profiler);


      /// Tabbed Window

      menu_tree* Root = GetNewMenuTree(Interface);
      Root->Root = NewContainer(Interface, container_type::Root);

      container_node* RootContainer = Root->Root;
      RootContainer->Region = Rect2f(0.2,0.2,0.5,0.5);
      root_window* RootWindow = GetContainerPayload(root_window,Root->Root);
      RootWindow->BorderSize = Interface->BorderSize;
      RootWindow->MinSize = Interface->MinSize; 

      container_node* RootHeader = NewContainer(Interface, container_type::MenuHeader);
      menu_header_window* MenuHeader = GetContainerPayload(menu_header_window, RootHeader);
      MenuHeader->HeaderSize = Interface->HeaderSize;
      MenuHeader->RootWindow = RootContainer;

      container_node* TabbedHeader = NewContainer(Interface, container_type::TabbedHeader);
      tabbed_header_window* TabbedHeaderWindow = GetContainerPayload(tabbed_header_window, TabbedHeader);
      TabbedHeaderWindow->HeaderSize = Interface->HeaderSize;

      container_node*  EmptyContainer2 = NewContainer(Interface, container_type::Empty);
      GetContainerPayload(empty_window, EmptyContainer2)->Color = V4(0.4,0,0,1);
      container_node*  EmptyContainer3 = NewContainer(Interface, container_type::Empty);
      GetContainerPayload(empty_window, EmptyContainer3)->Color = V4(0,0.4,0,1);

      ConnectNode(0, Root->Root);
      ConnectNode(RootContainer, RootHeader);
      ConnectNode(RootHeader,    TabbedHeader);
      ConnectNode(TabbedHeader,  ContainerList);
      TabbedHeaderWindow->Tabs[TabbedHeaderWindow->TabCount++] = ContainerList;
      TabbedHeaderWindow->Tabs[TabbedHeaderWindow->TabCount++] = ProfilingContainer;
      TabbedHeaderWindow->Tabs[TabbedHeaderWindow->TabCount++] = EmptyContainer2;
      TabbedHeaderWindow->Tabs[TabbedHeaderWindow->TabCount++] = EmptyContainer3;      

      TreeSensus(Root);
      UpdateRegions( &DebugState->Arena, Root->NodeCount, Root->Root);
    }
    RestartCollation();
  }
  return DebugState;
}

internal void
RestartCollation()
{
    debug_state* DebugState = DEBUGGetState();
    // We can store MAX_DEBUG_EVENT_ARRAY_COUNT*4 frames of collated debug records
    // However, when we change Scope we clear ALL the collated memory.
    // So when we recollate we only have MAX_DEBUG_EVENT_ARRAY_COUNT frames worth of data
    // Thus we loose the 3 * MAX_DEBUG_EVENT_ARRAY_COUNT worth of collated data.
    // One effect of this is that we can display 10 frames, but MAX_DEBUG_EVENT_ARRAY_COUNT is atm 8;
    // Thus when we klick a function to inspect we suddenly only display 7 frames
    EndTemporaryMemory(DebugState->CollateTemp);
    DebugState->CollateTemp = BeginTemporaryMemory(&DebugState->Arena);

    DebugState->FirstThread = 0;
    DebugState->FirstFreeBlock = 0;
    DebugState->Frames = PushArray(&DebugState->Arena, MAX_DEBUG_EVENT_ARRAY_COUNT*4, debug_frame);
    DebugState->FrameBarLaneCount = 0;
    DebugState->FrameCount = 0;
    DebugState->FrameBarRange = 0;//60000000.0f;
    DebugState->CollationArrayIndex = GlobalDebugTable->CurrentEventArrayIndex+1;
    DebugState->CollationFrame = 0;    
}

void BeginDebugStatistics(debug_statistics* Statistic)
{
  Statistic->Count = 0;
  Statistic->Min =  R32Max;
  Statistic->Max = -R32Max;
  Statistic->Avg = 0;
}

void EndDebugStatistics(debug_statistics* Statistic)
{
  if(Statistic->Count != 0)
  {
    Statistic->Avg /= Statistic->Count;
  }else{
    Statistic->Min = 0;
    Statistic->Max = 0;
  }
}

void AccumulateStatistic(debug_statistics* Statistic, r32 Value)
{
  if(Statistic->Min > Value)
  {
    Statistic->Min = Value;
  }
  if(Statistic->Max < Value)
  {
    Statistic->Max = Value;
  }
  Statistic->Avg += Value;
  ++Statistic->Count;
}


#define DebugRecords_Main_Count __COUNTER__

#if HANDMADE_PROFILE
global_variable debug_table GlobalDebugTable_;
debug_table* GlobalDebugTable = &GlobalDebugTable_;
#else
debug_table* GlobalDebugTable = 0;
#endif
internal debug_thread* GetDebugThread( debug_state* DebugState, u32 ThreadID)
{
  debug_thread* Result = 0;
  for(debug_thread* Thread = DebugState->FirstThread;
      Thread;
      Thread = Thread->Next)
  {
    if(Thread->ID == ThreadID)
    {
      Result = Thread;
      break;
    }
  }

  if(!Result)
  {
    Result = PushStruct(&DebugState->Arena, debug_thread );
    Result->ID = ThreadID;
    Result->LaneIndex = DebugState->FrameBarLaneCount++;
    Result->FirstOpenBlock = 0;
    Result->Next = DebugState->FirstThread;
    DebugState->FirstThread = Result;
  }

  return Result;
}

internal debug_frame_region*
AddRegion(debug_frame* CurrentFrame)
{
  debug_frame_region* Result = CurrentFrame->Regions + CurrentFrame->RegionCount;
  if(CurrentFrame->RegionCount < MAX_REGIONS_PER_FRAME-1)
  {
    CurrentFrame->RegionCount++;
  }
  return Result;
}

internal debug_record*
GetRecordFrom(open_debug_block* OpenBlock)
{
  debug_record* Result = OpenBlock ? OpenBlock->Record : 0;
  return Result;
}

void CollateDebugRecords()
{
  debug_state* DebugState = DEBUGGetState();
  for(;;++DebugState->CollationArrayIndex)
  {
    if( DebugState->CollationArrayIndex  == MAX_DEBUG_EVENT_ARRAY_COUNT)
    {
       DebugState->CollationArrayIndex = 0;
    }

    u32 EventArrayIndex = DebugState->CollationArrayIndex;
    if( EventArrayIndex == GlobalDebugTable->CurrentEventArrayIndex)
    {
      break;
    }

    debug_frame* CurrentFrame = DebugState->CollationFrame;

    for(u32 EventIndex = 0;
            EventIndex < GlobalDebugTable->EventCount[EventArrayIndex];
            ++EventIndex)
    {
      debug_event*  Event = GlobalDebugTable->Events[EventArrayIndex] + EventIndex;
      debug_record* Source = (GlobalDebugTable->Records[Event->TranslationUnit] + Event->DebugRecordIndex);

      if(Event->Type == DebugEvent_FrameMarker)
      {
        if(CurrentFrame)
        {
          CurrentFrame->EndClock = Event->Clock;
          CurrentFrame->WallSecondsElapsed = Event->SecondsElapsed;

          r32 ClockRange = (r32)(CurrentFrame->EndClock - CurrentFrame->BeginClock);
          if(ClockRange > 0.0f)
          {
            r32 FrameBarRange = ClockRange;
            if(DebugState->FrameBarRange < FrameBarRange)
            {
              DebugState->FrameBarRange = FrameBarRange;
            }
          }
        }

        DebugState->CollationFrame = DebugState->Frames + DebugState->FrameCount++;
        CurrentFrame = DebugState->CollationFrame;
        CurrentFrame->BeginClock = Event->Clock;
        CurrentFrame->EndClock = 0;
        CurrentFrame->RegionCount = 0;
        CurrentFrame->WallSecondsElapsed = 0;
        CurrentFrame->Regions = PushArray(&DebugState->Arena, MAX_REGIONS_PER_FRAME, debug_frame_region);

      }else if(CurrentFrame){
        u32 FrameIndex = DebugState->FrameCount - 1;
        debug_thread* Thread = GetDebugThread(DebugState, Event->TC.ThreadID);
        u64 RelativeClock = Event->Clock - CurrentFrame->BeginClock;

        if(Event->Type == DebugEvent_BeginBlock)
        {
          open_debug_block* DebugBlock = DebugState->FirstFreeBlock;
          if(DebugBlock)
          {
            DebugState->FirstFreeBlock = DebugBlock->NextFree;
          }else{
            DebugBlock = PushStruct(&DebugState->Arena, open_debug_block);
          }

          DebugBlock->StartingFrameIndex = FrameIndex;
          DebugBlock->OpeningEvent = *Event;
          DebugBlock->Record = Source;

          DebugBlock->Parent = Thread->FirstOpenBlock;
          Thread->FirstOpenBlock = DebugBlock;
          DebugBlock->NextFree = 0;
        }else if( Event->Type == DebugEvent_EndBlock){
          if(Thread->FirstOpenBlock)
          {
            Assert(CurrentFrame->Regions);
            open_debug_block* MatchingBlock = Thread->FirstOpenBlock;
            debug_event* OpeningEvent = &MatchingBlock->OpeningEvent;
            if((OpeningEvent->TC.ThreadID         == Event->TC.ThreadID) &&
               (OpeningEvent->DebugRecordIndex == Event->DebugRecordIndex) &&
               (OpeningEvent->TranslationUnit  == Event->TranslationUnit))
            {
              if(MatchingBlock->StartingFrameIndex == FrameIndex)
              {
                if(GetRecordFrom(MatchingBlock->Parent) == DebugState->ScopeToRecord)
                {
                  r32 MinT = (r32)(OpeningEvent->Clock - CurrentFrame->BeginClock);
                  r32 MaxT = (r32)(Event->Clock -  CurrentFrame->BeginClock);
                  r32 ThresholdT = 2000;
                  if((MaxT-MinT) > ThresholdT )
                  {
                    debug_frame_region* Region = AddRegion(CurrentFrame);
                    Region->LaneIndex = Thread->LaneIndex;
                    Region->MinT = (r32)(OpeningEvent->Clock - CurrentFrame->BeginClock);
                    Region->MaxT = (r32)(Event->Clock -  CurrentFrame->BeginClock);
                    Region->Record = Source;
                    Region->ColorIndex = (u16)OpeningEvent->DebugRecordIndex;
                  }
                }
              }else{
                // Record All frames in between and begin/end spans
              }

              Thread->FirstOpenBlock->NextFree = DebugState->FirstFreeBlock;
              DebugState->FirstFreeBlock = Thread->FirstOpenBlock;
              Thread->FirstOpenBlock = MatchingBlock->Parent;
            }else{
              // Record span that goes to the beginning of the frame
            }
          }
        }else{
          Assert(!"Invalid event type");
        }
      }
    }
  }
}
internal void
RefreshCollation()
{
  RestartCollation();
  CollateDebugRecords();
}

internal inline void
DebugRewriteConfigFile()
{
  debug_state* DebugState = DEBUGGetState();

  c8 Buffer[4096] = {};
  u32 Size = _snprintf_s(Buffer, sizeof(Buffer),
"#define MULTI_THREADED %d // b32\n\
#define SHOW_COLLISION_POINTS %d // b32\n\
#define SHOW_COLLIDER %d // b32\n\
#define SHOW_AABB_TREE %d // b32",
    DebugState->ConfigMultiThreaded,
    DebugState->ConfigCollisionPoints,
    DebugState->ConfigCollider,
    DebugState->ConfigAABBTree);
  thread_context Dummy = {};

  Platform.DEBUGPlatformWriteEntireFile(&Dummy, "W:\\handmade\\code\\debug_config.h", Size, Buffer);

  //DebugState->UpdateConfig = false;
  DebugState->Compiler = Platform.DEBUGExecuteSystemCommand("W:\\handmade\\code", "C:\\windows\\system32\\cmd.exe", "/C build_game.bat");
  DebugState->Compiling = true;

}


extern "C" DEBUG_GAME_FRAME_END(DEBUGGameFrameEnd)
{
  if(!GlobalDebugTable) return 0;
  GlobalDebugTable->RecordCount[0] = DebugRecords_Main_Count;

  ++GlobalDebugTable->CurrentEventArrayIndex;
  if(GlobalDebugTable->CurrentEventArrayIndex >= ArrayCount(GlobalDebugTable->Events))
  {
    GlobalDebugTable->CurrentEventArrayIndex=0;
  }
  u64 ArrayIndex_EventIndex = AtomicExchangeu64(&GlobalDebugTable->EventArrayIndex_EventIndex,
                                               ((u64)GlobalDebugTable->CurrentEventArrayIndex << 32));

  u32 EventArrayIndex = (ArrayIndex_EventIndex >> 32);
  u32 EventCount = (ArrayIndex_EventIndex & 0xFFFFFFFF);
  GlobalDebugTable->EventCount[EventArrayIndex] = EventCount;

  debug_state* DebugState = DEBUGGetState();
  if(DebugState)
  {
    if(!DebugState->Paused)
    {
      if(DebugState->FrameCount >= 4*MAX_DEBUG_EVENT_ARRAY_COUNT)
      {
        RestartCollation();
      }
      CollateDebugRecords();
    }
  }
  return GlobalDebugTable;
}

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


u32 IsMouseInRegion( const radial_menu_region* Region, r32 MouseAngle, r32 MouseRadius )
{
  b32 MouseInRadialRegion = (MouseRadius >= Region->RadiusStart);
  b32 IsInAngularRegion = IsInRegion(Region->AngleStart, Region->AngleEnd, MouseAngle);
  b32 Result = MouseInRadialRegion && IsInAngularRegion;

  return Result;
}



internal void
BeginRadialMenu(radial_menu* RadialMenu, v2 MouseButtonPos)
{
  r32 Radius = 1/8.f;
  r32 RadiusBegin = 0.5f*Radius;

  r32 AngleCenter = Tau32/4.f;
  r32 AngleHalfSlice  = Pi32/(r32)RadialMenu->MenuRegionCount;
  for (u32 RegionIndex = 0; RegionIndex < RadialMenu->MenuRegionCount; ++RegionIndex)
  {
    RadialMenu->Regions[RegionIndex] =
      RadialMenuRegion(AngleCenter - AngleHalfSlice,
                       AngleCenter + AngleHalfSlice,
                       RadiusBegin, Radius);
      AngleCenter+=2*AngleHalfSlice;
  }

  RadialMenu->MenuX = MouseButtonPos.X;
  RadialMenu->MenuY = MouseButtonPos.Y;
}

internal void
EndRadialMenu(radial_menu* RadialMenu)
{
  // Reset all-non persistant event states
  for(u32 MenuItemIndex = 0; MenuItemIndex < RadialMenu->MenuRegionCount; ++MenuItemIndex)
  {
    menu_item* MenuItem = &RadialMenu->MenuItems[MenuItemIndex];
    MenuItem->MouseOverState = {};
    MenuItem->MenuActivationState  = {};
  }

  RadialMenu->MouseRadius = 0;
  RadialMenu->MouseAngle  = 0;
  RadialMenu->MenuX = 0;
  RadialMenu->MenuY = 0;

}
void SetMenuInput(game_input* GameInput, debug_state* DebugState, radial_menu* RadialMenu)
{
  b32 RightButtonReleased = GameInput->MouseButton[PlatformMouseButton_Right].Released;
  v2 MouseButtonPos = V2(GameInput->MouseX, GameInput->MouseY);

  v2  MousePosMenuSpace = MouseButtonPos - V2(RadialMenu->MenuX, RadialMenu->MenuY);
  RadialMenu->MouseRadius = Norm(MousePosMenuSpace);
  RadialMenu->MouseAngle  = ATan2(MousePosMenuSpace.Y, MousePosMenuSpace.X);
  if( RadialMenu->MouseAngle < 0 ) RadialMenu->MouseAngle+=Tau32;

  // Update Menu Input State (Take the input)
  for(u32 RegionIndex = 0; RegionIndex < RadialMenu->MenuRegionCount; ++RegionIndex)
  {
    radial_menu_region* Region = &RadialMenu->Regions[RegionIndex];
    menu_item* MenuItem = &RadialMenu->MenuItems[RegionIndex];
#if 0
    DEBUGDrawRadialRegion(
      DebugState->RadialMenu->MenuX, DebugState->RadialMenu->MenuY,
      GameInput->MouseX, GameInput->MouseY,
      Region->AngleStart, Region->AngleEnd, Region->RadiusStart, Region->Radius );
#endif

    b32 MouseInRegion = IsMouseInRegion( Region, RadialMenu->MouseAngle, RadialMenu->MouseRadius );

    Update( &MenuItem->MouseOverState, MouseInRegion );
    Update( &MenuItem->MenuActivationState, MouseInRegion && RightButtonReleased );

#if 0
    if(MenuItem->MouseOverState.Edge)
    {
      if(MenuItem->MouseOverState.Active){
        Platform.DEBUGPrint("Mouse Entered: %d %d\n", RegionIndex, MouseInRegion );
      }else{
        Platform.DEBUGPrint("Mouse Left: %d %d\n", RegionIndex, MouseInRegion );
      }
    }

    if(MenuItem->MenuActivationState.Edge)
    {
      if(MenuItem->MenuActivationState.Active){
        Platform.DEBUGPrint("Menu Active: %d\n", RegionIndex );
      }else{
        Platform.DEBUGPrint("Menu Inactive: %d\n", RegionIndex );
      }
    }
#endif
  }
}

void ActOnInput(debug_state* DebugState, radial_menu* RadialMenu )
{
  // Update Menu Internal State (Act on the input)
  b32 MenuActivated = false;
  for (u32 MenuItemIndex = 0; MenuItemIndex < RadialMenu->MenuRegionCount; ++MenuItemIndex)
  {
    menu_item* MenuItem = &RadialMenu->MenuItems[MenuItemIndex];

    if(MenuItem->MouseOverState.Active)
    {
      if((MenuItem->MenuActivationState.Active) &&
         (MenuItem->MenuActivationState.Edge))
      {
        // Todo: Make callbacks use DebugGetState() ?
        MenuItem->Activate(DebugState, MenuItem);
      }
    }
  }
}

void DrawMenu( radial_menu* RadialMenu )
{
  v4 IdleColor              = V4(0  ,0  ,0.4,1.f);
  v4 IdleHighlightedColor   = V4(0.2,0.2,0.5,1.f);
  v4 ActiveColor            = V4(0  ,0.4,  0,1.f);
  v4 ActiveHighlightedColor = V4(0.2,0.5,0.2,1.f);

  if(!RadialMenu)
  {
    return;
  }


  for(u32 ItemIndex = 0; ItemIndex < RadialMenu->MenuRegionCount; ++ItemIndex)
  {
    radial_menu_region* Region = &RadialMenu->Regions[ItemIndex];
    menu_item* MenuItem = &RadialMenu->MenuItems[ItemIndex];


    r32 AngleCenter = RecanonicalizeAngle(Region->AngleStart + 0.5f * GetDeltaAngle(Region->AngleStart, Region->AngleEnd));

    v2 ItemLine   = V2(RadialMenu->MenuX + Region->Radius * Cos(AngleCenter),
                     RadialMenu->MenuY + Region->Radius * Sin(AngleCenter));
    DEBUGDrawDottedLine(V2(RadialMenu->MenuX, RadialMenu->MenuY) , ItemLine,  V4(0,0.7,0,1));


    v4 Color = IdleColor;

    if(MenuItem->MouseOverState.Active)
    {
      if(MenuItem->Active)
      {
        Color = ActiveHighlightedColor;
      }else{
        Color = IdleHighlightedColor;
      }
    }else{
      if(MenuItem->Active)
      {
        Color = ActiveColor;
      }else{
        Color = IdleColor;
      }
    }

    r32 ItemAngle = AngleCenter;

    r32 TextPosX = RadialMenu->MenuX + Region->Radius*Cos(AngleCenter);
    r32 TextPosY = RadialMenu->MenuY + Region->Radius*Sin(AngleCenter);

    rect2f TextBox = DEBUGTextSize(TextPosX, TextPosY, MenuItem->Header);

    r32 Anglef0 = Tau32/8.f;
    r32 Anglef1 = 3*Tau32/8.f;
    r32 Anglef2 = 5*Tau32/8.f;
    r32 Anglef3 = 7*Tau32/8.f;

    v2 BottomLeft    = V2(0,0);
    v2 BottomRight   = -1.0f * V2(TextBox.W, 0);
    v2 TopRight      = -1.0f * V2(TextBox.W, TextBox.H);
    v2 TopLeft       = -1.0f * V2(0, TextBox.H);

    v2 Start,End;
    r32 StartAngle, StopAngle;
    if(IsInRegion(Anglef0, Anglef1,ItemAngle))
    {
      Start = BottomLeft;
      End = BottomRight;
      StartAngle = Anglef0;
      StopAngle = Anglef1;
    }else if(IsInRegion(Anglef1, Anglef2,ItemAngle)){
      Start = BottomRight;
      End = TopRight;
      StartAngle = Anglef1;
      StopAngle = Anglef2;
    }else if(IsInRegion(Anglef2, Anglef3,ItemAngle)){
      Start = TopRight;
      End = TopLeft;
      StartAngle = Anglef2;
      StopAngle = Anglef3;
    }else{
      Start = TopLeft;
      End = BottomLeft;
      StartAngle = Anglef3;
      StopAngle = Anglef0;
    }


    r32 AngleParameter = GetParametarizedAngle(StartAngle, StopAngle, ItemAngle);

    //TextBox
    r32 X0 = TextPosX - TextBox.W*0.5f;
    r32 Y0 = TextPosY - TextBox.H*0.5f;

    v2 Offset = Start + AngleParameter * (End - Start);

    rect2f QuadRect = Rect2f(TextBox.X + Offset.X,
                             TextBox.Y + Offset.Y,
                             TextBox.W,TextBox.H);
    DEBUGPushQuad(QuadRect, Color);

    DEBUGTextOutAt(TextPosX+Offset.X, TextPosY+Offset.Y, MenuItem->Header, V4(1,1,1,1));
  }

  v2 MouseLine  = V2(RadialMenu->MenuX + RadialMenu->MouseRadius * Cos(RadialMenu->MouseAngle),
                     RadialMenu->MenuY + RadialMenu->MouseRadius * Sin(RadialMenu->MouseAngle));
  DEBUGDrawDottedLine(V2(RadialMenu->MenuX, RadialMenu->MenuY) , MouseLine,  V4(0.7,0,0,1));
}




void MoveMenuToTop(menu_interface* Interface, u32 WindowIndex)
{
  Assert(WindowIndex < Interface->RootContainerCount)
  menu_tree Menu = Interface->RootContainers[WindowIndex];
  while(WindowIndex > 0)
  {
    Interface->RootContainers[WindowIndex] = Interface->RootContainers[WindowIndex-1];
    WindowIndex--;
  }
  Interface->RootContainers[0] = Menu;
}

void DebugMainWindow(game_input* GameInput)
{
  game_window_size WindowSize = GameGetWindowSize();
  r32 Width  = WindowSize.WidthPx;
  r32 Height = WindowSize.HeightPx;

  r32 AspectRatio = Width/Height;
  r32 ScreenWidth = AspectRatio;
  r32 ScreenHeight = 1;

  b32 RightButtonPushed = GameInput->MouseButton[PlatformMouseButton_Right].Pushed;
  b32 RightButtonReleased = GameInput->MouseButton[PlatformMouseButton_Right].Released;

  debug_state* DebugState = DEBUGGetState();

  v2 MouseButtonPos = V2(GameInput->MouseX, GameInput->MouseY);

  if(RightButtonPushed )
  {
    DebugState->ActiveMenu.Type = menu_type::Radial;
    DebugState->ActiveMenu.RadialMenu = &DebugState->RadialMenues[0];
    BeginRadialMenu(DebugState->ActiveMenu.RadialMenu, MouseButtonPos);
  }

  switch(DebugState->ActiveMenu.Type)
  {
    case menu_type::Radial:
    {
      Assert(DebugState->ActiveMenu.RadialMenu);
      radial_menu* RadialMenu = DebugState->ActiveMenu.RadialMenu;

      SetMenuInput(GameInput, DebugState, RadialMenu);

      ActOnInput(DebugState, RadialMenu );

    }break;
    case menu_type::Box:
    {

    }break;

  }

  switch(DebugState->ActiveMenu.Type)
  {
    case menu_type::Radial:
    {
      Assert(DebugState->ActiveMenu.RadialMenu);
      radial_menu* RadialMenu = DebugState->ActiveMenu.RadialMenu;

      DrawMenu(RadialMenu);

    }break;
    case menu_type::Box:
    {
      DEBUGPushQuad(Rect2f(0.1,0.1,0.3,0.3), V4(0.3,0.3,0.3,0.5));
    }break;
  }


  if(RightButtonReleased &&
    (DebugState->ActiveMenu.Type == menu_type::Radial))
  {
    Assert(DebugState->ActiveMenu.RadialMenu);
    EndRadialMenu(DebugState->ActiveMenu.RadialMenu);
    DebugState->ActiveMenu = {};
  }


  menu_interface* Interface = DebugState->MenuInterface;

  SetMouseInput(GameInput, Interface);

  // Hot Window is at index 0; Here we sort the windows such that the klicked window
  // Has index 0 and we push all the other windows up one.
  // [w1]            | Becomes -> | [w3] |
  // [w2]            |            | [w1] |
  // [w3] <- Clicked |            | [w2] |
  // [w4]            |            | [w4] |

  // Find the clicked window and set HotWindow
  if(Interface->MouseLeftButton.Active &&
     Interface->MouseLeftButton.Edge)
  {
    u32 WindowIndex = 0;
    u32 HotWindowIndex = 0;
    b32 MenuClicked = false;
    while(true)
    {
      menu_tree MenuTree = Interface->RootContainers[WindowIndex];
      if(!MenuTree.Root)
      {
        break;
      }
      if(Intersects(MenuTree.Root->Region, Interface->MousePos))
      {
        HotWindowIndex = WindowIndex;
        MenuClicked = true;
        break;
      }
      ++WindowIndex;
    }

    if(MenuClicked)
    {
      MoveMenuToTop(Interface, HotWindowIndex);
    }
  }

  ActOnInput(&DebugState->Arena, Interface, &Interface->RootContainers[0]);
  for (s32 WindowIndex = Interface->RootContainerCount-1;
           WindowIndex >= 0;
         --WindowIndex)
  {
    menu_tree* MenuTree = &Interface->RootContainers[WindowIndex];
    TreeSensus(MenuTree);
    UpdateRegions( &DebugState->Arena, MenuTree->NodeCount, MenuTree->Root);
    DrawMenu( &DebugState->Arena, Interface, MenuTree->NodeCount, MenuTree->Root);  
  }
}

inline internal debug_frame*
GetActiveDebugFrame(debug_state* DebugState)
{
  debug_frame* Result = DebugState->Frames + DebugState->FrameCount-2;
  return Result;
}


void PushDebugOverlay(game_input* GameInput)
{
  TIMED_FUNCTION();
  
  // TODO: We don't want to remember to always update function pointers.
  //       We also don't want to do it each frame.
  //       Let's create a sort of FunctionPointerPool that gets updated automatically.
  //       
  //       The question is: Do other request a function from the pool 
  //       CallFromPool(foo->FunctionID, (void*) foo->Args )  <-- Can be very hard to read
  //       or do we somehow register a function pointer to the pool at start.
  //       RegisterFunction(foo->Function, FunctionID);
  //       CallFunction(foo->Function);


  debug_state* DebugState = DEBUGGetState();
  UpdateFunctionPointers(DebugState);

  ResetRenderGroup(GlobalDebugRenderGroup);

  game_window_size WindowSize = GameGetWindowSize();
  r32 AspectRatio = WindowSize.WidthPx/WindowSize.HeightPx;
  m4 ScreenToCubeScale =  M4( 2/AspectRatio, 0, 0, 0,
                                           0, 2, 0, 0,
                                           0, 0, 0, 0,
                                           0, 0, 0, 1);
  m4 ScreenToCubeTrans =  M4( 1, 0, 0, -1,
                              0, 1, 0, -1,
                              0, 0, 1,  0,
                              0, 0, 0,  1);

  GlobalDebugRenderGroup->ProjectionMatrix = ScreenToCubeTrans*ScreenToCubeScale;

  DebugMainWindow(GameInput);

  r32 LineNumber = 0;
  if(DebugState->Compiling)
  {
    debug_process_state ProcessState = Platform.DEBUGGetProcessState(DebugState->Compiler);
    DebugState->Compiling = ProcessState.IsRunning;
    if(DebugState->Compiling)
    {
      DEBUGAddTextSTB("Compiling", LineNumber++);
    }
  }

  if(DebugState->Frames)
  {
    c8 StringBuffer[256] = {};
     debug_frame* Frame = GetActiveDebugFrame(DebugState);
    Platform.DEBUGFormatString(StringBuffer, sizeof(StringBuffer), sizeof(StringBuffer)-1,
  "%3.1f Hz, %4.2f ms", 1.f/Frame->WallSecondsElapsed, Frame->WallSecondsElapsed*1000);
    DEBUGAddTextSTB(StringBuffer, LineNumber++);
  }

  if(!DebugState->ChartVisible) return;
  if(!DebugState->MenuInterface->RootContainers[0].Root) return;

  rect2f Chart = DebugState->MenuInterface->RootContainers[0].Root->Functions.GetRegionRect(window_regions::WholeBody, DebugState->MenuInterface->RootContainers[0].Root);

  u32 MaxFramesToDisplay = DebugState->FrameCount < 10 ? DebugState->FrameCount : 10;
  r32 BarWidth = Chart.H/MaxFramesToDisplay;
  r32 LaneWidth = BarWidth/(r32)DebugState->FrameBarLaneCount;
  r32 LaneScale = Chart.W/(r32)DebugState->FrameBarRange;

  v4 ColorTable[] = {V4(1,0,0,1),
                     V4(0,1,0,1),
                     V4(0,0,1,1),
                     V4(1,1,0,1),
                     V4(1,0,1,1),
                     V4(0,1,1,1),
                     V4(1,1,1,1),
                     V4(0,0,0,1)};

  debug_record* HotRecord = 0;

#if 0
  for(u32 FrameIndex = 0; FrameIndex < MaxFramesToDisplay; ++FrameIndex)
  {
    debug_frame* Frame = DebugState->Frames + DebugState->FrameCount - (FrameIndex+1);
    r32 StackX = Chart.X;
    r32 StackY = Chart.Y+Chart.H - (r32)(FrameIndex+1)*BarWidth;
    for(u32 RegionIndex = 0; RegionIndex < Frame->RegionCount; ++RegionIndex)
    {
      debug_frame_region* Region = Frame->Regions + RegionIndex;
      v4 Color = ColorTable[(u32)(Region->ColorIndex%ArrayCount(ColorTable))];
      r32 MinX = StackX + LaneScale*Region->MinT;
      r32 MaxX = StackX + LaneScale*Region->MaxT;
      r32 MinY = StackY + LaneWidth*Region->LaneIndex;
      r32 MaxY = MinY + LaneWidth;
      rect2f Rect = {};
      Rect.X = MinX;
      Rect.Y = MinY;
      Rect.W = MaxX-MinX;
      Rect.H = (MaxY-MinY)*0.9f;

      DEBUGPushQuad(Rect, Color);

      if((GameInput->MouseX >= Rect.X) && (GameInput->MouseX <= Rect.X+Rect.W) &&
         (GameInput->MouseY >= Rect.Y) && (GameInput->MouseY <= Rect.Y+Rect.H))
      {
        c8 StringBuffer[256] = {};
        Platform.DEBUGFormatString( StringBuffer, sizeof(StringBuffer), sizeof(StringBuffer)-1,
        "%s : %2.2f MCy", Region->Record->BlockName, (Region->MaxT-Region->MinT)/1000000.f);
        DEBUGTextOutAt(GameInput->MouseX, GameInput->MouseY+0.02f, StringBuffer);
        if(GameInput->MouseButton[PlatformMouseButton_Left].Pushed)
        {
          HotRecord = Region->Record;
        }
      }
    }
  }
  if(GameInput->MouseButton[PlatformMouseButton_Left].Pushed)
  {
    if((GameInput->MouseX >= 0) && (GameInput->MouseX <= AspectRatio) &&
       (GameInput->MouseY >= 0) && (GameInput->MouseY <= 1))
    {
      if(HotRecord)
      {
        DebugState->ScopeToRecord = HotRecord;
      }else if(DebugState->ScopeToRecord){
        DebugState->ScopeToRecord = 0;
      }
      RefreshCollation();
    }
  }

  #endif
}


void DrawFunctionCount(){
  //Assert(DebugState->SnapShotIndex < SNAPSHOT_COUNT);

#if 0
  v4 Yellow = V4(1,1,0,1);
  v4 Green  = V4(0,1,0,1);
  debug_state* DebugState = DEBUGGetState();
  u32 TotalCounterStateCount = ArrayCount(DebugState->CounterStates);
  for(u32 i = 0; i<TotalCounterStateCount; ++i)
  {
    debug_counter_state* CounterState = &DebugState->CounterStates[i];
    if(!CounterState->FileName) continue;

    stb_font_map* FontMap = &GlobalDebugRenderGroup->Assets->STBFontMap;

    r32 ChartLeft = 1/4.f;
    r32 ChartRight = 4/8.f;
    r32 BarSegmentWidth = (ChartRight - ChartLeft)/SNAPSHOT_COUNT;

    r32 BaselinePixels = GlobalDebugRenderGroup->ScreenHeight - (Line+1) * FontMap->FontHeightPx;
    r32 ChartBot = Ky*BaselinePixels;
    r32 ChartTop = ChartBot + Ky*FontMap->Ascent;

    debug_frame_snapshot* SnapShotStat = &CounterState->Snapshots[DebugState->SnapShotIndex];
    debug_statistics* HitCount   = &SnapShotStat->HitCountStat;
    debug_statistics* CycleCount = &SnapShotStat->CycleCountStat;
    BeginDebugStatistics(HitCount);
    BeginDebugStatistics(CycleCount);
    for(u32 j = 0; j<SNAPSHOT_COUNT; ++j)
    {
      debug_frame_snapshot* SnapShot = &CounterState->Snapshots[j];
      AccumulateStatistic(HitCount, (r32) SnapShot->HitCount);
      AccumulateStatistic(CycleCount, (r32) SnapShot->CycleCount);
    }
    EndDebugStatistics(HitCount);
    EndDebugStatistics(CycleCount);

    r32 xMin = ChartLeft;
    for(u32 j = 0; j<SNAPSHOT_COUNT; ++j)
    {
      debug_frame_snapshot* SnapShot = &CounterState->Snapshots[j];
      r32 xMax = xMin + BarSegmentWidth;

      if(HitCount->Avg)
      {
        r32 BarScale = (ChartTop - ChartBot)/(2.f*SnapShot->CycleCountStat.Avg);
        r32 yMax = ChartBot + BarScale*SnapShot->CycleCount;
        aabb2f Rect = AABB2f( V2(xMin,ChartBot), V2(xMax,yMax));
        v4 Color = Green + ((SnapShot->CycleCountStat.Avg) / (SnapShot->CycleCountStat.Max) ) * (Yellow - Green);
        DEBUGPushQuad(GlobalDebugRenderGroup, Rect,Color);
      }

      xMin = ChartLeft + j * BarSegmentWidth;
    }

    s32 CyPerHit = (HitCount->Avg == 0) ? 0 : (s32) (CycleCount->Avg / HitCount->Avg);
    c8 StringBuffer[256] = {};
    _snprintf_s( StringBuffer, sizeof(StringBuffer), sizeof(StringBuffer)-1,
  "(%5d)%-25s:%10dCy:%5dh:%10dcy/h",
      CounterState->LineNumber, CounterState->FunctionName, (u32) CycleCount->Avg, (u32) HitCount->Avg,
      (u32) CyPerHit);

    DEBUGAddTextSTB(GlobalDebugRenderGroup, StringBuffer, CornerPaddingPx, Line);
    Line++;
  }
#endif
}

void SetMouseInput(game_input* GameInput, menu_interface* Interface)
{
  v2 MousePos = V2(GameInput->MouseX, GameInput->MouseY);

  Update(&Interface->MouseLeftButton, GameInput->MouseButton[PlatformMouseButton_Left].EndedDown);
  if( Interface->MouseLeftButton.Edge )
  {
    if(Interface->MouseLeftButton.Active )
    {
      Interface->MouseLeftButtonPush = MousePos;
    }else{
      Interface->MouseLeftButtonRelese = MousePos;
    }
  }
  Interface->MousePos = MousePos;
}

void ActOnInput(memory_arena* Arena, menu_interface* Interface, menu_tree* Menu)
{
  v2 MousePos = Interface->MousePos;

  Assert(Menu->Root);
  node_region_pair NodeRegion = GetRegion(Arena, Menu->NodeCount, Menu->Root, MousePos);

  if(Interface->MouseLeftButton.Active)
  {
    if(Interface->HotSubWindow)
    {
      // Mouse Clicked Event
      if(Interface->MouseLeftButton.Edge)
      {
        if(Interface->HotSubWindow)
        {
          Interface->HotSubWindow->Functions.MouseDown(Interface, Interface->HotSubWindow, Interface->HotRegion, 0);
        }
      // Mouse Down Movement State
      }else{

      }

      Interface->HotSubWindow->Functions.HandleInput(Interface, Interface->HotSubWindow, 0);
    }
  }else{

    if(NodeRegion.Node != Interface->HotSubWindow)
    {
      if(NodeRegion.Node)
      {
        NodeRegion.Node->Functions.MouseEnter(Interface, NodeRegion.Node, NodeRegion.Region, 0);  
      }

      if(Interface->HotSubWindow)
      {
        Interface->HotSubWindow->Functions.MouseExit(Interface, Interface->HotSubWindow, Interface->HotRegion, 0);
      }
    }

    Interface->HotRegion = NodeRegion.Region;
    Interface->HotSubWindow = NodeRegion.Node;

    if(Interface->HotSubWindow)
    {
      Interface->HotSubWindow->Functions.MouseUp(Interface, Interface->HotSubWindow, Interface->HotRegion, 0);
    }

    // Mouse Released Event
    if(Interface->MouseLeftButton.Edge)
    {
      Interface->HotRegion = window_regions::None;
      Interface->HotSubWindow = 0;
      // Mouse Exploration State
    }else{
      
    }
  }
}

#include "menu_functions.h"
