#include "debug.h"

u8* DEBUGPushSize_(debug_state* DebugState, u32 Size)
{
  u8* Result = DebugState->Memory;
  DebugState->Memory += Size;
  Assert( (DebugState->Memory ) < (DebugState->MemoryBase + DebugState->MemorySize)  );
  return Result;
}

MENU_DRAW(DrawFunctionTimeline);
#define DEBUGPushStruct(DebugState, type) (type*) DEBUGPushSize_(DebugState, sizeof(type))
#define DEBUGPushArray(DebugState, count, type) (type*) DEBUGPushSize_(DebugState, count*sizeof(type))
#define DEBUGPushCopy(DebugState, Size, Src) utils::Copy(Size, (void*) (Src), (void*) DEBUGPushSize_(DebugState, Size))

internal void RefreshCollation();
internal void RestartCollation();


//MENU_UPDATE_CHILD_REGIONS(DropDownUpdateChildRegions)
//{
//  container_node* Child = Parent->FirstChild;
//  if(Child)
//  {
//    Assert(!Child->NextSibling);
//    Assert(Child->Region.W > 0 && Child->Region.H > 0);
//    Child->Region.Y = Parent->Region.Y - Parent->Region.H - Child->Region.H;
//    Child->Region.X = Parent->Region.X;
//  }
//}

BUTTON_ATTRIBUTE_UPDATE(DebugToggleButton)
{
  v4 InactiveColor = V4(0.3,0.1,0.1,1);
  v4 ActiveColor = V4(0.1,0.3,0.1,1);

  if(Interface->MouseLeftButton.Active && Interface->MouseLeftButton.Edge)
  {
    b32* ButtonFlag = (b32*) Attr->Data;
    *ButtonFlag = !*ButtonFlag;
    color_attribute* ColorAttr =  (color_attribute*) GetAttributePointer(Node, ATTRIBUTE_COLOR);
    ColorAttr->Color = (*ButtonFlag) ?  ActiveColor : InactiveColor;
  }
}

BUTTON_ATTRIBUTE_UPDATE(DebugRecompileButton)
{
  if(Interface->MouseLeftButton.Active && Interface->MouseLeftButton.Edge)
  {
    DebugRewriteConfigFile();
  }
}

BUTTON_ATTRIBUTE_UPDATE(DebugPauseCollationButton)      
{
  debug_state* DebugState = DEBUGGetState();
  if(Interface->MouseLeftButton.Active && Interface->MouseLeftButton.Edge)
  {
    DebugState->Paused = !DebugState->Paused;
    if(!DebugState->Paused)
    {
      RefreshCollation();
    }
  }
}

