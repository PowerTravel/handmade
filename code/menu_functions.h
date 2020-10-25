#include "menu_interface.h"

MENU_HANDLE_INPUT( HandleInput )
{

}
MENU_UPDATE_CHILD_REGIONS( UpdateChildRegions )
{
  container_node* Child = Parent->FirstChild;
  while(Child)
  {
    Child->Region = Parent->Region;
    Child = Child->NextSibling;
  }
}
MENU_DRAW( Draw )
{

}

menu_functions GetDefaultFunctions()
{
  menu_functions Result = {};
  Result.HandleInput = HandleInput;
  Result.UpdateChildRegions = UpdateChildRegions;
  Result.Draw = Draw;
  return Result;
}

MENU_DRAW( ColorDraw )
{
  DEBUGPushQuad(Node->Region, GetColorNode(Node)->Color);
}

menu_functions GetColorFunctions()
{
  menu_functions Result = GetDefaultFunctions();
  Result.Draw = ColorDraw;
  return Result;
}


MENU_DRAW( BorderDraw )
{
  DEBUGPushQuad(Node->Region, GetBorderNode(Node)->Color);
}

menu_functions GetBorderFunctions()
{
  menu_functions Result = GetDefaultFunctions();
  Result.Draw = BorderDraw;
  return Result;
}

MENU_UPDATE_CHILD_REGIONS( RootUpdateChildRegions )
{
  container_node* Child = Parent->FirstChild;
  u32 BorderIndex = 0;
  container_node* Body = 0;
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
      Assert(!Body);
      Body = Child; 
    }
    Child = Child->NextSibling;
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

  rect2f BorderedRegion = Rect2f(
    Borders[0]->Position + 0.5f*Borders[0]->Thickness,
    Borders[2]->Position + 0.5f*Borders[2]->Thickness,
    Width  - (Borders[0]->Thickness + Borders[1]->Thickness),
    Height - (Borders[2]->Thickness + Borders[3]->Thickness));

  Body->Region = Rect2f(BorderedRegion.X, BorderedRegion.Y, BorderedRegion.W, BorderedRegion.H);

  Parent->Region = Rect2f(
    Borders[0]->Position - 0.5f*Borders[0]->Thickness,
    Borders[2]->Position - 0.5f*Borders[2]->Thickness,
    Width,
    Height);
  
}

menu_functions GetRootMenuFunctions()
{
  menu_functions Result = GetDefaultFunctions();
  Result.UpdateChildRegions = RootUpdateChildRegions;
  return Result;
}


MENU_UPDATE_CHILD_REGIONS( UpdateSplitChildRegions )
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
    Child = Child->NextSibling;
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
  Result.UpdateChildRegions = UpdateSplitChildRegions;
  return Result;
}


MENU_UPDATE_CHILD_REGIONS( UpdateHBFChildRegions )
{
  container_node* Child = Parent->FirstChild;
  u32 Index = 0;
  hbf_node* HBF = GetHBFNode(Parent);
  while(Child)
  {
    switch(Index)
    {
      case 0: // Header
      {
        Child->Region = Rect2f(
          Parent->Region.X,
          Parent->Region.Y + Parent->Region.H - HBF->HeaderSize,
          Parent->Region.W,
          HBF->HeaderSize);
      }break;
      case 1: // Body
      {
        Child->Region = Rect2f(
          Parent->Region.X,
          Parent->Region.Y + HBF->FooterSize,
          Parent->Region.W,
          Parent->Region.H - HBF->HeaderSize - HBF->FooterSize);
      }break;
      case 2: // Footer
      {
        Child->Region = Rect2f(
          Parent->Region.X,
          Parent->Region.Y,
          Parent->Region.W,
          HBF->FooterSize);
      }break;
      default:
      {
        INVALID_CODE_PATH
      }break;
    }
    ++Index;
    Child = Child->NextSibling;
  }
}

menu_functions GetHBFFunctions()
{
  menu_functions Result = GetDefaultFunctions();
  Result.UpdateChildRegions = UpdateHBFChildRegions;
  return Result;
}

MENU_UPDATE_CHILD_REGIONS( UpdateGridChildRegions )
{

  r32 Count = (r32) GetChildCount(Parent);
  if(!Count) return;

  container_node* Child = Parent->FirstChild;
  rect2f ParentRegion = Parent->Region; 

  grid_node* GridNode = GetGridNode(Parent);
  Assert(GridNode->Row || GridNode->Col);
  
  r32 Row = (r32)GridNode->Row;
  r32 Col = (r32)GridNode->Col;

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

menu_functions GetGridFunctions()
{
  menu_functions Result = GetDefaultFunctions();
  Result.UpdateChildRegions = UpdateGridChildRegions;
  return Result;
}

menu_functions GetMenuFunction(container_type Type)
{
  switch(Type)
  {   
    case container_type::None: return GetDefaultFunctions();
    case container_type::Root: return GetRootMenuFunctions();
    case container_type::Color: return GetColorFunctions();
    case container_type::Border: return GetBorderFunctions();
    case container_type::Split: return GetSplitFunctions();
    case container_type::HBF: return GetHBFFunctions();
    case container_type::Grid: return GetGridFunctions();
    default: Assert(0);
  }
  return {};
}
