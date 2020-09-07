


u32 GetContainerSize(container_type Type)
{
  u32 Result = sizeof(container_node);
  switch(Type)
  {
    case container_type::None:          break;
    case container_type::Root:          {Result += sizeof(root_window);}break;
    case container_type::Body:
    case container_type::Empty:         {Result += sizeof(empty_window);}break;
    case container_type::Split:         {Result += sizeof(split_window);}break;
    case container_type::TabbedHeader:  {Result += sizeof(tabbed_header_window);}break;
    case container_type::MenuHeader:    {Result += sizeof(menu_header_window);}break;
    case container_type::ContainerList: {Result += sizeof(container_list);}break;
    case container_type::Button:        {Result += sizeof(menu_button);}break;
    case container_type::Profiler:      {Result += sizeof(profiling_window);}break;
    case container_type::Border:        {Result += sizeof(border_leaf);}break;
    case container_type::Header:        {Result += sizeof(header_leaf);}break;
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


#if 0
inline internal rect2f
GetRegionSize(rect2f ParentRegion, rect2f ChildRegion)
{
  rect2f Result = {};
  Result.X = ParentRegion.X + ParentRegion.W * ChildRegion.X;
  Result.Y = ParentRegion.Y + ParentRegion.H * ChildRegion.Y;
  Result.W = ParentRegion.W * ChildRegion.W;
  Result.H = ParentRegion.H * ChildRegion.H;
  return Result;
}

inline internal rect2f
GetRegionSize(container_node* Parent, container_node* Child)
{
  rect2f Result = GetRegionSize(Parent->Region, Child->SubRegion);

  Result.X += Child->LeftOffset;
  Result.Y += Child->BotOffset;
  Result.W -= Child->LeftOffset+Parent->RightOffset;
  Result.H -= Child->BotOffset+Parent->TopOffset;

  return Result;
}


void UpdateRegions( memory_arena* Arena, menu_tree* Menu )
{
  u32 StackElementSize = sizeof(container_node*);
  u32 StackByteSize = Menu->NodeCount * StackElementSize;

  u32 StackCount = 0;
  container_node** ContainerStack = PushArray(Arena, Menu->NodeCount, container_node*);

  // Push Root
  ContainerStack[StackCount++] = Menu->Root;

  while(StackCount>0)
  {
    // Pop new parent from Stack
    container_node* Parent = ContainerStack[--StackCount];
    ContainerStack[StackCount] = 0;

    // Update the region of all children and push them to the stack
    container_node* Child = Parent->FirstChild;
    while(Child)
    {
      Child->Region = GetRegionSize(Parent, Child);
      ContainerStack[StackCount++] = Child;
      Child = Child->NextSibling;
    }
  }
}
#else

internal inline u32
GetChildCount(container_node* Node)
{
  u32 Result = 0;
  container_node* Child = Node->FirstChild;
  while(Child)
  {
    Child = Child->NextSibling;
    Result++;
  }
  return Result;
}

internal inline container_node*
GetNextHorizontalBorder(container_node* Node)
{
  container_node* Result = 0;
  while(Node)
  {
    if(Node->Type == container_type::Border &&
      !GetContainerPayload(border_leaf, Node)->Vertical)
    {
      Result = Node;
      break;  
    }
    Node = Node->NextSibling;
  }
  return Result;
}

internal inline container_node*
GetNextVerticalBorder(container_node* Node)
{
  container_node* Result = 0;
  while(Node)
  {
    if(Node->Type == container_type::Border &&
       GetContainerPayload(border_leaf, Node)->Vertical)
    {
      Result = Node;
      break;  
    }
    Node = Node->NextSibling;
  }
  return Result;
}
border_leaf* GetBorder(container_node* Node)
{
  border_leaf* Result = GetContainerPayload(border_leaf,Node);
  return Result;
}

container_node* GetNextBody(container_node* Node)
{
  container_node* Result = 0;
  while(Node)
  {
    if(Node->Type != container_type::Border)
    {
      Result = Node;
      break;
    }
    Node = Node->NextSibling;
  }
  return Result;
}

internal inline u32
GetVerticalBorders(container_node* Node, container_node** NodeArray)
{
  container_node* Child = Node->FirstChild;
  container_node* Sibling = GetNextVerticalBorder(Child);
  u32 Count = 0;
  while(Sibling)
  {
    container_node* Border = Sibling;
    for (u32 i = 0; i < Count; ++i)
    {
      if(GetBorder(Border)->Position < GetBorder(NodeArray[i])->Position)
      {
        container_node* Swp = NodeArray[i];
        NodeArray[i] = Border;
        Border = Swp;
      }
    }
    NodeArray[Count++] = Border;
    Sibling = GetNextVerticalBorder(Sibling->NextSibling);
  }
  return Count;
}

internal inline u32
GetHorizontalBorders(container_node* Node, container_node** NodeArray)
{
  container_node* Child = Node->FirstChild;
  container_node* Sibling = GetNextHorizontalBorder(Child);
  u32 Count = 0;
  while(Sibling)
  {
    container_node* Border = Sibling;
    for (u32 i = 0; i < Count; ++i)
    {
      if(GetBorder(Border)->Position < GetBorder(NodeArray[i])->Position)
      {
        container_node* Swp = NodeArray[i];
        NodeArray[i] = Border;
        Border = Swp;
      }
    }

    NodeArray[Count++] = Border;
    Sibling = GetNextHorizontalBorder(Sibling->NextSibling);
  }
  return Count;
}

internal inline u32
GetRows(u32 HorizontalBorderCount)
{
  u32 Result = (HorizontalBorderCount <= 1) ? HorizontalBorderCount + 1 : HorizontalBorderCount-1;
  return Result;
}


internal inline u32
GetColumns( u32 VerticalBorderCount )
{
  u32 Result  = (VerticalBorderCount <= 1) ? VerticalBorderCount + 1 : VerticalBorderCount-1;
  return Result;
}


internal inline void
UpdateHorizontalBorders(s32 HorizontalBorderCount, container_node** HorizontalBorders, rect2f Surroundings, rect2f SubWindow)
{ 
  for(s32 BorderIndex = 0;
      BorderIndex < HorizontalBorderCount; BorderIndex++)
  {
    border_leaf* Border = GetBorder( HorizontalBorders[BorderIndex]);
    r32 HT = Border->Thickness*0.5f;

    r32 Y0 = Surroundings.Y + Border->Position * Surroundings.H;

    HorizontalBorders[BorderIndex]->Region = Rect2f(
      SubWindow.X - HT,
      Y0 - HT,
      SubWindow.W+Border->Thickness,
      Border->Thickness);
  }
}

internal inline void
UpdateVerticalBorders(s32 VerticalBorderCount, container_node** VerticalBorders, rect2f Surroundings, rect2f SubWindow)
{
  for(s32 BorderIndex = 0; BorderIndex < VerticalBorderCount; ++BorderIndex)
  {
    border_leaf* Border = GetBorder(VerticalBorders[BorderIndex]);
    r32 HT = Border->Thickness*0.5f;

    r32 X0 =Surroundings.X + Border->Position*Surroundings.W;
    
    VerticalBorders[BorderIndex]->Region = Rect2f(
      X0 - HT ,
      SubWindow.Y - HT,
      Border->Thickness,
      SubWindow.H + Border->Thickness);
  }
}

internal inline rect2f
GetSubRegion(u32 Row, u32 Column, 
  u32 HorizontalBorderCount, container_node** HorizontalBorders,
  u32 VerticalBorderCount,   container_node** VerticalBorders, rect2f Surroundings)
{
  rect2f Result = Surroundings;

  if(VerticalBorderCount == 1)
  {
    Assert(Column == 0 || Column == 1);

    rect2f BorderRegion = VerticalBorders[0]->Region;
    if(Column == 0)
    {
      Result.X =  Surroundings.X;
      Result.W =  BorderRegion.X - Surroundings.X;
    }else{
      Result.X = BorderRegion.X+BorderRegion.W;
      Result.W = (Surroundings.X + Surroundings.W) - Result.X;  
    }
  }else if(VerticalBorderCount > 1){
    Assert((Column + 1)< VerticalBorderCount );
    rect2f LeftBorder = VerticalBorders[Column]->Region;
    rect2f RightBorder = VerticalBorders[Column+1]->Region;
    Result.X = LeftBorder.X + LeftBorder.W;
    Result.W = RightBorder.X - Result.X;
  }

  if(HorizontalBorderCount == 1)
  {
    Assert(Row == 0 || Row == 1);

    rect2f BorderRegion = HorizontalBorders[0]->Region;
    if(Row == 0)
    {
      Result.Y = BorderRegion.Y + BorderRegion.H;
      Result.H = Surroundings.Y + Surroundings.H - Result.Y;
    }else{
      Result.Y = Surroundings.Y;
      Result.H = BorderRegion.Y - Result.Y;  
    }
  }else if(HorizontalBorderCount > 1){
    Assert((Row + 1)<HorizontalBorderCount);
    rect2f TopBorder = HorizontalBorders[Row+1]->Region;
    rect2f BotBorder = HorizontalBorders[Row]->Region;
    Result.Y = BotBorder.Y + BotBorder.H;
    Result.H = TopBorder.Y - Result.Y;
  }
  return Result;
}

void SetChildRegions(memory_arena* Arena,container_node* Node)
{
  temporary_memory TempMem =  BeginTemporaryMemory(Arena);

  u32 ChildCount = GetChildCount(Node);


  rect2f Surroundings = Node->Region;
  rect2f SubRegion = Surroundings;

  container_node** HorizontalBorders = PushArray(Arena,ChildCount,container_node*);
  u32 HorizontalBorderCount = GetHorizontalBorders(Node, HorizontalBorders);
  if(HorizontalBorderCount > 1)
  {
    r32 Min = R32Max;
    r32 Max = R32Min;
    for (u32 i = 0; i < HorizontalBorderCount; ++i)
    {
      Min = Minimum(Min, GetBorder(HorizontalBorders[i])->Position);
      Max = Maximum(Max, GetBorder(HorizontalBorders[i])->Position);
    }
    r32 Y0 = Surroundings.Y + Min*Surroundings.H;
    r32 Y1 = Surroundings.Y + Max*Surroundings.H;
    SubRegion.Y = Y0;
    SubRegion.H = Y1-Y0;
  }

  container_node** VerticalBorders = PushArray(Arena,ChildCount,container_node*);
  u32 VerticalBorderCount =  GetVerticalBorders(Node, VerticalBorders);
  if(VerticalBorderCount > 1)
  {
    r32 Min = R32Max;
    r32 Max = R32Min;
    for (u32 i = 0; i < VerticalBorderCount; ++i)
    {
      Min = Minimum(Min, GetBorder(VerticalBorders[i])->Position);
      Max = Maximum(Max, GetBorder(VerticalBorders[i])->Position);
    }
    r32 X0 = Surroundings.X + Min*Surroundings.W;
    r32 X1 = Surroundings.X + Max*Surroundings.W;
    SubRegion.X = X0;
    SubRegion.W = X1-X0;
  }

  UpdateHorizontalBorders(HorizontalBorderCount, HorizontalBorders, Surroundings, SubRegion);
  UpdateVerticalBorders(VerticalBorderCount, VerticalBorders, Surroundings, SubRegion);

  if(VerticalBorderCount || HorizontalBorderCount)
  {
    u32 RowCount = GetRows(HorizontalBorderCount);
    u32 ColumnCount = GetColumns(VerticalBorderCount);
    u32 NodeCount = 0;
    u32 Row = 0;
    u32 Column = 0;
    container_node* Body = GetNextBody(Node->FirstChild);
    while (Body)
    {
      rect2f SubWindow = GetSubRegion( Row, Column,
          HorizontalBorderCount, HorizontalBorders,
          VerticalBorderCount, VerticalBorders, SubRegion);

      Column++;
      if(Column >= ColumnCount)
      {
        Row++;
        Column=0;
      }
      Body->Region = SubWindow;
      Body = GetNextBody(Body->NextSibling);
      NodeCount++;
    }
    Assert(RowCount * ColumnCount == NodeCount); 
  }else{

    container_node* Child = Node->FirstChild;
    while(Child)
    {
      Child->Region = Node->Region;
      Child = Child->NextSibling;
    }

    container_node* HeaderNode = Node->FirstChild;
    r32 Y0 = Node->Region.Y + Node->Region.H;
    while(HeaderNode)
    {
      if(HeaderNode->Type == container_type::Header)
      {
        header_leaf* Header = GetContainerPayload(header_leaf, HeaderNode);
        HeaderNode->Region = Node->Region;
        HeaderNode->Region.Y = Y0 - Header->Thickness;
        HeaderNode->Region.H =  Header->Thickness;
        Y0 -= Header->Thickness;
        break;
      }
      HeaderNode = HeaderNode->NextSibling;
    }

    container_node* EmptyBody = Node->FirstChild;
    while(EmptyBody)
    {
      if(EmptyBody->Type == container_type::Empty)
      {
        EmptyBody->Region = Node->Region;
        EmptyBody->Region.Y = Node->Region.Y;
        EmptyBody->Region.H = Y0 - Node->Region.Y;
        break;
      }
      EmptyBody = EmptyBody->NextSibling;
    }
    
    /*
    container_node* Body = Node->FirstChild;
    r32 X0 = Surroundings.X;
    r32 Y0 = Surroundings.Y + Surroundings.H;
    r32 YStep = 0;
    while(Body)
    {

    }
    
    
    if(Body->Fixed[2])
    {
      Body->Region.Y = Y0 - Body->Size.Y;
      YStep = Max(YStep, Body->Size.Y);
    }else{

    }
    if(Body->Fixed[0])
    {
      Body->Region.X = X0 + Body->Size.X;
      X0 += Body->Size.X;
    }
    while (Body)
    {
      rect2f SubRegion = GetSubRegion( Row, Column,
          HorizontalBorderCount, HorizontalBorders,
          VerticalBorderCount, VerticalBorders, Surroundings);
      if(Row >= RowCount)
      {
        Row = 0;
        Column++;
      }
      Body->Region = SubRegion;
      Body = GetNextBody(Body->NextSibling);
      NodeCount++;
    } 
    */ 
  }
  

#if 0
  container_node* Child =  Node->FirstChild;

  v2 Cursor = V2(Surroundings.X, Surroundings.Y + Surroundings.H);
  while(Child)
  {
    switch(Child->Type)
    {
      case container_type::Border:
      {
        if(Child->Fixed[0])
        {
          Assert(!Child->Fixed[2]);
          Cursor.X += Child->Region.W;
        }else{
          Assert(Child->Fixed[2]);
          Assert(!Child->Fixed[0]);
          Cursor.Y -= Child->Region.H;
        }
      }break;
      case container_type::Body:
      {

        // Note: For now we don't support Mixing (Child->Fixed[0] && Child->Fixed[2]) with Mixed Fixed Bodies because
        //       Im not sure how they should interact with each other. I don't have a use case either for when they
        //       would be necessary.
        //       For example:
        //       Fixed Bodies could fill it's surroundings like a grid. ( Child->Fixed[0] &&  Child->Fixed[2])
        //       Fixed[0] Bodies could fill it's surroundings in Columns. ( Child->Fixed[0] && !Child->Fixed[2]) 
        //       Fixed[2] Bodies could fill it's surroundings in Rows     (!Child->Fixed[0] &&  Child->Fixed[2])
        //       Not sure how multiple UnFixed would fill all of it's surroundings in Rows

        r32 X0, Y0, W, H;
        if(Child->Fixed[0])
        {
          W = Child->Size.X;
          Cursor.X += W;
        }else{
          container_node* Border = GetNextVerticalBorder(Child);

          if(Border)
          {
            W = Border->Region.X - Cursor.X;
            Cursor.X = Border->Region.X + Border->Region.W;
            if(Cursor.X >= (Surroundings.X + Surroundings.W))
            {
              Cursor.X  = Surroundings.X;
            }
          }else{
            W = Surroundings.W - (Cursor.X - Surroundings.X);
            Cursor.X = Border->Region.X + Border->Region.W;
          }
          Assert(W>0);
        }

        if(Child->Fixed[2])
        {
          H = Child->Size.Y;
          Cursor.Y -= H;
        }else{
          container_node* Border = GetNextHorizontalBorder(Child);
          if(Border)
          {
            H = Cursor.Y - (Border->Region.Y + Border->Region.H);
            Cursor.Y = Border->Region.Y;
            if(Cursor.Y < Surroundings.Y)
            {
              Cursor.X  = Surroundings.X;
            }
          }else{
            H = Cursor.Y - Surroundings.Y;
          }
          Assert(H>0);
        }

        if(Child->Free)
        {
          X0 = Child->Offset.X;
          Y0 = Child->Offset.Y;
        }else{
          X0 = Cursor.X;
          Y0 = Cursor.Y - H; 
        }

        Child->Region = Rect2f(X0,Y0,W,H);
      }break;
    }
    Child = Child->NextSibling;
  }
  #endif
  EndTemporaryMemory(TempMem);
}

void UpdateRegions( memory_arena* Arena, menu_tree* Menu )
{
  temporary_memory TempMem =  BeginTemporaryMemory(Arena);

  u32 StackElementSize = sizeof(container_node*);
  u32 StackByteSize = Menu->NodeCount * StackElementSize;

  u32 StackCount = 0;
  container_node** ContainerStack = PushArray(Arena, Menu->NodeCount, container_node*);

  // Push Root
  ContainerStack[StackCount++] = Menu->Root;

  while(StackCount>0)
  {
    // Pop new parent from Stack
    container_node* Parent = ContainerStack[--StackCount];
    ContainerStack[StackCount] = 0;

    // Update the region of all children and push them to the stack
    SetChildRegions(Arena, Parent);
    container_node* Child = Parent->FirstChild;
    while(Child)
    {
      ContainerStack[StackCount++] = Child;
      Child = Child->NextSibling;
    }
  }

  EndTemporaryMemory(TempMem);
}
#endif

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
void DrawMenu( memory_arena* Arena, menu_interface* Interface, menu_tree* Menu )
{
  u32 StackElementSize = sizeof(container_node*);
  u32 StackByteSize = Menu->NodeCount * StackElementSize;

  u32 StackCount = 0;
  container_node** ContainerStack = PushArray(Arena, Menu->NodeCount, container_node*);

  // Push Root
  ContainerStack[StackCount++] = Menu->Root;

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
  u32 FunctionIndex = 0;
  Interface->Activate[FunctionIndex++] = toggle_colliders;
  Interface->Activate[FunctionIndex++] = toggle_multi_thread;
  Interface->Activate[FunctionIndex++] = toggle_aabb_tree;
  Interface->Activate[FunctionIndex++] = toggle_collision_points;
  Interface->Activate[FunctionIndex++] = recompile;
  Assert(FunctionIndex == ArrayCount(Interface->Activate));
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



void SetMouseInput(memory_arena* Arena, game_input* GameInput, menu_interface* Interface)
{
  v2 MousePos = V2(GameInput->MouseX, GameInput->MouseY);

  Update(&Interface->MouseLeftButton, GameInput->MouseButton[PlatformMouseButton_Left].EndedDown);

  if( Interface->MouseLeftButton.Edge )
  {
    if(Interface->MouseLeftButton.Active )
    {
      Interface->MouseLeftButtonPush = MousePos;
      Interface->HotLeafCount = GetIntersectingNodes(Arena,  Interface->RootContainers[0].NodeCount, Interface->RootContainers[0].Root, Interface->MousePos, Interface->HotLeaves);
    }else{
      Interface->MouseLeftButtonRelese = MousePos;
      Interface->HotLeafCount = 0;
    }
  }
  Interface->MousePos = MousePos;
}

void ActOnInput(memory_arena* Arena, menu_interface* Interface, menu_tree* Menu)
{
  v2 MousePos = Interface->MousePos;

  Assert(Menu->Root);

  for (u32 i = 0; i <  Interface->HotLeafCount; ++i)
  {
    rect2f R = Interface->HotLeaves[i]->Region;
    Platform.DEBUGPrint("%2.2f, %2.2f, %2.2f, %2.2f\n", R.X, R.Y, R.W, R.H );
    Interface->HotLeaves[i]->Functions.HandleInput(Interface, Interface->HotLeaves[i], 0);
  }
#if 0
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
  #endif
}

void UpdateAndRenderMenuInterface(game_input* GameInput, menu_interface* Interface)
{
  SetMouseInput(GlobalGameState->TransientArena, GameInput, Interface);

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

  ActOnInput(GlobalGameState->TransientArena, Interface, &Interface->RootContainers[0]);
  for (s32 WindowIndex = Interface->RootContainerCount-1;
           WindowIndex >= 0;
         --WindowIndex)
  {
    menu_tree* MenuTree = &Interface->RootContainers[WindowIndex];
    TreeSensus(MenuTree);
    UpdateRegions( GlobalGameState->TransientArena, MenuTree );
    DrawMenu( GlobalGameState->TransientArena, Interface, MenuTree->NodeCount, MenuTree->Root);
  }
}

container_node* PushBorder(menu_interface* Interface, container_node* Parent, border_leaf Border)
{
  container_node* Node = NewContainer(Interface, container_type::Border);
  *GetContainerPayload(border_leaf, Node) = Border;
  ConnectNode(Parent, Node);
  return Node;
}

container_node* PushEmptyWindow(menu_interface* Interface,  container_node* Parent, v4 Color = V4(0.2,0.4,0.2,1))
{
  container_node* Node = NewContainer(Interface, container_type::Empty);
  GetContainerPayload(empty_window, Node)->Color = Color;
  ConnectNode(Parent, Node);
  return Node;
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

  Interface->HotLeafCount = 0;

  container_node* Sentinel = &(Interface->Sentinel);
  ListInitiate(Sentinel);

#if 0
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
#endif

/*
  // Order and type determines position and actions

   Note: - Elements can be
            * Vertical   (Relative size in Height, Absolute size in Width)
            * Horizontal (Relative size in Width,  Absolute size in Height)
            * Both       (Block Container with relative size as % of parent window).
            * None       (Block Container with absolute size, they stack up first left to right, then top to bottom).
         - Order goes from Top->Bot, Left->Right
         - Order doesn't matter Between Orthogonal elements.  {Top,Left} = {Left,Top}
         - Corners are regions where Borders intersect
         - Only BlockContainers can be Nodes with Children.
         - Everything else is leaves.

    RootContainer:
      -----------------------
      Border, Vertical    // VerticalBorder at beginning is a Left border  , Absolute Height, Relative Width
      Border, Horizontal  // HorizontalBorder at beginning is a top border , Absolute Width,  Relative Height
      -----------------------
      Header,Vertical    // A Vertical container region,                  , Absolute Height, Relative Width
      -----------------------
      Container,Vertical, Horizontal // Just a container region,          , Relative Width,  Relative Height
      -----------------------
      Border,Vertical    (Top)
      Border,Horizontal  (Left)
      ------------------------

    Horizontal SplitWindow:
      -----------------------
      Container, Vertical, Horizontal
      -----------------------
      Border,Vertical
      -----------------------
      Container, Vertical, Horizontal
      -----------------------

Example Window: A  H-split window with Button grid in the top widnow and Profiling window in the bottom

  Container           (Container) (Root, Defines valid drag-region for borders and headers)
      VBorder         (V-Container)
      HBorder         (H-Container)
      Header          (V-Container)
      Container       (V-H-Container)
        Container     (V-H-Container)  -> A grid of buttons
          Button      (Container)
          Button      (Container)
          Button      (Container)
          Button      (Container)
        HBorder       (H-Container)
        Container     (V-H-Container)
          ProfilerA   (V-Container) - Some Profiler Widget
          ProfilerB   (V-Container) - Some Profiler Widget
      VBorder
      HBorder


struct container_node
{
  // Memory
  u32 ContainerSize;
  container_type Type;

  // List (Memory)
  container_node* Next;
  container_node* Previous;


  // Tree (Menu Structure)
  u32 Depth;
  container_node* Parent;
  container_node* FirstChild;
  container_node* NextSibling;

  // b32 Horizontal;
  // b32 Vertical;

};

*/

/*
    {
      menu_tree* Root = GetNewMenuTree(Interface);
      Root->Root = NewContainer(Interface, container_type::Root);

      container_node* RootContainer = Root->Root;
      RootContainer->Region = Rect2f(0.6,0.2,0.5,0.5);
      RootContainer->SubRegion = Rect2f(0,0,1,1);
      root_window* RootWindow = GetContainerPayload(root_window,Root->Root);
      RootWindow->BorderSize = Interface->BorderSize;
      RootWindow->HeaderSize = Interface->HeaderSize;
      RootWindow->MinSize = Interface->MinSize;


      container_node* Container = NewContainer(Interface, container_type::Split);

      // Do a function called something  like GetOffsetFrom(  root_window*, container_node* )
      Container->TopOffset = RootWindow->BorderSize + RootWindow->HeaderSize;
      Container->BotOffset = RootWindow->BorderSize;
      Container->LeftOffset = RootWindow->BorderSize;
      Container->RightOffset = RootWindow->BorderSize;

      //split_window* Split = GetContainerPayload(split_window, Container);
      //RootContainer->TopOffset = Interface->BorderSize + Interface->HeaderSize;
      //RootContainer->BotOffset = Interface->BorderSize;
      //Split->BorderSize = Interface->BorderSize;
      //Split->MinSize = 0.1;
      //Split->SplitFraction = 0.5;

      container_node*  EmptyContainer0 = NewContainer(Interface, container_type::Empty);
      GetContainerPayload(empty_window, EmptyContainer0)->Color = V4(0,0.4,0,1);

      container_node*  EmptyContainer1 = NewContainer(Interface, container_type::Empty);
      // Do a function called something  like GetOffsetFrom(  split_window*, container_node* )
      Container->TopOffset = RootWindow->BorderSize + RootWindow->HeaderSize;
      Container->BotOffset = RootWindow->BorderSize;
      Container->LeftOffset = RootWindow->BorderSize;
      Container->RightOffset = RootWindow->BorderSize;
      EmptyContainer1->SubRegion = Rect2f(0, 0, 1, 1/2.f);
      GetContainerPayload(empty_window, EmptyContainer1)->Color = V4(0,0.4,0,1);



      container_node*  EmptyContainer2 = NewContainer(Interface, container_type::Empty);
      EmptyContainer2->SubRegion = Rect2f(0, 1/2.f, 1, 1/2.f);
      GetContainerPayload(empty_window, EmptyContainer2)->Color = V4(0.4,0,0,1);

      ConnectNode(0, RootContainer);
      ConnectNode(RootContainer,   EmptyContainer0);
      ConnectNode(EmptyContainer0, EmptyContainer1);
      ConnectNode(EmptyContainer0, EmptyContainer2);

      UpdateRegions( &DebugState->Arena, Root);
    }
*/
  {
    r32 BorderSize = 0.01;
    r32 HeaderSize = 0.07;

    v4 BorderColor = V4(0,0,0.4,1);
    v4 CornerColor = V4(0,0.4,0.2,1);
    v4 BodyColor   = V4(0.2,0.4,0.2,1);
    v4 HeaderColor = V4(0.4,0.2,0.2,1);

    menu_tree* Root = GetNewMenuTree(Interface);
    Root->Root = NewContainer(Interface, container_type::None);

    game_window_size WindowSize = GameGetWindowSize();
    r32 Width = WindowSize.WidthPx/WindowSize.HeightPx;

    container_node* R = Root->Root;
    R->Region = Rect2f(0,0,Width,1);
    
    PushBorder(Interface, R, Border(false, 0.25));
    PushBorder(Interface, R, Border(false, 0.75));
    PushBorder(Interface, R, Border(true,  0.25));
    PushBorder(Interface, R, Border(true,  0.75));

#if 1
    container_node* HW = ConnectNode(R, NewContainer(Interface, container_type::None));
    container_node* HeaderNode = ConnectNode(HW, NewContainer(Interface, container_type::Header));
    header_leaf* Header = GetContainerPayload(header_leaf, HeaderNode);
    Header->Color = V4(0.2,0.2,0.2,1);
    Header->Thickness = 0.1;
    
    container_node* HB = ConnectNode(HW, NewContainer(Interface, container_type::None));
    PushBorder(Interface, HB, Border(true));
    PushEmptyWindow(Interface, HB, V4(0.2,0,0,1));
    PushEmptyWindow(Interface, HB, V4(0,0.2,0,1));
    //container_node* W = ConnectNode(R, NewContainer(Interface, container_type::None));
#else
    container_node* HW = ConnectNode(R, NewContainer(Interface, container_type::None));
    container_node* H = ConnectNode(HW, NewContainer(Interface, container_type::None));
    container_node* W = ConnectNode(HW, NewContainer(Interface, container_type::None));

    PushBorder(Interface, W, Border(true,  0.5));

    container_node* C1 = ConnectNode(W, NewContainer(Interface, container_type::None));
    PushBorder(Interface, C1, Border(false, 0.5, 2*BorderColor));
    PushEmptyWindow(Interface, C1, BodyColor+V4(0.2,0,0,0));
    PushEmptyWindow(Interface, C1, BodyColor+V4(0,0.2,0,0));

    container_node* C2 = ConnectNode(W, NewContainer(Interface, container_type::None));
    PushBorder(Interface, C2, Border(false, 0.5, 2*BorderColor));
    PushEmptyWindow(Interface, C2, BodyColor+V4(0,0.2,0,0));
    PushEmptyWindow(Interface, C2, BodyColor+V4(0.2,0,0,0));
#endif


    TreeSensus(Root);

    UpdateRegions( GlobalGameState->TransientArena, Root);
  }

  return Interface;
}