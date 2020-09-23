#include "menu_interface.h"

u32 GetContainerPayloadSize(container_type Type)
{
  switch(Type)
  {
    case container_type::None:
    case container_type::Split:
    case container_type::Root:    return 0;
    case container_type::Color:   return sizeof(empty_leaf);
    case container_type::Border:  return sizeof(border_leaf);
    case container_type::HBF:     return sizeof(hbf_node);
    default: INVALID_CODE_PATH;
  }
  return 0;
}

u32 GetAttributeSize(container_attribute Attributes)
{
  switch(Attributes)
  {
    case ATTRIBUTE_DRAG:  return sizeof(draggable_attribute);
    case ATTRIBUTE_MERGE: return sizeof(mergable_attribute);
    case ATTRIBUTE_MERGE_SLOT: return 0;
    default: INVALID_CODE_PATH;
  }
  return 0; 
}

u32 GetAttributeSize(u32 Attributes)
{
  return GetAttributeSize((container_attribute)Attributes);
}

inline b32 HasAttribute(container_node* Node, container_attribute Attri)
{
  return Node->Attributes & ((u32) Attri);
}

u32 GetAttributeBatchSize(container_attribute Attri)
{
  u32 Result = 0;
  u32 Attributes = (u32) Attri;
  while(Attributes)
  {
    bit_scan_result ScanResult = FindLeastSignificantSetBit(Attributes);
    Assert(ScanResult.Found);

    u32 Attribute = (1 << ScanResult.Index);
    Result += GetAttributeSize(Attribute);
    Attributes -= Attribute;
  }
  
  Result += (Attributes & ATTRIBUTE_DRAG)  * sizeof(draggable_attribute);
  Result += (Attributes & ATTRIBUTE_MERGE) * sizeof(mergable_attribute);
  Result += (Attributes & ATTRIBUTE_MERGE_SLOT) * 0;
  return Result;
}

u8* GetAttributePointer(container_node* Node, container_attribute Attri)
{
  u8* Result = 0;
  u32 NodeAttributes = (u32) Node->Attributes;
  u32 Attribute = (u32) Attri; 
  u32 AttributeOffset = 0;
  if(NodeAttributes & Attribute)
  {
    bit_scan_result ScanResult = FindLeastSignificantSetBit(NodeAttributes);
    Assert(ScanResult.Found);
    u32 NodeAttribute = (1 << ScanResult.Index);
    while(Attribute != NodeAttribute)
    {
      AttributeOffset += GetAttributeSize(NodeAttribute);
      NodeAttributes -= NodeAttribute;
      ScanResult = FindLeastSignificantSetBit(NodeAttributes);
      Assert(ScanResult.Found);
      NodeAttribute = (1 << ScanResult.Index);
    }
    
  }
  //                     offset to first attribute    Offset to actual attribute
  Result = ((u8*)Node) + Node->AttributeBaseOffset  + AttributeOffset;
  return Result;
}