internal debug_state*
DEBUGGetState()
{
  debug_state* DebugState = DebugGlobalMemory->DebugState;
  if(!DebugState)
  {
    DebugGlobalMemory->DebugState = BootstrapPushStruct(debug_state, Arena);
    DebugState = DebugGlobalMemory->DebugState;
  }

  if(!DebugState->Initialized)
  {
    // Permanent Memory
    DebugState->MemorySize = (midx) Megabytes(1);
    DebugState->MemoryBase = PushArray(&DebugState->Arena, DebugState->MemorySize, u8);
    DebugState->Memory = DebugState->MemoryBase;

    // Transient Memory Begin
    DebugState->CollateTemp = BeginTemporaryMemory(&DebugState->Arena);
    DebugState->Initialized = true;
    DebugState->Paused = false;
    DebugState->ScopeToRecord = 0;

    // Config state
    DebugState->ConfigMultiThreaded = MULTI_THREADED;
    DebugState->ConfigCollisionPoints = SHOW_COLLISION_POINTS;
    DebugState->ConfigCollider = SHOW_COLLIDER;
    DebugState->ConfigAABBTree = SHOW_AABB_TREE;

    // Set menu window
    u32 FontSize = 18;
    rect2f ButtonSize = GetTextSize(0, 0, "CollisionPoints", FontSize);
    v4 TextColor = V4(1,1,1,1);

    ButtonSize.W+=0.02f;
    ButtonSize.H+=0.02f;
    r32 ContainerWidth = 0.7;
    container_node* HBF1 = 0;
    {
      // Create Option Window
      container_node* ButtonContainer =  NewContainer(GlobalGameState->MenuInterface, container_type::Grid);
      grid_node* Grid = GetGridNode(ButtonContainer);
      Grid->Col = 1;
      Grid->Row = 0;
      Grid->TotalMarginX = 0.0;
      Grid->TotalMarginY = 0.0;

      color_attribute* BackgroundColor = (color_attribute* ) PushAttribute(GlobalGameState->MenuInterface, ButtonContainer, ATTRIBUTE_COLOR);
      BackgroundColor->Color = V4(0,0,0,0.7);

      auto CreateButton = [&ButtonSize, &FontSize, &TextColor]( b32* ButtonFlag, c8* ButtonText)
      {
        container_node* ButtonNode = NewContainer(GlobalGameState->MenuInterface);
        text_attribute* Text = (text_attribute*) PushAttribute(GlobalGameState->MenuInterface, ButtonNode, ATTRIBUTE_TEXT);
        Assert(str::StringLength( ButtonText ) < ArrayCount(Text->Text) );
        str::CopyStringsUnchecked( ButtonText, Text->Text );
        Text->FontSize = FontSize;
        Text->Color = TextColor;

        size_attribute* SizeAttr = (size_attribute*) PushAttribute(GlobalGameState->MenuInterface, ButtonNode, ATTRIBUTE_SIZE);
        SizeAttr->Width = ContainerSizeT(menu_size_type::ABSOLUTE, ButtonSize.W);
        SizeAttr->Height = ContainerSizeT(menu_size_type::ABSOLUTE, ButtonSize.H);
        SizeAttr->LeftOffset = ContainerSizeT(menu_size_type::RELATIVE, 0);
        SizeAttr->TopOffset = ContainerSizeT(menu_size_type::RELATIVE, 0);
        SizeAttr->XAlignment = menu_region_alignment::CENTER;
        SizeAttr->YAlignment = menu_region_alignment::CENTER;

        v4 InactiveColor = V4(0.3,0.1,0.1,1);
        v4 ActiveColor = V4(0.1,0.3,0.1,1);

        color_attribute* ColorAttr = (color_attribute*) PushAttribute(GlobalGameState->MenuInterface, ButtonNode, ATTRIBUTE_COLOR);
        ColorAttr->Color = *ButtonFlag ?  ActiveColor : InactiveColor;

        button_attribute* ButtonAttribute = (button_attribute*) PushAttribute(GlobalGameState->MenuInterface, ButtonNode, ATTRIBUTE_BUTTON);
        ButtonAttribute->Data = ButtonFlag;
        ButtonAttribute->Update = DeclareFunction(button_attribute_update, DebugToggleButton);

        return ButtonNode;
      };

      ConnectNode(ButtonContainer, CreateButton(&DebugState->ConfigMultiThreaded, "Multithread"));
      ConnectNode(ButtonContainer, CreateButton(&DebugState->ConfigCollisionPoints, "CollisionPoints"));
      ConnectNode(ButtonContainer, CreateButton(&DebugState->ConfigCollider, "Colliders"));
      ConnectNode(ButtonContainer, CreateButton(&DebugState->ConfigAABBTree, "AABBTree"));
      container_node* RecompileButton = ConnectNode(ButtonContainer, NewContainer(GlobalGameState->MenuInterface));
      {
        color_attribute* Color = (color_attribute*) PushAttribute(GlobalGameState->MenuInterface, RecompileButton, ATTRIBUTE_COLOR);
        Color->Color = V4(0.2,0.1,0.3,1);
        
        text_attribute* Text = (text_attribute*) PushAttribute(GlobalGameState->MenuInterface, RecompileButton, ATTRIBUTE_TEXT);
        str::CopyStringsUnchecked( "Recompile", Text->Text );
        Text->FontSize = FontSize;
        Text->Color = TextColor;

        size_attribute* SizeAttr = (size_attribute*) PushAttribute(GlobalGameState->MenuInterface, RecompileButton, ATTRIBUTE_SIZE);
        SizeAttr->Width = ContainerSizeT(menu_size_type::ABSOLUTE, ButtonSize.W);
        SizeAttr->Height = ContainerSizeT(menu_size_type::RELATIVE, 1);
        SizeAttr->LeftOffset = ContainerSizeT(menu_size_type::RELATIVE, 0);
        SizeAttr->TopOffset = ContainerSizeT(menu_size_type::RELATIVE, 0);
        SizeAttr->XAlignment = menu_region_alignment::CENTER;
        SizeAttr->YAlignment = menu_region_alignment::CENTER;

        button_attribute* ButtonAttribute = (button_attribute*) PushAttribute(GlobalGameState->MenuInterface, RecompileButton, ATTRIBUTE_BUTTON);
        ButtonAttribute->Update = DeclareFunction(button_attribute_update, DebugRecompileButton);
      }
      HBF1 = CreateHBF(GlobalGameState->MenuInterface, "Settings", V4(0.5,0.5,0.5,1), ButtonContainer);
    }

    // Create graph window
    container_node* HBF2 = 0;
    {
      container_node* ButtonContainer = NewContainer(GlobalGameState->MenuInterface, container_type::Grid);
      grid_node* Grid = GetGridNode(ButtonContainer);
      Grid->Col = 1;
      Grid->TotalMarginX = 0.3;
      Grid->TotalMarginY = 0.2;

      container_node* Button =  ConnectNode(ButtonContainer, NewContainer(GlobalGameState->MenuInterface));
      color_attribute* ButtonColor = (color_attribute*) PushAttribute(GlobalGameState->MenuInterface, Button, ATTRIBUTE_COLOR);
      ButtonColor->Color = V4(0.3,0.1,0.3,1);

      text_attribute* Text = (text_attribute*) PushAttribute(GlobalGameState->MenuInterface, Button, ATTRIBUTE_TEXT);
      str::CopyStringsUnchecked( "Pause", Text->Text );
      Text->FontSize = FontSize;
      Text->Color = TextColor;

      button_attribute* ButtonAttribute = (button_attribute*) PushAttribute(GlobalGameState->MenuInterface, Button, ATTRIBUTE_BUTTON);
      ButtonAttribute->Update = DeclareFunction(button_attribute_update, DebugPauseCollationButton);

      container_node* GraphContainer = NewContainer(GlobalGameState->MenuInterface);
      GraphContainer->Functions.Draw = DeclareFunction(menu_draw, DrawFunctionTimeline);
      
      container_node* SplitNode  = NewContainer(GlobalGameState->MenuInterface, container_type::Split);
      color_attribute* BackgroundColor = (color_attribute* ) PushAttribute(GlobalGameState->MenuInterface, SplitNode, ATTRIBUTE_COLOR);
      BackgroundColor->Color = V4(0,0,0,0.7);

      container_node* BorderNode = CreateBorderNode(GlobalGameState->MenuInterface, false, 0.2);
      ConnectNode(SplitNode, BorderNode);
      ConnectNode(SplitNode, ButtonContainer);
      ConnectNode(SplitNode, GraphContainer);

      HBF2 = CreateHBF(GlobalGameState->MenuInterface, "Profiler", V4(0.5,0.5,ContainerWidth,1), SplitNode);
    }


    #if 1

    menu_tree* WindowsDropDownMenu = RegisterMenu(GlobalGameState->MenuInterface, "Windows");
    RegisterWindow(GlobalGameState->MenuInterface, WindowsDropDownMenu, HBF1, "Settings");
    RegisterWindow(GlobalGameState->MenuInterface, WindowsDropDownMenu, HBF2, "Profiler");

    #else

    container_node* Split = SetSplitWindows(GlobalGameState->MenuInterface, CreateSplitWindow(GlobalGameState->MenuInterface, true, ButtonSize.W/ContainerWidth), HBF1, HBF2);

    menu_tree* Root = CreateNewRootContainer(GlobalGameState->MenuInterface, Split, Rect2f( 0.85, 0.25, 0.7, 0.5));
    container_node* RootHBF = Root->Root->FirstChild->NextSibling->NextSibling->NextSibling->NextSibling;
    Assert(RootHBF->Type == container_type::HBF);
    container_node* RootHeader = RootHBF->FirstChild;

    container_node* DropDownContainer = ConnectNode(RootHeader, NewContainer(GlobalGameState->MenuInterface, container_type::Grid));
    grid_node* Grid = GetGridNode(DropDownContainer);
    Grid->Col = 0;
    Grid->Row = 1;
    Grid->TotalMarginX = 0.0;
    Grid->TotalMarginY = 0.0;

    container_node* ViewMenu = ConnectNode(DropDownContainer, NewContainer(GlobalGameState->MenuInterface));

    
    //ViewMenu->Functions.UpdateChildRegions = DeclareFunction(menu_get_region, DropDownUpdateChildRegions);

    color_attribute* ColorAttr = (color_attribute*) PushAttribute(GlobalGameState->MenuInterface, ViewMenu, ATTRIBUTE_COLOR);
    ColorAttr->Color = V4(0.3,0,0.3,1);
    
    text_attribute* Text = (text_attribute*) PushAttribute(GlobalGameState->MenuInterface, ViewMenu, ATTRIBUTE_TEXT);
    str::CopyStringsUnchecked( "Windows", Text->Text );
    Text->FontSize = 12;
    Text->Color = V4(0,0,0,1);
    rect2f WindowsSize = GetTextSize(0, 0, Text->Text, Text->FontSize);

    size_attribute* SizeAttr = (size_attribute*) PushAttribute(GlobalGameState->MenuInterface, DropDownContainer, ATTRIBUTE_SIZE);
    SizeAttr->Width = ContainerSizeT(menu_size_type::ABSOLUTE, WindowsSize.W+0.015f);
    SizeAttr->Height = ContainerSizeT(menu_size_type::RELATIVE, 1);
    SizeAttr->LeftOffset = ContainerSizeT(menu_size_type::ABSOLUTE, 0);
    SizeAttr->TopOffset = ContainerSizeT(menu_size_type::ABSOLUTE, 0);
    SizeAttr->XAlignment = menu_region_alignment::LEFT;
    SizeAttr->YAlignment = menu_region_alignment::CENTER;

    menu_tree* ViewMenuRoot = NewMenuTree(GlobalGameState->MenuInterface); // Root
    ViewMenuRoot->Root = NewContainer(GlobalGameState->MenuInterface);
    ViewMenuRoot->Root->Region = {};
    ViewMenuRoot->LosingFocus = DeclareFunction(menu_losing_focus, DropDownLosingFocus);
    ViewMenuRoot->GainingFocus = DeclareFunction(menu_losing_focus, DropDownGainingFocus);

    container_node* ViewMenuItems = ConnectNode(ViewMenuRoot->Root, NewContainer(GlobalGameState->MenuInterface, container_type::Grid));
    Grid = GetGridNode(ViewMenuItems);
    Grid->Col = 1;
    Grid->Row = 0;
    Grid->TotalMarginX = 0.0;
    Grid->TotalMarginY = 0.0;


    {
      container_node* MenuItem = ConnectNode(ViewMenuItems, NewContainer(GlobalGameState->MenuInterface));
      text_attribute* MenuText = (text_attribute*) PushAttribute(GlobalGameState->MenuInterface, MenuItem, ATTRIBUTE_TEXT);
      str::CopyStringsUnchecked( "Profiler", MenuText->Text );
      MenuText->FontSize = 12;
      MenuText->Color = V4(0,0,0,1);
      
      rect2f TextSize = GetTextSize(0, 0, MenuText->Text, MenuText->FontSize);
      ViewMenuRoot->Root->Region.H += TextSize.H;
      ViewMenuRoot->Root->Region.W = ViewMenuRoot->Root->Region.W >= TextSize.W ? ViewMenuRoot->Root->Region.W : TextSize.W;
    }

    {
      container_node* MenuItem = ConnectNode(ViewMenuItems, NewContainer(GlobalGameState->MenuInterface));
      text_attribute* MenuText = (text_attribute*) PushAttribute(GlobalGameState->MenuInterface, MenuItem, ATTRIBUTE_TEXT);
      str::CopyStringsUnchecked( "Settings", MenuText->Text );
      MenuText->FontSize = 12;
      MenuText->Color = V4(0,0,0,1);
      
      rect2f TextSize = GetTextSize(0, 0, MenuText->Text, MenuText->FontSize);
      ViewMenuRoot->Root->Region.H += TextSize.H;
      ViewMenuRoot->Root->Region.W = ViewMenuRoot->Root->Region.W >= TextSize.W ? ViewMenuRoot->Root->Region.W : TextSize.W;
    }



    ColorAttr = (color_attribute*) PushAttribute(GlobalGameState->MenuInterface, ViewMenuItems, ATTRIBUTE_COLOR);
    ColorAttr->Color = V4(1,1,1,1);

    button_attribute* ButtonAttribute = (button_attribute*) PushAttribute(GlobalGameState->MenuInterface, ViewMenu, ATTRIBUTE_BUTTON);
    ButtonAttribute->Update = DeclareFunction(button_attribute_update, DropDownMenuButton);
    ButtonAttribute->Data = ViewMenuRoot;

    TreeSensus(ViewMenuRoot);

    UpdateRegions(ViewMenuRoot);

    #endif
    
    RestartCollation();
  }
  return DebugState;
}

