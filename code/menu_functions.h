
#include "debug.h"

window_regions CheckRegions( v2 MousePos, u32 RegionCount, window_regions* RegionArray, container_node* Node )
{
  for (u32 RegionIndex = 0; RegionIndex < RegionCount; ++RegionIndex)
  {
    window_regions RegionEnum = RegionArray[RegionIndex];
    if(Intersects(Node->Functions.GetRegionRect(RegionEnum, Node), MousePos))
    {
      return RegionEnum;
    }
  }
  return window_regions::None;
}

MENU_MOUSE_DOWN( MouseDown )
{

}
MENU_MOUSE_UP( MouseUp )
{

}
MENU_MOUSE_ENTER( MouseEnter )
{

}
MENU_MOUSE_EXIT( MouseExit )
{

}
MENU_HANDLE_INPUT( HandleInput )
{

}
MENU_GET_REGION_RECT( GetRegionRect )
{
  return Node->Region;
}
MENU_GET_CHILD_REGION( GetChildRegion )
{
  return Child->Region;
}
MENU_GET_MOUSE_OVER_REGION( GetMouseOverRegion )
{
  if(Intersects(Node->Region, MousePos))
  {
    return window_regions::WholeBody;
  }
  return window_regions::None;
}
MENU_DRAW( Draw )
{

}

menu_functions GetDefaultFunctions()
{
  menu_functions Result = {};
  Result.MouseDown = MouseDown;
  Result.MouseUp = MouseUp;
  Result.MouseEnter = MouseEnter;
  Result.MouseExit = MouseExit;
  Result.HandleInput = HandleInput;
  Result.GetChildRegion = GetChildRegion;
  Result.GetRegionRect = GetRegionRect;
  Result.GetMouseOverRegion = GetMouseOverRegion;
  Result.Draw = Draw;
  return Result;
}

MENU_DRAW( EmptyDraw )
{
  DEBUGPushQuad(Node->Region, GetContainerPayload(empty_window, Node)->Color);
}

menu_functions GetEmptyFunctions()
{
  menu_functions Result = GetDefaultFunctions();
  Result.Draw = EmptyDraw;
  return Result;
}

internal inline
v2 NewWindowPos( v2 DraggingStart, v2 MousePos, v2 MouseDownPos)
{
  game_window_size WindowSize = GameGetWindowSize();
  r32 Width = WindowSize.WidthPx/WindowSize.HeightPx;
  r32 Boundary = 0.01;
  rect2f Limit = Rect2f(Boundary,Boundary,
                        Width-2*Boundary,1-2*Boundary);

  if(MousePos.X < Limit.X)
  {
    MousePos.X = Limit.X;
  }
  if( MousePos.X > (Limit.X + Limit.W))
  {
    MousePos.X = (Limit.X + Limit.W);
  }
  if(MousePos.Y < Limit.Y)
  {
    MousePos.Y = Limit.Y;
  }
  if(MousePos.Y > (Limit.Y + Limit.H))
  {
    MousePos.Y = (Limit.Y + Limit.H);
  }

  v2 Delta = MousePos - MouseDownPos;
  v2 Result = DraggingStart + Delta;
  return Result;
}

MENU_HANDLE_INPUT( RootWindowHandleInput )
{
  root_window* Window = GetContainerPayload(root_window, Node);

  rect2f R = Node->Region;
  r32 W = Node->Region.W;
  r32 H = Node->Region.H;
  r32 BS = Window->BorderSize;
  r32 HS = Window->HeaderSize;
  r32 X0 = Node->Region.X;
  r32 Y0 = Node->Region.Y;
  r32 X1 = X0 + W;
  r32 Y1 = Y0 + H;
  
  rect2f Header         = Rect2f(X0 + BS, Y1-HS-BS, W-2*BS, HS);
  rect2f LeftBorder     = Rect2f(X0,      Y0 + BS, BS, H - 2*BS);
  rect2f RightBorder    = Rect2f(X1 - BS, Y0 + BS, BS, H - 2*BS);
  rect2f BotBorder      = Rect2f(X0 + BS, Y0,    W - 2*BS, BS);  
  rect2f TopBorder      = Rect2f(X0 + BS, Y1-BS, W - 2*BS, BS);  
  rect2f BotLeftCorner  = Rect2f(X0,    Y0,      BS, BS); 
  rect2f BotRightCorner = Rect2f(X1-BS, Y0,      BS, BS); 
  rect2f TopLeftCorner  = Rect2f(X0,    Y1 - BS, BS, BS); 
  rect2f TopRightCorner = Rect2f(X1-BS, Y1 - BS, BS, BS); 

  v2 MousePos = Interface->MousePos;
  if(Interface->MouseLeftButton.Edge)
  {
    if(Interface->MouseLeftButton.Active)
    {
      Window->DraggingStart = Node->Region;

      if(Intersects(Header, MousePos))
      {
        Window->HeaderDrag = true;
      }
      else if(Intersects(LeftBorder, MousePos))
      {
        Window->LeftBorderDrag = true;
      }
      else if(Intersects(RightBorder, MousePos))
      {
        Window->RightBorderDrag = true;
      }
      else if(Intersects(BotBorder, MousePos))
      {
        Window->BotBorderDrag = true;
      }
      else if(Intersects(TopBorder, MousePos))
      {
        Window->TopBorderDrag = true;
      }
      else if(Intersects(BotLeftCorner, MousePos))
      {
        Window->BotBorderDrag = true;
        Window->LeftBorderDrag = true;
      }
      else if(Intersects(BotRightCorner, MousePos))
      {
        Window->BotBorderDrag = true;
        Window->RightBorderDrag = true;
      }
      else if(Intersects(TopLeftCorner, MousePos))
      {
        Window->TopBorderDrag = true;
        Window->LeftBorderDrag = true;
      }
      else if(Intersects(TopRightCorner, MousePos))
      {
        Window->TopBorderDrag = true;
        Window->RightBorderDrag = true;
      }
    }else{
      Window->HeaderDrag      = false;
      Window->LeftBorderDrag  = false;
      Window->RightBorderDrag = false;
      Window->BotBorderDrag   = false;
      Window->TopBorderDrag   = false;
    }
  }else{
    if(Interface->MouseLeftButton.Active)
    {
      r32 MinWidth  = Window->MinSize + 2*Window->BorderSize;
      r32 MinHeight = Window->MinSize + 2*Window->BorderSize;

      v2 Delta = Interface->MousePos - Interface->MouseLeftButtonPush;

      if(Window->HeaderDrag)
      {
        v2 NewPos = NewWindowPos(V2(Window->DraggingStart.X,Window->DraggingStart.Y), MousePos, Interface->MouseLeftButtonPush);
        Node->Region.X = NewPos.X;
        Node->Region.Y = NewPos.Y;
      }

      if(Window->RightBorderDrag)
      {
        game_window_size WindowSize = GameGetWindowSize();
        r32 Width = WindowSize.WidthPx/WindowSize.HeightPx;

        r32 NewWidth = Window->DraggingStart.W + Delta.X;
        if(Window->DraggingStart.X + NewWidth > Width )
        {
          NewWidth = Width - Window->DraggingStart.X;
        }else if(NewWidth < MinWidth)
        {
          NewWidth = MinWidth;
        }
        Node->Region.W = NewWidth;
      }

      if(Window->LeftBorderDrag)
      {
        r32 NewXPos   = Window->DraggingStart.X + Delta.X;
        r32 NewWidth  = Window->DraggingStart.W - Delta.X;
        if(NewXPos < 0)
        {
          NewXPos = 0;
          NewWidth = (Window->DraggingStart.X) + Window->DraggingStart.W;
        }else if(NewWidth < MinWidth)
        {
          NewXPos  = Window->DraggingStart.X + Window->DraggingStart.W - MinWidth;
          NewWidth = MinWidth;
        }
        Node->Region.X = NewXPos;
        Node->Region.W = NewWidth;
      }

      if(Window->BotBorderDrag)
      {
        r32 NewYPos   = Window->DraggingStart.Y + Delta.Y;
        r32 NewHeight = Window->DraggingStart.H - Delta.Y;
        if(NewYPos < 0)
        {
          NewYPos = 0;
          NewHeight = (Window->DraggingStart.Y) + Window->DraggingStart.H;
        }else if(NewHeight < MinHeight)
        {
          NewYPos = Window->DraggingStart.Y + Window->DraggingStart.H - MinHeight;
          NewHeight = MinHeight;
        }
        Node->Region.Y = NewYPos;
        Node->Region.H = NewHeight;
      }

      if(Window->TopBorderDrag)
      {
        r32 NewHeight = Window->DraggingStart.H + Delta.Y;
        if((Window->DraggingStart.Y + NewHeight) > 1)
        {
          NewHeight = 1 - Window->DraggingStart.Y;
        }else if(NewHeight < MinHeight)
        {
          NewHeight = MinHeight;
        }
        Node->Region.H = NewHeight;
      }
    }
  }
}

