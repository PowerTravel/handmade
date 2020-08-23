
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



#if 0
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
}
MENU_GET_MOUSE_OVER_REGION( GetMouseOverRegion )
{

}
MENU_DRAW( Draw )
{

}
menu_functions GetFunctions()
{
  menu_functions Result = {};
  Result.MouseDown = MouseDown;
  Result.MouseUp = MouseUp;
  Result.MouseEnter = MouseEnter;
  Result.MouseExit = MouseExit;
  Result.HandleInput = HandleInput;
  Result.GetRegionRect = GetRegionRect;
  Result.GetMouseOverRegion = GetMouseOverRegion;
  Result.Draw = Draw;
  return Result;
}
#endif

MENU_MOUSE_DOWN( EmptyMouseDown )
{

}
MENU_MOUSE_UP( EmptyMouseUp )
{

}

MENU_MOUSE_ENTER( EmptyMouseEnter )
{

}
MENU_MOUSE_EXIT( EmptyMouseExit )
{

}

MENU_HANDLE_INPUT( EmptyHandleInput )
{

}

MENU_GET_REGION_RECT( EmptyGetRegionRect )
{
  Assert(Type == window_regions::WholeBody);
  return Node->Region;
}
MENU_GET_MOUSE_OVER_REGION( EmptyGetMouseOverRegion )
{
  if(Intersects(Node->Region, MousePos))
  {
    return window_regions::WholeBody;
  }
  return window_regions::None;
}
MENU_DRAW( EmptyDraw )
{
  DEBUGPushQuad(Node->Region, GetContainerPayload(empty_window, Node)->Color);
}

menu_functions GetEmptyFunctions()
{
  menu_functions Result = {};
  Result.MouseDown = EmptyMouseDown;
  Result.MouseUp = EmptyMouseUp;
  Result.MouseEnter = EmptyMouseEnter;
  Result.MouseExit = EmptyMouseExit;
  Result.HandleInput = EmptyHandleInput;
  Result.GetRegionRect = EmptyGetRegionRect;
  Result.GetMouseOverRegion = EmptyGetMouseOverRegion;
  Result.Draw = EmptyDraw;
  return Result;
}


MENU_MOUSE_DOWN( RootWindowMouseDown )
{
  root_window* Window = GetContainerPayload(root_window, Node);

  Window->WindowDraggingStart = Node->Region;
  switch(HotRegion)
  {
    case window_regions::LeftBorder:
    {
      Window->LeftBorderDrag = true;
    }break;
    case window_regions::RightBorder:
    {
      Window->RightBorderDrag = true;
    }break;
    case window_regions::BotBorder:
    {
      Window->BotBorderDrag = true;
    }break;
    case window_regions::TopBorder:
    {
      Window->TopBorderDrag = true;
    }break;

    case window_regions::BotLeftCorner:
    {
      Window->BotBorderDrag = true;
      Window->LeftBorderDrag = true;
    }break;
    case window_regions::BotRightCorner:
    {
      Window->BotBorderDrag = true;
      Window->RightBorderDrag = true;
    }break;
    case window_regions::TopLeftCorner:
    {
      Window->TopBorderDrag = true;
      Window->LeftBorderDrag = true;
    }break;
    case window_regions::TopRightCorner:
    {
      Window->TopBorderDrag = true;
      Window->RightBorderDrag = true;
    }break;

    default:
    {
      Assert(0);
    }break;
  }
}

MENU_MOUSE_UP( RootWindowMouseUp )
{
  root_window* Window = GetContainerPayload(root_window, Node);
  Window->LeftBorderDrag  = false;
  Window->RightBorderDrag = false;
  Window->BotBorderDrag   = false;
  Window->TopBorderDrag   = false;
}

MENU_MOUSE_ENTER( RootWindowMouseEnter )
{
}
MENU_MOUSE_EXIT( RootWindowMouseExit )
{
}