internal void
RestartCollation()
{
    debug_state* DebugState = DEBUGGetState();
    // We can store MAX_DEBUG_EVENT_ARRAY_COUNT*4 frames of collated debug records
    // However, when we change Scope we clear ALL the collated memory.
    // So when we recollate we only have MAX_DEBUG_EVENT_ARRAY_COUNT frames worth of data
    // Thus we loose the 3 * MAX_DEBUG_EVENT_ARRAY_COUNT worth of collated data.
    // One effect of this is that we can display 10 frames, but MAX_DEBUG_EVENT_ARRAY_COUNT is atm 8;
    // Thus when we klick a function to inspect we suddenly only display 7 frames
    EndTemporaryMemory(DebugState->CollateTemp);
    DebugState->CollateTemp = BeginTemporaryMemory(&DebugState->Arena);

    DebugState->FirstThread = 0;
    DebugState->FirstFreeBlock = 0;
    DebugState->Frames = PushArray(&DebugState->Arena, MAX_DEBUG_EVENT_ARRAY_COUNT*4, debug_frame);
    DebugState->FrameBarLaneCount = 0;
    DebugState->FrameCount = 0;
    DebugState->FrameBarRange = 0;//60000000.0f;
    DebugState->CollationArrayIndex = GlobalDebugTable->CurrentEventArrayIndex+1;
    DebugState->CollationFrame = 0;
}