MENU_DRAW( RootWindowDraw )
{
  root_window* Window = GetContainerPayload(root_window, Node);
  v4 CornerColor = V4(1,0,1,1);
  v4 BorderColor = V4(0,0,1,1);
  v4 HeaderColor = V4(1,0,0,1);

  rect2f R = Node->Region;
  r32 W = Node->Region.W;
  r32 H = Node->Region.H;
  r32 BS = Window->BorderSize;
  r32 HS = Window->HeaderSize;
  r32 X0 = Node->Region.X;
  r32 Y0 = Node->Region.Y;
  r32 X1 = X0 + W;
  r32 Y1 = Y0 + H;
  
  rect2f Header         = Rect2f(X0 + BS, Y1-HS-BS, W-2*BS, HS);
  rect2f LeftBorder     = Rect2f(X0,      Y0 + BS, BS, H - 2*BS);
  rect2f RightBorder    = Rect2f(X1 - BS, Y0 + BS, BS, H - 2*BS);
  rect2f BotBorder      = Rect2f(X0 + BS, Y0,    W - 2*BS, BS);  
  rect2f TopBorder      = Rect2f(X0 + BS, Y1-BS, W - 2*BS, BS);  
  rect2f BotLeftCorner  = Rect2f(X0,    Y0,      BS, BS); 
  rect2f BotRightCorner = Rect2f(X1-BS, Y0,      BS, BS); 
  rect2f TopLeftCorner  = Rect2f(X0,    Y1 - BS, BS, BS); 
  rect2f TopRightCorner = Rect2f(X1-BS, Y1 - BS, BS, BS); 

  DEBUGPushQuad(LeftBorder, BorderColor); // Left Border
  DEBUGPushQuad(RightBorder, BorderColor); // Right Border

  DEBUGPushQuad(BotBorder, BorderColor);   // Bot Border
  DEBUGPushQuad(TopBorder, BorderColor);   // Top Border

  DEBUGPushQuad(BotLeftCorner, CornerColor);  // Bot Left Corner
  DEBUGPushQuad(BotRightCorner, CornerColor);  // Bot Right Corner
  DEBUGPushQuad(TopLeftCorner, CornerColor);  // Top Left Corner
  DEBUGPushQuad(TopRightCorner, CornerColor);  // Top Right Corner

  DEBUGPushQuad(Rect2f(X0 + BS, Y1-HS-BS, W-2*BS, HS),  HeaderColor);   // Header
}

menu_functions GetRootMenuFunctions()
{
  menu_functions Result = GetDefaultFunctions();
  Result.HandleInput = RootWindowHandleInput;
  Result.Draw = RootWindowDraw;
  return Result;
}


MENU_MOUSE_DOWN( MenuHeaderMouseDown )
{
  menu_header_window* Window = GetContainerPayload(menu_header_window, Node);
  rect2f RootContainer = Window->RootWindow->Region;
  Window->DraggingStart = V2(RootContainer.X,RootContainer.Y);
  switch(HotRegion)
  {
    case window_regions::Header:
    {
      Window->WindowDrag = true;
    }break;
    default:
    {
      Assert(0);
    }break;
  }
}

r32 GetSplitFraction(r32 ParentSize, r32 ChildSize, r32 MinSize)
{
  r32 ResultSize = Minimum(ChildSize, ParentSize-MinSize);
  r32 Result = Maximum(ResultSize/ParentSize, 0.05f);
  return Result;
}

