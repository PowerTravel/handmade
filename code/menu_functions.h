#include "menu_interface.h"

MENU_UPDATE_CHILD_REGIONS(UpdateChildRegions)
{
  container_node* Child = Parent->FirstChild;
  while(Child)
  {
    Child->Region = Parent->Region;
    Child = Next(Child);
  }
}

menu_functions GetDefaultFunctions()
{
  menu_functions Result = {};
  Result.UpdateChildRegions = DeclareFunction(menu_get_region, UpdateChildRegions);
  Result.Draw = 0;
  return Result;
}

MENU_UPDATE_CHILD_REGIONS(RootUpdateChildRegions)
{
  container_node* Child = Parent->FirstChild;
  u32 BorderIndex = 0;
  container_node* Body = 0;
  container_node* Header = 0;
  container_node* BorderNodes[4] = {};
  border_leaf* Borders[4] = {};
  while(Child)
  {
    if(Child->Type == container_type::Border)
    {
      // Left->Right->Bot->Top;
      BorderNodes[BorderIndex] = Child;
      Borders[BorderIndex] = GetBorderNode(Child);
      BorderIndex++;
    }else{
      if(!Body)
      {
        Assert(!Body);
        Body = Child;
      }
      
    }
    Child = Next(Child);
  }

  r32 Width  = (Borders[1]->Position + 0.5f*Borders[1]->Thickness) - ( Borders[0]->Position - 0.5f*Borders[0]->Thickness);
  r32 Height = (Borders[3]->Position + 0.5f*Borders[3]->Thickness) - ( Borders[2]->Position - 0.5f*Borders[2]->Thickness);
  for (BorderIndex = 0; BorderIndex < ArrayCount(Borders); ++BorderIndex)
  {
    switch(BorderIndex)
    {
      case 0: // Left
      {
        BorderNodes[0]->Region = Rect2f(
          Borders[0]->Position - 0.5f * Borders[0]->Thickness,
          Borders[2]->Position - 0.5f * Borders[2]->Thickness,
          Borders[0]->Thickness,
          Height);
      }break;
      case 1: // Right
      {
        BorderNodes[1]->Region = Rect2f(
          Borders[1]->Position - 0.5f * Borders[1]->Thickness,
          Borders[2]->Position - 0.5f * Borders[2]->Thickness,
          Borders[1]->Thickness,
          Height);
      }break;
      case 2: // Bot
      {
        BorderNodes[2]->Region = Rect2f(
          Borders[0]->Position - 0.5f * Borders[0]->Thickness,
          Borders[2]->Position - 0.5f * Borders[2]->Thickness,
          Width,
          Borders[2]->Thickness);
      }break;
      case 3: // Top
      {
        BorderNodes[3]->Region = Rect2f(
          Borders[0]->Position - 0.5f * Borders[0]->Thickness,
          Borders[3]->Position - 0.5f * Borders[3]->Thickness,
          Width,
          Borders[3]->Thickness);
      }break;
    }
  }

  Assert(BorderIndex == 4);
  Assert(Body);

  rect2f InteralRegion = Rect2f(
    Borders[0]->Position + 0.5f*Borders[0]->Thickness,
    Borders[2]->Position + 0.5f*Borders[2]->Thickness,
    Width  - (Borders[0]->Thickness + Borders[1]->Thickness),
    Height - (Borders[2]->Thickness + Borders[3]->Thickness));

  r32 HeaderHeight = 0.02f;
  r32 BodyHeight = InteralRegion.H;//-HeaderHeight;

 // Header->Region = Rect2f(InteralRegion.X, InteralRegion.Y+BodyHeight, InteralRegion.W, HeaderHeight);
  Body->Region   = Rect2f(InteralRegion.X, InteralRegion.Y, InteralRegion.W, BodyHeight);

  Parent->Region = Rect2f(
    Borders[0]->Position - 0.5f*Borders[0]->Thickness,
    Borders[2]->Position - 0.5f*Borders[2]->Thickness,
    Width,
    Height);
  
}

menu_functions GetRootMenuFunctions()
{
  menu_functions Result = GetDefaultFunctions();
  Result.UpdateChildRegions = DeclareFunction(menu_get_region, RootUpdateChildRegions);
  return Result;
}