void BeginDebugStatistics(debug_statistics* Statistic)
{
  Statistic->Count = 0;
  Statistic->Min =  R32Max;
  Statistic->Max = -R32Max;
  Statistic->Avg = 0;
}

void EndDebugStatistics(debug_statistics* Statistic)
{
  if(Statistic->Count != 0)
  {
    Statistic->Avg /= Statistic->Count;
  }else{
    Statistic->Min = 0;
    Statistic->Max = 0;
  }
}

void AccumulateStatistic(debug_statistics* Statistic, r32 Value)
{
  if(Statistic->Min > Value)
  {
    Statistic->Min = Value;
  }
  if(Statistic->Max < Value)
  {
    Statistic->Max = Value;
  }
  Statistic->Avg += Value;
  ++Statistic->Count;
}


#define DebugRecords_Main_Count __COUNTER__

#if HANDMADE_PROFILE
global_variable debug_table GlobalDebugTable_;
debug_table* GlobalDebugTable = &GlobalDebugTable_;
#else
debug_table* GlobalDebugTable = 0;
#endif
internal debug_thread* GetDebugThread( debug_state* DebugState, u32 ThreadID)
{
  debug_thread* Result = 0;
  for(debug_thread* Thread = DebugState->FirstThread;
      Thread;
      Thread = Thread->Next)
  {
    if(Thread->ID == ThreadID)
    {
      Result = Thread;
      break;
    }
  }

  if(!Result)
  {
    Result = PushStruct(&DebugState->Arena, debug_thread );
    Result->ID = ThreadID;
    Result->LaneIndex = DebugState->FrameBarLaneCount++;
    Result->FirstOpenBlock = 0;
    Result->Next = DebugState->FirstThread;
    DebugState->FirstThread = Result;
  }

  return Result;
}

internal debug_frame_region*
AddRegion(debug_frame* CurrentFrame)
{
  debug_frame_region* Result = CurrentFrame->Regions + CurrentFrame->RegionCount;
  if(CurrentFrame->RegionCount < MAX_REGIONS_PER_FRAME-1)
  {
    CurrentFrame->RegionCount++;
  }
  return Result;
}