MENU_HANDLE_INPUT( RootWindowHandleInput )
{
  root_window* Window = GetContainerPayload(root_window, Node);

  r32 MinWidth  = Window->MinSize + 2*Window->BorderSize;
  r32 MinHeight = Window->MinSize + 2*Window->BorderSize;

  v2 Delta = Interface->MousePos - Interface->MouseLeftButtonPush;

  if(Window->RightBorderDrag)
  {
    game_window_size WindowSize = GameGetWindowSize();
    r32 Width = WindowSize.WidthPx/WindowSize.HeightPx;

    r32 NewWidth = Window->WindowDraggingStart.W + Delta.X;
    if(Window->WindowDraggingStart.X + NewWidth > Width )
    {
      NewWidth = Width - Window->WindowDraggingStart.X;
    }else if(NewWidth < MinWidth)
    {
      NewWidth = MinWidth;
    }
    Node->Region.W = NewWidth;
  }

  if(Window->LeftBorderDrag)
  {
    r32 NewXPos   = Window->WindowDraggingStart.X + Delta.X;
    r32 NewWidth  = Window->WindowDraggingStart.W - Delta.X;
    if(NewXPos < 0)
    {
      NewXPos = 0;
      NewWidth = (Window->WindowDraggingStart.X) + Window->WindowDraggingStart.W;
    }else if(NewWidth < MinWidth)
    {
      NewXPos  = Window->WindowDraggingStart.X + Window->WindowDraggingStart.W - MinWidth;
      NewWidth = MinWidth;
    }
    Node->Region.X = NewXPos;
    Node->Region.W = NewWidth;
  }

  if(Window->BotBorderDrag)
  {
    r32 NewYPos   = Window->WindowDraggingStart.Y + Delta.Y;
    r32 NewHeight = Window->WindowDraggingStart.H - Delta.Y;
    if(NewYPos < 0)
    {
      NewYPos = 0;
      NewHeight = (Window->WindowDraggingStart.Y) + Window->WindowDraggingStart.H;
    }else if(NewHeight < MinHeight)
    {
      NewYPos = Window->WindowDraggingStart.Y + Window->WindowDraggingStart.H - MinHeight;
      NewHeight = MinHeight;
    }
    Node->Region.Y = NewYPos;
    Node->Region.H = NewHeight;
  }

  if(Window->TopBorderDrag)
  {
    r32 NewHeight = Window->WindowDraggingStart.H + Delta.Y;
    if((Window->WindowDraggingStart.Y + NewHeight) > 1)
    {
      NewHeight = 1 - Window->WindowDraggingStart.Y;
    }else if(NewHeight < MinHeight)
    {
      NewHeight = MinHeight;
    }
    Node->Region.H = NewHeight;
  }
}


