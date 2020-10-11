#include "menu_interface.h"
#include "string.h"
u32 GetContainerPayloadSize(container_type Type)
{
  switch(Type)
  {
    case container_type::None:
    case container_type::Split:
    case container_type::Root:    return 0;
    case container_type::Color:   return sizeof(color_leaf);
    case container_type::Border:  return sizeof(border_leaf);
    case container_type::HBF:     return sizeof(hbf_node);
    default: INVALID_CODE_PATH;
  }
  return 0;
}

u32 GetAttributeSize(container_attribute Attribute)
{
  switch(Attribute)
  {
    case ATTRIBUTE_DRAG:       return sizeof(draggable_attribute);
    case ATTRIBUTE_MERGE:      return sizeof(mergable_attribute);
    case ATTRIBUTE_MERGE_SLOT: return sizeof(merge_slot_attribute);
    case ATTRIBUTE_TABS: return sizeof(tabbed_attribute);
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

  return Result;
}

u8* GetAttributePointer(container_node* Node, container_attribute Attri)
{
  Assert(Attri & Node->Attributes);
  menu_attribute_header * Attribute = Node->FirstAttribute;
  u8* Result = 0;
  while(Attribute)
  {
    if(Attribute->Type == Attri)
    {
      Result = ( (u8*) Attribute  ) + sizeof(menu_attribute_header);
      break;
    }
    Attribute = Attribute->Next;
  }
  return Result;
}

#define DEBUG_PRINT_FRAGMENTED_MEMORY_ALLOCATION 1

#ifdef HANDMADE_SLOW

void __CheckMemoryListIntegrity(menu_interface* Interface)
{
  memory_link* IntegrityLink = Interface->Sentinel.Next;
  u32 ListCount = 0;
  while(IntegrityLink != &Interface->Sentinel)
  {
    Assert(IntegrityLink == IntegrityLink->Next->Previous);
    Assert(IntegrityLink == IntegrityLink->Previous->Next);
    IntegrityLink = IntegrityLink->Next;
    ListCount++;
  }
}
#define CheckMemoryListIntegrity(Interface) __CheckMemoryListIntegrity(Interface)
#elif 
#define CheckMemoryListIntegrity(Interface)
#endif


// TODO (Add option to align by a certain byte?)
void* PushSize(menu_interface* Interface, u32 RequestedSize)
{
  memory_link* NewLink = 0;
  u32 ActualSize = RequestedSize + sizeof(memory_link);
  // Get memory from the middle if we have continous space that is big enough
  u32 RegionUsed = (u32)(Interface->Memory - Interface->MemoryBase);
  r32 MemoryFragmentation = Interface->ActiveMemory/(r32)RegionUsed;
  b32 MemoryTooFragmented = MemoryFragmentation < 0.8;
  if( MemoryTooFragmented || RegionUsed == Interface->MaxMemSize )
  {
    #if DEBUG_PRINT_FRAGMENTED_MEMORY_ALLOCATION
    u32 Slot = 0;
    u32 SlotSpace = 0;
    u32 SlotSize = 0;
    #endif

    memory_link* CurrentLink = Interface->Sentinel.Next;
    while( CurrentLink->Next != &Interface->Sentinel)
    {
      midx Base = (midx) CurrentLink + CurrentLink->Size;
      midx NextAddress = (midx)  CurrentLink->Next;
      Assert(Base <= NextAddress);

      midx OpenSpace = NextAddress - Base;

      if(OpenSpace >= ActualSize)
      {
        NewLink = (memory_link*) Base;
        NewLink->Size = ActualSize;
        ListInsertAfter(CurrentLink, NewLink);
        Assert(CurrentLink < CurrentLink->Next);
        Assert(NewLink < NewLink->Next);
        Assert(CurrentLink->Next == NewLink);

        Assert(((u8*)NewLink - (u8*)CurrentLink) == CurrentLink->Size);
        Assert(((u8*)NewLink->Next - (u8*)NewLink) >= ActualSize);
        #if DEBUG_PRINT_FRAGMENTED_MEMORY_ALLOCATION
        SlotSpace = (u32) Slot;
        SlotSize  = (u32) OpenSpace;
        #endif

        break;
      }
      #if DEBUG_PRINT_FRAGMENTED_MEMORY_ALLOCATION
      Slot++;
      #endif
      CurrentLink =  CurrentLink->Next;
    }

    #if DEBUG_PRINT_FRAGMENTED_MEMORY_ALLOCATION
    {
      u32 SlotCount = 0;
      memory_link* CurrentLink2 = Interface->Sentinel.Next;

      while( CurrentLink2->Next != &Interface->Sentinel)
      {
        SlotCount++;
        CurrentLink2 = CurrentLink2->Next;
      }
      
      Platform.DEBUGPrint("--==<< Middle Inset >>==--\n");
      Platform.DEBUGPrint(" - Slot: [%d,%d]\n", SlotSpace, SlotCount);
      Platform.DEBUGPrint(" - Size: [%d,%d]\n", ActualSize, SlotSize);
    }
    #endif
  }

  // Otherwise push it to the end
  if(!NewLink)
  {
    Assert(RegionUsed+ActualSize < Interface->MaxMemSize);
    #if 0
    Platform.DEBUGPrint("--==<< Post Inset >>==--\n");
    Platform.DEBUGPrint(" - Memory Left  : %d\n",Interface->MaxMemSize - (u32)RegionUsed + Size);
    Platform.DEBUGPrint(" - Size: %d\n\n", Size);
    #endif
    NewLink = (memory_link*) Interface->Memory;
    NewLink->Size = ActualSize;
    Interface->Memory += ActualSize;
    ListInsertBefore( &Interface->Sentinel, NewLink );
  }
  
  Interface->ActiveMemory += ActualSize;
  Assert(NewLink);
  void* Result = (void*)(((u8*)NewLink)+sizeof(memory_link));
  utils::ZeroSize(RequestedSize, Result);
  return Result;
}

container_node* NewContainer(menu_interface* Interface, container_type Type)
{
  CheckMemoryListIntegrity(Interface);

  u32 BaseNodeSize    = sizeof(container_node) + sizeof(memory_link);
  u32 NodePayloadSize = GetContainerPayloadSize(Type);
  u32 ContainerSize = (BaseNodeSize + NodePayloadSize);
 

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

  container_node* Result = (container_node*) PushSize(Interface, ContainerSize);
  Result->Type = Type;
  Result->Functions = GetMenuFunction(Type);
  
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

  CheckMemoryListIntegrity(Interface);

  return Result;
}

#define GetMemoryLinkFromPayload( Payload ) (memory_link*) (((u8*) (Payload)) - sizeof(memory_link))

void FreeMemory(menu_interface* Interface, void * Payload)
{
  memory_link* MemoryLink = GetMemoryLinkFromPayload(Payload);
  MemoryLink->Previous->Next = MemoryLink->Next;
  MemoryLink->Next->Previous = MemoryLink->Previous;
  Interface->ActiveMemory -= MemoryLink->Size;

  // Note: We should in theory not have to zerosize the deleted containers,
  //       But if we don't we sometimes crash, so look out for that when refactoring.
  //       This is probably masking some bug.
  utils::ZeroSize(MemoryLink->Size, (void*)MemoryLink);

  CheckMemoryListIntegrity(Interface);
}

void DeleteAllAttributes(menu_interface* Interface, container_node* Node)
{
  while(Node->FirstAttribute)
  {
    menu_attribute_header* Attribute = Node->FirstAttribute;
    Node->FirstAttribute = Node->FirstAttribute->Next;
    FreeMemory(Interface, (void*) Attribute);
  }
  Node->Attributes = ATTRIBUTE_NONE;
}

void DeleteAttribute(menu_interface* Interface, container_node* Node, container_attribute AttributeType)
{
  Assert(Node->Attributes & (u32)AttributeType);
  
  #if 1
  menu_attribute_header** AttributePtr = &Node->FirstAttribute;
  while((*AttributePtr)->Type != AttributeType)
  {
    AttributePtr = &(*AttributePtr)->Next;
  }  

  menu_attribute_header* AttributeToRemove = *AttributePtr;
  *AttributePtr = AttributeToRemove->Next;

  FreeMemory(Interface, (void*) AttributeToRemove);
  Node->Attributes =Node->Attributes - (u32)AttributeType;
  #else
  menu_attribute_header* PreviousAttribute = 0;
  menu_attribute_header* Attribute = Node->FirstAttribute;

  while(Attribute->Type != AttributeType)
  {
    PreviousAttribute = Attribute;
    Attribute = PreviousAttribute->Next;
  }

  Assert(Attribute->Type == AttributeType);
  if(!PreviousAttribute)
  {
    Node->FirstAttribute = Attribute->Next;
  }else{
    PreviousAttribute->Next = Attribute->Next;  
  }

  FreeMemory(Interface, (void*) Attribute);
  Node->Attributes =Node->Attributes - (u32)AttributeType;
  #endif
}

void DeleteContainer( menu_interface* Interface, container_node* Node)
{
  CheckMemoryListIntegrity(Interface);
  DeleteAllAttributes(Interface, Node);
  FreeMemory(Interface, (void*) Node);
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

void DrawMergeSlots(container_node* Node)
{
  mergable_attribute* Merge = (mergable_attribute*) GetAttributePointer(Node, ATTRIBUTE_MERGE);
  merge_slot_attribute* Slot = Merge->Slot;
  if(Slot)
  {
    for (u32 Index = 0; Index < ArrayCount(Slot->MergeZone); ++Index)
    {
      DEBUGPushQuad(Slot->MergeZone[Index], Index == Slot->HotMergeZone ? V4(0,1,0,0.5) : V4(0,1,0,0.3));
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
    if(Parent->Attributes & ATTRIBUTE_MERGE)
    {
      DrawMergeSlots(Parent);
    }

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

void FormatNodeString(container_node* Node, u32 BufferSize, c8 StringBuffer[])
{
  u32 Attributes = Node->Attributes;
  Platform.DEBUGFormatString(StringBuffer, sizeof(StringBuffer), sizeof(StringBuffer),
  "%s", ToString(Node->Type));
  b32 First = true;
  while(Attributes)
  {
    bit_scan_result ScanResult = FindLeastSignificantSetBit(Attributes);
    Assert(ScanResult.Found);
    u32 Attribute = (1 << ScanResult.Index);
    Attributes -= Attribute;
    if(First)
    {
      First = false;
      str::CatStrings( str::StringLength(StringBuffer), StringBuffer,
              2, ": ",
              BufferSize, StringBuffer);  
    }else{
      str::CatStrings( str::StringLength(StringBuffer), StringBuffer,
              3, " | ",
              BufferSize, StringBuffer);  
    }
    

    const c8* AttributeString = ToString(Attribute);
    str::CatStrings( str::StringLength(StringBuffer), StringBuffer,
                str::StringLength(AttributeString), AttributeString,
                ArrayCount(StringBuffer), StringBuffer);


  }

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

          u32 Attributes = Sibling->Attributes;
          c8 StringBuffer[1024] = {};
          FormatNodeString(Sibling, ArrayCount(StringBuffer), StringBuffer);

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

          c8 StringBuffer[1024] = {};
          FormatNodeString(Sibling, ArrayCount(StringBuffer), StringBuffer);
          XOff = WidthStep * Sibling->Depth;
          DEBUGTextOutAt(XOff, YOff, StringBuffer, Color);
          YOff -= HeightStep;

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
inline container_node* GetRoot(container_node* Node)
{
  while(Node->Parent)
  {
    Node = Node->Parent; 
  }
  return Node;
}

container_node* GetMergeSlotNode(menu_interface* Interface, container_node* Node)
{
  container_node* Result = 0;
  container_node* OwnRoot = GetRoot(Node);
  for (u32 MenuIndex = 0; MenuIndex < Interface->RootContainerCount; ++MenuIndex)
  {
    menu_tree* MenuRoot = &Interface->RootContainers[MenuIndex];
    if(OwnRoot == MenuRoot->Root) continue;

    for (u32 LeafIndex = 0; LeafIndex < MenuRoot->HotLeafCount; ++LeafIndex)
    {
      container_node* HotLeafNode = MenuRoot->HotLeafs[LeafIndex];
      while(HotLeafNode)
      {
        if(HasAttribute(HotLeafNode, ATTRIBUTE_MERGE_SLOT))
        {
          Result = HotLeafNode;
          return Result;
        }
        HotLeafNode = HotLeafNode->Parent;
      }
    }
  }
  return Result;
}

menu_tree* GetMenu(menu_interface* Interface, container_node* Node)
{
  menu_tree* Result = 0;
  container_node* Root = GetRoot(Node);
  for (u32 MenuIndex = 0; MenuIndex < Interface->RootContainerCount; ++MenuIndex)
  {
    menu_tree* MenuRoot = &Interface->RootContainers[MenuIndex];
    if(Root == MenuRoot->Root)
    {
      Result = MenuRoot;
      break;
    }
  }
  return Result;
}

void * PushAttribute(menu_interface* Interface, container_node* Node, container_attribute AttributeType)
{
  Assert(!(Node->Attributes & AttributeType));

  Node->Attributes = Node->Attributes | AttributeType;
  menu_attribute_header** HeaderPtr = &Node->FirstAttribute;
  while(*HeaderPtr)
  {
    menu_attribute_header* Header = *HeaderPtr;
    HeaderPtr = &(Header->Next);
  }

  u32 TotalSize = sizeof(menu_attribute_header) + GetAttributeSize(AttributeType);
  menu_attribute_header* Attr =  (menu_attribute_header*) PushSize(Interface, TotalSize);
  Attr->Type = AttributeType;
  *HeaderPtr = Attr;
  void* Result = (void*)(((u8*)Attr) + sizeof(menu_attribute_header));
  return Result;
}

void * PushOrGetAttribute(menu_interface* Interface, container_node* Node, container_attribute AttributeType)
{
  void * Result = 0;
  if(Node->Attributes)
  {
    Result = GetAttributePointer(Node, AttributeType);
  }else{
    Result = PushAttribute(Interface, Node, AttributeType);
  }
  return Result;
}

void UpdateMergableAttribute( menu_interface* Interface, container_node* Node )
{
  container_node* SlotNode = GetMergeSlotNode(Interface, Node);
  merge_slot_attribute* Slot = 0;
  if(SlotNode)
  {
    Slot = (merge_slot_attribute*) GetAttributePointer(SlotNode, ATTRIBUTE_MERGE_SLOT);
    rect2f Rect = SlotNode->Region;
    r32 W = Rect.W;
    r32 H = Rect.H;
    r32 S = Minimum(W,H)/4;

    v2 MP = V2(Rect.X+W/2,Rect.Y+H/2); // Middle Point
    v2 LQ = V2(MP.X-S, MP.Y);          // Left Quarter
    v2 RQ = V2(MP.X+S, MP.Y);          // Right Quarter
    v2 BQ = V2(MP.X,   MP.Y-S);        // Bot Quarter
    v2 TQ = V2(MP.X,   MP.Y+S);        // Top Quarter

    Slot->MergeZone[0] = Rect2f(LQ.X-S/2.f, LQ.Y-S/2.f,S/1.1f,S/1.1f); // Left Quarter
    Slot->MergeZone[1] = Rect2f(MP.X-S/2.f, MP.Y-S/2.f,S/1.1f,S/1.1f); // Middle Point
    Slot->MergeZone[2] = Rect2f(RQ.X-S/2.f, RQ.Y-S/2.f,S/1.1f,S/1.1f); // Right Quarter
    Slot->MergeZone[3] = Rect2f(BQ.X-S/2.f, BQ.Y-S/2.f,S/1.1f,S/1.1f); // Bot Quarter
    Slot->MergeZone[4] = Rect2f(TQ.X-S/2.f, TQ.Y-S/2.f,S/1.1f,S/1.1f); // Top Quarter

    if(Intersects(Slot->MergeZone[0], Interface->MousePos))
    {
      Slot->HotMergeZone = 0;
    }else if(Intersects(Slot->MergeZone[1], Interface->MousePos)){
      Slot->HotMergeZone = 1;
    }else if(Intersects(Slot->MergeZone[2], Interface->MousePos)){
      Slot->HotMergeZone = 2;
    }else if(Intersects(Slot->MergeZone[3], Interface->MousePos)){
      Slot->HotMergeZone = 3;
    }else if(Intersects(Slot->MergeZone[4], Interface->MousePos)){
      Slot->HotMergeZone = 4;
    }else{
      Slot->HotMergeZone = ArrayCount(Slot->MergeZone);
    }
  }

  mergable_attribute* Merge = (mergable_attribute*) GetAttributePointer(Node, ATTRIBUTE_MERGE);
  Merge->Slot = Slot;

  if(Slot && Interface->MouseLeftButton.Edge && !Interface->MouseLeftButton.Active )
  {
    u32 ZoneIndex = Slot->HotMergeZone;
    Merge->Slot = 0;
    if( ZoneIndex == 1)
    {
      #if 0
      // Middle
      header_leaf* SrcHeader = GetContainerPayload(header_leaf, Node);
      header_leaf* DstHeader = GetContainerPayload(header_leaf, GetHeaderNode(Header->NodeToMerge->Parent));
      DstHeader->SelectedTabOrdinal = SrcHeader->SelectedTabOrdinal + DstHeader->TabCount;
      for(u32 TabIndex = 0; TabIndex < SrcHeader->TabCount; TabIndex++)
      {
        DstHeader->Tabs[DstHeader->TabCount++] = SrcHeader->Tabs[TabIndex];
        Assert( DstHeader->TabCount < ArrayCount(DstHeader->Tabs))
      }
//
      DisconnectNode(Node->Parent);
      
      FreeMenuTree(Interface, &Interface->RootContainers[0]);
      #endif
    }else if( ZoneIndex == 0  || ZoneIndex == 2 ||  // Vertical
              ZoneIndex == 3  || ZoneIndex == 4)    // Horizontal
    {
      container_node* HomeHBF = SlotNode->Parent;
      container_node* Visitor = Node->Parent;

      Assert(HomeHBF->Type == container_type::HBF);
      Assert(Visitor->Type == container_type::HBF);

      container_node* HomeHead = HomeHBF->FirstChild;
      draggable_attribute* HomeDrag = (draggable_attribute*) PushOrGetAttribute(Interface, HomeHead, ATTRIBUTE_DRAG);
      HomeDrag->Data = 0;
      HomeDrag->Update = SplitWindowHeaderDrag;
      
      if(Visitor->Parent->Type == container_type::Root)
      {
        Visitor = Node->NextSibling;
      }
      
      if(Visitor->Type == container_type::HBF)
      {
        container_node* VisitorHead = Visitor->FirstChild;
        draggable_attribute* VisitorDrag = (draggable_attribute*) PushOrGetAttribute(Interface, VisitorHead, ATTRIBUTE_DRAG);
        VisitorDrag->Data = 0;
        VisitorDrag->Update = SplitWindowHeaderDrag;  
      }
      
      
      menu_tree* MenuToRemove = GetMenu(Interface, Visitor);
      DisconnectNode(Visitor);
      TreeSensus(MenuToRemove);
      FreeMenuTree(Interface, MenuToRemove);

      container_node* SplitNode = NewContainer(Interface, container_type::Split);
      container_node* BorderNode = CreateBorderNode(Interface, (ZoneIndex == 0 || ZoneIndex == 2), 0.5);
      SetSplitDragAttribute(SplitNode, BorderNode);
      ConnectNode(SplitNode, BorderNode);
      SwapNode(HomeHBF, SplitNode);

      if( ZoneIndex == 0  || ZoneIndex == 3)
      {
        ConnectNode(SplitNode, Visitor);  
        ConnectNode(SplitNode, HomeHBF);
      }else{
        ConnectNode(SplitNode, HomeHBF);
        ConnectNode(SplitNode, Visitor);  
      }
    }

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
      if(Draggable->Update)
      {
        Draggable->Update(Interface, Node, Draggable);  
      }
    }

    if(HasAttribute(Node, ATTRIBUTE_MERGE))
    {
      UpdateMergableAttribute(Interface, Node);
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
  GetColorNode(Node)->Color = Color;
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
  UpdateFrameBorder(Interface, GetBorderNode(Node));
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
  border_leaf* Border = GetBorderNode(Node);
  UpdateSplitBorder(Interface, (container_node*) Attr->Data, Border);
}

void UpdateHeaderPosition( menu_interface* Interface, container_node* Node, draggable_attribute* Attr )
{
  container_node* Root = (container_node*) Attr->Data;
  container_node* Child = Root->FirstChild;
  while(Child->Type == container_type::Border)
  {
    border_leaf* Border = GetBorderNode(Child);
    UpdateFrameBorder(Interface, Border);
    Child = Child->NextSibling;
  }
}

b32 SplitWindowSignal(menu_interface* Interface, container_node* HeaderNode, draggable_attribute* Attr )
{
  rect2f HeaderSplitRegion = Shrink(HeaderNode->Region, -2*HeaderNode->Region.H);
  if(!Intersects(HeaderSplitRegion, Interface->MousePos))
  {
    return true;
  }
  return false;
}

container_node* GetPreviousSibling(container_node* Node)
{
  container_node* Result = 0;
  if(Node->Parent)
  {
    container_node* Sibling = Node->Parent->FirstChild;
    if(Sibling != Node)
    {
      while(Sibling->NextSibling != Node)
      {
        Sibling = Sibling->NextSibling;
      }
      Result = Sibling;
    }
  }
  return Result;
}

inline void
SwapNode(container_node* Out, container_node* In)
{  
  In->Parent = Out->Parent;
  In->NextSibling = Out->NextSibling;

  container_node* Sibling = GetPreviousSibling(Out);
  if(Sibling)
  {
    Assert(Sibling->NextSibling == Out);
    Sibling->NextSibling = In;
  }else{
    Assert(Out->Parent->FirstChild == Out);
    In->Parent->FirstChild = In;
  }

  Out->NextSibling = 0;
  Out->Parent = 0;
}

void SetFrameDragAttribute(container_node* BorderNode)
{
  border_leaf* BorderLeaf = GetBorderNode(BorderNode);
  draggable_attribute* Draggable = (draggable_attribute*) GetAttributePointer(BorderNode, ATTRIBUTE_DRAG);
  Draggable->Data = 0;
  Draggable->Update = UpdateFrameBorder;
}


void SplitWindow(menu_interface* Interface, container_node* WindowToRemove, rect2f Region)
{  
  Assert(WindowToRemove->Type == container_type::HBF);
  if(WindowToRemove->Parent->Type != container_type::Split)
  {
    return;
  }

  container_node* WindowToRemain = WindowToRemove->NextSibling;
  if(!WindowToRemain)
  {
    WindowToRemain = WindowToRemove->Parent->FirstChild->NextSibling;
    Assert(WindowToRemain);
    Assert(WindowToRemain->NextSibling == WindowToRemove);
  }

  container_node* SplitNodeToSwapOut = WindowToRemain->Parent;

  DisconnectNode(WindowToRemove);
  DisconnectNode(WindowToRemain);

  SwapNode(SplitNodeToSwapOut, WindowToRemain);

  // Remove Drag attribute from windows to remain (if it's a single HBF under a RootHBF)
  // and windows to remove
  {
    if(WindowToRemain->Parent->Type == container_type::HBF &&
       WindowToRemain->Type == container_type::HBF)
    {
      Assert(WindowToRemain->FirstChild->Attributes & ATTRIBUTE_DRAG);
      DeleteAttribute(Interface, WindowToRemain->FirstChild, ATTRIBUTE_DRAG);
    }

    Assert(WindowToRemove->Type == container_type::HBF);
    Assert(WindowToRemove->FirstChild->Attributes & ATTRIBUTE_DRAG);
    DeleteAttribute(Interface, WindowToRemove->FirstChild, ATTRIBUTE_DRAG);  
  }
  
  SplitNodeToSwapOut->NextSibling = 0;

  container_node* Border = SplitNodeToSwapOut->FirstChild;
  Assert(Border->Type == container_type::Border);
  Assert(Border->NextSibling == 0);
  DisconnectNode(Border);
  DisconnectNode(SplitNodeToSwapOut);
  DeleteContainer(Interface, Border);
  DeleteContainer(Interface, SplitNodeToSwapOut);

  menu_tree* Root = NewMenuTree(Interface); // Root
  Root->Root = NewContainer(Interface, container_type::Root);

  container_node* RootHeader = 0;
  // Root Header
  {
    RootHeader = NewContainer(Interface, container_type::Color);
    GetColorNode(RootHeader)->Color = V4(0.4,0.2,0.2,1);

    draggable_attribute* Draggable = (draggable_attribute*) PushAttribute(Interface, RootHeader, ATTRIBUTE_DRAG);
    Draggable->Data = (void*) Root->Root;
    Draggable->Update = UpdateHeaderPosition;

    PushAttribute(Interface, RootHeader, ATTRIBUTE_MERGE);
  }


  // Header Body Footer
  container_node* RHBF = 0;    
  {
    RHBF = NewContainer(Interface, container_type::HBF);
    hbf_node* HBFP = GetHBFNode(RHBF);
    HBFP->HeaderSize = 0.02;
    HBFP->FooterSize = 0;

    ConnectNode(RHBF, RootHeader); // Header
    ConnectNode(RHBF, WindowToRemove); // Body
    ConnectNode(RHBF, NewContainer(Interface, container_type::Color)); // Footer
  }


  //  Root Node Complex, 4 Borders, 1 Header, 1 None
  container_node* RootBody = 0; // Output
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
  Root->HotLeafCount = GetIntersectingNodes( GlobalGameState->TransientArena,
    Root->NodeCount,
    Root->Root,
    Interface->MousePos, Root->HotLeafs);
  Root->ActiveLeafCount = GetIntersectingNodes( GlobalGameState->TransientArena,
    Root->NodeCount,
    Root->Root,
    Interface->MousePos, Root->ActiveLeafs);

  MoveMenuToTop(Interface, Interface->RootContainerCount-1);
}


void SplitWindowHeaderDrag( menu_interface* Interface, container_node* Node, draggable_attribute* Attr )
{
  if(SplitWindowSignal(Interface, Node, Attr))
  {
    v2 MouseDownPos = V2(Interface->MouseLeftButtonPush.X, Interface->MouseLeftButtonPush.Y);
    v2 HeaderOrigin = V2(Node->Region.X, Node->Region.Y);
    v2 WindowSize = V2(Node->Parent->Region.W, Node->Parent->Region.H);

    v2 MouseDownHeaderFrame = MouseDownPos - HeaderOrigin;
    v2 NewOrigin = Interface->MousePos - MouseDownHeaderFrame - V2(0, WindowSize.Y - Interface->HeaderSize);
    rect2f NewRegion = Rect2f(NewOrigin.X, NewOrigin.Y, WindowSize.X, WindowSize.Y);
    NewRegion = Shrink(NewRegion, -Interface->BorderSize/2);
    SplitWindow(Interface, Node->Parent, NewRegion);
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
  container_node* Result = NewContainer(Interface, container_type::Border);
  PushAttribute(Interface, Result, ATTRIBUTE_DRAG);
  border_leaf* BorderLeaf = GetBorderNode(Result);
  Assert(Result->Type == container_type::Border);
  *BorderLeaf = Border;
  return Result;
}

void SetSplitDragAttribute(container_node* SplitNode, container_node* BorderNode)
{
  border_leaf* BorderLeaf = GetBorderNode(BorderNode);
  draggable_attribute* Draggable = (draggable_attribute*) GetAttributePointer(BorderNode, ATTRIBUTE_DRAG);

  Draggable->Data = (void*) SplitNode;
  Draggable->Update = UpdateSplitBorder;
}

void CreateSplitRootWindow( menu_interface* Interface, rect2f Region, b32 Vertical,
                              v4 HeaderColor1, v4 BodyColor1, v4 FooterColor1,
                              v4 HeaderColor2, v4 BodyColor2, v4 FooterColor2)
{
  container_node* Header1 = 0;
  {
    Header1 = NewContainer(Interface, container_type::Color);
    draggable_attribute* Draggable = (draggable_attribute*) PushAttribute(Interface, Header1, ATTRIBUTE_DRAG);
    Draggable->Data = 0;
    Draggable->Update = SplitWindowHeaderDrag;

    GetColorNode(Header1)->Color = HeaderColor1;
    Assert(Header1->Type == container_type::Color);

  }

  // "Widget Body 1
  container_node* HBF1 = 0; // Root
  {
    container_node* Body = NewContainer(Interface, container_type::Color);
    PushAttribute(Interface, Body, ATTRIBUTE_MERGE_SLOT);

    GetColorNode(Body)->Color = BodyColor1;
    container_node* Footer = NewContainer(Interface, container_type::Color);
    GetColorNode(Footer)->Color = FooterColor1;

    HBF1 = NewContainer(Interface, container_type::HBF);
    hbf_node* HBFP = GetHBFNode(HBF1);
    HBFP->HeaderSize = 0.02;
    HBFP->FooterSize = 0.02;
    ConnectNode(HBF1, Header1);
    ConnectNode(HBF1, Body);
    ConnectNode(HBF1, Footer);
  }

  container_node* Header2 = 0;
  {
    Header2 = NewContainer(Interface, container_type::Color); 
    GetColorNode(Header2)->Color = HeaderColor2;

    draggable_attribute* Draggable = (draggable_attribute*) PushAttribute(Interface, Header2, ATTRIBUTE_DRAG);
    Draggable->Data = 0;
    Draggable->Update = SplitWindowHeaderDrag;
  }

  // "Widget Body 2
  container_node* HBF2 = 0; // Root
  {
    container_node* Body = NewContainer(Interface, container_type::Color);
    GetColorNode(Body)->Color = BodyColor2;
    container_node* Footer = NewContainer(Interface, container_type::Color);
    GetColorNode(Footer)->Color = FooterColor2;
    

    HBF2 = NewContainer(Interface, container_type::HBF);
    hbf_node* HBFP = GetHBFNode(HBF2);
    HBFP->HeaderSize = 0.02;
    HBFP->FooterSize = 0.02;

    merge_slot_attribute* MergeSlot = (merge_slot_attribute*) PushAttribute(Interface, Body, ATTRIBUTE_MERGE_SLOT);
    MergeSlot->HotMergeZone = ArrayCount(MergeSlot->MergeZone);
    *MergeSlot->MergeZone = {};
    ConnectNode(HBF2, Header2);
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
    ConnectNode(SplitNode, HBF1);

    // Connection 2
    ConnectNode(SplitNode, HBF2);
  }

  menu_tree* Root = NewMenuTree(Interface); // Root
  Root->Root = NewContainer(Interface, container_type::Root);

  container_node* RootHeader = 0;
  // Root Header
  {
    RootHeader = NewContainer(Interface, container_type::Color); 
    GetColorNode(RootHeader)->Color = V4(0.4,0.2,0.2,1);

    draggable_attribute* Draggable = (draggable_attribute*) PushAttribute(Interface, RootHeader, ATTRIBUTE_DRAG);
    Draggable->Data = (void*) Root->Root;
    Draggable->Update = UpdateHeaderPosition;

    PushAttribute(Interface, RootHeader, ATTRIBUTE_MERGE);
  }

  // Header Body Footer
  container_node* RHBF = 0;    
  {
    RHBF = NewContainer(Interface, container_type::HBF);
    hbf_node* HBFP = GetHBFNode(RHBF);
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

  memory_link* Sentinel = &(Interface->Sentinel);
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

