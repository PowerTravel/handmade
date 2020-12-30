#include "menu_interface.h"
#include "string.h"


internal u32
GetContainerPayloadSize(container_type Type)
{
  switch(Type)
  {
    case container_type::None:
    case container_type::Split:
    case container_type::Root:      return 0;
    case container_type::Border:    return sizeof(border_leaf);
    case container_type::Grid:      return sizeof(grid_node);
    case container_type::TabWindow: return sizeof(tab_window_node);
    case container_type::Tab:       return sizeof(tab_node);
    case container_type::Plugin:    return sizeof(plugin_node);
    default: INVALID_CODE_PATH;
  }
  return 0;
}

internal u32
GetAttributeSize(container_attribute Attribute)
{
  switch(Attribute)
  {
    case ATTRIBUTE_MERGE:       return sizeof(mergable_attribute);
    case ATTRIBUTE_COLOR:       return sizeof(color_attribute);
    case ATTRIBUTE_TEXT:        return sizeof(text_attribute);
    case ATTRIBUTE_SIZE:        return sizeof(size_attribute);
    case ATTRIBUTE_MENU_EVENT_HANDLE: return sizeof(menu_event_handle_attribtue);
    default: INVALID_CODE_PATH;
  }
  return 0; 
}

inline container_node*
Previous(container_node* Node)
{
  return Node->PreviousSibling;
}

inline container_node*
Next(container_node* Node)
{
  Assert(Node);
  return Node->NextSibling;
}

internal u32
GetChildCount(container_node* Node)
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

internal u32
GetChildIndex(container_node* Node)
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

internal container_node*
GetChildFromIndex(container_node* Parent, u32 ChildIndex)
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


internal void
PivotNodes(container_node* ShiftLeft, container_node* ShiftRight)
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

internal void
ShiftLeft(container_node* ShiftLeft)
{
  if(!ShiftLeft->PreviousSibling)
  {
    return;
  }
  PivotNodes(ShiftLeft, ShiftLeft->PreviousSibling);
}

internal void
ShiftRight(container_node* ShiftRight)
{
  if(!ShiftRight->NextSibling)
  {
    return;
  }
  PivotNodes(ShiftRight->NextSibling, ShiftRight); 
}

internal void
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

inline internal u32
GetAttributeSize(u32 Attributes)
{
  return GetAttributeSize((container_attribute)Attributes);
}

inline b32
HasAttribute(container_node* Node, container_attribute Attri)
{
  return Node->Attributes & ((u32) Attri);
}

internal u32
GetAttributeBatchSize(container_attribute Attri)
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
internal void* PushSize(menu_interface* Interface, u32 RequestedSize)
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

internal void FreeMemory(menu_interface* Interface, void * Payload)
{
  memory_link* MemoryLink = GetMemoryLinkFromPayload(Payload);
  MemoryLink->Previous->Next = MemoryLink->Next;
  MemoryLink->Next->Previous = MemoryLink->Previous;
  Interface->ActiveMemory -= MemoryLink->Size;
  CheckMemoryListIntegrity(Interface);
}

internal void
DeleteAllAttributes(menu_interface* Interface, container_node* Node)
{
  // TODO: Free attribute data here too?
  while(Node->FirstAttribute)
  {
    menu_attribute_header* Attribute = Node->FirstAttribute;
    Node->FirstAttribute = Node->FirstAttribute->Next;
    FreeMemory(Interface, (void*) Attribute);
  }
  Node->Attributes = ATTRIBUTE_NONE;
}

internal void
DeleteAttribute(menu_interface* Interface, container_node* Node, container_attribute AttributeType)
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


internal void CancelAllUpdateFunctions(menu_interface* Interface, container_node* Node )
{
  for(u32 i = 0; i < ArrayCount(Interface->UpdateQueue); ++i)
  {
    update_args* Entry = &Interface->UpdateQueue[i];
    if(Entry->InUse && Entry->Caller == Node)
    {
      *Entry = {};
    }
  }
}

void ClearMenuEvents( menu_interface* Interface, container_node* Node);
void DeleteContainer( menu_interface* Interface, container_node* Node)
{
  CheckMemoryListIntegrity(Interface);
  CancelAllUpdateFunctions(Interface, Node );
  ClearMenuEvents(Interface, Node);
  DeleteAllAttributes(Interface, Node);
  FreeMemory(Interface, (void*) Node);
}

menu_tree* NewMenuTree(menu_interface* Interface)
{
  menu_tree* Result = (menu_tree*) PushSize(Interface, sizeof(menu_tree));
  Result->LosingFocus = DeclareFunction(menu_losing_focus, DefaultLosingFocus);
  Result->GainingFocus = DeclareFunction(menu_losing_focus, DefaultGainingFocus);
  ListInsertBefore(&Interface->MenuSentinel, Result);
  return Result;
}