MENU_MOUSE_UP( MenuHeaderMouseUp )
{
  menu_header_window* Window = GetContainerPayload(menu_header_window, Node);
  Window->WindowDrag = false;
  if(Window->NodeToMerge)
  {
    Assert(Window->NodeToMerge->Type == container_type::TabbedHeader);
    u32 ZoneIndex = Window->HotMergeZone;
    if( ZoneIndex == 1 && Node->FirstChild->Type == container_type::TabbedHeader)
    {
      // Middle
      tabbed_header_window* SrcTabbedHeader = GetContainerPayload(tabbed_header_window, Node->FirstChild);
      tabbed_header_window* DstTabbedHeader = GetContainerPayload(tabbed_header_window, Window->NodeToMerge);
      DstTabbedHeader->SelectedTabOrdinal = SrcTabbedHeader->SelectedTabOrdinal + DstTabbedHeader->TabCount;
      for(u32 TabIndex = 0; TabIndex < SrcTabbedHeader->TabCount; TabIndex++)
      {
        
        DstTabbedHeader->Tabs[DstTabbedHeader->TabCount++] = SrcTabbedHeader->Tabs[TabIndex];
        Assert( DstTabbedHeader->TabCount < ArrayCount(DstTabbedHeader->Tabs))
      }

      DisconnectNode(Node->FirstChild);
      FreeMenuTree(Interface, &Interface->RootContainers[0]);

    }else if( ZoneIndex == 0  || ZoneIndex == 2 ||  // Vertical
              ZoneIndex == 3  || ZoneIndex == 4)    // Horizontal
    {
      container_node* OppositeNode = Window->NodeToMerge;
      container_node* SplitContainer = NewContainer(Interface, container_type::Split);
      split_window* SplitWindow = GetContainerPayload(split_window, SplitContainer);
      SplitWindow->BorderSize = 0.007f;
      SplitWindow->MinSize = 0.02f;
      SplitWindow->VerticalSplit = (ZoneIndex == 0  || ZoneIndex == 2);

      container_node* CommonParent = OppositeNode->Parent;
      rect2f ParentRegion = {};
      if(CommonParent->Type ==  container_type::Split)
      {
        ParentRegion = CommonParent->Region;
        if(CommonParent->FirstChild == OppositeNode)
        {
          container_node* Sibling = OppositeNode->NextSibling;
          DisconnectNode(Sibling);
          DisconnectNode(OppositeNode);

          ConnectNode(CommonParent, SplitContainer);
          ConnectNode(CommonParent, Sibling);
        }else{
          DisconnectNode(OppositeNode);
          ConnectNode(CommonParent, SplitContainer);  
        }
      }else{
        DisconnectNode(OppositeNode);
        ConnectNode(CommonParent, SplitContainer);
        ParentRegion = CommonParent->Functions.GetRegionRect(window_regions::WholeBody, CommonParent);
      }
      container_node* NodeToConnect = Node->FirstChild;
      DisconnectNode(NodeToConnect);
      
      if( ZoneIndex == 0  || ZoneIndex == 3 )
      {
        // Left || Bot
        ConnectNode(SplitContainer, NodeToConnect);
        ConnectNode(SplitContainer, OppositeNode);

        r32 Fraction = 0;
        if(SplitWindow->VerticalSplit){
          Fraction = GetSplitFraction(ParentRegion.W, NodeToConnect->Region.W, Interface->MinSize);
        }else{
          Fraction = GetSplitFraction(ParentRegion.H, NodeToConnect->Region.H, Interface->MinSize);
        }
        SplitWindow->SplitFraction = Fraction;
      }else if(ZoneIndex == 2  ||  ZoneIndex == 4){  
        // Right || Top
        ConnectNode(SplitContainer, OppositeNode);
        ConnectNode(SplitContainer, NodeToConnect);
        r32 Fraction = 0;
        if(SplitWindow->VerticalSplit){
          Fraction = GetSplitFraction(ParentRegion.W, NodeToConnect->Region.W, Interface->MinSize);
        }else{
          Fraction = GetSplitFraction(ParentRegion.H, NodeToConnect->Region.H, Interface->MinSize);
        }
        SplitWindow->SplitFraction = 1.f-Fraction;
      }

      Assert(OppositeNode->Type == container_type::TabbedHeader);
  
      FreeMenuTree(Interface, &Interface->RootContainers[0]);
    }
  }
}

MENU_MOUSE_ENTER( MenuHeaderMouseEnter )
{

}
MENU_MOUSE_EXIT( MenuHeaderMouseExit )
{

}


MENU_HANDLE_INPUT( MenuHeaderHandleInput )
{
  menu_header_window* Window = GetContainerPayload(menu_header_window, Node);

  if(Window->WindowDrag)
  {
    container_node* NodeToMerge = 0;
    debug_state* DebugState = DEBUGGetState();
    Window->NodeToMerge = 0;
    for (u32 WindowIndex = 1;
         WindowIndex < Interface->RootContainerCount;
         ++WindowIndex)
    {
      menu_tree Menu = Interface->RootContainers[WindowIndex];
      node_region_pair NodeRegion = GetRegion(&DebugState->Arena, Menu.NodeCount, Menu.Root, Interface->MousePos);
      if(NodeRegion.Region == window_regions::WholeBody ||
         NodeRegion.Region == window_regions::BodyOne  ||
         NodeRegion.Region == window_regions::BodyTwo )
      {
        while(NodeRegion.Node && NodeRegion.Node->Type != container_type::TabbedHeader)
        {
          NodeRegion.Node = NodeRegion.Node->Parent;
        }
        
        if(NodeRegion.Node)
        {
          Window->NodeToMerge = NodeRegion.Node;
          break;
        }
      } 
    }

    if(Window->NodeToMerge)
    {
      Assert(Window->NodeToMerge->Type == container_type::TabbedHeader);

      rect2f Rect = Window->NodeToMerge->Functions.GetRegionRect(window_regions::WholeBody, Window->NodeToMerge);
      r32 W = Rect.W;
      r32 H = Rect.H;
      r32 S = Minimum(W,H)/4;

      v2 MP = V2(Rect.X+W/2,Rect.Y+H/2); // Middle Point
      v2 LQ = V2(MP.X-S, MP.Y);          // Left Quarter
      v2 RQ = V2(MP.X+S, MP.Y);          // Right Quarter
      v2 BQ = V2(MP.X,   MP.Y-S);        // Bot Quarter
      v2 TQ = V2(MP.X,   MP.Y+S);        // Top Quarter

      Window->MergeZone[0] = Rect2f(LQ.X-S/2.f, LQ.Y-S/2.f,S/1.1f,S/1.1f); // Left Quarter
      Window->MergeZone[1] = Rect2f(MP.X-S/2.f, MP.Y-S/2.f,S/1.1f,S/1.1f); // Middle Point
      Window->MergeZone[2] = Rect2f(RQ.X-S/2.f, RQ.Y-S/2.f,S/1.1f,S/1.1f); // Right Quarter
      Window->MergeZone[3] = Rect2f(BQ.X-S/2.f, BQ.Y-S/2.f,S/1.1f,S/1.1f); // Bot Quarter
      Window->MergeZone[4] = Rect2f(TQ.X-S/2.f, TQ.Y-S/2.f,S/1.1f,S/1.1f); // Top Quarter

      if(Intersects(Window->MergeZone[0], Interface->MousePos))
      {
        Window->HotMergeZone = 0;
      }else if(Intersects(Window->MergeZone[1], Interface->MousePos)){
        Window->HotMergeZone = 1;
      }else if(Intersects(Window->MergeZone[2], Interface->MousePos)){
        Window->HotMergeZone = 2;
      }else if(Intersects(Window->MergeZone[3], Interface->MousePos)){
        Window->HotMergeZone = 3;
      }else if(Intersects(Window->MergeZone[4], Interface->MousePos)){
        Window->HotMergeZone = 4;
      }else{
        Window->HotMergeZone = ArrayCount(Window->MergeZone);
      }
    }

    v2 NewPos = NewWindowPos(Window->DraggingStart, Interface->MousePos, Interface->MouseLeftButtonPush);
    Window->RootWindow->Region.X = NewPos.X;
    Window->RootWindow->Region.Y = NewPos.Y;
  }
}

