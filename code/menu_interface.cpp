#include "menu_interface.h"
#include "string.h"
u32 GetContainerPayloadSize(container_type Type)
{
  switch(Type)
  {
    case container_type::None:
    case container_type::Split:
    case container_type::Root:    return 0;
    case container_type::Border:  return sizeof(border_leaf);
    case container_type::HBF:     return sizeof(hbf_node);
    case container_type::Grid:     return sizeof(grid_node);
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
    case ATTRIBUTE_BUTTON:     return sizeof(button_attribute);
    case ATTRIBUTE_COLOR:      return sizeof(color_attribute);
    case ATTRIBUTE_TEXT:      return sizeof(text_attribute);
    case ATTRIBUTE_SIZE:      return sizeof(size_attribute);
    default: INVALID_CODE_PATH;
  }
  return 0; 
}

inline container_node* Previous(container_node* Node)
{
  return Node->PreviousSibling;
}

inline container_node* Next(container_node* Node)
{
  Assert(Node);
  return Node->NextSibling;
}

u32 GetChildCount(container_node* Node)
{
  container_node* Child = Node->FirstChild;
  u32 Count = 0;
  while(Child)
  {
    Child = Next(Child);
    Count++;
  }
  return Count;
}

u32 GetChildIndex(container_node* Node)
{
  Assert(Node->Parent);
  container_node* Child = Node->Parent->FirstChild;
  u32 Count = 0;
  while(Child != Node)
  {
    Child = Next(Child);
    Count++;
  }
  return Count;
}

container_node* GetChildFromIndex(container_node* Parent, u32 ChildIndex)
{
  u32 Idx = 0;
  container_node* Result = Parent->FirstChild;
  while(Idx++ < ChildIndex)
  {
    Result = Next(Result);
    Assert(Result);
  }
  return Result; 
}

void PivotNodes(container_node* ShiftLeft, container_node* ShiftRight)
{
  Assert(ShiftLeft->PreviousSibling == ShiftRight);
  Assert(ShiftRight->NextSibling == ShiftLeft);
  
  ShiftRight->NextSibling = ShiftLeft->NextSibling;
  if(ShiftRight->NextSibling)
  {
    ShiftRight->NextSibling->PreviousSibling = ShiftRight;
  }

  ShiftLeft->PreviousSibling = ShiftRight->PreviousSibling;
  if(ShiftLeft->PreviousSibling)
  {
    ShiftLeft->PreviousSibling->NextSibling = ShiftLeft;
  }else{
    Assert(ShiftRight->Parent->FirstChild == ShiftRight);
    ShiftRight->Parent->FirstChild = ShiftLeft;
  }

  ShiftLeft->NextSibling = ShiftRight;
  ShiftRight->PreviousSibling = ShiftLeft;

  Assert(ShiftRight->PreviousSibling == ShiftLeft);
  Assert(ShiftLeft->NextSibling == ShiftRight);
  
}

void ShiftLeft(container_node* ShiftLeft)
{
  if(!ShiftLeft->PreviousSibling)
  {
    return;
  }
  PivotNodes(ShiftLeft, ShiftLeft->PreviousSibling);
}

void ShiftRight(container_node* ShiftRight)
{
  if(!ShiftRight->NextSibling)
  {
    return;
  }
  PivotNodes(ShiftRight->NextSibling, ShiftRight); 
}

inline void
ReplaceNode(container_node* Out, container_node* In)
{  
  if(In == Out) return;

  In->Parent = Out->Parent;
  if(In->Parent->FirstChild == Out)
  {
    In->Parent->FirstChild = In;
  }

  In->NextSibling = Out->NextSibling;
  if(In->NextSibling)
  {
    In->NextSibling->PreviousSibling = In;  
  }

  In->PreviousSibling = Out->PreviousSibling;
  if(In->PreviousSibling)
  {
    
    In->PreviousSibling->NextSibling = In;
  }

  Out->NextSibling = 0;
  Out->PreviousSibling = 0;
  Out->Parent = 0;
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

#define DEBUG_PRINT_FRAGMENTED_MEMORY_ALLOCATION 0
#define DEBUG_PRINT_MENU_MEMORY_ALLOCATION 0

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
  #if DEBUG_PRINT_MENU_MEMORY_ALLOCATION
  {
    u32 RegionUsed = (u32)(Interface->Memory - Interface->MemoryBase);
    u32 TotSize = (u32) Interface->MaxMemSize;
    r32 Percentage = RegionUsed / (r32) TotSize;
    u32 ActiveMemory = Interface->ActiveMemory;
    r32 Fragmentation = ActiveMemory/(r32)RegionUsed;
    Platform.DEBUGPrint("--==<< Pre Memory Insert >>==--\n");
    Platform.DEBUGPrint(" - Tot Mem Used   : %2.3f  (%d/%d)\n", Percentage, RegionUsed, TotSize );
    Platform.DEBUGPrint(" - Fragmentation  : %2.3f  (%d/%d)\n", Fragmentation, ActiveMemory, RegionUsed );
  }
  #endif
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
    #if DEBUG_PRINT_FRAGMENTED_MEMORY_ALLOCATION
    Platform.DEBUGPrint("--==<< Post Inset >>==--\n");
    Platform.DEBUGPrint(" - Memory Left  : %d\n", Interface->MaxMemSize - (u32)RegionUsed + ActualSize);
    Platform.DEBUGPrint(" - Size: %d\n\n", ActualSize);
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

  #if DEBUG_PRINT_MENU_MEMORY_ALLOCATION
  {
    u32 RegionUsed2 = (u32)(Interface->Memory - Interface->MemoryBase);
    u32 TotSize = (u32) Interface->MaxMemSize;
    r32 Percentage = RegionUsed2 / (r32) TotSize;
    u32 ActiveMemory = Interface->ActiveMemory;
    r32 Fragmentation = ActiveMemory/(r32)RegionUsed2;
    
    Platform.DEBUGPrint("--==<< Post Memory Insert >>==--\n");
    Platform.DEBUGPrint(" - Tot Mem Used   : %2.3f  (%d/%d)\n", Percentage, RegionUsed2 , TotSize );
    Platform.DEBUGPrint(" - Fragmentation  : %2.3f  (%d/%d)\n", Fragmentation, ActiveMemory, RegionUsed2 );
    
  }
  #endif
  return Result;
}