MENU_GET_REGION_RECT( RootWindowGetRegionRect )
{
  rect2f Result = {};
  r32 XStart = 0;
  r32 YStart = 0;
  r32 Width  = 0;
  r32 Height = 0;

  rect2f Region = Node->Region;
  root_window* Window = GetContainerPayload(root_window, Node);
  r32 BorderSize = Window->BorderSize;

  switch(Type)
  {
    // Root
    case window_regions::WholeBody:
    {
      XStart = Region.X + BorderSize;
      YStart = Region.Y + BorderSize;
      Width  = Region.W - 2*BorderSize;
      Height = Region.H - 2*BorderSize;
    }break;

    case window_regions::LeftBorder:
    {
      XStart = Region.X;
      YStart = Region.Y + BorderSize;
      Width  = BorderSize;
      Height = Region.H - 2*BorderSize;
    }break;
    case window_regions::RightBorder:
    {
      XStart = Region.X + Region.W - BorderSize;
      YStart = Region.Y + BorderSize;
      Width  = BorderSize;
      Height = Region.H - 2*BorderSize;
    }break;
    case window_regions::BotBorder:
    {
      XStart = Region.X + BorderSize;
      YStart = Region.Y;
      Width  = Region.W - 2*BorderSize;
      Height = BorderSize;
    }break;
    case window_regions::TopBorder:
    {
      XStart = Region.X + BorderSize;
      YStart = Region.Y + Region.H - BorderSize;
      Width  = Region.W - 2*BorderSize;
      Height = BorderSize;
    }break;
    case window_regions::BotLeftCorner:
    {
      XStart = Region.X;
      YStart = Region.Y;
      Width  = BorderSize;
      Height = BorderSize;
    }break;
    case window_regions::BotRightCorner:
    {
      XStart = Region.X + Region.W - BorderSize;
      YStart = Region.Y;
      Width  = BorderSize;
      Height = BorderSize;
    }break;
    case window_regions::TopLeftCorner:
    {
      XStart = Region.X;
      YStart = Region.Y + Region.H - BorderSize;
      Width  = BorderSize;
      Height = BorderSize;
    }break;
    case window_regions::TopRightCorner:
    {
      XStart = Region.X + Region.W - BorderSize;
      YStart = Region.Y + Region.H - BorderSize;
      Width  = BorderSize;
      Height = BorderSize;
    }break;
    default:
    {
      Assert(0);
    }break;
  }

  Result = Rect2f(XStart, YStart, Width, Height);
  return Result;
}

MENU_GET_MOUSE_OVER_REGION( RootWindowGetMouseOverRegion )
{
  window_regions RegionArray[] =
  {
    window_regions::WholeBody,
    window_regions::LeftBorder,
    window_regions::RightBorder,
    window_regions::BotBorder,
    window_regions::TopBorder,
    window_regions::BotLeftCorner,
    window_regions::BotRightCorner,
    window_regions::TopLeftCorner,
    window_regions::TopRightCorner
  };

  root_window* Window = GetContainerPayload(root_window, Node);

  window_regions Result = CheckRegions(MousePos, ArrayCount(RegionArray), RegionArray, Node);

  return Result;
}

MENU_DRAW( RootWindowDraw )
{
  root_window* Window = GetContainerPayload(root_window, Node);
  v4 CornerColor = V4(1,0,1,1);
  v4 BorderColor = V4(0,0,1,1);
  v4 HeaderColor = V4(1,0,0,1);
  DEBUGPushQuad(Node->Functions.GetRegionRect(window_regions::LeftBorder,     Node), BorderColor);
  DEBUGPushQuad(Node->Functions.GetRegionRect(window_regions::RightBorder,    Node), BorderColor);
  DEBUGPushQuad(Node->Functions.GetRegionRect(window_regions::TopBorder,      Node), BorderColor);
  DEBUGPushQuad(Node->Functions.GetRegionRect(window_regions::BotBorder,      Node), BorderColor);
  DEBUGPushQuad(Node->Functions.GetRegionRect(window_regions::BotLeftCorner,  Node), CornerColor);
  DEBUGPushQuad(Node->Functions.GetRegionRect(window_regions::BotRightCorner, Node), CornerColor);
  DEBUGPushQuad(Node->Functions.GetRegionRect(window_regions::TopLeftCorner,  Node), CornerColor);
  DEBUGPushQuad(Node->Functions.GetRegionRect(window_regions::TopRightCorner, Node), CornerColor);
}