container_node* NewContainer(menu_interface* Interface, container_type Type,  u32 Attributes)
{
  u32 BaseNodeSize    = sizeof(container_node);
  u32 NodePayloadSize = GetContainerPayloadSize(Type);
  u32 AttributeSize   = GetAttributeBatchSize( (container_attribute) Attributes);
  u32 ContainerSize = (BaseNodeSize + NodePayloadSize + AttributeSize);
  Interface->ActiveMemory += ContainerSize;

  #if 0
  {
    u32 RegionUsed = (u32)(Interface->Memory - Interface->MemoryBase);
    u32 TotSize = (u32) Interface->MaxMemSize;
    r32 Percentage = RegionUsed / (r32) TotSize;
    u32 ActiveMemory = Interface->ActiveMemory;
    r32 Fragmentation = ActiveMemory/(r32)RegionUsed;
    Platform.DEBUGPrint("--==<< Pre Memory >>==--\n");
    Platform.DEBUGPrint(" - Tot Mem Used   : %2.3f  (%d/%d)\n", Percentage, RegionUsed, TotSize );
    Platform.DEBUGPrint(" - Fragmentation  : %2.3f  (%d/%d)\n", Fragmentation, ActiveMemory, RegionUsed );
  }
  #endif
  
  container_node* Result = 0;

  // Get memory from the middle if we have continous space that is big enough
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
      midx Base = (midx) CurrentNode + ContainerSize;
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

    #if 0
    {
      u32 SlotCount = 0;
      container_node* CurrentNode2 = Interface->Sentinel.Next;
      container_node* NextNode2 = CurrentNode->Next;

      while( CurrentNode2->Next != &Interface->Sentinel)
      {
        SlotCount++;
        CurrentNode2 = CurrentNode2->Next;
      }
      
      Platform.DEBUGPrint("--==<< Middle Inset >>==--\n");
      Platform.DEBUGPrint(" - Slot: [%d,%d]\n", SlotSpace, SlotCount);
      Platform.DEBUGPrint(" - Size: [%d,%d]\n", ContainerSize, SlotSize);
    }
    #endif


  }

  // Otherwise push it to the end
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
  Result->Attributes = Attributes;
  Result->ContainerSize = ContainerSize;
  Result->Functions = GetMenuFunction(Type);
  Result->AttributeBaseOffset = sizeof(container_node) + NodePayloadSize;
  
  #if 0
  {
    u32 RegionUsed2 = (u32)(Interface->Memory - Interface->MemoryBase);
    u32 TotSize = (u32) Interface->MaxMemSize;
    r32 Percentage = RegionUsed2 / (r32) TotSize;
    u32 ActiveMemory = Interface->ActiveMemory;
    r32 Fragmentation = ActiveMemory/(r32)RegionUsed2;
    
    Platform.DEBUGPrint("--==<< Post Memory >>==--\n");
    Platform.DEBUGPrint(" - Tot Mem Used   : %2.3f  (%d/%d)\n", Percentage, RegionUsed, TotSize );
    Platform.DEBUGPrint(" - Fragmentation  : %2.3f  (%d/%d)\n", Fragmentation, ActiveMemory, RegionUsed );
    
  }
  #endif
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


menu_tree* NewMenuTree(menu_interface* Interface)
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

    CurrentDepth--;

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

void UpdateRegions( menu_tree* Menu )
{
  temporary_memory TempMem =  BeginTemporaryMemory(GlobalGameState->TransientArena);

  u32 StackElementSize = sizeof(container_node*);
  u32 StackByteSize = Menu->NodeCount * StackElementSize;

  u32 StackCount = 0;
  container_node** ContainerStack = PushArray(GlobalGameState->TransientArena, Menu->NodeCount, container_node*);

  // Push Root
  ContainerStack[StackCount++] = Menu->Root;

  while(StackCount>0)
  {
    // Pop new parent from Stack
    container_node* Parent = ContainerStack[--StackCount];
    ContainerStack[StackCount] = 0;

    // Update the region of all children and push them to the stack
    Parent->Functions.UpdateChildRegions(Parent);
    container_node* Child = Parent->FirstChild;
    while(Child)
    {
      ContainerStack[StackCount++] = Child;
      Child = Child->NextSibling;
    }
  }

  EndTemporaryMemory(TempMem);
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


u32 GetIntersectingNodes(memory_arena* Arena, u32 NodeCount, container_node* Container, v2 MousePos, container_node** Result)
{
  u32 StackElementSize = sizeof(container_node*);
  u32 StackByteSize = NodeCount * StackElementSize;

  u32 StackCount = 0;
  container_node** ContainerStack = PushArray(Arena, NodeCount, container_node*);

  u32 IntersectingLeafCount = 0;

  // Push Root
  ContainerStack[StackCount++] = Container;

  while(StackCount>0)
  {
    // Pop new parent from Stack
    container_node* Parent = ContainerStack[--StackCount];
    ContainerStack[StackCount] = 0;

    // Check if mouse is inside the child region and push those to the stack.
    if(Intersects(Parent->Region, MousePos))
    {
      if(Parent->FirstChild)
      {
        container_node* Child = Parent->FirstChild;
        while(Child)
        {
          if(Intersects(Child->Region, MousePos))
          {
            ContainerStack[StackCount++] = Child;
          }
          Child = Child->NextSibling;
        }  
      }else{
        Result[IntersectingLeafCount++] = Parent;
      }
    }
  }
  return IntersectingLeafCount;
}


container_node* ConnectNode(container_node* Parent, container_node* NewNode)
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

  return NewNode;
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
        Child = Child->NextSibling;
      }
    }
  }

  Node->NextSibling = 0;
  Node->Parent = 0;
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