MENU_UPDATE_CHILD_REGIONS(UpdateSplitChildRegions)
{
  container_node* Child = Parent->FirstChild;
  container_node* Body1Node = 0;
  container_node* Body2Node = 0;
  container_node* BorderNode = 0;
  while(Child)
  {
    if( Child->Type == container_type::Border)
    {
      BorderNode = Child;
    }else{
      if(!Body1Node)
      {
        Body1Node = Child;
      }else{
        Assert(!Body2Node);
        Body2Node = Child;
      }
    }
    Child = Next(Child);
  }

  Assert(Body1Node && Body2Node && BorderNode);
  border_leaf* Border = GetBorderNode(BorderNode);
  if(Border->Vertical)
  {
    BorderNode->Region = Rect2f(
      Parent->Region.X + (Border->Position * Parent->Region.W - 0.5f*Border->Thickness),
      Parent->Region.Y,
      Border->Thickness,
      Parent->Region.H);
    Body1Node->Region = Rect2f(
      Parent->Region.X,
      Parent->Region.Y,
      Border->Position * Parent->Region.W - 0.5f*Border->Thickness,
      Parent->Region.H);
    Body2Node->Region = Rect2f(
      Parent->Region.X + (Border->Position * Parent->Region.W + 0.5f*Border->Thickness),
      Parent->Region.Y,
      Parent->Region.W - (Border->Position * Parent->Region.W+0.5f*Border->Thickness),
      Parent->Region.H);
  }else{
    BorderNode->Region = Rect2f(
      Parent->Region.X,
      Parent->Region.Y + Border->Position * Parent->Region.H - 0.5f*Border->Thickness,
      Parent->Region.W,
      Border->Thickness);
    Body1Node->Region = Rect2f(
      Parent->Region.X,
      Parent->Region.Y,
      Parent->Region.W,
      Border->Position * Parent->Region.H - 0.5f*Border->Thickness);
    Body2Node->Region = Rect2f(
      Parent->Region.X,
      Parent->Region.Y + (Border->Position * Parent->Region.H + 0.5f*Border->Thickness),
      Parent->Region.W,
      Parent->Region.H - (Border->Position * Parent->Region.H+0.5f*Border->Thickness));
  }
}

menu_functions GetSplitFunctions()
{
  menu_functions Result = GetDefaultFunctions();
  Result.UpdateChildRegions = DeclareFunction(menu_get_region,UpdateSplitChildRegions);
  return Result;
}

MENU_UPDATE_CHILD_REGIONS( UpdateTabWindowChildRegions)
{
  container_node* Child = Parent->FirstChild;
  u32 Index = 0;
  tab_window_node* TabWindow = GetTabWindowNode(Parent);
  while(Child)
  {
    switch(Index)
    {
      case 0: // Header
      {
        Child->Region = Rect2f(
          Parent->Region.X,
          Parent->Region.Y + Parent->Region.H - TabWindow->HeaderSize,
          Parent->Region.W,
          TabWindow->HeaderSize);
      }break;
      case 1: // Body
      {
        Child->Region = Rect2f(
          Parent->Region.X,
          Parent->Region.Y,
          Parent->Region.W,
          Parent->Region.H - TabWindow->HeaderSize);
      }break;
      default:
      {
        INVALID_CODE_PATH
      }break;
    }
    ++Index;
    Child = Next(Child);
  }
}

menu_functions GetTabWindowFunctions()
{
  menu_functions Result = GetDefaultFunctions();
  Result.UpdateChildRegions = DeclareFunction(menu_get_region, UpdateTabWindowChildRegions);
  return Result; 
}