MENU_GET_REGION_RECT( MenuHeaderGetRegionRect )
{
  rect2f Result = {};
  r32 XStart = 0;
  r32 YStart = 0;
  r32 Width  = 0;
  r32 Height = 0;

  rect2f Region = Node->Region;

  r32 HeaderSize = GetContainerPayload(menu_header_window,Node)->HeaderSize;

  switch(Type)
  {
    case window_regions::WholeBody:
    {
      XStart = Region.X;
      YStart = Region.Y;
      Width  = Region.W;
      Height = Region.H - HeaderSize;
    }break;
    case window_regions::Header:
    {
      XStart = Region.X;
      YStart = Region.Y + Region.H - HeaderSize;
      Width  = Region.W;
      Height = HeaderSize;
    }break;
    default:
    {
      Assert(0);
    }break;
  }

  Result = Rect2f(XStart, YStart, Width, Height);
  return Result;
}

MENU_GET_MOUSE_OVER_REGION( MenuHeaderGetMouseOverRegion )
{
  window_regions RegionArray[] =
  {
    window_regions::WholeBody,
    window_regions::Header
  };

  menu_header_window* Window = GetContainerPayload(menu_header_window, Node);

  window_regions Result = CheckRegions(MousePos, ArrayCount(RegionArray), RegionArray, Node);

  return Result;
}


MENU_DRAW( MenuHeaderDraw )
{
  menu_header_window* Window = GetContainerPayload(menu_header_window, Node);
  v4 HeaderColor = V4(0.8,0.2,0.2,1);
  //DEBUGPushQuad(Node->Functions.GetRegionRect(window_regions::Header, Node), HeaderColor);
  DEBUGPushQuad(Node->Region, HeaderColor);
  if(Window->NodeToMerge)
  {
    rect2f Rect = Window->NodeToMerge->Functions.GetRegionRect(window_regions::WholeBody, Window->NodeToMerge);

    for (u32 Index = 0; Index < ArrayCount(Window->MergeZone); ++Index)
    {
      if(!(Index == 1 && Node->FirstChild->Type != container_type::TabbedHeader))
      {
        v4 Color = V4(0.3,0.5,0.3,0.5);
        if(Index == Window->HotMergeZone)
        {
          Color = V4(0.3,0.5,0.3,0.7);
        }
        DEBUGPushQuad(Window->MergeZone[Index], Color); 
      }
    }
  }
}

menu_functions MenuHeaderMenuFunctions()
{
  menu_functions Result = {};
  Result.MouseDown = MenuHeaderMouseDown;
  Result.MouseUp = MenuHeaderMouseUp;
  Result.MouseEnter = MenuHeaderMouseEnter;
  Result.MouseExit = MenuHeaderMouseExit;
  Result.HandleInput = MenuHeaderHandleInput;
  Result.GetRegionRect = MenuHeaderGetRegionRect;
  Result.GetMouseOverRegion = MenuHeaderGetMouseOverRegion;
  Result.Draw = MenuHeaderDraw;
  return Result;
}

rect2f GetTabRegion(rect2f HeaderRect, u32 TabIndex, u32 TabCount)
{
  r32 TabWidth = HeaderRect.W / (r32) TabCount;
  rect2f Result = HeaderRect;
  Result.W = TabWidth;
  Result.X = HeaderRect.X + TabIndex * TabWidth;
  return Result;
}

MENU_MOUSE_DOWN( TabbedHeaderMouseDown )
{
  Platform.DEBUGPrint("Tabbed Header Mouse Down\n");
  tabbed_header_window* TabbedHeader = GetContainerPayload(tabbed_header_window,Node);

  v2 MousePos = Interface->MousePos;
  rect2f HeaderRegion = Node->Functions.GetRegionRect(window_regions::Header, Node);
  for(u32 TabIndex = 0; TabIndex < TabbedHeader->TabCount; TabIndex++)
  {
    rect2f TabRegion = GetTabRegion(HeaderRegion, TabIndex, TabbedHeader->TabCount);
    if(Intersects(TabRegion,MousePos))
    {
      TabbedHeader->SelectedTabOrdinal = TabIndex+1;
      DisconnectNode(Node->FirstChild);
      ConnectNode(Node,TabbedHeader->Tabs[TabIndex]);
      TabbedHeader->TabDrag = true;
      TabbedHeader->TabMouseOffset = MousePos - V2(TabRegion.X, TabRegion.Y);
      break;
    }
  }
}

MENU_MOUSE_UP( TabbedHeaderMouseUp )
{
  tabbed_header_window* TabbedHeader = GetContainerPayload(tabbed_header_window,Node);
  TabbedHeader->SelectedTabOrdinal = 0;
  TabbedHeader->TabDrag = false;
}