internal debug_record*
GetRecordFrom(open_debug_block* OpenBlock)
{
  debug_record* Result = OpenBlock ? OpenBlock->Record : 0;
  return Result;
}

void CollateDebugRecords()
{
  debug_state* DebugState = DEBUGGetState();
  for(;;++DebugState->CollationArrayIndex)
  {
    if( DebugState->CollationArrayIndex  == MAX_DEBUG_EVENT_ARRAY_COUNT)
    {
       DebugState->CollationArrayIndex = 0;
    }

    u32 EventArrayIndex = DebugState->CollationArrayIndex;
    if( EventArrayIndex == GlobalDebugTable->CurrentEventArrayIndex)
    {
      break;
    }

    debug_frame* CurrentFrame = DebugState->CollationFrame;

    for(u32 EventIndex = 0;
            EventIndex < GlobalDebugTable->EventCount[EventArrayIndex];
            ++EventIndex)
    {
      debug_event*  Event = GlobalDebugTable->Events[EventArrayIndex] + EventIndex;
      debug_record* Source = (GlobalDebugTable->Records[Event->TranslationUnit] + Event->DebugRecordIndex);

      if(Event->Type == DebugEvent_FrameMarker)
      {
        if(CurrentFrame)
        {
          CurrentFrame->EndClock = Event->Clock;
          CurrentFrame->WallSecondsElapsed = Event->SecondsElapsed;

          r32 ClockRange = (r32)(CurrentFrame->EndClock - CurrentFrame->BeginClock);
          if(ClockRange > 0.0f)
          {
            r32 FrameBarRange = ClockRange;
            if(DebugState->FrameBarRange < FrameBarRange)
            {
              DebugState->FrameBarRange = FrameBarRange;
            }
          }
        }

        DebugState->CollationFrame = DebugState->Frames + DebugState->FrameCount++;
        CurrentFrame = DebugState->CollationFrame;
        CurrentFrame->BeginClock = Event->Clock;
        CurrentFrame->EndClock = 0;
        CurrentFrame->RegionCount = 0;
        CurrentFrame->WallSecondsElapsed = 0;
        CurrentFrame->Regions = PushArray(&DebugState->Arena, MAX_REGIONS_PER_FRAME, debug_frame_region);

      }else if(CurrentFrame){
        u32 FrameIndex = DebugState->FrameCount - 1;
        debug_thread* Thread = GetDebugThread(DebugState, Event->TC.ThreadID);
        u64 RelativeClock = Event->Clock - CurrentFrame->BeginClock;

        if(Event->Type == DebugEvent_BeginBlock)
        {
          open_debug_block* DebugBlock = DebugState->FirstFreeBlock;
          if(DebugBlock)
          {
            DebugState->FirstFreeBlock = DebugBlock->NextFree;
          }else{
            DebugBlock = PushStruct(&DebugState->Arena, open_debug_block);
          }

          DebugBlock->StartingFrameIndex = FrameIndex;
          DebugBlock->OpeningEvent = *Event;
          DebugBlock->Record = Source;

          DebugBlock->Parent = Thread->FirstOpenBlock;
          Thread->FirstOpenBlock = DebugBlock;
          DebugBlock->NextFree = 0;
        }else if( Event->Type == DebugEvent_EndBlock){
          if(Thread->FirstOpenBlock)
          {
            Assert(CurrentFrame->Regions);
            open_debug_block* MatchingBlock = Thread->FirstOpenBlock;
            debug_event* OpeningEvent = &MatchingBlock->OpeningEvent;
            if((OpeningEvent->TC.ThreadID         == Event->TC.ThreadID) &&
               (OpeningEvent->DebugRecordIndex == Event->DebugRecordIndex) &&
               (OpeningEvent->TranslationUnit  == Event->TranslationUnit))
            {
              if(MatchingBlock->StartingFrameIndex == FrameIndex)
              {
                if(GetRecordFrom(MatchingBlock->Parent) == DebugState->ScopeToRecord)
                {
                  r32 MinT = (r32)(OpeningEvent->Clock - CurrentFrame->BeginClock);
                  r32 MaxT = (r32)(Event->Clock -  CurrentFrame->BeginClock);
                  r32 ThresholdT = 2000;
                  if((MaxT-MinT) > ThresholdT )
                  {
                    debug_frame_region* Region = AddRegion(CurrentFrame);
                    Region->LaneIndex = Thread->LaneIndex;
                    Region->MinT = (r32)(OpeningEvent->Clock - CurrentFrame->BeginClock);
                    Region->MaxT = (r32)(Event->Clock -  CurrentFrame->BeginClock);
                    Region->Record = Source;
                    Region->ColorIndex = (u16)OpeningEvent->DebugRecordIndex;
                  }
                }
              }else{
                // Record All frames in between and begin/end spans
              }

              Thread->FirstOpenBlock->NextFree = DebugState->FirstFreeBlock;
              DebugState->FirstFreeBlock = Thread->FirstOpenBlock;
              Thread->FirstOpenBlock = MatchingBlock->Parent;
            }else{
              // Record span that goes to the beginning of the frame
            }
          }
        }else{
          Assert(!"Invalid event type");
        }
      }
    }
  }
}
internal void
RefreshCollation()
{
  RestartCollation();
  CollateDebugRecords();
}