void PrintHotLeafs(menu_interface* Interface)
{
  stb_font_map* FontMap = &GlobalGameState->AssetManager->FontMap;
  game_window_size WindowSize = GameGetWindowSize();
  r32 HeightStep = (FontMap->Ascent - FontMap->Descent)/WindowSize.HeightPx;
  r32 WidthStep  = 0.02;
  r32 YOff = 1 - 2*HeightStep;
  for (u32 ContainerIndex = 0; ContainerIndex < Interface->RootContainerCount; ++ContainerIndex)
  {
    menu_tree* Menu = &Interface->RootContainers[ContainerIndex];
    /* code */
    if(Menu->HotLeafCount>0)
    {
      container_node* Node = Menu->HotLeafs[0];

      r32 XOff = 0;

      container_node* Nodes[32] = {};
      container_node* CheckPoints[32] = {};
      u32 DepthCount = Node->Depth;
      while(Node)
      {
        Nodes[Node->Depth] = Node;
        Node = Node->Parent;
      }
      Assert(Nodes[0]->Depth == 0);

      for (u32 Depth = 0; Depth <= DepthCount; ++Depth)
      {
        container_node* Sibling = Nodes[Depth];
        if(Depth!=0)
        {
          Sibling = Nodes[Depth]->Parent->FirstChild;
        }
        while(Sibling)
        {
          v4 Color = V4(1,1,1,1);
          if(Sibling == Nodes[Depth])
          {
            Color = V4(1,1,0,1);
            CheckPoints[Depth] = Sibling->NextSibling;
          }

          for (u32 LeadIndex = 0; LeadIndex < Menu->HotLeafCount; ++LeadIndex)
          {
            if (Sibling == Menu->HotLeafs[LeadIndex])
            {
              Color = V4(1,0,0,1);
            }
          }

          c8 StringBuffer[512] = {};
          Platform.DEBUGFormatString(StringBuffer, sizeof(StringBuffer), sizeof(StringBuffer),
          "%s %d", ToString(Sibling->Type), Sibling->Attributes);

          XOff = WidthStep * Sibling->Depth;
          DEBUGTextOutAt(XOff, YOff, StringBuffer, Color);
          YOff -= HeightStep;

          if(CheckPoints[Depth])
          {
            break;
          }
          Sibling = Sibling->NextSibling;
        }
      }

      for (s32 Depth = DepthCount; Depth >= 0; --Depth)
      {
        container_node* Sibling = CheckPoints[Depth];
        while(Sibling)
        {
          v4 Color = V4(1,1,1,1);
          if(Sibling == Nodes[Depth])
          {
            Color = V4(1,1,0,1);
            CheckPoints[Depth] = Sibling;
          }

          for (u32 LeadIndex = 0; LeadIndex < Menu->HotLeafCount; ++LeadIndex)
          {
            if (Sibling == Menu->HotLeafs[LeadIndex])
            {
              Color = V4(1,0,0,1);
            }
          }

          c8 StringBuffer[512] = {};
          Platform.DEBUGFormatString(StringBuffer, sizeof(StringBuffer), sizeof(StringBuffer),
          "%s %d", ToString(Sibling->Type), Sibling->Attributes);

          XOff = WidthStep * Sibling->Depth;
          DEBUGTextOutAt(XOff, YOff, StringBuffer, Color);
          YOff -= HeightStep;

          if(CheckPoints[Depth])
          {
            break;
          }
          Sibling = Sibling->NextSibling;
        }
      }
    }
    YOff -= HeightStep;
  }
}
/*
void UpdateDraggableAttribute( menu_interface* Interface, draggable_attribute* Attr )
{
  if(Interface->MouseLeftButton.Edge)
  {
    if(Interface->MouseLeftButton.Active)
    {
      Attr->Dragging = true;
    }else{
      Attr->Dragging = false;
    }
  }else{
    if(Interface->MouseLeftButton.Active)
    {
      if(Attr->Dragging)
      {
        Attr->Update(Interface, Attr);
      }
    }
  }
}
*/
inline container_node* GetRootNode(container_node* Node)
{
  while(Node->Parent)
  {
    Node = Node->Parent; 
  }
  return Node;
}

