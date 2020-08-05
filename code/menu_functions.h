
#include "debug.h"

window_regions CheckRegions( v2 MousePos, u32 RegionCount, window_regions* RegionArray, container_node* Node )
{
  for (u32 RegionIndex = 0; RegionIndex < RegionCount; ++RegionIndex)
  {
    window_regions RegionEnum = RegionArray[RegionIndex];
    if(Intersects(Node->Functions->GetRegionRect(RegionEnum, Node), MousePos))
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
MENU_UPDATE_REGIONS( UpdateRegions )
{

}
MENU_UPDATE_REGIONS( UpdateRegions )
{

}
MENU_GET_REGION_RECT( GetRegionRect )
{
}

MENU_GET_SUB_MENU_REGION_RECT( GetSubMenuRegionRect )
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
  Result.UpdateRegions = UpdateRegions;
  Result.GetRegionRect = GetRegionRect;
  Result.GetSubMenuRegionRect = GetSubMenuRegionRect;
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
MENU_UPDATE_REGIONS( EmptyUpdateRegions )
{

}
MENU_GET_REGION_RECT( EmptyGetRegionRect )
{
  Assert(Type == window_regions::WholeBody);
  return Node->Region;
}
MENU_GET_SUB_MENU_REGION_RECT( EmptyGetSubMenuRegionRect )
{
  rect2f Result = Node->Functions->GetRegionRect(window_regions::WholeBody, Node);
  return Result;
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
  DEBUGPushQuad(Node->Region, V4(0,0,0,1));
}

menu_functions GetEmptyFunctions()
{
  menu_functions Result = {};
  Result.MouseDown = EmptyMouseDown;
  Result.MouseUp = EmptyMouseUp;
  Result.MouseEnter = EmptyMouseEnter;
  Result.MouseExit = EmptyMouseExit;
  Result.HandleInput = EmptyHandleInput;
  Result.UpdateRegions = EmptyUpdateRegions;
  Result.GetRegionRect = EmptyGetRegionRect;
  Result.GetSubMenuRegionRect = EmptyGetSubMenuRegionRect;
  Result.GetMouseOverRegion = EmptyGetMouseOverRegion;
  Result.Draw = EmptyDraw;
  return Result;
}


MENU_MOUSE_DOWN( RootWindowMouseDown )
{
  root_window* Window = Node->RootWindow;

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
  root_window* Window = Node->RootWindow;
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
  root_window* Window = Node->RootWindow;

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


MENU_UPDATE_REGIONS( RootWindowUpdateRegions )
{
  root_window* Window = Node->RootWindow;
}

MENU_GET_REGION_RECT( RootWindowGetRegionRect )
{
  rect2f Result = {};
  r32 XStart = 0;
  r32 YStart = 0;
  r32 Width  = 0;
  r32 Height = 0;

  rect2f Region = Node->Region;
  r32 BorderSize = Node->RootWindow->BorderSize;

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

  root_window* Window = Node->RootWindow;

  window_regions Result = CheckRegions(MousePos, ArrayCount(RegionArray), RegionArray, Node);

  return Result;
}

MENU_DRAW( RootWindowDraw )
{
  root_window* Window = Node->RootWindow;
  v4 CornerColor = V4(1,0,1,1);
  v4 BorderColor = V4(0,0,1,1);
  v4 HeaderColor = V4(1,0,0,1);
  DEBUGPushQuad(Node->Functions->GetRegionRect(window_regions::LeftBorder,     Node), BorderColor);
  DEBUGPushQuad(Node->Functions->GetRegionRect(window_regions::RightBorder,    Node), BorderColor);
  DEBUGPushQuad(Node->Functions->GetRegionRect(window_regions::TopBorder,      Node), BorderColor);
  DEBUGPushQuad(Node->Functions->GetRegionRect(window_regions::BotBorder,      Node), BorderColor);
  DEBUGPushQuad(Node->Functions->GetRegionRect(window_regions::BotLeftCorner,  Node), CornerColor);
  DEBUGPushQuad(Node->Functions->GetRegionRect(window_regions::BotRightCorner, Node), CornerColor);
  DEBUGPushQuad(Node->Functions->GetRegionRect(window_regions::TopLeftCorner,  Node), CornerColor);
  DEBUGPushQuad(Node->Functions->GetRegionRect(window_regions::TopRightCorner, Node), CornerColor);
}

menu_functions GetRootMenuFunctions()
{
  menu_functions Result = {};
  Result.MouseDown = RootWindowMouseDown;
  Result.MouseUp = RootWindowMouseUp;
  Result.MouseEnter = RootWindowMouseEnter;
  Result.MouseExit = RootWindowMouseExit;
  Result.HandleInput = RootWindowHandleInput;

  Result.UpdateRegions = RootWindowUpdateRegions;
  Result.GetRegionRect = RootWindowGetRegionRect;
  Result.GetSubMenuRegionRect = EmptyGetSubMenuRegionRect;
  Result.GetMouseOverRegion = RootWindowGetMouseOverRegion;
  Result.Draw = RootWindowDraw;
  return Result;
}




MENU_MOUSE_DOWN( MenuHeaderMouseDown )
{
  menu_header_window* Window = Node->MenuHeader;
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
  Node->MenuHeader->WindowDrag = false;
}

MENU_MOUSE_ENTER( MenuHeaderMouseEnter )
{

}
MENU_MOUSE_EXIT( MenuHeaderMouseExit )
{

}

MENU_HANDLE_INPUT( MenuHeaderHandleInput )
{
  menu_header_window* Window = Node->MenuHeader;
  if(Window->WindowDrag)
  {
    v2 Delta = Interface->MousePos - Interface->MouseLeftButtonPush;
    game_window_size WindowSize = GameGetWindowSize();
    r32 Width = WindowSize.WidthPx/WindowSize.HeightPx;
    rect2f Surroundings = Rect2f(0,0,Width,1);

    rect2f* Region = &Window->RootWindow->Region;

    Assert((Surroundings.Y + Surroundings.H)<=1);

    v2 NewPos = Window->DraggingStart + Delta;
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

    Region->X = NewPos.X;
    Region->Y = NewPos.Y;
  }
}
MENU_UPDATE_REGIONS( MenuHeaderUpdateRegions )
{
  menu_header_window* Window = Node->MenuHeader;
}

MENU_GET_REGION_RECT( MenuHeaderGetRegionRect )
{
  rect2f Result = {};
  r32 XStart = 0;
  r32 YStart = 0;
  r32 Width  = 0;
  r32 Height = 0;

  rect2f Region = Node->Region;
  r32 HeaderSize = Node->MenuHeader->HeaderSize;

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

  menu_header_window* Window = Node->MenuHeader;

  window_regions Result = CheckRegions(MousePos, ArrayCount(RegionArray), RegionArray, Node);

  return Result;
}


MENU_DRAW( MenuHeaderDraw )
{
  menu_header_window* Window = Node->MenuHeader;
  v4 HeaderColor = V4(0.8,0.2,0.2,1);
  DEBUGPushQuad(Node->Functions->GetRegionRect(window_regions::Header, Node), HeaderColor);

}

menu_functions MenuHeaderMenuFunctions()
{
  menu_functions Result = {};
  Result.MouseDown = MenuHeaderMouseDown;
  Result.MouseUp = MenuHeaderMouseUp;
  Result.MouseEnter = MenuHeaderMouseEnter;
  Result.MouseExit = MenuHeaderMouseExit;
  Result.HandleInput = MenuHeaderHandleInput;
  Result.UpdateRegions = MenuHeaderUpdateRegions;
  Result.GetRegionRect = MenuHeaderGetRegionRect;
  Result.GetSubMenuRegionRect = EmptyGetSubMenuRegionRect;
  Result.GetMouseOverRegion = MenuHeaderGetMouseOverRegion;
  Result.Draw = MenuHeaderDraw;
  return Result;
}





MENU_MOUSE_DOWN( TabbedHeaderMouseDown )
{

}
MENU_MOUSE_UP( TabbedHeaderMouseUp )
{

}

MENU_MOUSE_ENTER( TabbedHeaderMouseEnter )
{

}
MENU_MOUSE_EXIT( TabbedHeaderMouseExit )
{

}

MENU_HANDLE_INPUT( TabbedHeaderHandleInput )
{

}

MENU_UPDATE_REGIONS( TabbedHeaderUpdateRegions )
{
  menu_header_window* Window = Node->MenuHeader;
}

MENU_GET_REGION_RECT( TabbedHeaderGetRegionRect )
{
  rect2f Result = {};
  r32 XStart = 0;
  r32 YStart = 0;
  r32 Width  = 0;
  r32 Height = 0;

  r32 HeaderSize = Node->MenuHeader->HeaderSize;
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

  tabbed_header_window* Window = Node->TabbedHeader;

  window_regions Result = CheckRegions(MousePos, ArrayCount(RegionArray), RegionArray, Node);

  return Result;
}

MENU_DRAW( TabbedHeaderDraw )
{
  tabbed_header_window* Window = Node->TabbedHeader;
  v4 HeaderColor = V4(0.3,0.5,0.3,1);
  DEBUGPushQuad(Node->Functions->GetRegionRect(window_regions::Header, Node), HeaderColor);
}


menu_functions TabbedHeaderMenuFunctions()
{
  menu_functions Result = {};
  Result.MouseDown = TabbedHeaderMouseDown;
  Result.MouseUp = TabbedHeaderMouseUp;
  Result.MouseEnter = TabbedHeaderMouseEnter;
  Result.MouseExit = TabbedHeaderMouseExit;
  Result.HandleInput = TabbedHeaderHandleInput;
  Result.UpdateRegions = TabbedHeaderUpdateRegions;
  Result.GetRegionRect = TabbedHeaderGetRegionRect;
  Result.GetSubMenuRegionRect = EmptyGetSubMenuRegionRect;
  Result.GetMouseOverRegion = TabbedHeaderGetMouseOverRegion;
  Result.Draw = TabbedHeaderDraw;
  return Result;
}

MENU_MOUSE_DOWN( VerticalSplitMouseDown )
{
  split_window* Window = Node->SplitWindow;
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
  split_window* Window = Node->SplitWindow;
  Window->BorderDrag = false;
  Window->WindowDrag[0] = false;
  Window->WindowDrag[1] = false;
}

MENU_MOUSE_ENTER( VerticalSplitMouseEnter )
{

}
MENU_MOUSE_EXIT( VerticalSplitMouseExit )
{

}

MENU_HANDLE_INPUT( VerticalSplitHandleInput )
{
  split_window* Window = Node->SplitWindow;
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

MENU_UPDATE_REGIONS( VerticalSplitUpdateRegions )
{
  split_window* Window = (split_window*) Node->Container;
  r32 MinFrac = Window->MinSize / Node->Region.W;

  r32 LeftSize  = Node->Region.W * Window->SplitFraction;
  r32 RightSize = Node->Region.W * (1-Window->SplitFraction);
  b32 LeftTooSmall = LeftSize < Window->MinSize;
  b32 RightTooSmall = RightSize < Window->MinSize;
  if(LeftTooSmall && RightTooSmall)
  {
    Window->SplitFraction = 0.5;
  }else if( LeftTooSmall )
  {
    Window->SplitFraction = MinFrac;
  }else if( RightTooSmall )
  {
    Window->SplitFraction = 1-MinFrac;
  }

//  Window->Container[0]->Region = Node->Functions->GetRegionRect(window_regions::LeftBody, Node);
//  Window->Container[1]->Region = Node->Functions->GetRegionRect(window_regions::RightBody, Node);
//  Window->Container[0]->Functions->UpdateRegions(Window->Container[0]);
//  Window->Container[1]->Functions->UpdateRegions(Window->Container[1]);
}

MENU_GET_REGION_RECT( VerticalSplitGetRegionRect )
{
  rect2f Result = {};
  r32 XStart = 0;
  r32 YStart = 0;
  r32 Width  = 0;
  r32 Height = 0;

  rect2f Region = Node->Region;
  r32 BorderSize = Node->SplitWindow->BorderSize;

  r32 LeftWidth = Node->SplitWindow->SplitFraction * Region.W - 0.5f * BorderSize;
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

MENU_GET_SUB_MENU_REGION_RECT( VerticalSplitGetSubMenuRegionRect )
{
  window_regions Result = (SubMenuIndex == 1) ? window_regions::WholeBody : window_regions::None;
  switch(SubMenuIndex)
  {
    case 0: return VerticalSplitGetRegionRect(window_regions::LeftBody, Node);
    case 1: return VerticalSplitGetRegionRect(window_regions::RightBody, Node);
    default: Assert(0);
  }
  return {};
}

MENU_GET_MOUSE_OVER_REGION( VerticalSplitGetMouseOverRegion )
{
  window_regions RegionArray[] =
  {
    window_regions::LeftBody,
    window_regions::RightBody,
    window_regions::VerticalBorder
  };

  split_window* Window = Node->SplitWindow;

  window_regions Result = CheckRegions(MousePos, ArrayCount(RegionArray), RegionArray, Node);

  return Result;
}

MENU_DRAW( VerticalSplitDraw )
{
  split_window* Window = Node->SplitWindow;
  v4 BodyColor   = V4(0.2,0.2,0.2,1);
  v4 BorderColor = V4(0,0,1,1);
  v4 HeaderColor = V4(0.4,0.4,0.4,1);
  DEBUGPushQuad(Node->Functions->GetRegionRect(window_regions::LeftBody, Node),  BodyColor);
  DEBUGPushQuad(Node->Functions->GetRegionRect(window_regions::RightBody, Node), BodyColor);
  DEBUGPushQuad(Node->Functions->GetRegionRect(window_regions::VerticalBorder, Node), BorderColor);

}

menu_functions VerticalSplitMenuFunctions()
{
  menu_functions Result = {};
  Result.MouseDown = VerticalSplitMouseDown;
  Result.MouseUp = SplitMouseUp;
  Result.MouseEnter = VerticalSplitMouseEnter;
  Result.MouseExit = VerticalSplitMouseExit;
  Result.HandleInput = VerticalSplitHandleInput;
  Result.UpdateRegions = VerticalSplitUpdateRegions;
  Result.GetRegionRect = VerticalSplitGetRegionRect;
  Result.GetSubMenuRegionRect = VerticalSplitGetSubMenuRegionRect;
  Result.GetMouseOverRegion = VerticalSplitGetMouseOverRegion;
  Result.Draw = VerticalSplitDraw;
  return Result;
}






MENU_MOUSE_DOWN( HorizontalMouseDown )
{
  split_window* Window = Node->SplitWindow;
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
  split_window* Window = Node->SplitWindow;
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
MENU_UPDATE_REGIONS( HorizontalUpdateRegions )
{
  split_window* Window = Node->SplitWindow;
  r32 MinFrac = Window->MinSize / Node->Region.H;

  r32 BotSize = Node->Region.H * Window->SplitFraction;
  r32 TopSize = Node->Region.H * (1-Window->SplitFraction);
  b32 BotTooSmall = BotSize < Window->MinSize;
  b32 TopTooSmall = TopSize < Window->MinSize;
  if(BotTooSmall && TopTooSmall)
  {
    Window->SplitFraction = 0.5;
  }else if( BotTooSmall )
  {
    Window->SplitFraction = MinFrac;
  }else if( TopTooSmall )
  {
    Window->SplitFraction = 1-MinFrac;
  }
//  Window->Container[0]->Region = Node->Functions->GetRegionRect(window_regions::BotBody, Node);
//  Window->Container[1]->Region = Node->Functions->GetRegionRect(window_regions::TopBody, Node);
//  Window->Container[0]->Functions->UpdateRegions(Window->Container[0]);
//  Window->Container[1]->Functions->UpdateRegions(Window->Container[1]);
}

MENU_GET_REGION_RECT( HorizontalGetRegionRect )
{
  rect2f Result = {};
  r32 XStart = 0;
  r32 YStart = 0;
  r32 Width  = 0;
  r32 Height = 0;

  rect2f Region = Node->Region;
  r32 BorderSize = Node->SplitWindow->BorderSize;

  r32 BotHeight = Node->SplitWindow->SplitFraction * Region.H - 0.5f * BorderSize;
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

  split_window* Window = Node->SplitWindow;

  window_regions Result = CheckRegions(MousePos, ArrayCount(RegionArray), RegionArray, Node);

  return Result;
}

MENU_GET_SUB_MENU_REGION_RECT( HorizontalSplitGetSubMenuRegionRect )
{
  window_regions Result = (SubMenuIndex == 1)? window_regions::WholeBody : window_regions::None;
  switch(SubMenuIndex)
  {
    case 0: return HorizontalGetRegionRect(window_regions::BotBody, Node);
    case 1: return HorizontalGetRegionRect(window_regions::TopBody, Node);
    default: Assert(0);
  }
  return {};
}
MENU_DRAW( HorizontalDraw )
{
  split_window* Window = Node->SplitWindow;
  v4 BodyColor   = V4(0.2,0.2,0.2,1);
  v4 BorderColor = V4(0,0,1,1);
  v4 HeaderColor = V4(0.4,0.4,0.4,1);
  DEBUGPushQuad(Node->Functions->GetRegionRect(window_regions::BotBody, Node), BodyColor);
  DEBUGPushQuad(Node->Functions->GetRegionRect(window_regions::TopBody, Node), BodyColor);
  DEBUGPushQuad(Node->Functions->GetRegionRect(window_regions::HorizontalBorder, Node), BorderColor);

}


menu_functions HorizontalMenuFunctions()
{
  menu_functions Result = {};
  Result.MouseDown = HorizontalMouseDown;
  Result.MouseUp = SplitMouseUp;
  Result.MouseEnter = HorizontalMouseEnter;
  Result.MouseExit = HorizontalMouseExit;
  Result.HandleInput = HorizontalHandleInput;
  Result.UpdateRegions = HorizontalUpdateRegions;
  Result.GetRegionRect = HorizontalGetRegionRect;
  Result.GetSubMenuRegionRect = HorizontalSplitGetSubMenuRegionRect;
  Result.GetMouseOverRegion = HorizontalGetMouseOverRegion;
  Result.Draw = HorizontalDraw;
  return Result;
}