inline void
DebugRewriteConfigFile()
{
  debug_state* DebugState = DEBUGGetState();

  c8 Buffer[4096] = {};
  u32 Size = Platform.DEBUGFormatString(Buffer, sizeof(Buffer), sizeof(Buffer)-1,
"#define MULTI_THREADED %d // b32\n\
#define SHOW_COLLISION_POINTS %d // b32\n\
#define SHOW_COLLIDER %d // b32\n\
#define SHOW_AABB_TREE %d // b32",
    DebugState->ConfigMultiThreaded,
    DebugState->ConfigCollisionPoints,
    DebugState->ConfigCollider,
    DebugState->ConfigAABBTree);
  thread_context Dummy = {};

  Platform.DEBUGPlatformWriteEntireFile(&Dummy, "W:\\handmade\\code\\debug_config.h", Size, Buffer);

  //DebugState->UpdateConfig = false;
  DebugState->Compiler = Platform.DEBUGExecuteSystemCommand("W:\\handmade\\code", "C:\\windows\\system32\\cmd.exe", "/C build_game.bat");
  DebugState->Compiling = true;

}


extern "C" DEBUG_GAME_FRAME_END(DEBUGGameFrameEnd)
{
  if(!GlobalDebugTable) return 0;
  GlobalDebugTable->RecordCount[0] = DebugRecords_Main_Count;

  ++GlobalDebugTable->CurrentEventArrayIndex;
  if(GlobalDebugTable->CurrentEventArrayIndex >= ArrayCount(GlobalDebugTable->Events))
  {
    GlobalDebugTable->CurrentEventArrayIndex=0;
  }
  u64 ArrayIndex_EventIndex = AtomicExchangeu64(&GlobalDebugTable->EventArrayIndex_EventIndex,
                                               ((u64)GlobalDebugTable->CurrentEventArrayIndex << 32));

  u32 EventArrayIndex = (ArrayIndex_EventIndex >> 32);
  u32 EventCount = (ArrayIndex_EventIndex & 0xFFFFFFFF);
  GlobalDebugTable->EventCount[EventArrayIndex] = EventCount;

  debug_state* DebugState = DEBUGGetState();
  if(DebugState)
  {
    if(!DebugState->Paused)
    {
      if(DebugState->FrameCount >= 4*MAX_DEBUG_EVENT_ARRAY_COUNT)
      {
        RestartCollation();
      }
      CollateDebugRecords();
    }
  }
  return GlobalDebugTable;
}



#if 0
internal void
BeginRadialMenu(radial_menu* RadialMenu, v2 MouseButtonPos)
{
  r32 Radius = 1/8.f;
  r32 RadiusBegin = 0.5f*Radius;

  r32 AngleCenter = Tau32/4.f;
  r32 AngleHalfSlice  = Pi32/(r32)RadialMenu->MenuRegionCount;
  for (u32 RegionIndex = 0; RegionIndex < RadialMenu->MenuRegionCount; ++RegionIndex)
  {
    RadialMenu->Regions[RegionIndex] =
      RadialMenuRegion(AngleCenter - AngleHalfSlice,
                       AngleCenter + AngleHalfSlice,
                       RadiusBegin, Radius);
      AngleCenter+=2*AngleHalfSlice;
  }

  RadialMenu->MenuX = MouseButtonPos.X;
  RadialMenu->MenuY = MouseButtonPos.Y;
}

internal void
EndRadialMenu(radial_menu* RadialMenu)
{
  // Reset all-non persistant event states
  for(u32 MenuItemIndex = 0; MenuItemIndex < RadialMenu->MenuRegionCount; ++MenuItemIndex)
  {
    menu_item* MenuItem = &RadialMenu->MenuItems[MenuItemIndex];
    MenuItem->MouseOverState = {};
    MenuItem->MenuActivationState  = {};
  }

  RadialMenu->MouseRadius = 0;
  RadialMenu->MouseAngle  = 0;
  RadialMenu->MenuX = 0;
  RadialMenu->MenuY = 0;

}