menu_functions GetRootMenuFunctions()
{
  menu_functions Result = {};
  Result.MouseDown = RootWindowMouseDown;
  Result.MouseUp = RootWindowMouseUp;
  Result.MouseEnter = RootWindowMouseEnter;
  Result.MouseExit = RootWindowMouseExit;
  Result.HandleInput = RootWindowHandleInput;

  Result.GetRegionRect = RootWindowGetRegionRect;
  Result.GetMouseOverRegion = RootWindowGetMouseOverRegion;
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
      FreeMenuTree(Interface, &Interface->HotWindow);

    }else if( ZoneIndex == 0  || ZoneIndex == 2 ||  // Vertical
              ZoneIndex == 3  || ZoneIndex == 4)    // Horizontal
    {
      container_node* OppositeNode = Window->NodeToMerge;
      container_type Type = (ZoneIndex == 0  || ZoneIndex == 2) ? container_type::VerticalSplit : container_type::HorizontalSplit;
      container_node* SplitContainer = NewContainer(Interface, Type);
      split_window* SplitWindow = GetContainerPayload(split_window, SplitContainer);
      SplitWindow->BorderSize = 0.007f;
      SplitWindow->MinSize = 0.02f;
      SplitWindow->SplitFraction = 0.5f; 

      container_node* CommonParent = OppositeNode->Parent;
      if(CommonParent->Type ==  container_type::VerticalSplit ||
         CommonParent->Type ==  container_type::HorizontalSplit)
      {
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
      }

      container_node* NodeToConnect = Node->FirstChild;
      DisconnectNode(NodeToConnect);
      if( ZoneIndex == 0  || ZoneIndex == 3 )
      {
        // Left || Bot
        ConnectNode(SplitContainer, NodeToConnect);
        ConnectNode(SplitContainer, OppositeNode);
      }else if(ZoneIndex == 2  ||  ZoneIndex == 4){  
        // Right || Top
        ConnectNode(SplitContainer, OppositeNode);
        ConnectNode(SplitContainer, NodeToConnect);
      }

      Assert(OppositeNode->Type == container_type::TabbedHeader);
  
      FreeMenuTree(Interface, &Interface->HotWindow);
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
         NodeRegion.Region == window_regions::LeftBody  ||
         NodeRegion.Region == window_regions::RightBody )
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

      Platform.DEBUGPrint("MergeRegion: %d\n", Window->HotMergeZone);
    }


    v2 Delta = Interface->MousePos - Interface->MouseLeftButtonPush;
    game_window_size WindowSize = GameGetWindowSize();
    r32 Width = WindowSize.WidthPx/WindowSize.HeightPx;
    rect2f Surroundings = Rect2f(0,0,Width,1);

    rect2f* Region = &Window->RootWindow->Region;

    Assert((Surroundings.Y + Surroundings.H)<=1);

    v2 NewPos = Window->DraggingStart + Delta;
    #if 0
    if(NewPos.X < Surroundings.X)
    {
      NewPos.X = Surroundings.X;
    }
    if( (NewPos.X + Region->W) > (Surroundings.X + Surroundings.W))
    {
      NewPos.X = (Surroundings.X + Surroundings.W) - Region->W;
    }
    if(NewPos.Y < Surroundings.Y)
    {
      NewPos.Y = Surroundings.Y;
    }
    if(NewPos.Y + Region->H > (Surroundings.Y + Surroundings.H))
    {
      NewPos.Y = (Surroundings.Y + Surroundings.H) - Region->H;
    }
    #endif

    Region->X = NewPos.X;
    Region->Y = NewPos.Y;
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
  DEBUGPushQuad(Node->Functions.GetRegionRect(window_regions::Header, Node), HeaderColor);
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