MENU_MOUSE_ENTER( TabbedHeaderMouseEnter )
{
  Platform.DEBUGPrint("Tabbed Header Mouse Enter\n");
}
MENU_MOUSE_EXIT( TabbedHeaderMouseExit )
{
  Platform.DEBUGPrint("Tabbed Header Mouse Exit\n");
}

container_node* CreateRootWindow(menu_interface* Interface, rect2f Region)
{
  // Menu Tree -> RootHeader
  menu_tree* Root = GetNewMenuTree(Interface);

  Root->Root = NewContainer(Interface, container_type::Root);

  container_node* RootContainer = Root->Root;
  RootContainer->Region = Rect2f(Region.X, Region.Y + Interface->BorderSize, Region.W, Region.H);
  RootContainer->Region.W = Maximum(RootContainer->Region.W, Interface->MinSize);
  RootContainer->Region.H = Maximum(RootContainer->Region.H, Interface->MinSize);

  root_window* RootWindow = GetContainerPayload(root_window, RootContainer);
  RootWindow->BorderSize = Interface->BorderSize;
  RootWindow->MinSize = Interface->MinSize;

  container_node* RootHeader = NewContainer(Interface, container_type::MenuHeader);
  menu_header_window* MenuHeader = GetContainerPayload(menu_header_window, RootHeader);
  MenuHeader->HeaderSize = Interface->HeaderSize;
  MenuHeader->RootWindow = RootContainer;
  MenuHeader->WindowDrag = true;
  MenuHeader->RootWindow = Root->Root;
  MenuHeader->DraggingStart = V2(RootContainer->Region.X, RootContainer->Region.Y);

  ConnectNode(0, Root->Root);
  ConnectNode(RootContainer, RootHeader);

  // Make Current
  Interface->HotRegion = window_regions::Header;
  Interface->HotSubWindow = RootHeader;
  MoveMenuToTop(Interface, Interface->RootContainerCount-1);

  return RootHeader;
}

MENU_HANDLE_INPUT( TabbedHeaderHandleInput )
{
  tabbed_header_window* TabbedHeader = GetContainerPayload(tabbed_header_window,Node);

  rect2f HeaderRegion = Node->Functions.GetRegionRect(window_regions::Header, Node);

  
  if(TabbedHeader->TabDrag)
  {
    debug_state* DebugState = DEBUGGetState();
    r32 EnvelopeSize = 1.5f*HeaderRegion.H;
    rect2f HeaderSplitRegion = HeaderRegion;
    HeaderSplitRegion.X -= EnvelopeSize;
    HeaderSplitRegion.W += 2*EnvelopeSize;
    HeaderSplitRegion.Y -= EnvelopeSize;
    HeaderSplitRegion.H += 2*EnvelopeSize;

    if(TabbedHeader->SelectedTabOrdinal > 0 )
    {
      Assert(TabbedHeader->SelectedTabOrdinal <= TabbedHeader->TabCount)

      u32 SelectedTabIndex = TabbedHeader->SelectedTabOrdinal-1;
      if( SelectedTabIndex > 0 )
      {
        rect2f LeftTabRegion = GetTabRegion(HeaderSplitRegion, SelectedTabIndex-1,TabbedHeader->TabCount);
        if(Intersects(LeftTabRegion, Interface->MousePos))
        {
          container_node* Swap = TabbedHeader->Tabs[SelectedTabIndex-1];
          TabbedHeader->Tabs[SelectedTabIndex-1] = TabbedHeader->Tabs[SelectedTabIndex];
          TabbedHeader->Tabs[SelectedTabIndex] = Swap;
          TabbedHeader->SelectedTabOrdinal--;
        }
      }

      if(SelectedTabIndex < TabbedHeader->TabCount-1)
      {
        rect2f RightTabRegion = GetTabRegion(HeaderSplitRegion, SelectedTabIndex+1,TabbedHeader->TabCount);
        if(Intersects(RightTabRegion, Interface->MousePos))
        {
          container_node* Swap = TabbedHeader->Tabs[SelectedTabIndex+1];
          TabbedHeader->Tabs[SelectedTabIndex+1] = TabbedHeader->Tabs[SelectedTabIndex];
          TabbedHeader->Tabs[SelectedTabIndex] = Swap;
          TabbedHeader->SelectedTabOrdinal++;
        }
      }
    }

    v2 Delta = Interface->MousePos - Interface->MouseLeftButtonPush;


    if(!Intersects(HeaderSplitRegion, Interface->MousePos))
    {
      TabbedHeader->TabDrag = false;
      Assert(Node->Parent)

      if(TabbedHeader->TabCount == 1)
      {
        container_node* ParentContainer = Node->Parent;
        container_node* OppositeNode = 0;
        if(ParentContainer->Type == container_type::Split)
        {
          if(Node == ParentContainer->FirstChild)
          {
            OppositeNode = Node->NextSibling;
          }else{
            OppositeNode = ParentContainer->FirstChild;
            Assert(Node == OppositeNode->NextSibling);
          }

          if(OppositeNode)
          {
            container_node* GrandParentContainer = ParentContainer->Parent;

            if(GrandParentContainer->Type ==  container_type::Split)
            {
              if(GrandParentContainer->FirstChild == ParentContainer)
              {
                container_node* Sibling = ParentContainer->NextSibling;
                DisconnectNode(Sibling);
                DisconnectNode(ParentContainer);

                ConnectNode(GrandParentContainer, OppositeNode);
                ConnectNode(GrandParentContainer, Sibling);
              }else{
                DisconnectNode(ParentContainer);
                ConnectNode(GrandParentContainer, OppositeNode);  
              }
            }else{
              DisconnectNode(ParentContainer);
              ConnectNode(GrandParentContainer, OppositeNode);
            }
            DisconnectNode(Node);
            DeleteContainer(Interface, ParentContainer);

            container_node* RootHeader = CreateRootWindow(Interface, Node->Region);
            ConnectNode(RootHeader, Node);
          }
        }  
      }else{
        Assert(TabbedHeader->SelectedTabOrdinal!=0);
        u32 SelectedTabIndex = TabbedHeader->SelectedTabOrdinal-1;
        container_node* TabToBreakOut = TabbedHeader->Tabs[SelectedTabIndex];
        Assert(TabToBreakOut == Node->FirstChild);

        for (u32 TabIndex = SelectedTabIndex; TabIndex < TabbedHeader->TabCount-1; ++TabIndex)
        {
          TabbedHeader->Tabs[TabIndex] = TabbedHeader->Tabs[TabIndex+1];
        }
        if(TabbedHeader->SelectedTabOrdinal == TabbedHeader->TabCount)
        {
          --TabbedHeader->SelectedTabOrdinal;
        }
        
        Assert( TabbedHeader->SelectedTabOrdinal > 0 );
        TabbedHeader->TabCount--;

        DisconnectNode(TabToBreakOut);
        ConnectNode(Node, TabbedHeader->Tabs[TabbedHeader->SelectedTabOrdinal-1] );

        container_node* RootHeader = CreateRootWindow(Interface, Node->Region);

        container_node* NewTabbedHeader = NewContainer(Interface, container_type::TabbedHeader);
        tabbed_header_window* TabbedHeaderWindow = GetContainerPayload(tabbed_header_window, NewTabbedHeader);
        TabbedHeaderWindow->HeaderSize = Interface->HeaderSize;
        TabbedHeaderWindow->Tabs[TabbedHeaderWindow->TabCount++] = TabToBreakOut;
        
        ConnectNode(RootHeader, NewTabbedHeader);
        ConnectNode(NewTabbedHeader, TabToBreakOut);

      }
    }
  }
}