void DrawMenu( radial_menu* RadialMenu )
{
  v4 IdleColor              = V4(0  ,0  ,0.4,1.f);
  v4 IdleHighlightedColor   = V4(0.2,0.2,0.5,1.f);
  v4 ActiveColor            = V4(0  ,0.4,  0,1.f);
  v4 ActiveHighlightedColor = V4(0.2,0.5,0.2,1.f);

  if(!RadialMenu)
  {
    return;
  }


  for(u32 ItemIndex = 0; ItemIndex < RadialMenu->MenuRegionCount; ++ItemIndex)
  {
    radial_menu_region* Region = &RadialMenu->Regions[ItemIndex];
    menu_item* MenuItem = &RadialMenu->MenuItems[ItemIndex];


    r32 AngleCenter = RecanonicalizeAngle(Region->AngleStart + 0.5f * GetDeltaAngle(Region->AngleStart, Region->AngleEnd));

    v2 ItemLine   = V2(RadialMenu->MenuX + Region->Radius * Cos(AngleCenter),
                     RadialMenu->MenuY + Region->Radius * Sin(AngleCenter));
    DEBUGDrawDottedLine(V2(RadialMenu->MenuX, RadialMenu->MenuY) , ItemLine,  V4(0,0.7,0,1));

    v4 Color = IdleColor;

    if(MenuItem->MouseOverState.Active)
    {
      if(MenuItem->Active)
      {
        Color = ActiveHighlightedColor;
      }else{
        Color = IdleHighlightedColor;
      }
    }else{
      if(MenuItem->Active)
      {
        Color = ActiveColor;
      }else{
        Color = IdleColor;
      }
    }

    r32 ItemAngle = AngleCenter;

    r32 TextPosX = RadialMenu->MenuX + Region->Radius*Cos(AngleCenter);
    r32 TextPosY = RadialMenu->MenuY + Region->Radius*Sin(AngleCenter);

    rect2f TextBox = GetTextSize(TextPosX, TextPosY, MenuItem->Header);

    r32 Anglef0 = Tau32/8.f;
    r32 Anglef1 = 3*Tau32/8.f;
    r32 Anglef2 = 5*Tau32/8.f;
    r32 Anglef3 = 7*Tau32/8.f;

    v2 BottomLeft    = V2(0,0);
    v2 BottomRight   = -1.0f * V2(TextBox.W, 0);
    v2 TopRight      = -1.0f * V2(TextBox.W, TextBox.H);
    v2 TopLeft       = -1.0f * V2(0, TextBox.H);

    v2 Start,End;
    r32 StartAngle, StopAngle;
    if(IsInRegion(Anglef0, Anglef1,ItemAngle))
    {
      Start = BottomLeft;
      End = BottomRight;
      StartAngle = Anglef0;
      StopAngle = Anglef1;
    }else if(IsInRegion(Anglef1, Anglef2,ItemAngle)){
      Start = BottomRight;
      End = TopRight;
      StartAngle = Anglef1;
      StopAngle = Anglef2;
    }else if(IsInRegion(Anglef2, Anglef3,ItemAngle)){
      Start = TopRight;
      End = TopLeft;
      StartAngle = Anglef2;
      StopAngle = Anglef3;
    }else{
      Start = TopLeft;
      End = BottomLeft;
      StartAngle = Anglef3;
      StopAngle = Anglef0;
    }

    r32 AngleParameter = GetParametarizedAngle(StartAngle, StopAngle, ItemAngle);

    //TextBox
    r32 X0 = TextPosX - TextBox.W*0.5f;
    r32 Y0 = TextPosY - TextBox.H*0.5f;

    v2 Offset = Start + AngleParameter * (End - Start);

    rect2f QuadRect = Rect2f(TextBox.X + Offset.X,
                             TextBox.Y + Offset.Y,
                             TextBox.W,TextBox.H);
    PushOverlayQuad(QuadRect, Color);

    DEBUGTextOutAt(TextPosX+Offset.X, TextPosY+Offset.Y, MenuItem->Header, V4(1,1,1,1));
  }

  v2 MouseLine  = V2(RadialMenu->MenuX + RadialMenu->MouseRadius * Cos(RadialMenu->MouseAngle),
                     RadialMenu->MenuY + RadialMenu->MouseRadius * Sin(RadialMenu->MouseAngle));
  DEBUGDrawDottedLine(V2(RadialMenu->MenuX, RadialMenu->MenuY) , MouseLine,  V4(0.7,0,0,1));
}
#endif

inline internal debug_frame*
GetActiveDebugFrame(debug_state* DebugState)
{
  debug_frame* Result = DebugState->Frames + DebugState->FrameCount-2;
  return Result;
}

MENU_DRAW(DrawFunctionTimeline)
{
  rect2f Chart = Node->Region;

  game_window_size WindowSize = GameGetWindowSize();
  r32 AspectRatio = WindowSize.WidthPx/WindowSize.HeightPx;
  m4 ScreenToCubeScale =  M4( 2/AspectRatio, 0, 0, 0,
                                           0, 2, 0, 0,
                                           0, 0, 0, 0,
                                           0, 0, 0, 1);
  m4 ScreenToCubeTrans =  M4( 1, 0, 0, -1,
                              0, 1, 0, -1,
                              0, 0, 1,  0,
                              0, 0, 0,  1);

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

  r32 MouseX = Interface->MousePos.X;
  r32 MouseY = Interface->MousePos.Y;

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

      PushOverlayQuad(Rect, Color);

      if(Intersects(Rect,Interface->MousePos))
      {
        c8 StringBuffer[1048] = {};
        Platform.DEBUGFormatString( StringBuffer, sizeof(StringBuffer), sizeof(StringBuffer)-1,
        "%s : %2.2f MCy", Region->Record->BlockName, (Region->MaxT-Region->MinT)/1000000.f);
        PushTextAt(MouseX, MouseY+0.02f, StringBuffer, 24, V4(1,1,1,1));
        if(Interface->MouseLeftButton.Edge && Interface->MouseLeftButton.Active)
        {
          HotRecord = Region->Record;
        }
      }
    }
  }
  if(Interface->MouseLeftButton.Edge && Interface->MouseLeftButton.Active)
  {
    if((MouseX >= 0) && (MouseX <= AspectRatio) &&
       (MouseY >= 0) && (MouseY <= 1))
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
}