void UpdateMergableAttribute( menu_interface* Interface, mergable_attribute* Attr )
{
  Attr->DstNode = 0;
  Attr->HotMergeZone = ArrayCount(Attr->MergeZone);
  for (u32 MenuIndex = 0; MenuIndex < Interface->RootContainerCount; ++MenuIndex)
  {
    container_node* SrcRoot = GetRootNode(Attr->SrcNode);
    
    menu_tree* DstMenu = &Interface->RootContainers[MenuIndex];
    if(SrcRoot!=DstMenu->Root)
    {
      for (u32 LeafIndex = 0; LeafIndex < DstMenu->HotLeafCount; ++LeafIndex)
      {
        container_node* Node = DstMenu->HotLeafs[LeafIndex];
        while(Node)
        {
          if(HasAttribute(Node, ATTRIBUTE_MERGE_SLOT))
          {
            Attr->DstNode = Node;
            break;
          }
          Node = Node->Parent;
        }
      }
    }
  }

  if(Attr->DstNode)
  {
    rect2f Rect = Attr->DstNode->Region;
    r32 W = Rect.W;
    r32 H = Rect.H;
    r32 S = Minimum(W,H)/4;

    v2 MP = V2(Rect.X+W/2,Rect.Y+H/2); // Middle Point
    v2 LQ = V2(MP.X-S, MP.Y);          // Left Quarter
    v2 RQ = V2(MP.X+S, MP.Y);          // Right Quarter
    v2 BQ = V2(MP.X,   MP.Y-S);        // Bot Quarter
    v2 TQ = V2(MP.X,   MP.Y+S);        // Top Quarter

    Attr->MergeZone[0] = Rect2f(LQ.X-S/2.f, LQ.Y-S/2.f,S/1.1f,S/1.1f); // Left Quarter
    Attr->MergeZone[1] = Rect2f(MP.X-S/2.f, MP.Y-S/2.f,S/1.1f,S/1.1f); // Middle Point
    Attr->MergeZone[2] = Rect2f(RQ.X-S/2.f, RQ.Y-S/2.f,S/1.1f,S/1.1f); // Right Quarter
    Attr->MergeZone[3] = Rect2f(BQ.X-S/2.f, BQ.Y-S/2.f,S/1.1f,S/1.1f); // Bot Quarter
    Attr->MergeZone[4] = Rect2f(TQ.X-S/2.f, TQ.Y-S/2.f,S/1.1f,S/1.1f); // Top Quarter

    if(Intersects(Attr->MergeZone[0], Interface->MousePos))
    {
      Attr->HotMergeZone = 0;
    }else if(Intersects(Attr->MergeZone[1], Interface->MousePos)){
      Attr->HotMergeZone = 1;
    }else if(Intersects(Attr->MergeZone[2], Interface->MousePos)){
      Attr->HotMergeZone = 2;
    }else if(Intersects(Attr->MergeZone[3], Interface->MousePos)){
      Attr->HotMergeZone = 3;
    }else if(Intersects(Attr->MergeZone[4], Interface->MousePos)){
      Attr->HotMergeZone = 4;
    }else{
      Attr->HotMergeZone = ArrayCount(Attr->MergeZone);
    }
  }

  if(Interface->MouseLeftButton.Edge && !Interface->MouseLeftButton.Active && Attr->HotMergeZone <  ArrayCount(Attr->MergeZone) )
  {
    Platform.DEBUGPrint("Merging Event!");
  } 
}

void ActOnInput(menu_interface* Interface, menu_tree* Menu)
{
  Assert(Menu->Root);
  menu_tree* ActiveMenu = &Interface->RootContainers[0];
  for (u32 i = 0; i < ActiveMenu->ActiveLeafCount; ++i)
  {
    container_node* Node =  ActiveMenu->ActiveLeafs[i];
    Node->Functions.HandleInput(Interface, Node);
    if(HasAttribute(Node, ATTRIBUTE_DRAG))
    {
      draggable_attribute* Draggable = (draggable_attribute*) GetAttributePointer(Node, ATTRIBUTE_DRAG);
      Draggable->Update(Interface, Node ,Draggable);
    }

    if(HasAttribute(Node, ATTRIBUTE_MERGE))
    {
      mergable_attribute* Mergable = (mergable_attribute*) GetAttributePointer(Node, ATTRIBUTE_MERGE);
      UpdateMergableAttribute(Interface, Mergable);
    }
  }
}