MENU_GET_REGION_RECT( TabbedHeaderGetRegionRect )
{
  rect2f Result = {};
  r32 XStart = 0;
  r32 YStart = 0;
  r32 Width  = 0;
  r32 Height = 0;

  r32 HeaderSize = GetContainerPayload(tabbed_header_window, Node)->HeaderSize;
  rect2f Region = Node->Region;

  switch(Type)
  {
    case window_regions::WholeBody:
    {
      XStart = Region.X;
      YStart = Region.Y;
      Width  = Region.W;
      Height = Region.H - HeaderSize;
    }break;
    case window_regions::Header:
    {
      XStart = Region.X;
      YStart = Region.Y + Region.H - HeaderSize;
      Width  = Region.W;
      Height = HeaderSize;
    }break;
    default:
    {
      Assert(0);
    }break;
  }

  Result = Rect2f(XStart, YStart, Width, Height);
  return Result;
}

MENU_GET_MOUSE_OVER_REGION( TabbedHeaderGetMouseOverRegion )
{
  window_regions RegionArray[] =
  {
    window_regions::WholeBody,
    window_regions::Header
  };

  tabbed_header_window* Window = GetContainerPayload(tabbed_header_window,Node);

  window_regions Result = CheckRegions(MousePos, ArrayCount(RegionArray), RegionArray, Node);

  return Result;
}

MENU_DRAW( TabbedHeaderDraw )
{
  tabbed_header_window* Window = GetContainerPayload(tabbed_header_window, Node);

  r32 Fill = 0.8;
  v4 HeaderColor = V4(0.3,0.5,0.3,1);
  v4 DefaultHeaderColor = V4(0.5,0.5,0.5,1);

  rect2f HeaderRegion = Node->Functions.GetRegionRect(window_regions::Header, Node);

  s32 SelectedTabIndex = Window->SelectedTabOrdinal-1;
  for(u32 TabIndex = 0; TabIndex < Window->TabCount; TabIndex++)
  {
    rect2f TabRegion = GetTabRegion(HeaderRegion, TabIndex,Window->TabCount);
    if(Window->TabDrag && TabIndex+1 == Window->SelectedTabOrdinal)
    {
      TabRegion.X = Interface->MousePos.X - Window->TabMouseOffset.X; 
      if(TabRegion.X < HeaderRegion.X)
      {
        TabRegion.X = HeaderRegion.X;
      }

      if(TabRegion.X + TabRegion.W  > HeaderRegion.X + HeaderRegion.W)
      {
        TabRegion.X = HeaderRegion.X + HeaderRegion.W - TabRegion.W;
      }
    }
    
    container_node* Child = Window->Tabs[TabIndex];
    while(Child->FirstChild)
    {
      Child = Child->FirstChild;
    }


    v4 ColorTable[] = {V4(1,0,0,1),
                       V4(0,1,0,1),
                       V4(0,0,1,1),
                       V4(1,1,0,1),
                       V4(1,0,1,1),
                       V4(0,1,1,1),
                       V4(1,1,1,1),
                       V4(0,0,0,1)};


    if(Child->Type == container_type::Empty)
    {
      empty_window* EmptyWindow = GetContainerPayload(empty_window, Child);
      DEBUGPushQuad(TabRegion, V4(Fill*V3(EmptyWindow->Color),1));
    }else{
      DEBUGPushQuad(TabRegion, ColorTable[ TabIndex & ArrayCount(ColorTable)]);
    }
    
    
  }
}

menu_functions TabbedHeaderMenuFunctions()
{
  menu_functions Result = {};
  Result.MouseDown = TabbedHeaderMouseDown;
  Result.MouseUp = TabbedHeaderMouseUp;
  Result.MouseEnter = TabbedHeaderMouseEnter;
  Result.MouseExit = TabbedHeaderMouseExit;
  Result.HandleInput = TabbedHeaderHandleInput;
  Result.GetRegionRect = TabbedHeaderGetRegionRect;
  Result.GetMouseOverRegion = TabbedHeaderGetMouseOverRegion;
  Result.Draw = TabbedHeaderDraw;
  return Result;
}

MENU_MOUSE_DOWN( SplitMouseDown )
{
  split_window* Window = GetContainerPayload(split_window, Node);
  Window->DraggingStart = Window->SplitFraction;
  switch(HotRegion)
  {
    case window_regions::MiddleBorder:
    {
      Window->BorderDrag = true;
    }break;

    default:
    {
      Assert(0);  
    }break;
  }
}
MENU_MOUSE_UP( SplitMouseUp )
{
  split_window* Window = GetContainerPayload(split_window, Node);
  Window->BorderDrag = false;
}

MENU_MOUSE_ENTER( SplitMouseEnter )
{
  Platform.DEBUGPrint("Split Mouse Enter\n");
}
MENU_MOUSE_EXIT( SplitMouseExit )
{
  Platform.DEBUGPrint("Split Mouse Exit\n");
}