void DEBUGAddTextSTB(const c8* String, r32 LineNumber, u32 FontSize)
{
  TIMED_FUNCTION();
  game_window_size WindowSize = GameGetWindowSize();
  stb_font_map* FontMap = GetFontMap(GlobalGameState->AssetManager, FontSize);
  r32 CanPosX = 1/100.f;
  r32 CanPosY = 1 - ((LineNumber+1) * FontMap->Ascent - LineNumber*FontMap->Descent)/WindowSize.HeightPx;
  PushTextAt(CanPosX, CanPosY, String, FontSize, V4(1,1,1,1));
}

void PushDebugOverlay(game_input* GameInput)
{
  TIMED_FUNCTION();

  debug_state* DebugState = DEBUGGetState();

  
  r32 LineNumber = 0;
  if(DebugState->Compiling)
  {
    debug_process_state ProcessState = Platform.DEBUGGetProcessState(DebugState->Compiler);
    DebugState->Compiling = ProcessState.IsRunning;
    if(DebugState->Compiling)
    {
      DEBUGAddTextSTB("Compiling", LineNumber++, 24);
    }
  }

  if(DebugState->Frames)
  {
    c8 StringBuffer[256] = {};
     debug_frame* Frame = GetActiveDebugFrame(DebugState);
    Platform.DEBUGFormatString(StringBuffer, sizeof(StringBuffer), sizeof(StringBuffer)-1,
  "%3.1f Hz, %4.2f ms", 1.f/Frame->WallSecondsElapsed, Frame->WallSecondsElapsed*1000);
    DEBUGAddTextSTB(StringBuffer, LineNumber++, 24);
  }
}


#if 0
void DrawFunctionCount(){
  //Assert(DebugState->SnapShotIndex < SNAPSHOT_COUNT);

  v4 Yellow = V4(1,1,0,1);
  v4 Green  = V4(0,1,0,1);
  debug_state* DebugState = DEBUGGetState();
  u32 TotalCounterStateCount = ArrayCount(DebugState->CounterStates);
  for(u32 i = 0; i<TotalCounterStateCount; ++i)
  {
    debug_counter_state* CounterState = &DebugState->CounterStates[i];
    if(!CounterState->FileName) continue;

    stb_font_map* FontMap = &GlobalDebugRenderGroup->Assets->STBFontMap;

    r32 ChartLeft = 1/4.f;
    r32 ChartRight = 4/8.f;
    r32 BarSegmentWidth = (ChartRight - ChartLeft)/SNAPSHOT_COUNT;

    r32 BaselinePixels = GlobalDebugRenderGroup->ScreenHeight - (Line+1) * FontMap->FontHeightPx;
    r32 ChartBot = Ky*BaselinePixels;
    r32 ChartTop = ChartBot + Ky*FontMap->Ascent;

    debug_frame_snapshot* SnapShotStat = &CounterState->Snapshots[DebugState->SnapShotIndex];
    debug_statistics* HitCount   = &SnapShotStat->HitCountStat;
    debug_statistics* CycleCount = &SnapShotStat->CycleCountStat;
    BeginDebugStatistics(HitCount);
    BeginDebugStatistics(CycleCount);
    for(u32 j = 0; j<SNAPSHOT_COUNT; ++j)
    {
      debug_frame_snapshot* SnapShot = &CounterState->Snapshots[j];
      AccumulateStatistic(HitCount, (r32) SnapShot->HitCount);
      AccumulateStatistic(CycleCount, (r32) SnapShot->CycleCount);
    }
    EndDebugStatistics(HitCount);
    EndDebugStatistics(CycleCount);

    r32 xMin = ChartLeft;
    for(u32 j = 0; j<SNAPSHOT_COUNT; ++j)
    {
      debug_frame_snapshot* SnapShot = &CounterState->Snapshots[j];
      r32 xMax = xMin + BarSegmentWidth;

      if(HitCount->Avg)
      {
        r32 BarScale = (ChartTop - ChartBot)/(2.f*SnapShot->CycleCountStat.Avg);
        r32 yMax = ChartBot + BarScale*SnapShot->CycleCount;
        aabb2f Rect = AABB2f( V2(xMin,ChartBot), V2(xMax,yMax));
        v4 Color = Green + ((SnapShot->CycleCountStat.Avg) / (SnapShot->CycleCountStat.Max) ) * (Yellow - Green);
        PushOverlayQuad(GlobalDebugRenderGroup, Rect,Color);
      }

      xMin = ChartLeft + j * BarSegmentWidth;
    }

    s32 CyPerHit = (HitCount->Avg == 0) ? 0 : (s32) (CycleCount->Avg / HitCount->Avg);
    c8 StringBuffer[256] = {};
    _snprintf_s( StringBuffer, sizeof(StringBuffer), sizeof(StringBuffer)-1,
  "(%5d)%-25s:%10dCy:%5dh:%10dcy/h",
      CounterState->LineNumber, CounterState->FunctionName, (u32) CycleCount->Avg, (u32) HitCount->Avg,
      (u32) CyPerHit);

    DEBUGAddTextSTB(GlobalDebugRenderGroup, StringBuffer, CornerPaddingPx, Line);
    Line++;
  }
}
#endif


#include "menu_functions.h"