MENU_UPDATE_CHILD_REGIONS( UpdateGridChildRegions )
{
  r32 Count = (r32) GetChildCount(Parent);
  if(!Count) return;

  rect2f ParentRegion = Parent->Region; 

  grid_node* GridNode = GetGridNode(Parent);
  r32 Row = (r32)GridNode->Row;
  r32 Col = (r32)GridNode->Col;

  Assert(GridNode->Row || GridNode->Col);
  
  if(Row == 0)
  {
    Assert(Col != 0);
    Row = Ciel(Count / Col);
  }else if(Col == 0){
    Assert(Row != 0);
    Col = Ciel(Count / Row);
  }else{
    Assert((Row * Col) >= Count);
  }

  container_node* Child = Parent->FirstChild;
  r32 MaxHeightForRow = 0;
  if(GridNode->Stack)
  {
    r32 xMargin = (GridNode->TotalMarginX / Col) * ParentRegion.W;
    r32 yMargin = (GridNode->TotalMarginY / Row) * ParentRegion.H;

    r32 Y0 = ParentRegion.Y + ParentRegion.H;
    r32 MaxWidth = 0;
    r32 MaxHeight = 0;
    for (r32 i = 0; i < Row; ++i)
    {
      r32 X0 = ParentRegion.X;
      for (r32 j = 0; j < Col; ++j)
      {
        r32 CellWidth = ParentRegion.W/Col;
        r32 CellHeight = ParentRegion.H/Row;
        size_attribute* Size = (size_attribute*) GetAttributePointer(Child, ATTRIBUTE_SIZE);
        if(Size->Width.Type == menu_size_type::ABSOLUTE)
        {
          CellWidth = Size->Width.Value;
        }else if(Size->Height.Type == menu_size_type::ABSOLUTE)
        {
          CellHeight = Size->Height.Value;
        }

        Child->Region = Rect2f(X0, Y0-CellHeight, CellWidth, CellHeight);
        Child->Region.X += xMargin/2.f;
        Child->Region.W -= xMargin;
        Child->Region.Y += yMargin/2.f;
        Child->Region.H -= yMargin;

        X0 += CellWidth;

        if(MaxHeightForRow < CellHeight)
        {
          MaxHeightForRow = CellHeight;
        }

        Child = Next(Child);
        if(!Child)
        {
          break;
        }
      }

      Y0 -= MaxHeightForRow;
      MaxHeightForRow = 0;
      r32 WidhtOfRow = X0 - ParentRegion.X;
      if(MaxWidth < WidhtOfRow)
      {
        MaxWidth = WidhtOfRow;
      }
      if(!Child)
      {
        break;
      }
    }
    r32 HeightOfColumn = (ParentRegion.Y + ParentRegion.H) - Y0;
    ParentRegion.W = MaxWidth;
    ParentRegion.H = HeightOfColumn;
    Parent->Region = ParentRegion;
  }else{

    r32 xMargin = (GridNode->TotalMarginX / Col) * ParentRegion.W;
    r32 yMargin = (GridNode->TotalMarginY / Row) * ParentRegion.H;

    r32 CellWidth  = ParentRegion.W / Col;
    r32 CellHeight = ParentRegion.H / Row;
    r32 X0 = ParentRegion.X;
    r32 Y0 = ParentRegion.Y + ParentRegion.H - CellHeight;
    for (r32 i = 0; i < Row; ++i)
    {
      for (r32 j = 0; j < Col; ++j)
      {
        Child->Region = Rect2f(X0 + j * CellWidth, Y0 - i* CellHeight, CellWidth, CellHeight);
        Child->Region.X += xMargin/2.f;
        Child->Region.W -= xMargin;
        Child->Region.Y += yMargin/2.f;
        Child->Region.H -= yMargin;
        Child = Next(Child);
        if(!Child)
        {
          break;
        }
      }
      if(!Child)
      {
        break;
      }
    }
  }
}

menu_functions GetGridFunctions()
{
  menu_functions Result = GetDefaultFunctions();
  Result.UpdateChildRegions = DeclareFunction(menu_get_region,UpdateGridChildRegions);
  return Result;
}

menu_functions GetMenuFunction(container_type Type)
{
  switch(Type)
  {   
    case container_type::None:      return GetDefaultFunctions();
    case container_type::Root:      return GetRootMenuFunctions();
    case container_type::Border:    return GetDefaultFunctions();
    case container_type::Split:     return GetSplitFunctions();
    case container_type::Grid:      return GetGridFunctions();
    case container_type::TabWindow: return GetTabWindowFunctions();
    case container_type::Tab:       return GetDefaultFunctions();
    case container_type::Plugin:    return GetDefaultFunctions();

    default: Assert(0);
  }
  return {};
}