container_node* NewContainer(menu_interface* Interface, container_type Type)
{
  CheckMemoryListIntegrity(Interface);

  u32 BaseNodeSize    = sizeof(container_node) + sizeof(memory_link);
  u32 NodePayloadSize = GetContainerPayloadSize(Type);
  u32 ContainerSize = (BaseNodeSize + NodePayloadSize);
 
  container_node* Result = (container_node*) PushSize(Interface, ContainerSize);
  Result->Type = Type;
  Result->Functions = GetMenuFunction(Type);
  
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

  menu_attribute_header** AttributePtr = &Node->FirstAttribute;
  while((*AttributePtr)->Type != AttributeType)
  {
    AttributePtr = &(*AttributePtr)->Next;
  }  

  menu_attribute_header* AttributeToRemove = *AttributePtr;
  *AttributePtr = AttributeToRemove->Next;

  FreeMemory(Interface, (void*) AttributeToRemove);
  Node->Attributes =Node->Attributes - (u32)AttributeType;
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
      Node->FirstChild = Next(NodeToDelete);
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
  container_node* SubTreeSibling = Next(SubTreeRoot);

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
    while(!Next(CurrentNode) && CurrentNode->Parent)
    {
      CurrentNode = CurrentNode->Parent;
      CurrentDepth--;
      Assert(CurrentDepth >= 0)
    }

    CurrentDepth--;

    // Either we found another sibling and we can traverse that part of the tree
    //  or we are at root and root has no siblings and we are done.
    CurrentNode = Next(CurrentNode);
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

rect2f GetSizedParentRegion(size_attribute* SizeAttr, rect2f BaseRegion)
{
  rect2f Result = {};
  if(SizeAttr->Width.Type == menu_size_type::RELATIVE)
  {
    Result.W = SizeAttr->Width.Value * BaseRegion.W;
  }else if(SizeAttr->Width.Type == menu_size_type::ABSOLUTE){
    Result.W = SizeAttr->Width.Value;  
  }

  if(SizeAttr->Height.Type == menu_size_type::RELATIVE)
  {
    Result.H = SizeAttr->Height.Value * BaseRegion.H;
  }else if(SizeAttr->Height.Type == menu_size_type::ABSOLUTE){
    Result.H = SizeAttr->Height.Value;
  }
  
  
  switch(SizeAttr->XAlignment)
  {
    case menu_region_alignment::LEFT:
    {
      Result.X = BaseRegion.X;
    }break;
    case menu_region_alignment::RIGHT:
    {
      Result.X = BaseRegion.X + BaseRegion.W - Result.W;
    }break;
    case menu_region_alignment::CENTER:
    {
      Result.X = BaseRegion.X + (BaseRegion.W - Result.W)*0.5f;
    }break;
  }
  switch(SizeAttr->YAlignment)
  {
    case menu_region_alignment::TOP:
    {
      Result.Y = BaseRegion.Y + BaseRegion.H - Result.Y;
    }break;
    case menu_region_alignment::BOT:
    {
      Result.Y = BaseRegion.Y;
    }break;
    case menu_region_alignment::CENTER:
    {
      Result.Y = BaseRegion.Y + (BaseRegion.H - Result.H)*0.5f;
    }break;
  }

  if(SizeAttr->LeftOffset.Type == menu_size_type::RELATIVE)
  {
    Result.X += SizeAttr->LeftOffset.Value * BaseRegion.W;
  }else if(SizeAttr->LeftOffset.Type == menu_size_type::ABSOLUTE){
    Result.X += SizeAttr->LeftOffset.Value;
  }

  if(SizeAttr->TopOffset.Type == menu_size_type::RELATIVE)
  {
    Result.Y -= SizeAttr->TopOffset.Value * BaseRegion.H;
  }else if(SizeAttr->TopOffset.Type == menu_size_type::ABSOLUTE){
    Result.Y -= SizeAttr->TopOffset.Value;
  }
  
  return Result;
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

    if(HasAttribute(Parent, ATTRIBUTE_SIZE))
    {
      size_attribute* SizeAttr = (size_attribute*) GetAttributePointer(Parent, ATTRIBUTE_SIZE);
      Parent->Region = GetSizedParentRegion(SizeAttr, Parent->Region);
    }

    // Update the region of all children and push them to the stack
    CallFunctionPointer(Parent->Functions.UpdateChildRegions, Parent);
    container_node* Child = Parent->FirstChild;
    while(Child)
    {
      ContainerStack[StackCount++] = Child;
      Child = Next(Child);
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


    if(HasAttribute(Parent, ATTRIBUTE_COLOR))
    {
      color_attribute* Color = (color_attribute*) GetAttributePointer(Parent, ATTRIBUTE_COLOR);
      DEBUGPushQuad(Parent->Region, Color->Color);
    }

    if(Parent->Functions.Draw)
    {
      CallFunctionPointer(Parent->Functions.Draw, Interface, Parent);  
    }


    if(HasAttribute(Parent, ATTRIBUTE_TEXT))
    {
      text_attribute* Text = (text_attribute*) GetAttributePointer(Parent, ATTRIBUTE_TEXT);
      rect2f TextBox = DEBUGTextSize(0, 0, Text->Text, Text->FontSize);
      DEBUGTextOutAt(Parent->Region.X + Parent->Region.W/2.f - TextBox.W/2.f,
                     Parent->Region.Y + Parent->Region.H/2.f - TextBox.H/3.f, Text->Text, Text->FontSize, V4(1,1,1,1));
    }

    if(HasAttribute(Parent,ATTRIBUTE_MERGE))
    {
      DrawMergeSlots(Parent);
    }
    // Update the region of all children and push them to the stack
    container_node* Child = Parent->FirstChild;
    while(Child)
    {
      ContainerStack[StackCount++] = Child;
      Child = Next(Child);
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
      u32 IntersectingChildren = 0;
      container_node* Child = Parent->FirstChild;
      while(Child)
      {
        if(Intersects(Child->Region, MousePos))
        {
          ContainerStack[StackCount++] = Child;
          IntersectingChildren++;
        }
        Child = Next(Child);
      }  

      if(IntersectingChildren==0)
      {
        Result[IntersectingLeafCount++] = Parent;
      }
    }
  }
  return IntersectingLeafCount;
}


container_node* ConnectNode(container_node* Parent, container_node* NewNode)
{
  NewNode->Parent = Parent;

  container_node* Child = Parent->FirstChild;
  if(!Parent->FirstChild){
    Parent->FirstChild = NewNode;
  }else{
    while(Next(Child))
    {
      Child = Next(Child);
    }  
    Child->NextSibling = NewNode;
    NewNode->PreviousSibling = Child;
  }

  return NewNode;
}

void DisconnectNode(container_node* Node)
{
  container_node* Parent = Node->Parent;
  if(Parent)
  {
    Assert(Parent->FirstChild);
    if(Node->PreviousSibling)
    {
      Node->PreviousSibling->NextSibling = Node->NextSibling;  
    }
    if(Node->NextSibling)
    {
      Node->NextSibling->PreviousSibling = Node->PreviousSibling;  
    }
    if(Parent->FirstChild == Node)
    {
      Parent->FirstChild = Node->NextSibling;
    }  
  }
  Node->Parent = 0;
  Node->NextSibling = 0;
  Node->PreviousSibling = 0;
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
  
  Platform.DEBUGFormatString(StringBuffer, BufferSize, BufferSize-1,
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
  u32 FontSize = 24;
  stb_font_map* FontMap = GetFontMap(GlobalGameState->AssetManager, FontSize);
  game_window_size WindowSize = GameGetWindowSize();
  r32 HeightStep = (FontMap->Ascent - FontMap->Descent)/WindowSize.HeightPx;
  r32 WidthStep  = 0.02;
  r32 YOff = 1 - 2*HeightStep;
  for (u32 ContainerIndex = 0; ContainerIndex < Interface->RootContainerCount; ++ContainerIndex)
  {
    menu_tree* Menu = &Interface->RootContainers[ContainerIndex];
    if(Menu->HotLeafCount>0 && Menu->Visible)
    {
      container_node* Node = Menu->HotLeafs[0];

      r32 XOff = 0;

      container_node* Nodes[64] = {};
      container_node* CheckPoints[64] = {};
      u32 DepthCount = Node->Depth;
      while(Node)
      {
        Nodes[Node->Depth] = Node;
        Node = Node->Parent;
      }
      Assert(Nodes[0]->Depth == 0);

      for (u32 Depth = 0; Depth <= DepthCount; ++Depth)
      {
        Assert(Depth < ArrayCount(Nodes));
        Assert(Depth < ArrayCount(CheckPoints));
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
            CheckPoints[Depth] = Next(Sibling);
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
          DEBUGTextOutAt(XOff, YOff, StringBuffer, FontSize, Color);
          YOff -= HeightStep;

          if(CheckPoints[Depth])
          {
            break;
          }
          Sibling = Next(Sibling);
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
          DEBUGTextOutAt(XOff, YOff, StringBuffer, FontSize, Color);
          YOff -= HeightStep;

          Sibling = Next(Sibling);
        }
      }
    }
    YOff -= HeightStep;
  }
}

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
  if(Node->Attributes & AttributeType)
  {
    Result = GetAttributePointer(Node, AttributeType);
  }else{
    Result = PushAttribute(Interface, Node, AttributeType);
  }
  return Result;
}