void SetMouseInput(memory_arena* Arena, game_input* GameInput, menu_interface* Interface)
{
  v2 MousePos = V2(GameInput->MouseX, GameInput->MouseY);

  Update(&Interface->MouseLeftButton, GameInput->MouseButton[PlatformMouseButton_Left].EndedDown);

  if( Interface->MouseLeftButton.Edge )
  {
    if(Interface->MouseLeftButton.Active )
    {
      Interface->MouseLeftButtonPush = MousePos;
      u32 WindowIndex = 0;
      u32 HotWindowIndex = 0;
      b32 MenuClicked = false;
      while(true)
      {
        menu_tree* Menu = &Interface->RootContainers[WindowIndex];
        if(!Menu->Root)
        {
          break;
        }
        if(Intersects(Menu->Root->Region, Interface->MousePos))
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

      menu_tree* SelectedMenu = &Interface->RootContainers[0];
      SelectedMenu->ActiveLeafCount = GetIntersectingNodes(Arena,  SelectedMenu->NodeCount, SelectedMenu->Root, Interface->MousePos, SelectedMenu->ActiveLeafs);
    }else{
      Interface->MouseLeftButtonRelese = MousePos;
      ActOnInput(Interface, &Interface->RootContainers[0]);
      Interface->RootContainers[0].ActiveLeafCount = 0;
    }
  }
  for (u32 Index = 0; Index < Interface->RootContainerCount; ++Index)
  {
    menu_tree* Menu = &Interface->RootContainers[Index];
    Menu->HotLeafCount = GetIntersectingNodes(Arena,
      Menu->NodeCount,
      Menu->Root,
      Interface->MousePos, Menu->HotLeafs);
  }
  PrintHotLeafs(Interface);

  Interface->PreviousMousePos = Interface->MousePos;
  Interface->MousePos = MousePos;
}



void UpdateAndRenderMenuInterface(game_input* GameInput, menu_interface* Interface)
{
  SetMouseInput(GlobalGameState->TransientArena, GameInput, Interface);

  ActOnInput(Interface, &Interface->RootContainers[0]);
  for (s32 WindowIndex = Interface->RootContainerCount-1;
           WindowIndex >= 0;
         --WindowIndex)
  {
    menu_tree* MenuTree = &Interface->RootContainers[WindowIndex];
    TreeSensus(MenuTree);
    UpdateRegions( MenuTree );
    DrawMenu( GlobalGameState->TransientArena, Interface, MenuTree->NodeCount, MenuTree->Root);
  }
}

container_node* PushEmptyWindow(menu_interface* Interface,  container_node* Parent, v4 Color = V4(0.2,0.4,0.2,1))
{
  container_node* Node = NewContainer(Interface, container_type::Color);
  GetContainerPayload(empty_leaf, Node)->Color = Color;
  ConnectNode(Parent, Node);
  return Node;
}

void UpdateFrameBorder( menu_interface* Interface, border_leaf* Border )
{
  if(Border->Vertical)
  {
    Border->Position += Interface->MousePos.X - Interface->PreviousMousePos.X;
  }else{
    Border->Position += Interface->MousePos.Y - Interface->PreviousMousePos.Y;
  }
}

void UpdateFrameBorder( menu_interface* Interface, container_node* Node, draggable_attribute* Attr )
{
  UpdateFrameBorder(Interface, GetContainerPayload(border_leaf, Node));
}

void UpdateSplitBorder( menu_interface* Interface, container_node* SplitNode, border_leaf* Border)
{
  if(Border->Vertical)
  {
    Border->Position += (Interface->MousePos.X - Interface->PreviousMousePos.X)/SplitNode->Region.W;
  }else{
    Border->Position += (Interface->MousePos.Y - Interface->PreviousMousePos.Y)/SplitNode->Region.H;
  }
}

void UpdateSplitBorder( menu_interface* Interface, container_node* Node, draggable_attribute* Attr )
{
  border_leaf* Border = GetContainerPayload(border_leaf, Node);
  UpdateSplitBorder(Interface, (container_node*) Attr->Data, Border);
}

void UpdateHeaderPosition( menu_interface* Interface, container_node* Node, draggable_attribute* Attr )
{
  container_node* Root = (container_node*) Attr->Data;
  container_node* Child = Root->FirstChild;
  while(Child->Type == container_type::Border)
  {
    border_leaf* Border = GetContainerPayload(border_leaf, Child);
    UpdateFrameBorder(Interface, Border);
    Child = Child->NextSibling;
  }
}

inline container_node*
CreateBorderNode(menu_interface* Interface, b32 Vertical, r32 Position, v4 Color)
{
  border_leaf Border = {};
  Border.Vertical = Vertical;
  Border.Position = Position;
  Border.Thickness = Interface->BorderSize;
  Border.Color = Color;  
  container_node* Result = NewContainer(Interface, container_type::Border, ATTRIBUTE_DRAG);
  border_leaf* BorderLeaf = GetContainerPayload(border_leaf, Result);
  *BorderLeaf = Border;
  return Result;
}

void SetFrameDragAttribute(container_node* BorderNode)
{
  border_leaf* BorderLeaf = GetContainerPayload(border_leaf, BorderNode);
  draggable_attribute* Draggable = (draggable_attribute*) GetAttributePointer(BorderNode, ATTRIBUTE_DRAG);
  Draggable->Data = 0;
  Draggable->Update = UpdateFrameBorder;
}


void SetSplitDragAttribute(container_node* SplitNode, container_node* BorderNode)
{
  border_leaf* BorderLeaf = GetContainerPayload(border_leaf, BorderNode);
  draggable_attribute* Draggable = (draggable_attribute*) GetAttributePointer(BorderNode, ATTRIBUTE_DRAG);

  Draggable->Data = (void*) SplitNode;
  Draggable->Update = UpdateSplitBorder;
}

void CreateSplitRootWindow( menu_interface* Interface, rect2f Region, b32 Vertical,
                              v4 HeaderColor1, v4 BodyColor1, v4 FooterColor1,
                              v4 HeaderColor2, v4 BodyColor2, v4 FooterColor2)
{
  // "Widget Body 1
  container_node* HBF1 = 0; // Root
  {
    container_node* Header = NewContainer(Interface, container_type::Color);
    GetContainerPayload(empty_leaf, Header)->Color = HeaderColor1;
    container_node* Body = NewContainer(Interface, container_type::Color);
    GetContainerPayload(empty_leaf, Body)->Color = BodyColor1;
    container_node* Footer = NewContainer(Interface, container_type::Color);
    GetContainerPayload(empty_leaf, Footer)->Color = FooterColor1;

    HBF1 = NewContainer(Interface, container_type::HBF);
    hbf_node* HBFP = GetContainerPayload(hbf_node, HBF1);
    HBFP->HeaderSize = 0.02;
    HBFP->FooterSize = 0.02;
    ConnectNode(HBF1, Header);
    ConnectNode(HBF1, Body);
    ConnectNode(HBF1, Footer);
  }

  // "Widget Body 2
  container_node* HBF2 = 0; // Root
  {
    container_node* Header = NewContainer(Interface, container_type::Color);
    GetContainerPayload(empty_leaf, Header)->Color = HeaderColor2;
    container_node* Body = NewContainer(Interface, container_type::Color);
    GetContainerPayload(empty_leaf, Body)->Color = BodyColor2;
    container_node* Footer = NewContainer(Interface, container_type::Color);
    GetContainerPayload(empty_leaf, Footer)->Color = FooterColor2;

    HBF2 = NewContainer(Interface, container_type::HBF);
    hbf_node* HBFP = GetContainerPayload(hbf_node, HBF2);
    HBFP->HeaderSize = 0.02;
    HBFP->FooterSize = 0.02;
    ConnectNode(HBF2, Header);
    ConnectNode(HBF2, Body);
    ConnectNode(HBF2, Footer);
  }

  container_node* SplitNode = 0; // Root
  { // Split Node Complex  1 Border, 2 None
    SplitNode = NewContainer(Interface, container_type::Split);
    container_node* BorderNode = CreateBorderNode(Interface, Vertical, 0.5);
    ConnectNode(SplitNode, BorderNode);
    SetSplitDragAttribute(SplitNode,BorderNode);

    // Connection 1
    container_node* SplitBody1 = NewContainer(Interface, container_type::None, ATTRIBUTE_MERGE_SLOT);
    ConnectNode(SplitNode, SplitBody1);
    ConnectNode(SplitBody1, HBF1);

    // Connection 2
    container_node* SplitBody2 = NewContainer(Interface, container_type::None, ATTRIBUTE_MERGE_SLOT);
    ConnectNode(SplitNode, SplitBody2);      
    ConnectNode(SplitBody2, HBF2);
  }

  menu_tree* Root = NewMenuTree(Interface); // Root
  Root->Root = NewContainer(Interface, container_type::Root);

  container_node* RootHeader = 0;
  // Root Header
  {
    RootHeader = NewContainer(Interface, container_type::Color, ATTRIBUTE_DRAG| ATTRIBUTE_MERGE); 
    GetContainerPayload(empty_leaf, RootHeader)->Color = V4(0.4,0.2,0.2,1);
    
    draggable_attribute* Draggable = (draggable_attribute*) GetAttributePointer(RootHeader, ATTRIBUTE_DRAG);
    Draggable->Data = (void*) Root->Root;
    Draggable->Update = UpdateHeaderPosition;

    mergable_attribute* Mergable = (mergable_attribute*) GetAttributePointer(RootHeader, ATTRIBUTE_MERGE);
    Mergable->SrcNode = RootHeader;
    Mergable->DstNode = 0;
    Mergable->HotMergeZone = ArrayCount(Mergable->MergeZone);
    *Mergable->MergeZone ={};
  }

  // Header Body Footer
  container_node* RHBF = 0;    
  {
    RHBF = NewContainer(Interface, container_type::HBF, ATTRIBUTE_MERGE_SLOT);
    hbf_node* HBFP = GetContainerPayload(hbf_node, RHBF);
    HBFP->HeaderSize = 0.02;
    HBFP->FooterSize = 0;
    
    ConnectNode(RHBF, RootHeader); // Header
    ConnectNode(RHBF, SplitNode); // Body
    ConnectNode(RHBF, NewContainer(Interface, container_type::Color)); // Footer
  }


  //  Root Node Complex, 4 Borders, 1 Header, 1 None

  container_node* RootBody = 0;             // Output
  { 
    container_node* Border1 = CreateBorderNode(Interface, true, Region.X);
    ConnectNode(Root->Root, Border1);
    SetFrameDragAttribute(Border1);
    container_node* Border2 = CreateBorderNode(Interface, true, Region.X + Region.W);
    ConnectNode(Root->Root, Border2);
    SetFrameDragAttribute(Border2);
    container_node* Border3 = CreateBorderNode(Interface, false,Region.Y);
    ConnectNode(Root->Root, Border3);
    SetFrameDragAttribute(Border3);
    container_node* Border4 = CreateBorderNode(Interface, false, Region.Y + Region.H);
    ConnectNode(Root->Root, Border4);
    SetFrameDragAttribute(Border4);
    
    ConnectNode(Root->Root, RHBF);
  }

  TreeSensus(Root);

  UpdateRegions(Root);

}
menu_interface* CreateMenuInterface(memory_arena* Arena, midx MaxMemSize)
{
  menu_interface* Interface = PushStruct(Arena, menu_interface);
  Interface->ActiveMemory = 0;
  Interface->MaxMemSize = (u32) MaxMemSize;
  Interface->MemoryBase = (u8*) PushSize(Arena, Interface->MaxMemSize);
  Interface->Memory = Interface->MemoryBase;
  Interface->BorderSize = 0.007;
  Interface->HeaderSize = 0.02;
  Interface->MinSize = 0.2f;

  container_node* Sentinel = &(Interface->Sentinel);
  ListInitiate(Sentinel);

  v4 BorderColor = V4(0,0,0.4,1);
  v4 CornerColor = V4(0,0.4,0.2,1);
  v4 BodyColor   = V4(0.2,0.4,0.2,1);
  v4 HeaderColor = V4(0.4,0.2,0.2,1);
  CreateSplitRootWindow( Interface, Rect2f( 0.85, 0.25, 0.5, 0.5), false,
                         V4(0.15,0.5,0.15,1), V4(0.2,0.4,0.2,1), V4(0.15,0.5,0.15,1),
                         V4(0.35,0,0.35,1),   V4(0.3,0.1,0.3,1), V4(0.35,0,0.35,1));

  CreateSplitRootWindow( Interface, Rect2f( 0.25, 0.25, 0.5, 0.5), true,
                         V4(0.15,0.15,0.5,1), V4(0.2,0.2,0.4,1), V4(0.15,0.15,0.5,1),
                         V4(0.0,0.35,0.35,1), V4(0.1,0.3,0.3,1), V4(0.0,0.35,0.35,1));
  
  return Interface;
}