MENU_HANDLE_INPUT( SplitHandleInput )
{
  split_window* Window = GetContainerPayload(split_window, Node);
  game_window_size WindowSize = GameGetWindowSize();
  r32 Width = WindowSize.WidthPx/WindowSize.HeightPx;

  r32 Delta = BranchlessArithmatic(Window->VerticalSplit,
                (Interface->MousePos.X - Interface->MouseLeftButtonPush.X) / Node->Region.W,
                (Interface->MousePos.Y - Interface->MouseLeftButtonPush.Y) / Node->Region.H);

  r32 MinFrac = BranchlessArithmatic(Window->VerticalSplit,
                              Window->MinSize / Node->Region.H,
                              Window->MinSize / Node->Region.W);

  if(Window->BorderDrag)
  {
    Window->SplitFraction = Window->DraggingStart + Delta;

    if(Window->SplitFraction < MinFrac)
    {
      Window->SplitFraction = MinFrac;
    }else if(Window->SplitFraction > 1 - MinFrac)
    {
      Window->SplitFraction = 1 - MinFrac;
    }
  }
}

MENU_GET_REGION_RECT( SplitGetRegionRect )
{
  rect2f Result = {};
  r32 XStart = 0;
  r32 YStart = 0;
  r32 Width  = 0;
  r32 Height = 0;

  split_window* Window = GetContainerPayload(split_window, Node);
  rect2f Region = Node->Region;
  r32 BorderSize = Window->BorderSize;

 if(Window->VerticalSplit)
 {
    r32 LeftWidth = Window->SplitFraction * Region.W - 0.5f * BorderSize;
    r32 RightXStart = Region.X + LeftWidth + BorderSize;
    r32 RightWidth = Region.W - LeftWidth - BorderSize;
    switch(Type)
    {
      case window_regions::BodyOne:
      {
        XStart = Region.X;
        YStart = Region.Y;
        Width  = LeftWidth;
        Height = Region.H;
      }break;
      case window_regions::BodyTwo:
      {
        XStart = RightXStart;
        YStart = Region.Y;
        Width  = RightWidth;
        Height = Region.H;
      }break;
      case window_regions::MiddleBorder:
      {
        XStart = Region.X + LeftWidth;
        YStart = Region.Y;
        Width  = BorderSize;
        Height = Region.H;
      }break;

      default:
      {
        Assert(0);
      }break;
    }
  }else{
    r32 BotHeight = Window->SplitFraction * Region.H - 0.5f * BorderSize;
    r32 TopYStart = Region.Y + BotHeight + BorderSize;
    r32 TopHeight = Region.H - BotHeight - BorderSize;
    switch(Type)
    {
      // Bot
      case window_regions::BodyOne:
      {
        XStart = Region.X;
        YStart = Region.Y;
        Width  = Region.W;
        Height = BotHeight;
      }break;
      // Top
      case window_regions::BodyTwo:
      {
        XStart = Region.X;
        YStart = TopYStart;
        Width  = Region.W;
        Height = TopHeight;
      }break;
      case window_regions::MiddleBorder:
      {
        XStart = Region.X;
        YStart = TopYStart - BorderSize;
        Width  = Region.W;
        Height = BorderSize;
      }break;
      
      default:
      {
        Assert(0);
      }break;
    }
  }

  Result = Rect2f(XStart, YStart, Width, Height);
  return Result;
}

MENU_GET_MOUSE_OVER_REGION( SplitGetMouseOverRegion )
{
  window_regions RegionArray[] =
  {
    window_regions::BodyOne,
    window_regions::BodyTwo,
    window_regions::MiddleBorder
  };

  split_window* Window = GetContainerPayload(split_window, Node);

  window_regions Result = CheckRegions(MousePos, ArrayCount(RegionArray), RegionArray, Node);

  return Result;
}

MENU_DRAW( SplitDraw )
{
  split_window* Window = GetContainerPayload(split_window, Node);
  v4 BodyColor   = V4(0.2,0.2,0.2,1);
  v4 BorderColor = V4(0,0,1,1);
  v4 HeaderColor = V4(0.4,0.4,0.4,1);

  DEBUGPushQuad(Node->Functions.GetRegionRect(window_regions::BodyOne, Node),      BodyColor);
  DEBUGPushQuad(Node->Functions.GetRegionRect(window_regions::BodyTwo, Node),      BodyColor);
  DEBUGPushQuad(Node->Functions.GetRegionRect(window_regions::MiddleBorder, Node), BorderColor); 

}

menu_functions SplitMenuFunctions()
{
  menu_functions Result = {};
  Result.MouseDown = SplitMouseDown;
  Result.MouseUp = SplitMouseUp;
  Result.MouseEnter = SplitMouseEnter;
  Result.MouseExit = SplitMouseExit;
  Result.HandleInput = SplitHandleInput;
  Result.GetRegionRect = SplitGetRegionRect;
  Result.GetMouseOverRegion = SplitGetMouseOverRegion;
  Result.Draw = SplitDraw;
  return Result;
}


MENU_MOUSE_DOWN( ContainerListMouseDown )
{

}
MENU_MOUSE_UP( ContainerListMouseUp )
{

}

MENU_MOUSE_ENTER( ContainerListMouseEnter )
{

}
MENU_MOUSE_EXIT( ContainerListMouseExit )
{

}

MENU_HANDLE_INPUT( ContainerListHandleInput )
{

}

MENU_GET_CHILD_REGION(ContainerListGetChildRegion)
{
  container_node* ChildNode = Parent->FirstChild;
  rect2f Result = {};
  if(ChildNode)
  {
    r32 ChildIndex = 0;
    r32 ChildCount = 0;
    b32 ChildFound = false;
    while(ChildNode)
    {
      if(ChildNode == Child)
      {
        ChildIndex = ChildCount;
        ChildFound = true; 
      }
      ++ChildCount;
      ChildNode = ChildNode->NextSibling;
    }

    Assert(ChildFound)

    rect2f Region = Parent->Region;
    r32 H = Region.H/ChildCount;
    r32 Y0 = Region.Y + ChildIndex * H;

    Result = Rect2f(Region.X, Y0, Region.W, H);
  }
  return Result;
}

MENU_GET_REGION_RECT( ContainerListGetRegionRect )
{
  return Node->Region;
}
MENU_GET_MOUSE_OVER_REGION( ContainerListGetMouseOverRegion )
{
  return window_regions::WholeBody;
}
MENU_DRAW( ContainerListDraw )
{

}