void MoveAttribute(container_node* From, container_node* To, container_attribute AttributeType)
{
  Assert(HasAttribute(From, AttributeType));
  Assert(!HasAttribute(To, AttributeType));

  menu_attribute_header** AttributePtr = &From->FirstAttribute;
  while((*AttributePtr)->Type != AttributeType)
  {
    AttributePtr = &(*AttributePtr)->Next;
  }

  menu_attribute_header* AttributeToMove = *AttributePtr;
  *AttributePtr = AttributeToMove->Next;

  menu_attribute_header* DstAttribute = To->FirstAttribute;
  if(To->FirstAttribute)
  {
    while(DstAttribute->Next)
    {
      DstAttribute = DstAttribute->Next;
    }
    DstAttribute->Next = AttributeToMove;
  }else{
    To->FirstAttribute = AttributeToMove;
  }

  From->Attributes = From->Attributes - (u32)AttributeType;
  To->Attributes = To->Attributes + (u32)AttributeType;
}

rect2f GetHorizontalListRegion(rect2f HeaderRect, u32 TabIndex, u32 TabCount)
{
  r32 TabWidth = HeaderRect.W / (r32) TabCount;
  rect2f Result = HeaderRect;
  Result.W = TabWidth;
  Result.X = HeaderRect.X + TabIndex * TabWidth;
  return Result;
}

