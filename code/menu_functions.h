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

MENU_UPDATE_CHILD_REGIONS( UpdateListChildRegions )
{
  container_node* Child = Parent->FirstChild;
  u32 Count = 0;
  while(Child)
  {
    Child = Child->NextSibling;
    Count++;
  }

  Child = Parent->FirstChild;
  u32 Index = 0;
  b32 Horizontal = GetListNode(Parent)->Horizontal;
  while(Child)
  {
    Child->Region =  Horizontal ? GetHorizontalListRegion(Parent->Region, Index, Count) : GetVerticalListRegion(Parent->Region, Index, Count);
    Child = Child->NextSibling;
    Index++;
  }
}

menu_functions GetListFunctions()
{
  menu_functions Result = GetDefaultFunctions();
  Result.UpdateChildRegions = UpdateListChildRegions;
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
    case container_type::List: return GetListFunctions();
    default: Assert(0);
  }
  return {};
}