menu_functions ContainerListFunctions()
{
  menu_functions Result = {};
  Result.MouseDown = ContainerListMouseDown;
  Result.MouseUp = ContainerListMouseUp;
  Result.MouseEnter = ContainerListMouseEnter;
  Result.MouseExit = ContainerListMouseExit;
  Result.HandleInput = ContainerListHandleInput;
  Result.GetChildRegion = ContainerListGetChildRegion;
  Result.GetRegionRect = ContainerListGetRegionRect;
  Result.GetMouseOverRegion = ContainerListGetMouseOverRegion;
  Result.Draw = ContainerListDraw;
  return Result;
}

MENU_MOUSE_DOWN( ButtonMouseDown )
{
  menu_button* Button = GetContainerPayload(menu_button,Node);
  debug_state* DebugState = DEBUGGetState();
  (*Button->Activate)(Button, DebugState);
}
MENU_MOUSE_UP( ButtonMouseUp )
{
}
MENU_MOUSE_ENTER( ButtonMouseEnter )
{
  menu_button* Button = GetContainerPayload(menu_button,Node);
  Button->Hot = true;
}
MENU_MOUSE_EXIT( ButtonMouseExit )
{
  menu_button* Button = GetContainerPayload(menu_button,Node);
  Button->Hot = false;
}

MENU_DRAW( ButtonDraw )
{
  menu_button* Button = GetContainerPayload(menu_button,Node);
  rect2f TextRect = DEBUGTextSize(0, 0, Button->Text);
  v2 MiddlePoint = V2(Node->Region.X + Node->Region.W/2.f, Node->Region.Y + Node->Region.H/2.f);
  v2 TextOffset  = V2(TextRect.W/2.f, TextRect.H/2.f);
  TextRect.X = MiddlePoint.X - TextOffset.X;
  TextRect.Y = MiddlePoint.Y - TextOffset.Y;

  r32 ColorMultipier = BranchlessArithmatic(Button->Hot, 0.5f, 0.2f);
  if(Button->Active)
  {
    DEBUGPushQuad(Node->Region, Button->Color);
    DEBUGPushQuad(Shrink(Node->Region, Interface->BorderSize), V4(ColorMultipier*V3(Button->Color),1));
  }else{
    DEBUGPushQuad(Node->Region, V4(ColorMultipier*V3(Button->Color),1));
  }



  DEBUGTextOutAt(TextRect.X,TextRect.Y, Button->Text, V4(1,1,1,1));
}

menu_functions GetButtonFunctions()
{
  menu_functions Result = GetDefaultFunctions();
  Result.MouseDown  = ButtonMouseDown;
  Result.MouseUp    = ButtonMouseUp;
  Result.MouseEnter = ButtonMouseEnter;
  Result.MouseExit  = ButtonMouseExit;
  Result.Draw = ButtonDraw;
  return Result;
}




MENU_MOUSE_DOWN( ProfilerMouseDown )
{
  rect2f Chart = Node->Region;
  profiling_window* Profiler = GetContainerPayload(profiling_window,Node);
  debug_state* DebugState = DEBUGGetState();

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

      if(Intersects(Rect, Interface->MousePos))
      {
        HotRecord = Region->Record;
      }
    }
  }

  game_window_size WindowSize = GameGetWindowSize();
  r32 Width = WindowSize.WidthPx/WindowSize.HeightPx;
  rect2f Window = Rect2f(0,0,Width,1);
  if(Intersects(Window, Interface->MousePos))
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

MENU_DRAW( ProfilerDraw )
{
  rect2f Chart = Node->Region;
  profiling_window* Profiler = GetContainerPayload(profiling_window,Node);
  debug_state* DebugState = DEBUGGetState();

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

      if(Intersects(Rect, Interface->MousePos))
      {
        c8 StringBuffer[256] = {};
        Platform.DEBUGFormatString( StringBuffer, sizeof(StringBuffer), sizeof(StringBuffer)-1,
        "%s : %2.2f MCy", Region->Record->BlockName, (Region->MaxT-Region->MinT)/1000000.f);
        DEBUGTextOutAt(Interface->MousePos.X, Interface->MousePos.Y+0.02f, StringBuffer);
      }
    }
  }
}

menu_functions GetProfilerFunctions()
{
  menu_functions Result = GetDefaultFunctions();
  Result.MouseDown = ProfilerMouseDown;
  Result.Draw = ProfilerDraw;
  return Result;
}

MENU_HANDLE_INPUT( BorderHandleInput )
{
  border_leaf* Border = GetBorder(Node);

  v2 MousePos = Interface->MousePos;
  if(Interface->MouseLeftButton.Edge)
  {
    if(Interface->MouseLeftButton.Active)
    {
      Border->DraggingStart = Border->Position;
      Border->Drag = true;
    }else{
      Border->Drag = false;
    }
  }else{
    if(Interface->MouseLeftButton.Active)
    {
      if(Border->Drag)
      {
        if(Border->Vertical)
        {
          r32 Delta = (Interface->MousePos.X - Interface->MouseLeftButtonPush.X)/Node->Parent->Region.W;
          Border->Position = Border->DraggingStart + Delta;
        }else{
          r32 Delta = (Interface->MousePos.Y - Interface->MouseLeftButtonPush.Y)/Node->Parent->Region.H;
          Border->Position = Border->DraggingStart + Delta;
        }
        
      }
    }
  }
}

MENU_DRAW( BorderDraw )
{
  DEBUGPushQuad(Node->Region, GetContainerPayload(border_leaf, Node)->Color);
}

menu_functions GetBorderFunctions()
{
  menu_functions Result = GetDefaultFunctions();
  Result.HandleInput = BorderHandleInput;
  Result.Draw = BorderDraw;
  return Result;
}


menu_functions GetMenuFunction(container_type Type)
{
  switch(Type)
  {   
    case container_type::None: return GetDefaultFunctions();
    case container_type::Root: return GetRootMenuFunctions();
    case container_type::Body:
    case container_type::Empty: return GetEmptyFunctions();
    case container_type::Split: return SplitMenuFunctions();
    case container_type::TabbedHeader: return TabbedHeaderMenuFunctions();
    case container_type::MenuHeader: return MenuHeaderMenuFunctions();
    case container_type::ContainerList: return ContainerListFunctions();
    case container_type::Button: return GetButtonFunctions();
    case container_type::Profiler: return GetProfilerFunctions();
    case container_type::Border: return GetBorderFunctions();
    default: Assert(0);
  }
  return {};
}