rect2f GetVerticalListRegion(rect2f HeaderRect, u32 TabIndex, u32 TabCount)
{
  r32 TabHeight = HeaderRect.H / (r32) TabCount;
  rect2f Result = HeaderRect;
  Result.H = TabHeight;
  Result.Y = HeaderRect.Y + HeaderRect.H - (TabIndex+1) * TabHeight;
  return Result;
}

b32 SplitWindowSignal(menu_interface* Interface, container_node* HeaderNode)
{
  rect2f HeaderSplitRegion = Shrink(HeaderNode->Region, -2*HeaderNode->Region.H);
  if(!Intersects(HeaderSplitRegion, Interface->MousePos))
  {
    return true;
  }
  return false;
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

DRAGGABLE_ATTRIBUTE_UPDATE(UpdateFrameBorderCallback)
{
  UpdateFrameBorder(Interface, GetBorderNode(Node));
}

void SetFrameDragAttribute(container_node* BorderNode)
{
  border_leaf* BorderLeaf = GetBorderNode(BorderNode);
  draggable_attribute* Draggable = (draggable_attribute*) GetAttributePointer(BorderNode, ATTRIBUTE_DRAG);
  Draggable->Node = 0;
  Draggable->Update = DeclareFunction(draggable_attribute_update, UpdateFrameBorderCallback);
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

DRAGGABLE_ATTRIBUTE_UPDATE(UpdateSplitBorderCallback)
{
  border_leaf* Border = GetBorderNode(Node);
  UpdateSplitBorder(Interface, Attr->Node, Border);
}


DRAGGABLE_ATTRIBUTE_UPDATE(UpdateHeaderPosition)
{
  container_node* Root = Attr->Node;
  container_node* Child = Root->FirstChild;
  while(Child->Type == container_type::Border)
  {
    border_leaf* Border = GetBorderNode(Child);
    UpdateFrameBorder(Interface, Border);
    Child = Next(Child);
  }
}


menu_tree* CreateNewRootContainer(menu_interface* Interface, container_node* BaseWindow, rect2f Region)
{
  menu_tree* Root = NewMenuTree(Interface); // Root
  Root->Visible = true;
  Root->Root = NewContainer(Interface, container_type::Root);

  container_node* RootHeader = 0;
  // Root Header
  {
    RootHeader = NewContainer(Interface);
    color_attribute* Color = (color_attribute*) PushAttribute(Interface, RootHeader, ATTRIBUTE_COLOR);
    Color->Color = V4(0.4,0.2,0.2,1);

    draggable_attribute* Draggable = (draggable_attribute*) PushAttribute(Interface, RootHeader, ATTRIBUTE_DRAG);
    Draggable->Node = Root->Root;
    Draggable->Update = DeclareFunction(draggable_attribute_update, UpdateHeaderPosition);

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
    ConnectNode(RHBF, BaseWindow); // Body
    ConnectNode(RHBF, NewContainer(Interface)); // Footer
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

  return Root;
}

void SetTabAsActive(container_node* TabbedHBF, u32 NewTabIndex)
{
  Assert(TabbedHBF->Type == container_type::HBF);
  Assert(TabbedHBF->FirstChild->Type == container_type::Grid);
  Assert(NewTabIndex >= 0);

  container_node* TabList = TabbedHBF->FirstChild;
  container_node* Tab = GetChildFromIndex(TabList, NewTabIndex);

  button_attribute* Button = (button_attribute*) GetAttributePointer(Tab, ATTRIBUTE_BUTTON);
  tabbed_button_data* Data = (tabbed_button_data*) Button->Data;
  hbf_node* HostHBF = GetHBFNode(TabbedHBF);
  hbf_node* TabHBF  = GetHBFNode(Data->ItemHBF);
  HostHBF->FooterSize = TabHBF->FooterSize;
  ReplaceNode(TabbedHBF->FirstChild->NextSibling, Data->ItemBody);
  ReplaceNode(TabbedHBF->FirstChild->NextSibling->NextSibling, Data->ItemFooter);
}

container_node* ExtractHBFFromTab(menu_interface* Interface, container_node* Tab)
{
  button_attribute* Button = (button_attribute*) GetAttributePointer(Tab, ATTRIBUTE_BUTTON);
  tabbed_button_data* Data = (tabbed_button_data*) Button->Data;
  DisconnectNode(Tab);
  Assert(Data->ItemHeader == Tab);

  container_node* HBF = Data->ItemHBF;
  container_node* Header = Data->ItemHeader;
  container_node* Body = Data->ItemBody;
  container_node* Footer = Data->ItemFooter;

  Assert(!HBF->Parent && !HBF->NextSibling && !HBF->FirstChild);
  Assert(!Header->Parent && !Header->NextSibling);
  Assert(!Body->Parent && !Body->NextSibling);
  Assert(!Footer->Parent && !Footer->NextSibling);

  ConnectNode(HBF,Header);
  ConnectNode(HBF,Body);
  ConnectNode(HBF,Footer);

  FreeMemory(Interface, Button->Data);
  DeleteAttribute(Interface, Header, ATTRIBUTE_BUTTON);
  DeleteAttribute(Interface, Header, ATTRIBUTE_DRAG);

  return HBF;  
}

BUTTON_ATTRIBUTE_UPDATE( TabButtonUpdate )
{
  tabbed_button_data* Data = (tabbed_button_data*) Attr->Data;
  container_node* TabbedHBF = Data->TabbedHBF;
  container_node* ItemHBF = Data->ItemHBF;
  if(Interface->MouseLeftButton.Edge && Interface->MouseLeftButton.Active)
  {
    u32 NewTabIndex = GetChildIndex(Data->ItemHeader);
    SetTabAsActive(TabbedHBF, NewTabIndex);
  }
};

DRAGGABLE_ATTRIBUTE_UPDATE(TabDrag)
{
  Assert(Node->Parent->Type == container_type::Grid);
  r32 dX = Interface->MousePos.X - Interface->PreviousMousePos.X;

  u32 Count = GetChildCount(Node->Parent);
  u32 Index = GetChildIndex(Node);
  if(dX < 0 && Index > 0)
  {
    container_node* PreviousTab = Previous(Node);
    Assert(PreviousTab);
    if(Interface->MousePos.X < (PreviousTab->Region.X +PreviousTab->Region.W/2.f))
    {
      ShiftLeft(Node);
    }
  }else if(dX > 0 && Index < Count-1){
    container_node* NextTab = Next(Node);
    Assert(NextTab);
    if(Interface->MousePos.X > (NextTab->Region.X + NextTab->Region.W/2.f))
    {
      ShiftRight(Node);
    }
  }else if(SplitWindowSignal(Interface, Node->Parent))
  {
    v2 MouseDownPos = Interface->MouseLeftButtonPush;
    v2 HeaderOrigin = V2(Node->Parent->Region.X, Node->Parent->Region.Y);
    v2 WindowSize   = V2(Node->Parent->Parent->Region.W, Node->Parent->Parent->Region.H);

    v2 MouseDownHeaderFrame = MouseDownPos - HeaderOrigin;
    v2 NewOrigin = Interface->MousePos - MouseDownHeaderFrame - V2(0, WindowSize.Y - Interface->HeaderSize);
    rect2f NewRegion = Rect2f(NewOrigin.X, NewOrigin.Y, WindowSize.X, WindowSize.Y);
    NewRegion = Shrink(NewRegion, -Interface->BorderSize/2);

    container_node* TabbedHBF = Node->Parent->Parent;
    u32 NewTabIndex = (Index < Count-1) ? Index+1 : Index-1;
    SetTabAsActive(TabbedHBF, NewTabIndex);

    container_node* HBF = ExtractHBFFromTab(Interface, Node);
    CreateNewRootContainer(Interface, HBF, NewRegion);

    container_node* Grid = TabbedHBF->FirstChild;
    if(GetChildCount(Grid) == 1)
    {
      DisconnectNode(TabbedHBF->FirstChild);
      DisconnectNode(TabbedHBF->FirstChild);
      DisconnectNode(TabbedHBF->FirstChild);
      container_node* LastHBF = ExtractHBFFromTab(Interface, Grid->FirstChild);

      ReplaceNode(TabbedHBF, LastHBF);

      DeleteContainer(Interface, Grid);
      DeleteContainer(Interface, TabbedHBF);

      if(LastHBF->Parent->Parent->Type != container_type::Root)
      {
        draggable_attribute* HomeDrag = (draggable_attribute*) PushOrGetAttribute(Interface, LastHBF->FirstChild, ATTRIBUTE_DRAG);
        HomeDrag->Node = 0;
        HomeDrag->Update = DeclareFunction(draggable_attribute_update, SplitWindowHeaderDrag);
      }
    }
  }
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
      container_node* HomeHBF = SlotNode->Parent;
      container_node* Visitor = Node->Parent;
      Assert(HomeHBF->Type == container_type::HBF);
      Assert(Visitor->Type == container_type::HBF);
      if(Visitor->Parent->Type == container_type::Root)
      {
        Visitor = Next(Node);
      }

      if(HomeHBF->FirstChild->Type != container_type::Grid)
      {
        // TODO: Create Tabbed HBF
        // ConvertToTabbedHBF(TabbedHBF, HomeHBF);

        container_node* TabbedHBF = NewContainer(Interface, container_type::HBF);
        *GetHBFNode(TabbedHBF) = *GetHBFNode(HomeHBF);

        container_node* TabbedHeader = NewContainer(Interface, container_type::Grid);
        ConnectNode(TabbedHBF, TabbedHeader);
        ReplaceNode(HomeHBF, TabbedHBF);

        grid_node* Grid = GetGridNode(TabbedHeader);
        Grid->Row = 1;

        if(HasAttribute(HomeHBF->FirstChild, ATTRIBUTE_DRAG))
        {
          DeleteAttribute(Interface, HomeHBF->FirstChild, ATTRIBUTE_DRAG);
        }

        draggable_attribute* Drag = (draggable_attribute*) PushAttribute(Interface, HomeHBF->FirstChild, ATTRIBUTE_DRAG);
        Drag->Node = HomeHBF->FirstChild;

        Drag->Update = DeclareFunction(draggable_attribute_update, TabDrag);

        button_attribute* Button = (button_attribute*) PushAttribute(Interface, HomeHBF->FirstChild, ATTRIBUTE_BUTTON);
        tabbed_button_data* ButtonData = (tabbed_button_data*) PushSize(Interface, sizeof(tabbed_button_data));
        ButtonData->TabbedHBF  = TabbedHBF;
        ButtonData->ItemHBF    = HomeHBF;
        ButtonData->ItemHeader = HomeHBF->FirstChild;
        ButtonData->ItemBody   = HomeHBF->FirstChild->NextSibling;
        ButtonData->ItemFooter = HomeHBF->FirstChild->NextSibling->NextSibling;
        DisconnectNode(ButtonData->ItemHBF);
        DisconnectNode(ButtonData->ItemHeader);
        DisconnectNode(ButtonData->ItemBody);
        DisconnectNode(ButtonData->ItemFooter);

        Button->Data = (void*) ButtonData;
        Button->Update = DeclareFunction(button_attribute_update, TabButtonUpdate);

        ConnectNode(TabbedHeader, ButtonData->ItemHeader); 
        ConnectNode(TabbedHBF,    ButtonData->ItemBody);
        ConnectNode(TabbedHBF,    ButtonData->ItemFooter);
        
        HomeHBF = TabbedHBF;
      }

      

      menu_tree* MenuToRemove = GetMenu(Interface, Visitor);
      DisconnectNode(Visitor);
      u32 HBFCount = 0;
      // TODO (Jakob): Make HBFArr dynamically sized;
      container_node* HBFArr[32];
      {
        temporary_memory TempMem =  BeginTemporaryMemory(GlobalGameState->TransientArena);

        u32 StackElementSize = sizeof(container_node*);
        u32 StackByteSize = MenuToRemove->NodeCount * StackElementSize;

        u32 StackCount = 0;
        container_node** ContainerStack = PushArray(GlobalGameState->TransientArena, MenuToRemove->NodeCount, container_node*);

        // Push Visitor
        ContainerStack[StackCount++] = Visitor;

        while(StackCount>0)
        {
          // Pop new parent from Stack
          container_node* Parent = ContainerStack[--StackCount];
          ContainerStack[StackCount] = 0;

          // Update the region of all children and push them to the stack
          if(Parent->Type == container_type::Split)
          {
            ContainerStack[StackCount++] = Parent->FirstChild->NextSibling;
            ContainerStack[StackCount++] = Parent->FirstChild->NextSibling->NextSibling;
          }else if(Parent->Type == container_type::HBF)
          {
            HBFArr[HBFCount++] = Parent;
            Assert(HBFCount < ArrayCount(HBFArr));
          }else{
            INVALID_CODE_PATH;
          }
        }

        EndTemporaryMemory(TempMem);

      }

      for (u32 HBFIndex = 0; HBFIndex < HBFCount; ++HBFIndex)
      {
        container_node* HBFTabToAdd = HBFArr[HBFIndex];
        tabbed_button_data* ButtonData = 0;
        if(HBFTabToAdd->FirstChild->Type == container_type::Grid)
        {
          container_node* GridHeader = HBFTabToAdd->FirstChild;
          while(GridHeader->FirstChild)
          {
            container_node* Tab = GridHeader->FirstChild;
            DisconnectNode(Tab);
            button_attribute* Button = (button_attribute*) GetAttributePointer(Tab, ATTRIBUTE_BUTTON);
            ButtonData = (tabbed_button_data*) Button->Data;
            ButtonData->TabbedHBF = HomeHBF;
            ConnectNode(HomeHBF->FirstChild, ButtonData->ItemHeader);
          }          
        }else{

          if(HasAttribute(HBFTabToAdd->FirstChild, ATTRIBUTE_DRAG))
          {
            DeleteAttribute(Interface, HBFTabToAdd->FirstChild, ATTRIBUTE_DRAG);  
          }

          draggable_attribute* Drag = (draggable_attribute*) PushAttribute(Interface, HBFTabToAdd->FirstChild, ATTRIBUTE_DRAG);
          Drag->Node = HBFTabToAdd->FirstChild;
          Drag->Update =  DeclareFunction(draggable_attribute_update, TabDrag); 
          button_attribute* Button = (button_attribute*) PushAttribute(Interface, HBFTabToAdd->FirstChild, ATTRIBUTE_BUTTON);
          ButtonData = (tabbed_button_data*) PushSize(Interface, sizeof(tabbed_button_data));
          ButtonData->TabbedHBF  = HomeHBF;
          ButtonData->ItemHBF    = HBFTabToAdd;
          ButtonData->ItemHeader = HBFTabToAdd->FirstChild;
          ButtonData->ItemBody   = HBFTabToAdd->FirstChild->NextSibling;
          ButtonData->ItemFooter = HBFTabToAdd->FirstChild->NextSibling->NextSibling;
          DisconnectNode(ButtonData->ItemHBF);
          DisconnectNode(ButtonData->ItemHeader);
          DisconnectNode(ButtonData->ItemBody);
          DisconnectNode(ButtonData->ItemFooter);
          ConnectNode(HomeHBF->FirstChild, ButtonData->ItemHeader);
          Button->Data = (void*)ButtonData;
          Button->Update = DeclareFunction(button_attribute_update, TabButtonUpdate);
        }
        Assert(ButtonData);
        if(HBFIndex == HBFCount-1)
        {
          u32 NewTabIndex = GetChildIndex(ButtonData->ItemHeader);
          SetTabAsActive(ButtonData->TabbedHBF, NewTabIndex);
        }
      }

      TreeSensus(MenuToRemove);
      FreeMenuTree(Interface, MenuToRemove);

    }else if( ZoneIndex == 0  || ZoneIndex == 2 ||  // Vertical
              ZoneIndex == 3  || ZoneIndex == 4)    // Horizontal
    {
      container_node* HomeHBF = SlotNode->Parent;
      container_node* Visitor = Node->Parent;

      Assert(HomeHBF->Type == container_type::HBF);
      Assert(Visitor->Type == container_type::HBF);

      container_node* HomeHead = HomeHBF->FirstChild;
      draggable_attribute* HomeDrag = (draggable_attribute*) PushOrGetAttribute(Interface, HomeHead, ATTRIBUTE_DRAG);
      HomeDrag->Node = 0;
      HomeDrag->Update =  DeclareFunction(draggable_attribute_update, SplitWindowHeaderDrag);
      
      if(Visitor->Parent->Type == container_type::Root)
      {
        Visitor = Next(Node);
      }

      menu_tree* MenuToRemove = GetMenu(Interface, Visitor);
      DisconnectNode(Visitor);

      container_node* SplitNode = SplitNode = CreateSplitWindow(Interface, (ZoneIndex == 0 || ZoneIndex == 2));
      ReplaceNode(HomeHBF, SplitNode);

      if( ZoneIndex == 0  || ZoneIndex == 3)
      {
        SetSplitWindows(Interface, SplitNode, Visitor, HomeHBF);
      }else{
        SetSplitWindows(Interface, SplitNode, HomeHBF, Visitor);
      }

      TreeSensus(MenuToRemove);
      FreeMenuTree(Interface, MenuToRemove);
    }

  }
}


void ActOnInput(menu_interface* Interface, menu_tree* Menu)
{
  if(!Menu->Visible) return;
  menu_tree* ActiveMenu = &Interface->RootContainers[0];
  for (u32 i = 0; i < ActiveMenu->ActiveLeafCount; ++i)
  {
    container_node* Node =  ActiveMenu->ActiveLeafs[i];
    if(HasAttribute(Node, ATTRIBUTE_DRAG))
    {
      draggable_attribute* Draggable = (draggable_attribute*) GetAttributePointer(Node, ATTRIBUTE_DRAG);
      if(Draggable->Update)
      {
        CallFunctionPointer(Draggable->Update,Interface, Node, Draggable);  
      }
    }

    if(HasAttribute(Node, ATTRIBUTE_BUTTON))
    {
      button_attribute* Button = (button_attribute*) GetAttributePointer(Node, ATTRIBUTE_BUTTON);
      CallFunctionPointer(Button->Update,Interface, Button, Node);
    }

    if(HasAttribute(Node, ATTRIBUTE_MERGE))
    {
      UpdateMergableAttribute(Interface, Node);
    }
  }
}

void SetMenuInput(memory_arena* Arena, game_input* GameInput, menu_interface* Interface)
{
  v2 MousePos = V2(GameInput->MouseX, GameInput->MouseY);

  Update(&Interface->MouseLeftButton, GameInput->MouseButton[PlatformMouseButton_Left].EndedDown);
  Update(&Interface->TAB, GameInput->Keyboard.Key_TAB.EndedDown);
  if(Interface->TAB.Active && Interface->TAB.Edge)
  {
    for( u32 MenuIndex = 0; MenuIndex < Interface->RootContainerCount; MenuIndex++)
    {
      menu_tree* Menu = &Interface->RootContainers[MenuIndex];
      Menu->Visible = !Menu->Visible;
    }
  }  

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
      utils::Copy(sizeof(container_node*)*SelectedMenu->HotLeafCount, SelectedMenu->HotLeafs, SelectedMenu->ActiveLeafs);
      SelectedMenu->ActiveLeafCount = SelectedMenu->HotLeafCount;

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
  SetMenuInput(GlobalGameState->TransientArena, GameInput, Interface);
 
  if(!Interface->RootContainerCount) return;

  ActOnInput(Interface, &Interface->RootContainers[0]);

  for (s32 WindowIndex = Interface->RootContainerCount-1;
           WindowIndex >= 0;
         --WindowIndex)
  {
    menu_tree* MenuTree = &Interface->RootContainers[WindowIndex];
    if(MenuTree->Visible)
    {
      TreeSensus(MenuTree);
      UpdateRegions( MenuTree );
      DrawMenu( GlobalGameState->TransientArena, Interface, MenuTree->NodeCount, MenuTree->Root);
    }
  }
  
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

  ReplaceNode(SplitNodeToSwapOut, WindowToRemain);

  // Remove Drag attribute from windows to remain (if it's a single HBF under a RootHBF)
  // and windows to remove
  {
    if(WindowToRemain->Parent->Type == container_type::HBF &&
       WindowToRemain->Type == container_type::HBF)
    {
      if( HasAttribute(WindowToRemain->FirstChild, ATTRIBUTE_DRAG))
      {
        DeleteAttribute(Interface, WindowToRemain->FirstChild, ATTRIBUTE_DRAG);  
      }
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

  CreateNewRootContainer(Interface, WindowToRemove, Region);  
}


DRAGGABLE_ATTRIBUTE_UPDATE(SplitWindowHeaderDrag)
{
  if(SplitWindowSignal(Interface, Node))
  {
    v2 MouseDownPos = V2(Interface->MouseLeftButtonPush.X, Interface->MouseLeftButtonPush.Y);
    v2 HeaderOrigin = V2(Node->Region.X, Node->Region.Y);
    v2 WindowSize = V2(Node->Parent->Region.W, Node->Parent->Region.H);

    v2 MouseDownHeaderFrame = MouseDownPos - HeaderOrigin;
    v2 NewOrigin = Interface->MousePos - MouseDownHeaderFrame - V2(0, WindowSize.Y - Interface->HeaderSize);
    rect2f NewRegion = Rect2f(NewOrigin.X, NewOrigin.Y, WindowSize.X, WindowSize.Y);
    NewRegion = Shrink(NewRegion, -Interface->BorderSize/2);

    if(Node->Parent->Type == container_type::HBF)
    {
      SplitWindow(Interface, Node->Parent, NewRegion);
    }    
  }
}

inline container_node*
CreateBorderNode(menu_interface* Interface, b32 Vertical, r32 Position, v4 Color)
{
  border_leaf Border = {};
  Border.Vertical = Vertical;
  Border.Position = Position;
  Border.Thickness = Interface->BorderSize;

  container_node* Result = NewContainer(Interface, container_type::Border);
  PushAttribute(Interface, Result, ATTRIBUTE_DRAG);
  border_leaf* BorderLeaf = GetBorderNode(Result);

  color_attribute* ColorAttr = (color_attribute*) PushAttribute(Interface, Result, ATTRIBUTE_COLOR);
  ColorAttr->Color = Color;

  Assert(Result->Type == container_type::Border);
  *BorderLeaf = Border;
  return Result;
}

void SetSplitDragAttribute(container_node* SplitNode, container_node* BorderNode)
{
  border_leaf* BorderLeaf = GetBorderNode(BorderNode);
  draggable_attribute* Draggable = (draggable_attribute*) GetAttributePointer(BorderNode, ATTRIBUTE_DRAG);

  Draggable->Node = SplitNode;
  Draggable->Update = DeclareFunction( draggable_attribute_update, UpdateSplitBorderCallback);
}

container_node* CreateDummyHBF(menu_interface* Interface, v4 HeaderColor, v4 BodyColor, v4 FooterColor)
{
  container_node* Header = NewContainer(Interface);

  color_attribute* HColorAttr = (color_attribute*) PushAttribute(Interface, Header, ATTRIBUTE_COLOR);
  HColorAttr->Color = HeaderColor;

  draggable_attribute* Draggable = (draggable_attribute*) PushAttribute(Interface, Header, ATTRIBUTE_DRAG);
  Draggable->Node = 0;
  Draggable->Update = DeclareFunction(draggable_attribute_update, SplitWindowHeaderDrag);

  container_node* Body = NewContainer(Interface);
  color_attribute* BColorAttr = (color_attribute*) PushAttribute(Interface, Body, ATTRIBUTE_COLOR);
  BColorAttr->Color = BodyColor;

  PushAttribute(Interface, Body, ATTRIBUTE_MERGE_SLOT);
  
  container_node* Footer = NewContainer(Interface);
  color_attribute* FColorAttr = (color_attribute*) PushAttribute(Interface, Footer, ATTRIBUTE_COLOR);
  FColorAttr->Color = FooterColor;


  container_node* HBF = NewContainer(Interface, container_type::HBF);
  hbf_node* HBFP = GetHBFNode(HBF);
  HBFP->HeaderSize = 0.02;
  HBFP->FooterSize = 0.02;
  ConnectNode(HBF, Header);
  ConnectNode(HBF, Body);
  ConnectNode(HBF, Footer);

  return HBF;
}

container_node* CreateHBF(menu_interface* Interface, v4 HeaderColor, container_node* BodyNode)
{
  container_node* Header = NewContainer(Interface);

  color_attribute* HColorAttr = (color_attribute*) PushAttribute(Interface, Header, ATTRIBUTE_COLOR);
  HColorAttr->Color = HeaderColor;

  PushOrGetAttribute(Interface, BodyNode, ATTRIBUTE_MERGE_SLOT);

  container_node* Footer = NewContainer(Interface, container_type::None);

  container_node* HBF = NewContainer(Interface, container_type::HBF);
  hbf_node* HBFP = GetHBFNode(HBF);
  HBFP->HeaderSize = 0.02;
  HBFP->FooterSize = 0;
  ConnectNode(HBF, Header);
  ConnectNode(HBF, BodyNode);
  ConnectNode(HBF, Footer);

  return HBF;
}

container_node* CreateSplitWindow( menu_interface* Interface, b32 Vertical, r32 BorderPos)
{
  container_node* SplitNode  = NewContainer(Interface, container_type::Split);
  container_node* BorderNode = CreateBorderNode(Interface, Vertical, BorderPos);
  ConnectNode(SplitNode, BorderNode);
  SetSplitDragAttribute(SplitNode, BorderNode);
  return SplitNode;
}

container_node* SetSplitWindows( menu_interface* Interface, container_node* SplitNode, container_node* HBF1, container_node* HBF2)
{
  Assert( HBF1->Type == container_type::HBF);
  Assert( HBF2->Type == container_type::HBF);

  draggable_attribute* Draggable1 = (draggable_attribute*) PushOrGetAttribute(Interface, HBF1->FirstChild, ATTRIBUTE_DRAG);
  Draggable1->Node = 0;
  Draggable1->Update = DeclareFunction(draggable_attribute_update, SplitWindowHeaderDrag);

  draggable_attribute* Draggable2 = (draggable_attribute*) PushOrGetAttribute(Interface, HBF2->FirstChild, ATTRIBUTE_DRAG);
  Draggable2->Node = 0;
  Draggable2->Update = DeclareFunction(draggable_attribute_update, SplitWindowHeaderDrag);

  ConnectNode(SplitNode, HBF1);
  ConnectNode(SplitNode, HBF2);
  return SplitNode;
}

void CreateSplitRootWindow( menu_interface* Interface, rect2f Region, b32 Vertical,
                            v4 HeaderColor1, v4 BodyColor1, v4 FooterColor1,
                            v4 HeaderColor2, v4 BodyColor2, v4 FooterColor2)
{
  container_node* HBF1 = CreateDummyHBF(Interface, HeaderColor1, BodyColor1, FooterColor1);
  container_node* HBF2 = CreateDummyHBF(Interface, HeaderColor2, BodyColor2, FooterColor2);
  container_node* SplitNode = CreateSplitWindow(Interface, Vertical);
  SetSplitWindows(Interface, SplitNode, HBF1, HBF2);

  menu_tree* Menu = CreateNewRootContainer(Interface, SplitNode, Region);
  Menu->Visible = true;
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

  ListInitiate(&Interface->Sentinel);

  //CreateSplitRootWindow( Interface, Rect2f( 0.25, 0.25, 0.5, 0.5), true,
  //                       V4(0.15,0.15,0.5,1), V4(0.2,0.2,0.4,1), V4(0.15,0.15,0.5,1),
  //                       V4(0.0,0.35,0.35,1), V4(0.1,0.3,0.3,1), V4(0.0,0.35,0.35,1));
  
  return Interface;
}