MENU_HANDLE_INPUT( TabbedHeaderHandleInput )
{
  tabbed_header_window* TabbedHeader = GetContainerPayload(tabbed_header_window,Node);

  rect2f HeaderRegion = Node->Functions.GetRegionRect(window_regions::Header, Node);

  if(TabbedHeader->TabDrag)
  {
    if(TabbedHeader->SelectedTabOrdinal > 0 )
    {
      Assert(TabbedHeader->SelectedTabOrdinal <= TabbedHeader->TabCount)

      u32 SelectedTabIndex = TabbedHeader->SelectedTabOrdinal-1;
      rect2f SelectedTabRegion = GetTabRegion(HeaderRegion, SelectedTabIndex,TabbedHeader->TabCount);
      if( SelectedTabIndex > 0 )
      {
        rect2f LeftTabRegion = GetTabRegion(HeaderRegion, SelectedTabIndex-1,TabbedHeader->TabCount);
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
        rect2f RightTabRegion = GetTabRegion(HeaderRegion, SelectedTabIndex+1,TabbedHeader->TabCount);
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

    debug_state* DebugState = DEBUGGetState();

    rect2f HeaderSplitRegion = HeaderRegion;
    HeaderSplitRegion.Y -= HeaderSplitRegion.H;
    HeaderSplitRegion.H += 2*HeaderSplitRegion.H;

    if(!Intersects(HeaderSplitRegion,Interface->MousePos))
    {
      TabbedHeader->TabDrag = false;
      Assert(Node->Parent)

      container_node* ParentContainer = Node->Parent;
      container_node* OppositeNode = 0;
      if(ParentContainer->Type == container_type::VerticalSplit || 
         ParentContainer->Type == container_type::HorizontalSplit)
      {
        if(Node == ParentContainer->FirstChild)
        {
          OppositeNode = Node->NextSibling;
        }else{
          OppositeNode = ParentContainer->FirstChild;
          Assert(Node == OppositeNode->NextSibling);
        }
      }
      if(OppositeNode)
      {
        container_node* GrandParentContainer = ParentContainer->Parent;

        if(GrandParentContainer->Type ==  container_type::VerticalSplit ||
           GrandParentContainer->Type ==  container_type::HorizontalSplit)
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


        menu_tree* Root = GetNewMenuTree(Interface);
        Root->Root = NewContainer(Interface, container_type::Root);

        r32 BorderSize = 0.007;
        r32 HeaderSize = 0.02;

        container_node* RootContainer = Root->Root;
        RootContainer->Region = Rect2f(Node->Region.X, Node->Region.Y + BorderSize, Node->Region.W, Node->Region.H);

        root_window* RootWindow = GetContainerPayload(root_window, RootContainer);
        RootWindow->BorderSize = BorderSize;
        RootWindow->MinSize = 0.2f;

        container_node* RootHeader = NewContainer(Interface, container_type::MenuHeader);
        menu_header_window* MenuHeader = GetContainerPayload(menu_header_window, RootHeader);
        MenuHeader->HeaderSize = HeaderSize;
        MenuHeader->RootWindow = RootContainer;
        MenuHeader->WindowDrag = true;
        MenuHeader->RootWindow = Root->Root;
        MenuHeader->DraggingStart = V2(RootContainer->Region.X, RootContainer->Region.Y);

        Interface->HotRegion = window_regions::Header;
        Interface->HotSubWindow = RootHeader;

        ConnectNode(0, Root->Root);
        ConnectNode(RootContainer, RootHeader);
        ConnectNode(RootHeader, Node);

        // TODO: Just random numbers that will give a large enought stack to traverse atm
        //       Fixit
        Root->Depth = 6;
        Root->NodeCount = 6;

        menu_tree TopMenu = *Root;
        MoveMenuToTop(Interface, Interface->RootContainerCount-1);
        Interface->HotWindow = TopMenu;
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

    Assert(Child->Type == container_type::Empty);

    empty_window* EmptyWindow = GetContainerPayload(empty_window, Child);
    DEBUGPushQuad(TabRegion, V4(0.8*V3(EmptyWindow->Color),1));
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

MENU_MOUSE_DOWN( VerticalSplitMouseDown )
{
  split_window* Window = GetContainerPayload(split_window, Node);
  Window->DraggingStart = Window->SplitFraction;
  switch(HotRegion)
  {
    case window_regions::LeftHeader:
    {
      Window->WindowDrag[0] = true;
    }break;
    case window_regions::RightHeader:
    {
      Window->WindowDrag[1] = true;
    }break;

    case window_regions::VerticalBorder:
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
  Window->WindowDrag[0] = false;
  Window->WindowDrag[1] = false;
}

MENU_MOUSE_ENTER( VerticalSplitMouseEnter )
{
  Platform.DEBUGPrint("Vertical Split Mouse Enter\n");
}
MENU_MOUSE_EXIT( VerticalSplitMouseExit )
{
  Platform.DEBUGPrint("Vertical Split Mouse Exit\n");
}

MENU_HANDLE_INPUT( VerticalSplitHandleInput )
{
  split_window* Window = GetContainerPayload(split_window, Node);
  game_window_size WindowSize = GameGetWindowSize();
  r32 Width = WindowSize.WidthPx/WindowSize.HeightPx;
  rect2f Surroundings = Rect2f(0,0,Width,1);

  r32 Delta = (Interface->MousePos.X - Interface->MouseLeftButtonPush.X) / Node->Region.W;
  r32 MinFrac = Window->MinSize/ Node->Region.H;

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
  }else if(Window->WindowDrag[0]){
  }else if(Window->WindowDrag[1]){
  }
}

MENU_GET_REGION_RECT( VerticalSplitGetRegionRect )
{
  rect2f Result = {};
  r32 XStart = 0;
  r32 YStart = 0;
  r32 Width  = 0;
  r32 Height = 0;

  split_window* Window = GetContainerPayload(split_window, Node);
  rect2f Region = Node->Region;
  r32 BorderSize = Window->BorderSize;

  r32 LeftWidth = Window->SplitFraction * Region.W - 0.5f * BorderSize;
  r32 RightXStart = Region.X + LeftWidth + BorderSize;
  r32 RightWidth = Region.W - LeftWidth - BorderSize;

  switch(Type)
  {
    case window_regions::LeftBody:
    {
      XStart = Region.X;
      YStart = Region.Y;
      Width  = LeftWidth;
      Height = Region.H;
    }break;
    case window_regions::RightBody:
    {
      XStart = RightXStart;
      YStart = Region.Y;
      Width  = RightWidth;
      Height = Region.H;
    }break;
    case window_regions::VerticalBorder:
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

  Result = Rect2f(XStart, YStart, Width, Height);
  return Result;
}


MENU_GET_MOUSE_OVER_REGION( VerticalSplitGetMouseOverRegion )
{
  window_regions RegionArray[] =
  {
    window_regions::LeftBody,
    window_regions::RightBody,
    window_regions::VerticalBorder
  };

  split_window* Window = GetContainerPayload(split_window, Node);

  window_regions Result = CheckRegions(MousePos, ArrayCount(RegionArray), RegionArray, Node);

  return Result;
}

MENU_DRAW( VerticalSplitDraw )
{
  split_window* Window = GetContainerPayload(split_window, Node);
  v4 BodyColor   = V4(0.2,0.2,0.2,1);
  v4 BorderColor = V4(0,0,1,1);
  v4 HeaderColor = V4(0.4,0.4,0.4,1);
  DEBUGPushQuad(Node->Functions.GetRegionRect(window_regions::LeftBody, Node),  BodyColor);
  DEBUGPushQuad(Node->Functions.GetRegionRect(window_regions::RightBody, Node), BodyColor);
  DEBUGPushQuad(Node->Functions.GetRegionRect(window_regions::VerticalBorder, Node), BorderColor);

}

menu_functions VerticalSplitMenuFunctions()
{
  menu_functions Result = {};
  Result.MouseDown = VerticalSplitMouseDown;
  Result.MouseUp = SplitMouseUp;
  Result.MouseEnter = VerticalSplitMouseEnter;
  Result.MouseExit = VerticalSplitMouseExit;
  Result.HandleInput = VerticalSplitHandleInput;
  Result.GetRegionRect = VerticalSplitGetRegionRect;
  Result.GetMouseOverRegion = VerticalSplitGetMouseOverRegion;
  Result.Draw = VerticalSplitDraw;
  return Result;
}

MENU_MOUSE_DOWN( HorizontalMouseDown )
{
  split_window* Window = GetContainerPayload(split_window, Node);
  Window->DraggingStart = Window->SplitFraction;
  switch(HotRegion)
  {
    case window_regions::BotHeader:
    {
      Window->WindowDrag[0] = true;
    }break;
    case window_regions::TopHeader:
    {
      Window->WindowDrag[1] = true;
    }break;

    case window_regions::HorizontalBorder:
    {
      Window->BorderDrag = true;
    }break;

    default:
    {
      Assert(0);
    }break;
  }
}

MENU_MOUSE_ENTER( HorizontalMouseEnter )
{

}
MENU_MOUSE_EXIT( HorizontalMouseExit )
{

}

MENU_HANDLE_INPUT( HorizontalHandleInput )
{
  split_window* Window = GetContainerPayload(split_window, Node);
  game_window_size WindowSize = GameGetWindowSize();
  r32 Width = WindowSize.WidthPx/WindowSize.HeightPx;
  rect2f Surroundings = Rect2f(0,0,Width,1);

  r32 Delta = (Interface->MousePos.Y - Interface->MouseLeftButtonPush.Y) / Node->Region.H;
  r32 MinFrac = Window->MinSize/ Node->Region.H;

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
  }else if(Window->WindowDrag[0]){
  }else if(Window->WindowDrag[1]){
  }
}

MENU_GET_REGION_RECT( HorizontalGetRegionRect )
{
  rect2f Result = {};
  r32 XStart = 0;
  r32 YStart = 0;
  r32 Width  = 0;
  r32 Height = 0;

  split_window* Window = GetContainerPayload(split_window, Node);
  rect2f Region = Node->Region;
  r32 BorderSize = Window->BorderSize;

  r32 BotHeight = Window->SplitFraction * Region.H - 0.5f * BorderSize;
  r32 TopYStart = Region.Y + BotHeight + BorderSize;
  r32 TopHeight = Region.H - BotHeight - BorderSize;

  switch(Type)
  {
    case window_regions::BotBody:
    {
      XStart = Region.X;
      YStart = Region.Y;
      Width  = Region.W;
      Height = BotHeight;
    }break;
    case window_regions::TopBody:
    {
      XStart = Region.X;
      YStart = TopYStart;
      Width  = Region.W;
      Height = TopHeight;
    }break;
    case window_regions::HorizontalBorder:
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

  Result = Rect2f(XStart, YStart, Width, Height);
  return Result;
}

MENU_GET_MOUSE_OVER_REGION( HorizontalGetMouseOverRegion )
{
  window_regions RegionArray[] =
  {
    window_regions::BotBody,
    window_regions::TopBody,
    window_regions::HorizontalBorder
  };

  split_window* Window = GetContainerPayload(split_window, Node);

  window_regions Result = CheckRegions(MousePos, ArrayCount(RegionArray), RegionArray, Node);

  return Result;
}

MENU_DRAW( HorizontalDraw )
{
  split_window* Window = GetContainerPayload(split_window, Node);
  v4 BodyColor   = V4(0.2,0.2,0.2,1);
  v4 BorderColor = V4(0,0,1,1);
  v4 HeaderColor = V4(0.4,0.4,0.4,1);
  DEBUGPushQuad(Node->Functions.GetRegionRect(window_regions::BotBody, Node), BodyColor);
  DEBUGPushQuad(Node->Functions.GetRegionRect(window_regions::TopBody, Node), BodyColor);
  DEBUGPushQuad(Node->Functions.GetRegionRect(window_regions::HorizontalBorder, Node), BorderColor);

}


menu_functions HorizontalMenuFunctions()
{
  menu_functions Result = {};
  Result.MouseDown = HorizontalMouseDown;
  Result.MouseUp = SplitMouseUp;
  Result.MouseEnter = HorizontalMouseEnter;
  Result.MouseExit = HorizontalMouseExit;
  Result.HandleInput = HorizontalHandleInput;
  Result.GetRegionRect = HorizontalGetRegionRect;
  Result.GetMouseOverRegion = HorizontalGetMouseOverRegion;
  Result.Draw = HorizontalDraw;
  return Result;
}