internal void DeleteMenuSubTree(menu_interface* Interface, container_node* Root)
{
  DisconnectNode(Root);
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

inline internal menu_tree* GetNextSpawningWindow(menu_interface* Interface)
{ 
  menu_tree* Result = 0;
  menu_tree* SpawningMenu = Interface->MenuSentinel.Next;
  while(SpawningMenu != &(Interface->MenuSentinel))
  {
    if(SpawningMenu->Root->Type == container_type::Root &&
       SpawningMenu != Interface->SpawningWindow )
    {
      Result = SpawningMenu;
      break;
    }
    SpawningMenu = SpawningMenu->Next;
  }

  return Result;
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


void FreeMenuTree(menu_interface* Interface, menu_tree* MenuToFree)
{
  TreeSensus(MenuToFree);
  if(MenuToFree == Interface->SpawningWindow)
  {
    Interface->SpawningWindow = GetNextSpawningWindow(Interface);
  }
  Assert(MenuToFree != &Interface->MenuSentinel);

  ListRemove( MenuToFree );
  container_node* Root = MenuToFree->Root;

  FreeMemory(Interface, (void*)MenuToFree);

  DeleteMenuSubTree(Interface, Root);
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
      //Result.Y = BaseRegion.Y + BaseRegion.H - Result.Y;
      Result.Y = BaseRegion.Y;
    }break;
    case menu_region_alignment::BOT:
    {
      Result.Y = BaseRegion.Y + BaseRegion.H - Result.Y;
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
  if(Merge->HotMergeZone != merge_zone::NONE)
  {
    for (u32 Index = 0; Index < ArrayCount(Merge->MergeZone); ++Index)
    {
      PushOverlayQuad(Merge->MergeZone[Index], Index == EnumToIdx(Merge->HotMergeZone) ? V4(0,1,0,0.5) : V4(0,1,0,0.3));
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
      PushOverlayQuad(Parent->Region, Color->Color);
    }

    if(Parent->Functions.Draw)
    {
      CallFunctionPointer(Parent->Functions.Draw, Interface, Parent);  
    }

    if(HasAttribute(Parent, ATTRIBUTE_TEXT))
    {
      text_attribute* Text = (text_attribute*) GetAttributePointer(Parent, ATTRIBUTE_TEXT);
      rect2f TextBox = GetTextSize(0, 0, Text->Text, Text->FontSize);
      PushTextAt(Parent->Region.X + Parent->Region.W/2.f - TextBox.W/2.f,
                     Parent->Region.Y + Parent->Region.H/2.f - TextBox.H/3.f, Text->Text, Text->FontSize, Text->Color);
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


u32 GetIntersectingNodes(u32 NodeCount, container_node* Container, v2 MousePos, u32 MaxCount, container_node** Result)
{
  u32 StackElementSize = sizeof(container_node*);
  u32 StackByteSize = NodeCount * StackElementSize;

  u32 StackCount = 0;
  temporary_memory TempMem = BeginTemporaryMemory(GlobalGameState->TransientArena);
  container_node** ContainerStack = PushArray(GlobalGameState->TransientArena, NodeCount, container_node*);

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
        Assert(IntersectingLeafCount < MaxCount);
        Result[IntersectingLeafCount++] = Parent;
      }
    }
  }
  EndTemporaryMemory(TempMem);
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


// Put Menu on top of the renderqueue and on the Focused Window Ptr and call it's "Gainding Focus"-Functions
// If Menu is 0 and Focused Window Ptr is not 0, call its LoosingFocus function and set Focused Window Ptr to 0
void SetFocusWindow(menu_interface* Interface, menu_tree* Menu)
{
  Assert(Menu != &Interface->MenuSentinel);

  if(Interface->MenuInFocus && Menu != Interface->MenuInFocus)
  {
    CallFunctionPointer(Interface->MenuInFocus->LosingFocus, Interface, Interface->MenuInFocus);
  }

  if(Menu)
  {
    if(Menu != Interface->MenuSentinel.Next)
    {
      // We put the menu in focus at the top of the queue so it get's rendered in the right order
      ListRemove( Menu );
      ListInsertAfter(&Interface->MenuSentinel, Menu); 
    }
    if(Menu != Interface->MenuInFocus)
    {
      CallFunctionPointer(Menu->GainingFocus, Interface, Menu);      
    }
  }

  Interface->MenuInFocus = Menu;
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

r32 PrintTree(u32 Count, container_node** HotLeafs, r32 YStart, u32 FontSize, r32 HeightStep, r32 WidthStep  )
{
  container_node* Node = HotLeafs[0];
  r32 XOff = 0;
  r32 YOff = YStart;
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

      for (u32 LeadIndex = 0; LeadIndex < Count; ++LeadIndex)
      {
        if (Sibling == HotLeafs[LeadIndex])
        {
          Color = V4(1,0,0,1);
        }
      }

      u32 Attributes = Sibling->Attributes;
      c8 StringBuffer[1024] = {};
      FormatNodeString(Sibling, ArrayCount(StringBuffer), StringBuffer);

      XOff = WidthStep * Sibling->Depth;
      PushTextAt(XOff, YOff, StringBuffer, FontSize, Color);
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

      for (u32 LeadIndex = 0; LeadIndex < Count; ++LeadIndex)
      {
        if (Sibling == HotLeafs[LeadIndex])
        {
          Color = V4(1,0,0,1);
        }
      }

      c8 StringBuffer[1024] = {};
      FormatNodeString(Sibling, ArrayCount(StringBuffer), StringBuffer);
      XOff = WidthStep * Sibling->Depth;
      PushTextAt(XOff, YOff, StringBuffer, FontSize, Color);
      YOff -= HeightStep;

      Sibling = Next(Sibling);
    }
  }

  return YStart-YOff;
}

void PrintHotLeafs(menu_interface* Interface)
{
  u32 FontSize = 24;
  stb_font_map* FontMap = GetFontMap(GlobalGameState->AssetManager, FontSize);
  game_window_size WindowSize = GameGetWindowSize();
  r32 HeightStep = (FontMap->Ascent - FontMap->Descent)/WindowSize.HeightPx;
  r32 WidthStep  = 0.02;
  r32 YOff = 1 - 2*HeightStep;

  menu_tree* Menu = Interface->MenuSentinel.Next;
  while(Menu != &Interface->MenuSentinel)
  {
    if(Menu->Visible && Menu->HotLeafCount > 0)
    {
       YOff -= PrintTree(Menu->HotLeafCount, Menu->HotLeafs, YOff, FontSize, HeightStep, WidthStep);
    }
    Menu = Menu->Next;
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

container_node* GetPluginWindow(menu_interface* Interface, container_node* Node)
{
  container_node* Result = 0;
  container_node* OwnRoot = GetRoot(Node);
  menu_tree* Menu = Interface->MenuSentinel.Next;
  
  menu_tree* MenuRoot = Interface->MenuSentinel.Next;
  while(MenuRoot != &Interface->MenuSentinel)
  {
    if(OwnRoot != MenuRoot->Root)
    {
      for (u32 LeafIndex = 0; LeafIndex < MenuRoot->HotLeafCount; ++LeafIndex)
      {
        container_node* HotLeafNode = MenuRoot->HotLeafs[LeafIndex];
        while(HotLeafNode)
        {
          if(HotLeafNode->Type == container_type::Plugin)
          {
            Result = HotLeafNode;
            return Result;
          }
          HotLeafNode = HotLeafNode->Parent;
        }
      }  
    }
    
    MenuRoot = MenuRoot->Next;
  }
  return Result;
}

menu_tree* GetMenu(menu_interface* Interface, container_node* Node)
{
  menu_tree* Result = 0;
  container_node* Root = GetRoot(Node);
  
  menu_tree* MenuRoot = Interface->MenuSentinel.Next;
  while(MenuRoot != &Interface->MenuSentinel)
  {
    if(Root == MenuRoot->Root)
    {
      Result = MenuRoot;
      break;
    }
    MenuRoot = MenuRoot->Next;
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

#if 0
// Could be nice to keep these around, theyre perfectly fine utility functions
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
#endif

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

void _PushToUpdateQueue(menu_interface* Interface, container_node* Caller, update_function** Function, void* Data)
{
  update_args* Entry = 0;
  for (u32 i = 0; i < ArrayCount(Interface->UpdateQueue); ++i)
  {
    Entry = &Interface->UpdateQueue[i];
    if(!Entry->InUse)
    {
      break;
    }
  }
  Assert(!Entry->InUse);

  Entry->Interface = Interface;
  Entry->Caller = Caller;
  Entry->InUse = true;
  Entry->Function = Function;
  Entry->Data = Data;
}

#define PushToUpdateQueue(Interface, Caller, FunctionName, Data) _PushToUpdateQueue(Interface, Caller, DeclareFunction(update_function, FunctionName), (void*) Data)

void RegisterEventWithNode(menu_interface* Interface, container_node* Node, u32 Handle)
{
  menu_event_handle_attribtue* EventHandles = (menu_event_handle_attribtue*) PushOrGetAttribute(Interface, Node, ATTRIBUTE_MENU_EVENT_HANDLE);
  EventHandles->Handles[EventHandles->HandleCount++] = Handle;
}


// From http://burtleburtle.net/bob/hash/integer.html
umm IntegerHash(umm a)
{
    a = (a ^ 61) ^ (a >> 16);
    a = a + (a << 3);
    a = a ^ (a >> 4);
    a = a * 0x27d4eb2d;
    a = a ^ (a >> 15);
    return a;
}
umm PointerToHash(void* Ptr)
{
  umm Val = (umm) Ptr;
  umm Result = IntegerHash(Val);
  return Result;
}

u32 GetEmptyMenuEventHandle(menu_interface* Interface, container_node* CallerNode)
{
  u32 MaxEventCount = ArrayCount(Interface->MenuEventCallbacks);
  Interface->MenuEventCallbackCount++;
  Assert(Interface->MenuEventCallbackCount < MaxEventCount);
  u32 Hash = (u32)(PointerToHash((void*)CallerNode) % MaxEventCount);
  menu_event* Event = &Interface->MenuEventCallbacks[Hash];
  u32 Handle = Hash;
  while(Event->Active)
  {
    ++Event;
    ++Handle;
    if(Handle >= MaxEventCount)
    {
      Handle = 0;
      Event = Interface->MenuEventCallbacks;
    }
  }
  ListInsertBefore(&Interface->EventSentinel, Event);
  return Handle;
}

menu_event* GetMenuEvent(menu_interface* Interface, u32 Handle)
{
  Assert(Handle < ArrayCount(Interface->MenuEventCallbacks));
  menu_event* Event = &Interface->MenuEventCallbacks[Handle];
  return Event;
}

void _RegisterMenuEvent(menu_interface* Interface, menu_event_type EventType, container_node* CallerNode, void* Data, menu_event_callback** Callback,  menu_event_callback** OnDelete)
{

  u32 Index = GetEmptyMenuEventHandle(Interface, CallerNode);
  menu_event* Event  = GetMenuEvent(Interface, Index);
  Assert(!Event->Active);
  Event->Active = true;
  Event->Index = Index;
  Event->CallerNode = CallerNode;
  Event->EventType = EventType;
  Event->Callback = Callback;
  Event->OnDelete = OnDelete;
  Event->Data = Data;
  RegisterEventWithNode(Interface, CallerNode, Index);
}

MENU_UPDATE_FUNCTION(SplitWindowBorderUpdate)
{
  container_node* BorderNode = Args->Caller;
  Assert(BorderNode->Type == container_type::Border);
  container_node* SplitNode = BorderNode->Parent;
  Assert(SplitNode->Type == container_type::Split);

  // The Caller is the border which we want to update
  border_leaf* Border = GetBorderNode(BorderNode);
  if(Border->Vertical)
  {
    Border->Position += (Interface->MousePos.X - Interface->PreviousMousePos.X)/SplitNode->Region.W;
  }else{
    Border->Position += (Interface->MousePos.Y - Interface->PreviousMousePos.Y)/SplitNode->Region.H;
  }
  
  return Interface->MouseLeftButton.Active;
}

MENU_EVENT_CALLBACK(InitiateSplitWindowBorderDrag)
{
  PushToUpdateQueue(Interface, CallerNode, SplitWindowBorderUpdate, 0);
}

MENU_UPDATE_FUNCTION(RootBorderDragUpdate)
{
  border_leaf* Border = GetBorderNode(Args->Caller);
  if(Border->Vertical)
  {
    Border->Position += Interface->MousePos.X - Interface->PreviousMousePos.X;
  }else{
    Border->Position += Interface->MousePos.Y - Interface->PreviousMousePos.Y;
  }
  return Interface->MouseLeftButton.Active;
}

MENU_EVENT_CALLBACK(InitiateBorderDrag)
{
  PushToUpdateQueue(Interface, CallerNode, RootBorderDragUpdate, 0);
}

container_node* CreateBorderNode(menu_interface* Interface, b32 Vertical, r32 Position, v4 Color)
{
  border_leaf Border = {};
  Border.Vertical = Vertical;
  Border.Position = Position;
  Border.Thickness = Interface->BorderSize;

  container_node* Result = NewContainer(Interface, container_type::Border);
  border_leaf* BorderLeaf = GetBorderNode(Result);
 
  color_attribute* ColorAttr = (color_attribute*) PushAttribute(Interface, Result, ATTRIBUTE_COLOR);
  ColorAttr->Color = Color;

  Assert(Result->Type == container_type::Border);
  *BorderLeaf = Border;
  return Result;
}

b32 IsPointerInArray(u32 ArrayCount, container_node** NodeArray, container_node* NodeToCheck, u32* Position)
{
  b32 Result = false;
  for (u32 i = 0; i < ArrayCount; ++i)
  {
    container_node* Node = NodeArray[i];
    if(Node == NodeToCheck)
    {
      Result = true;
      *Position = i;
      break;
    }
  }
  return Result;
}

// Destroys NewHotLeafs;
void SortHotLeafs(u32 OldHotLeafCount,   container_node** OldHotLeafs,
                  u32 NewHotLeafCount,   container_node** NewHotLeafs,
                  u32* NewCount,      container_node** New,
                  u32* RemovedCount,  container_node** Removed,
                  u32* ExistingCount, container_node** Existing)
{
  u32 NrNew = 0;
  u32 NrRemoved = 0;
  u32 NrExisting = 0;

  // Put all ptr's that exist in both arrays in Existing
  // And remove it from OldHotLeafs and NewHotLeafs
  for (u32 i = 0; i < OldHotLeafCount; ++i)
  {
    container_node* Leaf = OldHotLeafs[i];
    u32 Position = 0;
    if(IsPointerInArray(NewHotLeafCount, NewHotLeafs, Leaf, &Position))
    {
      Existing[NrExisting++] = Leaf;
      NewHotLeafs[Position] = 0;
      OldHotLeafs[i] = 0;
    }
  }
  // The leftover ptr's in NewHotLeafs gets put in New
  for (u32 i = 0; i < NewHotLeafCount; ++i)
  {
    if(NewHotLeafs[i])
    {
      New[NrNew++] = NewHotLeafs[i];
    }
  }
  // The leftover ptr's in OldHotLeafs gets put in Removed
  for (u32 i = 0; i < OldHotLeafCount; ++i)
  {
    if(OldHotLeafs[i])
    {
      Removed[NrRemoved++] = OldHotLeafs[i];
    }
  }
 
  *NewCount = NrNew;
  *RemovedCount = NrRemoved;
  *ExistingCount = NrExisting;
}

internal void UpdateHotLeafs(menu_interface* Interface, menu_tree* Menu)
{
  memory_arena* Arena = GlobalGameState->TransientArena;
  ScopedMemory Memory(Arena);

  // Get all new intersecting nodes
  u32 HotLeafsMaxCount = ArrayCount(Menu->HotLeafs);
  container_node** CurrentHotLeafs = (container_node**) PushArray(Arena, HotLeafsMaxCount, container_node*);
  u32 CurrentHotLeafCount = GetIntersectingNodes(Menu->NodeCount, Menu->Root, Interface->MousePos, HotLeafsMaxCount, CurrentHotLeafs);

  Assert(CurrentHotLeafCount < HotLeafsMaxCount);

  container_node** OldHotLeafs = (container_node**) PushCopy(Arena, HotLeafsMaxCount, Menu->HotLeafs);
  
  // Sort the newly gathered hot leafs into "new", "existing", "removed"
  u32 RemovedHotLeafsMaxCount = ArrayCount(Menu->RemovedHotLeafs);

  u32 NewCount = 0;
  u32 RemovedCount = 0;
  u32 ExistingCount = 0;
  container_node** New = PushArray(Arena, HotLeafsMaxCount, container_node*);
  container_node** Existing = PushArray(Arena, HotLeafsMaxCount, container_node*);
  container_node** Removed = PushArray(Arena, RemovedHotLeafsMaxCount, container_node*);
  SortHotLeafs(Menu->HotLeafCount,  OldHotLeafs,
               CurrentHotLeafCount, CurrentHotLeafs,
               &NewCount,           New,
               &RemovedCount,       Removed,
               &ExistingCount,      Existing);

  Assert(NewCount + ExistingCount < HotLeafsMaxCount);
  CopyArray( ExistingCount, Existing, Menu->HotLeafs);
  CopyArray( NewCount, New, Menu->HotLeafs+ExistingCount);
  Menu->HotLeafCount = NewCount + ExistingCount;
  Menu->NewLeafOffset = ExistingCount;
  
  Assert(RemovedCount < RemovedHotLeafsMaxCount);
  CopyArray( RemovedCount, Removed, Menu->RemovedHotLeafs);
  Menu->RemovedHotLeafCount = RemovedCount;
}


internal inline container_node* GetRootBody(container_node* RootWindow)
{
  Assert(RootWindow->Type == container_type::Root);
  container_node* Result = RootWindow->FirstChild->NextSibling->NextSibling->NextSibling->NextSibling->NextSibling;
  return Result;
}


internal container_node* PushTab(container_node* TabbedWindow,  container_node* Tab)
{
  Assert(TabbedWindow->Type == container_type::TabWindow); // Could also be split window
  container_node* TabHeader = TabbedWindow->FirstChild;
  Assert(TabHeader->Type == container_type::Grid);
  Assert(Tab->Type == container_type::Tab);

  ConnectNode(TabHeader, Tab);

  return Tab;
}


internal void SetTabAsActive(container_node* Tab)
{
  tab_node* TabNode = GetTabNode(Tab);
  
  container_node* TabHeader = Tab->Parent;
  Assert(TabHeader);
  container_node* TabWindow = TabHeader->Parent;
  Assert(TabWindow);
  container_node* TabBody = TabWindow->FirstChild->NextSibling;
  if(!TabBody)
  {
    ConnectNode(TabWindow, TabNode->Payload);
  }else{
    ReplaceNode(TabBody, TabNode->Payload);  
  }
}


internal container_node* PopTab(container_node* Tab)
{
  tab_node* TabNode = GetTabNode(Tab);
  DisconnectNode(TabNode->Payload);

  container_node* TabHeader = Tab->Parent;
  Assert(TabHeader->Type == container_type::Grid);
  container_node* TabWindow = TabHeader->Parent;
  Assert(TabWindow->Type == container_type::TabWindow);
  
  if(Next(Tab))
  {
    SetTabAsActive(Next(Tab));  
  }else if(Previous(Tab)){
    SetTabAsActive(Previous(Tab));
  }

  DisconnectNode(Tab);
  return Tab;
}


internal u32 FillArrayWithHBFs(u32 MaxArrSize, container_node* TabArr[], container_node* StartNode)
{
  temporary_memory TempMem =  BeginTemporaryMemory(GlobalGameState->TransientArena);

  u32 StackCount = 0;
  u32 TabCount = 0;
  container_node** ContainerStack = PushArray(GlobalGameState->TransientArena, MaxArrSize, container_node*);

  // Push StartNode
  ContainerStack[StackCount++] = StartNode;

  while(StackCount>0)
  {
    // Pop new parent from Stack
    container_node* Parent = ContainerStack[--StackCount];
    ContainerStack[StackCount] = 0;

    // Update the region of all children and push them to the stack
    if(Parent->Type == container_type::Split)
    {
      ContainerStack[StackCount++] = Next(Parent->FirstChild);
      ContainerStack[StackCount++] = Next(Next(Parent->FirstChild));
    }else if(Parent->Type == container_type::TabWindow)
    {
      container_node* TabHeader = Parent->FirstChild;
      container_node* Tab = TabHeader->FirstChild;
      while(Tab)
      {
        container_node* TabToMove = Tab;
        Tab = Next(Tab);
        
        tab_node* TabNode = GetTabNode(TabToMove);
        if(TabNode->Payload->Type == container_type::Plugin)
        {
          PopTab(TabToMove);
          TabArr[TabCount++] = TabToMove;
        }else{
          ContainerStack[StackCount++] = TabNode->Payload;
        }
      }
      Assert(TabCount < MaxArrSize);
    }else{
      INVALID_CODE_PATH;
    }
  }

  EndTemporaryMemory(TempMem);
  return TabCount;
}

internal void UpdateMergableAttribute( menu_interface* Interface, container_node* Node )
{
  container_node* PluginWindow = GetPluginWindow(Interface, Node);
  mergable_attribute* Merge = (mergable_attribute*) GetAttributePointer(Node, ATTRIBUTE_MERGE);
  Merge->HotMergeZone = merge_zone::NONE;
  if(PluginWindow)
  {
    rect2f Rect = PluginWindow->Region;
    r32 W = Rect.W;
    r32 H = Rect.H;
    r32 S = Minimum(W,H)/4;

    v2 MP = V2(Rect.X+W/2,Rect.Y+H/2); // Middle Point
    v2 LS = V2(MP.X-S, MP.Y);          // Left Square
    v2 RS = V2(MP.X+S, MP.Y);          // Right Square
    v2 BS = V2(MP.X,   MP.Y-S);        // Bot Square
    v2 TS = V2(MP.X,   MP.Y+S);        // Top Square

    Merge->MergeZone[(u32) merge_zone::CENTER] = Rect2f(MP.X-S/2.f, MP.Y-S/2.f,S/1.1f,S/1.1f);
    Merge->MergeZone[(u32) merge_zone::LEFT]   = Rect2f(LS.X-S/2.f, LS.Y-S/2.f,S/1.1f,S/1.1f);
    Merge->MergeZone[(u32) merge_zone::RIGHT]  = Rect2f(RS.X-S/2.f, RS.Y-S/2.f,S/1.1f,S/1.1f);
    Merge->MergeZone[(u32) merge_zone::TOP]    = Rect2f(BS.X-S/2.f, BS.Y-S/2.f,S/1.1f,S/1.1f);
    Merge->MergeZone[(u32) merge_zone::BOT]    = Rect2f(TS.X-S/2.f, TS.Y-S/2.f,S/1.1f,S/1.1f);

    if(Intersects(Merge->MergeZone[EnumToIdx(merge_zone::CENTER)], Interface->MousePos))
    {
      Merge->HotMergeZone = merge_zone::CENTER;
    }else if(Intersects(Merge->MergeZone[EnumToIdx(merge_zone::LEFT)], Interface->MousePos)){
      Merge->HotMergeZone = merge_zone::LEFT;
    }else if(Intersects(Merge->MergeZone[EnumToIdx(merge_zone::RIGHT)], Interface->MousePos)){
      Merge->HotMergeZone = merge_zone::RIGHT;
    }else if(Intersects(Merge->MergeZone[EnumToIdx(merge_zone::TOP)], Interface->MousePos)){
      Merge->HotMergeZone = merge_zone::TOP;
    }else if(Intersects(Merge->MergeZone[EnumToIdx(merge_zone::BOT)], Interface->MousePos)){
      Merge->HotMergeZone = merge_zone::BOT;
    }else{
      Merge->HotMergeZone = merge_zone::HIGHLIGHTED;
    }
  }

  if(Interface->MouseLeftButton.Edge &&
    !Interface->MouseLeftButton.Active )
  {
    merge_zone MergeZone = Merge->HotMergeZone;
    switch(Merge->HotMergeZone)
    {
      case merge_zone::CENTER:
      {
        container_node* HomeTabWindow = PluginWindow->Parent;
        container_node* Visitor = Node->Parent;
        Assert(HomeTabWindow->Type == container_type::TabWindow);

        if(Visitor->Type == container_type::Root)
        {
          Visitor = GetRootBody(Visitor);
        }

        container_node* TabArr[64] = {};
        u32 TabCount = FillArrayWithHBFs(ArrayCount(TabArr), TabArr, Visitor);

        menu_tree* MenuToRemove = GetMenu(Interface, Visitor);
        DisconnectNode(Visitor);

        for(u32 Index = 0; Index < TabCount; ++Index)
        {
          container_node* TabToMove = TabArr[Index];
          PushTab(HomeTabWindow, TabToMove);
        }

        SetTabAsActive(TabArr[TabCount-1]);
        
        FreeMenuTree(Interface, MenuToRemove);
      }break;
      case merge_zone::LEFT:    // Fallthrough 
      case merge_zone::RIGHT:   // Fallthrough
      case merge_zone::TOP:     // Fallthrough
      case merge_zone::BOT:
      {
        container_node* HomeTabWindow = PluginWindow->Parent;
        container_node* Visitor = Node->Parent;

        Assert(HomeTabWindow->Type == container_type::TabWindow);

        if(Visitor->Type == container_type::Root)
        {
          Visitor = GetRootBody(Visitor);
        }

        menu_tree* MenuToRemove = GetMenu(Interface, Visitor);
        menu_tree* MenuToRemain = GetMenu(Interface, HomeTabWindow);

        DisconnectNode(Visitor);

        b32 Vertical = (Merge->HotMergeZone == merge_zone::LEFT || Merge->HotMergeZone == merge_zone::RIGHT);

        container_node* SplitNode = CreateSplitWindow(Interface, Vertical );
        ReplaceNode(HomeTabWindow, SplitNode);

        b32 VisitorIsFirstChild = (Merge->HotMergeZone == merge_zone::LEFT || Merge->HotMergeZone == merge_zone::BOT);
        if(VisitorIsFirstChild)
        {
          ConnectNode(SplitNode, Visitor);
          ConnectNode(SplitNode, HomeTabWindow);
        }else{
          ConnectNode(SplitNode, HomeTabWindow);
          ConnectNode(SplitNode, Visitor);
        }

        FreeMenuTree(Interface, MenuToRemove);
      }break;
    }
  }
}

MENU_UPDATE_FUNCTION(WindowDragUpdate)
{
  container_node* Root = (container_node*) Args->Data;
  Assert(Root->Type == container_type::Root);
  container_node* Child = Root->FirstChild;
  while(Child->Type == container_type::Border)
  {
    border_leaf* Border = GetBorderNode(Child);
    UpdateFrameBorder(Interface, Border);
    Child = Next(Child);
  }

  UpdateMergableAttribute(Interface, Args->Caller);
  return Interface->MouseLeftButton.Active;
}

MENU_EVENT_CALLBACK(InitiateWindowDrag)
{
  PushToUpdateQueue(Interface, CallerNode, WindowDragUpdate, Data);
}


void DeregisterEvent(menu_interface* Interface, u32 Handle)
{
  menu_event* Event = GetMenuEvent(Interface, Handle);
  Interface->MenuEventCallbackCount--;
  ListRemove(Event);
  if(Event->OnDelete)
  {
    CallFunctionPointer(Event->OnDelete, Interface, Event->CallerNode, Event->Data);
  }
  *Event = {};
}

void ClearMenuEvents(menu_interface* Interface, container_node* Node)
{
  if(HasAttribute(Node, ATTRIBUTE_MENU_EVENT_HANDLE))
  {
    menu_event_handle_attribtue* EventHandles = (menu_event_handle_attribtue*) GetAttributePointer(Node, ATTRIBUTE_MENU_EVENT_HANDLE);
    for(u32 Idx = 0; Idx < EventHandles->HandleCount; ++Idx)
    {
      u32 Handle = EventHandles->Handles[Idx];
      DeregisterEvent(Interface, Handle);
      EventHandles->Handles[Idx] = 0;
    }
    EventHandles->HandleCount = 0;  
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

    //draggable_attribute* Draggable = (draggable_attribute*) PushAttribute(Interface, RootHeader, ATTRIBUTE_DRAG);
    //Draggable->NodeToDrag = ;

    RegisterMenuEvent(Interface, menu_event_type::MouseDown, RootHeader, Root->Root, InitiateWindowDrag, 0);
    //RegisterMenuEvent(Interface, menu_event_type::MouseUp, RootHeader, 0, ApplyWindowMerge, 0);

    PushAttribute(Interface, RootHeader, ATTRIBUTE_MERGE);
  }

  //  Root Node Complex, 4 Borders, 1 Header, 1 None
  container_node* RootBody = 0; // Output
  { 
    container_node* Border1 = CreateBorderNode(Interface, true, Region.X);
    ConnectNode(Root->Root, Border1);
    RegisterMenuEvent(Interface, menu_event_type::MouseDown, Border1, 0, InitiateBorderDrag, 0);
    container_node* Border2 = CreateBorderNode(Interface, true, Region.X + Region.W);
    ConnectNode(Root->Root, Border2);
    RegisterMenuEvent(Interface, menu_event_type::MouseDown, Border2, 0, InitiateBorderDrag, 0);
    container_node* Border3 = CreateBorderNode(Interface, false,Region.Y);
    ConnectNode(Root->Root, Border3);
    RegisterMenuEvent(Interface, menu_event_type::MouseDown, Border3, 0, InitiateBorderDrag, 0);
    container_node* Border4 = CreateBorderNode(Interface, false, Region.Y + Region.H);
    ConnectNode(Root->Root, Border4);
    RegisterMenuEvent(Interface, menu_event_type::MouseDown, Border4, 0, InitiateBorderDrag, 0);

    ConnectNode(Root->Root, RootHeader); // Header
    ConnectNode(Root->Root, BaseWindow); // Body
  }

  TreeSensus(Root);

  UpdateRegions(Root);

  UpdateHotLeafs(Interface, Root);

  SetFocusWindow(Interface, Root);

  return Root;
}

container_node* CreateTabWindow(menu_interface* Interface)
{
  container_node* TabWindow = NewContainer(Interface, container_type::TabWindow);
  GetTabWindowNode(TabWindow)->HeaderSize = 0.02f;

  container_node* TabbedHeader = ConnectNode(TabWindow, NewContainer(Interface, container_type::Grid));
  grid_node* Grid = GetGridNode(TabbedHeader);
  Grid->Row = 1;
  return TabWindow;
}

void ReduceSplitWindowTree(menu_interface* Interface, container_node* WindowToRemove)
{
  container_node* SplitNodeToSwapOut = WindowToRemove->Parent;
  container_node* WindowToRemain = WindowToRemove->NextSibling;
  if(!WindowToRemain)
  {
    WindowToRemain = WindowToRemove->Parent->FirstChild->NextSibling;
    Assert(WindowToRemain);
    Assert(WindowToRemain->NextSibling == WindowToRemove);
  }

  DisconnectNode(WindowToRemove);
  DeleteMenuSubTree(Interface, WindowToRemove);

  DisconnectNode(WindowToRemain);

  ReplaceNode(SplitNodeToSwapOut, WindowToRemain);
  DeleteMenuSubTree(Interface, SplitNodeToSwapOut);
}

void ReduceWindowTree(menu_interface* Interface, container_node* WindowToRemove)
{
  // Just to let us know if we need to handle another case
  Assert(WindowToRemove->Type == container_type::TabWindow);

  // Make sure the tab window is empty
  Assert(GetChildCount(WindowToRemove->FirstChild) == 0);

  container_node* ParentContainer = WindowToRemove->Parent;
  switch(ParentContainer->Type)
  {
    case container_type::Split:
    {
      ReduceSplitWindowTree(Interface, WindowToRemove);
    }break;
    case container_type::Root:
    {
      FreeMenuTree(Interface, GetMenu(Interface,ParentContainer));
    }break;
    default:
    {
      INVALID_CODE_PATH;
    } break;
  }
}

void SplitTabToNewWindow(menu_interface* Interface, container_node* Tab, v2 WindowSize, v2 HeaderOrigin)
{
  v2 MouseDownPos = Interface->MouseLeftButtonPush;

  v2 MouseDownHeaderFrame = MouseDownPos - HeaderOrigin;
  v2 NewOrigin = Interface->MousePos - MouseDownHeaderFrame - V2(0, WindowSize.Y - Interface->HeaderSize);
  rect2f NewRegion = Rect2f(NewOrigin.X, NewOrigin.Y, WindowSize.X, WindowSize.Y);
  NewRegion = Shrink(NewRegion, -Interface->BorderSize/2);

  container_node* NewTabWindow = CreateTabWindow(Interface);
  menu_tree* NewMenu = CreateNewRootContainer(Interface, NewTabWindow, NewRegion);
  
  PopTab(Tab);
  PushTab(NewTabWindow, Tab);
  container_node* RootHeader = Previous(NewTabWindow);
  ConnectNode(NewTabWindow, GetTabNode(Tab)->Payload);

  // Trigger the window-drag instead of this tab-drag
  PushToUpdateQueue(Interface, RootHeader, WindowDragUpdate, NewMenu->Root);
}

b32 TabDrag(menu_interface* Interface, container_node* Tab)
{
  Assert(Tab->Type == container_type::Tab);
  container_node* TabHeader = Tab->Parent;
  Assert(TabHeader->Type == container_type::Grid);
  container_node* TabWindow = TabHeader->Parent;
  Assert(TabWindow->Type == container_type::TabWindow);

  b32 Continue = Interface->MouseLeftButton.Active; // Tells us if we should continue to call the update function;

  SetTabAsActive(Tab);

  u32 Count = GetChildCount(TabHeader);
  if(Count > 1)
  {
    r32 dX = Interface->MousePos.X - Interface->PreviousMousePos.X;
    u32 Index = GetChildIndex(Tab);
    if(dX < 0 && Index > 0)
    {
      container_node* PreviousTab = Previous(Tab);
      Assert(PreviousTab);
      if(Interface->MousePos.X < (PreviousTab->Region.X +PreviousTab->Region.W/2.f))
      {
        ShiftLeft(Tab);
      }
    }else if(dX > 0 && Index < Count-1){
      container_node* NextTab = Next(Tab);
      Assert(NextTab);
      if(Interface->MousePos.X > (NextTab->Region.X + NextTab->Region.W/2.f))
      {
        ShiftRight(Tab);
      }
    }else if(SplitWindowSignal(Interface, Tab->Parent))
    {
      v2 HeaderOrigin = V2(TabHeader->Region.X, TabHeader->Region.Y);
      v2 WindowSize   = V2(TabWindow->Region.W, TabWindow->Region.H);
      SplitTabToNewWindow(Interface, Tab, WindowSize, HeaderOrigin);
      Continue = false;
    }
  }else if(TabWindow->Parent->Type == container_type::Split)
  {
    if(SplitWindowSignal(Interface, Tab->Parent))
    {
      v2 HeaderOrigin = V2(TabHeader->Region.X, TabHeader->Region.Y);
      v2 WindowSize   = V2(TabWindow->Region.W, TabWindow->Region.H);
      SplitTabToNewWindow(Interface, Tab, WindowSize, HeaderOrigin);
      ReduceWindowTree(Interface, TabWindow);
      Continue = false;
    }  
  }

  return Continue;
}

internal void GetInput(game_input* GameInput, menu_interface* Interface)
{
  Interface->PreviousMousePos = Interface->MousePos;
  Interface->MousePos = V2(GameInput->MouseX, GameInput->MouseY);
  
  Update(&Interface->TAB, GameInput->Keyboard.Key_TAB.EndedDown);
  if(Interface->TAB.Active && Interface->TAB.Edge)
  {
    Interface->MenuVisible = !Interface->MenuVisible;
  }

  Update(&Interface->MouseLeftButton, GameInput->MouseButton[PlatformMouseButton_Left].EndedDown);
  if(Interface->MouseLeftButton.Edge)
  {
    if(Interface->MouseLeftButton.Active )
    {
      Interface->MouseLeftButtonPush = Interface->MousePos;
    }else{
      Interface->MouseLeftButtonRelese = Interface->MousePos;
    }
  }
}

internal b32 CallMouseExitFunctions(menu_interface* Interface, menu_tree* Menu)
{
  b32 Result = false;
  for (u32 i = 0; i < Menu->RemovedHotLeafCount; ++i)
  {
    container_node* Node = Menu->RemovedHotLeafs[i];
    while(Node)
    {
      if(HasAttribute(Node,ATTRIBUTE_MENU_EVENT_HANDLE))
      {
        menu_event_handle_attribtue* Attr = (menu_event_handle_attribtue*) GetAttributePointer(Node, ATTRIBUTE_MENU_EVENT_HANDLE);
        for(u32 Idx = 0; Idx < Attr->HandleCount; ++Idx)
        {
          u32 Handle = Attr->Handles[Idx];
          menu_event* Event =   GetMenuEvent(Interface, Handle);
          if(Event->EventType == menu_event_type::MouseExit)
          {
            Result = true;
            CallFunctionPointer(Event->Callback, Interface, Node, Event->Data);
          }
        }
      }
      Node = Node->Parent;
    }
  }
  return Result;
}

internal b32 CallMouseEnterFunctions(menu_interface* Interface, menu_tree* Menu)
{
  b32 Result = false;
  u32 NewLeafCount = Menu->HotLeafCount - Menu->NewLeafOffset;
  for (u32 i = Menu->NewLeafOffset; i < NewLeafCount; ++i)
  {
    container_node* Node = Menu->HotLeafs[i];
    while(Node)
    {
      if(HasAttribute(Node,ATTRIBUTE_MENU_EVENT_HANDLE))
      {
        menu_event_handle_attribtue* Attr = (menu_event_handle_attribtue*) GetAttributePointer(Node, ATTRIBUTE_MENU_EVENT_HANDLE);
        for(u32 Idx = 0; Idx < Attr->HandleCount; ++Idx)
        {
          u32 Handle = Attr->Handles[Idx];
          menu_event* Event =   GetMenuEvent(Interface, Handle);
          if(Event->EventType == menu_event_type::MouseEnter)
          {
            Result = true;
            CallFunctionPointer(Event->Callback, Interface, Node, Event->Data);
          }
        }
      }
      Node = Node->Parent;
    }
  }
  return Result;
}

internal b32 CallMouseDownFunctions(menu_interface* Interface, menu_tree* Menu)
{
  b32 Result = false;
  for (u32 i = 0; i < Menu->HotLeafCount; ++i)
  {
    container_node* Node = Menu->HotLeafs[i];
    while(Node)
    {
      if(HasAttribute(Node,ATTRIBUTE_MENU_EVENT_HANDLE))
      {
        menu_event_handle_attribtue* Attr = (menu_event_handle_attribtue*) GetAttributePointer(Node, ATTRIBUTE_MENU_EVENT_HANDLE);
        for(u32 Idx = 0; Idx < Attr->HandleCount; ++Idx)
        {
          u32 Handle = Attr->Handles[Idx];
          menu_event* Event = GetMenuEvent(Interface, Handle);
          if(Event->EventType == menu_event_type::MouseDown)
          {
            Result = true;
            CallFunctionPointer(Event->Callback, Interface, Node, Event->Data);
          }
        }
      }

      Node = Node->Parent;
    }
  }
  return Result;
}

internal b32 CallMouseUpFunctions(menu_interface* Interface, menu_tree* Menu)
{
  b32 Result = false;
  for (u32 i = 0; i < Menu->HotLeafCount; ++i)
  {
    container_node* Node = Menu->HotLeafs[i];
    while(Node)
    {
      if(HasAttribute(Node,ATTRIBUTE_MENU_EVENT_HANDLE))
      {
        menu_event_handle_attribtue* Attr = (menu_event_handle_attribtue*) GetAttributePointer(Node, ATTRIBUTE_MENU_EVENT_HANDLE);
        for(u32 Idx = 0; Idx < Attr->HandleCount; ++Idx)
        {
          u32 Handle = Attr->Handles[Idx];
          menu_event* Event = GetMenuEvent(Interface, Handle);
          if(Event->EventType == menu_event_type::MouseUp)
          {
            Result = true;
            CallFunctionPointer(Event->Callback, Interface, Node, Event->Data);
          }
        }
      }
      Node = Node->Parent;
    }
  }
  return Result;
}

internal void UpdateFocusWindow(menu_interface* Interface)
{
  if(Interface->MouseLeftButton.Edge)
  {
    if(Interface->MouseLeftButton.Active )
    {
      menu_tree* MenuFocusWindow = 0;
      menu_tree* Menu = Interface->MenuSentinel.Next;
      while(Menu != &Interface->MenuSentinel)
      {
        if(Menu->Visible && Intersects(Menu->Root->Region, Interface->MousePos))
        {
          MenuFocusWindow = Menu;
          break;
        }
        Menu = Menu->Next;
      }
      SetFocusWindow(Interface, MenuFocusWindow);
    }
  }
}

internal void CallUpdateFunctions(menu_interface* Interface)
{
  for (u32 i = 0; i < ArrayCount(Interface->UpdateQueue); ++i)
  {
    update_args* Entry = &Interface->UpdateQueue[i];
    if(Entry->InUse)
    {
      b32 Continue = CallFunctionPointer(Entry->Function, Interface, Entry);
      if(!Continue)
      {
        *Entry = {};
      }
    }
  }
}


void UpdateAndRenderMenuInterface(game_input* GameInput, menu_interface* Interface)
{ 
  // Sets input variables to Interface
  GetInput(GameInput, Interface);
  if(!Interface->MenuVisible)
  {
    return;
  }

  // Updates all the hot leaf struct for the active window windows
  menu_tree* Menu = Interface->MenuSentinel.Next;
  while(Menu != &Interface->MenuSentinel)
  {
    UpdateHotLeafs(Interface, Menu);
    Menu = Menu->Next;
  }

  PrintHotLeafs(Interface);

  // Checks if a window was selected/deselected
  // * Sets or clears MenuFocusWindow
  // * Calls Gaining/Loosing Focus Functions
  UpdateFocusWindow(Interface);
  
  // Calls Mouse Exit on the top most window
  Menu = Interface->MenuSentinel.Next;
  b32 MouseEnterCalled = false;
  b32 MouseExitCalled = false;
  b32 MouseDownCalled = false;
  b32 MouseUpCalled = false;
  while(Menu != &Interface->MenuSentinel)
  {
    // If CallMouseEnter/ExitFunctions returns true, it means that a function wass successfully called
    // We only want to call it on the top most window
    if(!MouseExitCalled)
    {
      MouseExitCalled = CallMouseExitFunctions(Interface, Menu);
    }
    if(!MouseEnterCalled)
    {
      MouseEnterCalled = CallMouseEnterFunctions(Interface, Menu);
    }

    if(Menu->Visible)
    {
      if(!MouseDownCalled && Interface->MouseLeftButton.Edge && Interface->MouseLeftButton.Active)
      {
        MouseDownCalled = CallMouseDownFunctions(Interface, Menu);  
      }

      if(!MouseUpCalled && Interface->MouseLeftButton.Edge && !Interface->MouseLeftButton.Active)
      {
        MouseUpCalled = CallMouseUpFunctions(Interface, Menu);  
      }

      if(MouseExitCalled && MouseEnterCalled && MouseUpCalled && MouseDownCalled)
      {
        break;
      }
    }
    Menu = Menu->Next;
  }

  CallUpdateFunctions(Interface);

  if(Interface->MenuVisible)
  {
    Menu = Interface->MenuSentinel.Previous;
    while(Menu != &Interface->MenuSentinel)
    {
      if(Menu->Visible)
      {
        TreeSensus(Menu);
        UpdateRegions( Menu );
        DrawMenu( GlobalGameState->TransientArena, Interface, Menu->NodeCount, Menu->Root);
        PushNewRenderLevel(GlobalGameState->RenderCommands->OverlayGroup);
      }
      Menu = Menu->Previous;
    }
  }
}

void SetSplitDragAttribute(menu_interface* Interface, container_node* SplitNode, container_node* BorderNode)
{

}


container_node* CreatePlugin(menu_interface* Interface, c8* HeaderName, v4 HeaderColor, container_node* BodyNode)
{
  container_node* Plugin = NewContainer(Interface, container_type::Plugin);
  plugin_node* PluginNode = GetPluginNode(Plugin);
  str::CopyStringsUnchecked(HeaderName, PluginNode->Title);
  PluginNode->Color = HeaderColor;

  ConnectNode(Plugin, BodyNode);

  return Plugin;
}

container_node* CreateSplitWindow( menu_interface* Interface, b32 Vertical, r32 BorderPos)
{
  container_node* SplitNode  = NewContainer(Interface, container_type::Split);
  container_node* BorderNode = CreateBorderNode(Interface, Vertical, BorderPos);
  ConnectNode(SplitNode, BorderNode);
  RegisterMenuEvent(Interface, menu_event_type::MouseDown, BorderNode, 0, InitiateSplitWindowBorderDrag, 0);
  return SplitNode;
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
  ListInitiate(&Interface->MenuSentinel);
  ListInitiate(&Interface->EventSentinel);
 
  return Interface;
}

void DisplayOrRemovePluginTab(menu_interface* Interface, container_node* Tab)
{
  Assert(Tab->Type == container_type::Tab);
  container_node* TabHeader = Tab->Parent;
  if(TabHeader)
  {
    Assert(TabHeader->Type == container_type::Grid);
    container_node* TabWindow = TabHeader->Parent;

    u32 TabIndex = GetChildIndex(Tab);
    PopTab(Tab);
    if(GetChildCount(TabHeader) == 0)
    {
      ReduceWindowTree(Interface, TabWindow);
    }

  }else{

    container_node* TabWindow = 0;
    if(!Interface->SpawningWindow)
    {
      TabWindow = CreateTabWindow(Interface);
      
      Interface->SpawningWindow = CreateNewRootContainer(Interface, TabWindow, Rect2f( 0.25, 0.25, 0.7, 0.5));
    }else{
      TabWindow = GetRootBody(Interface->SpawningWindow->Root);
      while(TabWindow->Type != container_type::TabWindow)
      {
        if(TabWindow->Type ==  container_type::Split)
        {
          TabWindow = Next(TabWindow->FirstChild);
        }else{
          INVALID_CODE_PATH;
        }
      }
    }

    PushTab(TabWindow, Tab);

    container_node* Body = Next(TabWindow->FirstChild);
    tab_node* TabNode = GetTabNode(Tab);
    if(Body)
    {
      ReplaceNode(Body, TabNode->Payload);
    }else{
      ConnectNode(TabWindow, TabNode->Payload);
    }
  }

  SetFocusWindow(Interface, Interface->SpawningWindow);
}

MENU_EVENT_CALLBACK(DropDownMenuButton)
{
  menu_tree* Menu = (menu_tree*) Data;
  Menu->Root->Region.X = CallerNode->Region.X;
  Menu->Root->Region.Y = CallerNode->Region.Y - Menu->Root->Region.H;
  SetFocusWindow(Interface, Menu);
}

MENU_LOSING_FOCUS(DropDownLosingFocus)
{
  Menu->Visible = false;
}

MENU_GAINING_FOCUS(DropDownGainingFocus)
{
  Menu->Visible = true;
}

menu_tree* RegisterMenu(menu_interface* Interface, const c8* Name)
{
  r32 FontSize = 16;
  rect2f WindowsSize = GetTextSize(0.f, 0.f, Name, (u32)FontSize);

  game_window_size WindowSize = GameGetWindowSize();
  r32 AspectRatio = WindowSize.WidthPx/WindowSize.HeightPx;
  if(!Interface->MenuBar)
  {
    Interface->MenuBar = NewMenuTree(Interface);
    Interface->MenuBar->Visible = true;
    Interface->MenuBar->Root = NewContainer(Interface);
    r32 MainMenuHeight = WindowsSize.H * 1.1f;
    Interface->MenuBar->Root->Region = Rect2f(0, 1 - MainMenuHeight, AspectRatio, MainMenuHeight);
  
    container_node* DropDownContainer = ConnectNode(Interface->MenuBar->Root, NewContainer(Interface, container_type::Grid));
    grid_node* Grid = GetGridNode(DropDownContainer);
    Grid->Col = 0;
    Grid->Row = 1;
    Grid->TotalMarginX = 0.0;
    Grid->TotalMarginY = 0.0;

    size_attribute* SizeAttr = (size_attribute*) PushAttribute(Interface, DropDownContainer, ATTRIBUTE_SIZE);
    SizeAttr->Width = ContainerSizeT(menu_size_type::ABSOLUTE, WindowsSize.W+0.015f);
    SizeAttr->Height = ContainerSizeT(menu_size_type::RELATIVE, 1);
    SizeAttr->LeftOffset = ContainerSizeT(menu_size_type::ABSOLUTE, 0);
    SizeAttr->TopOffset = ContainerSizeT(menu_size_type::ABSOLUTE, 0);
    SizeAttr->XAlignment = menu_region_alignment::RIGHT;
    SizeAttr->YAlignment = menu_region_alignment::CENTER;
  }

  container_node* DropDownContainer = Interface->MenuBar->Root->FirstChild;
  container_node* NewMenu = ConnectNode(DropDownContainer, NewContainer(Interface));

  color_attribute* ColorAttr = (color_attribute*) PushAttribute(Interface, NewMenu, ATTRIBUTE_COLOR);
  ColorAttr->Color = V4(0.3,0,0.3,1);
  
  text_attribute* Text = (text_attribute*) PushAttribute(Interface, NewMenu, ATTRIBUTE_TEXT);
  str::CopyStringsUnchecked(Name, Text->Text);
  Text->FontSize = (u32) FontSize;
  Text->Color = V4(0.9,0.9,0.9,1);

  menu_tree* ViewMenuRoot = NewMenuTree(Interface);
  ViewMenuRoot->Root = NewContainer(Interface);
  ViewMenuRoot->Root->Region = {};
  ViewMenuRoot->LosingFocus = DeclareFunction(menu_losing_focus, DropDownLosingFocus);
  ViewMenuRoot->GainingFocus = DeclareFunction(menu_losing_focus, DropDownGainingFocus);

  container_node* ViewMenuItems = ConnectNode(ViewMenuRoot->Root, NewContainer(Interface, container_type::Grid));
  grid_node* Grid = GetGridNode(ViewMenuItems);
  Grid->Col = 1;
  Grid->Row = 0;
  Grid->TotalMarginX = 0.0;
  Grid->TotalMarginY = 0.0;

  ColorAttr = (color_attribute*) PushAttribute(Interface, ViewMenuItems, ATTRIBUTE_COLOR);
  ColorAttr->Color = V4(1,1,1,1);

  RegisterMenuEvent(Interface, menu_event_type::MouseDown, DropDownContainer, (void*) ViewMenuRoot, DropDownMenuButton, 0);

  return ViewMenuRoot;
}


MENU_UPDATE_FUNCTION(TabDragUpdate)
{
  b32 Continue = TabDrag(Args->Interface, Args->Caller);
  return Interface->MouseLeftButton.Active && Continue;
}

MENU_EVENT_CALLBACK(InitiateTabDrag)
{
  PushToUpdateQueue(Interface, CallerNode, TabDragUpdate, 0);
}

container_node* CreateTab(menu_interface* Interface, container_node* Plugin)
{
  container_node* Tab = NewContainer(Interface, container_type::Tab);

  plugin_node* PluginNode = GetPluginNode(Plugin);

  color_attribute* ColorAttr = (color_attribute*) PushAttribute(Interface, Tab, ATTRIBUTE_COLOR);
  ColorAttr->Color = PluginNode->Color;

  RegisterMenuEvent(Interface, menu_event_type::MouseDown, Tab, 0, InitiateTabDrag, 0);

  text_attribute* MenuText = (text_attribute*) PushAttribute(Interface, Tab, ATTRIBUTE_TEXT);
  str::CopyStringsUnchecked(PluginNode->Title, MenuText->Text);
  MenuText->FontSize = 12;
  MenuText->Color =  HexCodeToColorV4(0x000000);

  GetTabNode(Tab)->Payload = Plugin;
  GetPluginNode(Plugin)->Tab = Tab;
  return Tab;
}

MENU_EVENT_CALLBACK(DropDownMouseEnter)
{
  color_attribute* MenuColor = (color_attribute*) GetAttributePointer(CallerNode, ATTRIBUTE_COLOR);
  MenuColor->Color = V4(0.3,0.2,0.5,0.5);
}

MENU_EVENT_CALLBACK(DropDownMouseExit)
{
  color_attribute* MenuColor = (color_attribute*) GetAttributePointer(CallerNode, ATTRIBUTE_COLOR);
  MenuColor->Color = V4(1,1,1,0); 
}

MENU_EVENT_CALLBACK(DropDownMouseUp)
{ 
  DisplayOrRemovePluginTab(Interface, (container_node*) Data);
}

void RegisterWindow(menu_interface* Interface, menu_tree* DropDownMenu, container_node* Plugin)
{
  container_node* DropDownGrid = DropDownMenu->Root->FirstChild;
  Assert(DropDownGrid->Type == container_type::Grid);
  Assert(Plugin->Type == container_type::Plugin);

  plugin_node* PluginNode = GetPluginNode(Plugin);

  container_node* MenuItem = ConnectNode(DropDownGrid, NewContainer(Interface));
  text_attribute* MenuText = (text_attribute*) PushAttribute(Interface, MenuItem, ATTRIBUTE_TEXT);
  str::CopyStringsUnchecked(PluginNode->Title, MenuText->Text);
  MenuText->FontSize = 12;
  MenuText->Color = V4(0,0,0,1);

  color_attribute* MenuColor = (color_attribute*) PushAttribute(Interface, MenuItem, ATTRIBUTE_COLOR);
  MenuColor->Color = V4(1,1,1,1);


  rect2f TextSize = GetTextSize(0, 0, PluginNode->Title, MenuText->FontSize);
  DropDownMenu->Root->Region.H += TextSize.H;
  DropDownMenu->Root->Region.W = DropDownMenu->Root->Region.W >= TextSize.W ? DropDownMenu->Root->Region.W : TextSize.W;
 
  container_node* Tab = CreateTab(Interface, Plugin);

  Interface->PermanentWindows[Interface->PermanentWindowCount++] = Tab;


  RegisterMenuEvent(Interface, menu_event_type::MouseUp, MenuItem, Tab, DropDownMouseUp, 0);
  RegisterMenuEvent(Interface, menu_event_type::MouseEnter, MenuItem, Tab, DropDownMouseEnter, 0);
  RegisterMenuEvent(Interface, menu_event_type::MouseExit, MenuItem, Tab, DropDownMouseExit, 0);
}
