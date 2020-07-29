#include "debug.h"

u8* DEBUGPushSize_(debug_state* DebugState, u32 Size)
{
  u8* Result = DebugState->Memory;
  DebugState->Memory += Size;
  Assert( (DebugState->Memory ) < (DebugState->MemoryBase + DebugState->MemorySize)  );
  return Result;
}

#define DEBUGPushStruct(DebugState, type) (type*) DEBUGPushSize_(DebugState, sizeof(type))
#define DEBUGPushArray(DebugState, count, type) (type*) DEBUGPushSize_(DebugState, count*sizeof(type))
#define DEBUGPushCopy(DebugState, Size, Src) utils::Copy(Size, (void*) (Src), (void*) DEBUGPushSize_(DebugState, Size))



internal void RefreshCollation();
internal void RestartCollation();
internal inline void DebugRewriteConfigFile();

internal void
TogglePause(debug_state* DebugState, menu_item* Item)
{
  if(!DebugState->Paused)
  {
    DebugState->Paused = true;
    DebugState->Resumed = false;
  }else{
    if(DebugState->Resumed)
    {
      RefreshCollation();
      DebugState->Resumed = false;
    }

    if(DebugState->Paused)
    {
      DebugState->Resumed = true;
    }

    DebugState->Paused = false;
  }

  Item->Active = DebugState->Paused;
}

void CreateMainMenuFunctions(debug_state* DebugState)
{
  radial_menu* MainMenu = &DebugState->RadialMenues[0];
  Assert(MainMenu->MenuRegionCount == 3);

  // "Show Collation"
  MainMenu->MenuItems[0].Activate = [](debug_state* DebugState, menu_item* Item)
  {
    DebugState->ChartVisible = !DebugState->ChartVisible;
    Item->Active = (b32) DebugState->ChartVisible;
  };

  // "Pause Collation"
  MainMenu->MenuItems[1].Activate = TogglePause;

  // "Options"
  MainMenu->MenuItems[2].Activate = [](debug_state* DebugState, menu_item* Item)
  {
    active_menu ActiveMenu = {};
    ActiveMenu.Type = menu_type::Box;
    ActiveMenu.BoxMenu = &DebugState->BoxMenues[0];
    DebugState->ActiveMenu = ActiveMenu;

  };
}

void CreateOptionsMenuFunctions(debug_state* DebugState)
{
  box_menu* OptionsMenu = &DebugState->BoxMenues[0];
  Assert(OptionsMenu->MenuItemCount == 4);

  // "Multi Threaded"
  OptionsMenu->MenuItems[0].Activate =
  [](debug_state* DebugState, menu_item* Item)
  {
    DebugState->ConfigMultiThreaded = !DebugState->ConfigMultiThreaded;
    Item->Active = (b32) DebugState->ConfigMultiThreaded;
    DebugRewriteConfigFile();
  };
  // "Collision Points"
  OptionsMenu->MenuItems[1].Activate =
  [](debug_state* DebugState, menu_item* Item)
  {
    DebugState->ConfigCollisionPoints = !DebugState->ConfigCollisionPoints;
    Item->Active = DebugState->ConfigCollisionPoints;
    DebugRewriteConfigFile();
  };
  // "Colliders"
  OptionsMenu->MenuItems[2].Activate =
  [](debug_state* DebugState, menu_item* Item)
  {
    DebugState->ConfigCollider = !DebugState->ConfigCollider;
    Item->Active = DebugState->ConfigCollider;
    DebugRewriteConfigFile();
  };
  // "AABBTree"
  OptionsMenu->MenuItems[3].Activate =
  [](debug_state* DebugState, menu_item* Item)
  {
    DebugState->ConfigAABBTree = !DebugState->ConfigAABBTree;
    Item->Active = DebugState->ConfigAABBTree;
    DebugRewriteConfigFile();
  };
}

void InitializeMenuFunctionPointers(debug_state* DebugState)
{
  CreateMainMenuFunctions(DebugState);
  CreateOptionsMenuFunctions(DebugState);
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
    DebugState->Resumed = false;
    DebugState->ScopeToRecord = 0;

    // Config state
    DebugState->ConfigMultiThreaded = MULTI_THREADED;
    DebugState->ConfigCollisionPoints = SHOW_COLLISION_POINTS;
    DebugState->ConfigCollider = SHOW_COLLIDER;
    DebugState->ConfigAABBTree = SHOW_AABB_TREE;

    // Allocate Menu Space
    DebugState->ActiveMenu = {};

    DebugState->RadialMenuEntries =  1;
    DebugState->RadialMenues = DEBUGPushArray(DebugState, DebugState->RadialMenuEntries, radial_menu);

    menu_item MainMenuItems[] =
    {
      MenuItem("Show Collation"),
      MenuItem("Pause Collation"),
      MenuItem("Options")
    };

    radial_menu* MainMenu = &DebugState->RadialMenues[0];
    MainMenu->MenuRegionCount = ArrayCount(MainMenuItems);
    MainMenu->MenuItems = (menu_item*) DEBUGPushCopy(DebugState, sizeof(MainMenuItems), MainMenuItems);
    MainMenu->Regions = DEBUGPushArray(DebugState, MainMenu->MenuRegionCount, radial_menu_region);


    DebugState->BoxMenuEntries =  1;
    DebugState->BoxMenues = DEBUGPushArray(DebugState, DebugState->RadialMenuEntries, box_menu);
    menu_item OptionMenuItems[] =
    {
      MenuItem("Multi Threaded", DebugState->ConfigMultiThreaded),
      MenuItem("Collision Points", DebugState->ConfigCollisionPoints),
      MenuItem("Colliders", DebugState->ConfigCollider),
      MenuItem("AABBTree", DebugState->ConfigAABBTree),
    };

    box_menu* OptionsMenu = &DebugState->BoxMenues[0];
    OptionsMenu->MenuItemCount = ArrayCount(OptionMenuItems);
    OptionsMenu->MenuItems = (menu_item*) DEBUGPushCopy(DebugState, sizeof(OptionMenuItems), OptionMenuItems);

    DebugState->MainWindow = GetMenu(DebugState, "MainWindow", Rect2f(0.2,0.2,0.5,0.5));
    DebugState->MainWindow->VerticalSplit = false;
    main_window* SubWindow = GetMenu(DebugState, "Widget1");
    main_window* SubWindow1 = GetMenu(DebugState, "Widget2");
    DebugState->MainWindow->SubWindows[0] = SubWindow;
    DebugState->MainWindow->SubWindows[1] = SubWindow1;

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

    //DebugState->ChartLeft = 1.f;
    //DebugState->ChartBot = 0.3f;
    //DebugState->ChartWidth = 0.3f;
    //DebugState->ChartHeight = 0.6f;

    InitializeMenuFunctionPointers(DebugState);
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

internal inline void
DebugRewriteConfigFile()
{
  debug_state* DebugState = DEBUGGetState();

  c8 Buffer[4096] = {};
  u32 Size = _snprintf_s(Buffer, sizeof(Buffer),
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

  DebugState->UpdateConfig = false;
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

void DEBUGDrawDottedLine(v2 Start, v2 End, v4 Color = V4(1,1,1,1), r32 NrDots = 100.f)
{
  for (r32 i = 0; i < NrDots; ++i)
  {
    v2 Pos = Start + (End-Start) * (i/NrDots);
    DEBUGPushQuad(Rect2f(Pos.X-0.005f,Pos.Y-0.005f, 0.01, 0.01), Color);
  }
}

void DEBUGDrawDottedCircularSection(v2 Origin, r32 AngleStart, r32 AngleEnd, r32 Radius, v4 Color = V4(1,1,1,1), r32 NrDots = 100.f)
{
  r32 AngleInterval = GetDeltaAngle(AngleStart, AngleEnd);
  for (r32 i = 0; i < NrDots; ++i)
  {
    r32 Angle = AngleStart + i * AngleInterval / NrDots;
    v2 Pos = V2(Origin.X + Radius * Cos(Angle),
                Origin.Y + Radius * Sin(Angle));
    DEBUGPushQuad(Rect2f(Pos.X-0.005f,Pos.Y-0.005f, 0.01, 0.01), Color);
  }
}

void DEBUGDrawRadialRegion( r32 OriginX, r32 OriginY, r32 MouseX, r32 MouseY, r32 AngleStart, r32 AngleEnd, r32 RadiusStart, r32 RadiusEnd )
{
  AssertCanonical(AngleStart);
  AssertCanonical(AngleEnd);

  // Draw Debug Regions
  v4 Color = V4(1,0,0,1);

  if(RadiusEnd < RadiusStart) RadiusEnd+=(Tau32-RadiusStart);

  v2 Origin = V2(OriginX, OriginY);
  v2 Mouse = V2(MouseX, MouseY) - Origin;
  r32 MouseAngle = ATan2(Mouse.Y, Mouse.X);
  MouseAngle = MouseAngle < 0 ? MouseAngle+Tau32 : MouseAngle;
  r32 MouseRadius = Norm(Mouse);

  v2 MouseLine  = V2(Origin.X +  MouseRadius * Cos(MouseAngle),
                     Origin.Y +  MouseRadius * Sin(MouseAngle));
  v2 StartLine  = V2(Origin.X +  RadiusEnd * Cos(AngleStart+0.1f),
                     Origin.Y +  RadiusEnd * Sin(AngleStart+0.1f));
  v2 StopLine   = V2(Origin.X +  RadiusEnd * Cos(AngleEnd-0.1f),
                     Origin.Y +  RadiusEnd * Sin(AngleEnd-0.1f));

  DEBUGDrawDottedLine(Origin, StartLine,  V4(0,1,0,1));
  DEBUGDrawDottedLine(Origin, StopLine,   V4(1,0,0,1));
  DEBUGDrawDottedLine(Origin, MouseLine,  V4(0,0,1,1));

  DEBUGDrawDottedCircularSection(Origin, AngleStart+0.1f, AngleEnd-0.1f, RadiusStart, Color);
  DEBUGDrawDottedCircularSection(Origin, AngleStart+0.1f, AngleEnd-0.1f, RadiusEnd,   Color);
  DEBUGDrawDottedCircularSection(Origin, 0, MouseAngle, MouseRadius, Color);
}


u32 IsMouseInRegion( const radial_menu_region* Region, r32 MouseAngle, r32 MouseRadius )
{
  b32 MouseInRadialRegion = (MouseRadius >= Region->RadiusStart);
  b32 IsInAngularRegion = IsInRegion(Region->AngleStart, Region->AngleEnd, MouseAngle);
  b32 Result = MouseInRadialRegion && IsInAngularRegion;

  return Result;
}



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
void SetMenuInput(game_input* GameInput, debug_state* DebugState, radial_menu* RadialMenu)
{
  b32 RightButtonReleased = GameInput->MouseButton[PlatformMouseButton_Right].Released;
  v2 MouseButtonPos = V2(GameInput->MouseX, GameInput->MouseY);

  v2  MousePosMenuSpace = MouseButtonPos - V2(RadialMenu->MenuX, RadialMenu->MenuY);
  RadialMenu->MouseRadius = Norm(MousePosMenuSpace);
  RadialMenu->MouseAngle  = ATan2(MousePosMenuSpace.Y, MousePosMenuSpace.X);
  if( RadialMenu->MouseAngle < 0 ) RadialMenu->MouseAngle+=Tau32;

  // Update Menu Input State (Take the input)
  for(u32 RegionIndex = 0; RegionIndex < RadialMenu->MenuRegionCount; ++RegionIndex)
  {
    radial_menu_region* Region = &RadialMenu->Regions[RegionIndex];
    menu_item* MenuItem = &RadialMenu->MenuItems[RegionIndex];
#if 0
    DEBUGDrawRadialRegion(
      DebugState->RadialMenu->MenuX, DebugState->RadialMenu->MenuY,
      GameInput->MouseX, GameInput->MouseY,
      Region->AngleStart, Region->AngleEnd, Region->RadiusStart, Region->Radius );
#endif

    b32 MouseInRegion = IsMouseInRegion( Region, RadialMenu->MouseAngle, RadialMenu->MouseRadius );

    Update( &MenuItem->MouseOverState, MouseInRegion );
    Update( &MenuItem->MenuActivationState, MouseInRegion && RightButtonReleased );

#if 0
    if(MenuItem->MouseOverState.Edge)
    {
      if(MenuItem->MouseOverState.Active){
        Platform.DEBUGPrint("Mouse Entered: %d %d\n", RegionIndex, MouseInRegion );
      }else{
        Platform.DEBUGPrint("Mouse Left: %d %d\n", RegionIndex, MouseInRegion );
      }
    }

    if(MenuItem->MenuActivationState.Edge)
    {
      if(MenuItem->MenuActivationState.Active){
        Platform.DEBUGPrint("Menu Active: %d\n", RegionIndex );
      }else{
        Platform.DEBUGPrint("Menu Inactive: %d\n", RegionIndex );
      }
    }
#endif
  }
}

void ActOnInput(debug_state* DebugState, radial_menu* RadialMenu )
{
  // Update Menu Internal State (Act on the input)
  b32 MenuActivated = false;
  for (u32 MenuItemIndex = 0; MenuItemIndex < RadialMenu->MenuRegionCount; ++MenuItemIndex)
  {
    menu_item* MenuItem = &RadialMenu->MenuItems[MenuItemIndex];

    if(MenuItem->MouseOverState.Active)
    {
      if((MenuItem->MenuActivationState.Active) &&
         (MenuItem->MenuActivationState.Edge))
      {
        // Todo: Make callbacks use DebugGetState() ?
        MenuItem->Activate(DebugState, MenuItem);
      }
    }
  }
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

    rect2f TextBox = DEBUGTextSize(TextPosX, TextPosY, MenuItem->Header);

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
    DEBUGPushQuad(QuadRect, Color);

    DEBUGTextOutAt(TextPosX+Offset.X, TextPosY+Offset.Y, MenuItem->Header, V4(1,1,1,1));
  }

  v2 MouseLine  = V2(RadialMenu->MenuX + RadialMenu->MouseRadius * Cos(RadialMenu->MouseAngle),
                     RadialMenu->MenuY + RadialMenu->MouseRadius * Sin(RadialMenu->MouseAngle));
  DEBUGDrawDottedLine(V2(RadialMenu->MenuX, RadialMenu->MenuY) , MouseLine,  V4(0.7,0,0,1));
}


void DebugMainWindow(game_input* GameInput)
{
  game_window_size WindowSize = GameGetWindowSize();
  r32 Width  = WindowSize.WidthPx;
  r32 Height = WindowSize.HeightPx;

  r32 AspectRatio = Width/Height;
  r32 ScreenWidth = AspectRatio;
  r32 ScreenHeight = 1;

  b32 RightButtonPushed = GameInput->MouseButton[PlatformMouseButton_Right].Pushed;
  b32 RightButtonReleased = GameInput->MouseButton[PlatformMouseButton_Right].Released;

  debug_state* DebugState = DEBUGGetState();

  v2 MouseButtonPos = V2(GameInput->MouseX, GameInput->MouseY);

  if(RightButtonPushed )
  {
    DebugState->ActiveMenu.Type = menu_type::Radial;
    DebugState->ActiveMenu.RadialMenu = &DebugState->RadialMenues[0];
    BeginRadialMenu(DebugState->ActiveMenu.RadialMenu, MouseButtonPos);
  }

  switch(DebugState->ActiveMenu.Type)
  {
    case menu_type::Radial:
    {
      Assert(DebugState->ActiveMenu.RadialMenu);
      radial_menu* RadialMenu = DebugState->ActiveMenu.RadialMenu;

      SetMenuInput(GameInput, DebugState, RadialMenu);

      ActOnInput(DebugState, RadialMenu );

    }break;
    case menu_type::Box:
    {

    }break;

  }

  switch(DebugState->ActiveMenu.Type)
  {
    case menu_type::Radial:
    {
      Assert(DebugState->ActiveMenu.RadialMenu);
      radial_menu* RadialMenu = DebugState->ActiveMenu.RadialMenu;

      DrawMenu(RadialMenu);

    }break;
    case menu_type::Box:
    {
      DEBUGPushQuad(Rect2f(0.1,0.1,0.3,0.3), V4(0.3,0.3,0.3,0.5));
    }break;
  }


  if(RightButtonReleased &&
    (DebugState->ActiveMenu.Type == menu_type::Radial))
  {
    Assert(DebugState->ActiveMenu.RadialMenu);
    EndRadialMenu(DebugState->ActiveMenu.RadialMenu);
    DebugState->ActiveMenu = {};
  }

  SetMenuInput(GameInput, DebugState, DebugState->MainWindow);

  ActOnInput(DebugState, DebugState->MainWindow);

  Draw(DebugState->MainWindow);

}

inline internal debug_frame*
GetActiveDebugFrame(debug_state* DebugState)
{
  debug_frame* Result = DebugState->Frames + DebugState->FrameCount-2;
  return Result;
}

void PushDebugOverlay(game_input* GameInput)
{
  TIMED_FUNCTION();

  debug_state* DebugState = DEBUGGetState();

  ResetRenderGroup(GlobalDebugRenderGroup);

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

  GlobalDebugRenderGroup->ProjectionMatrix = ScreenToCubeTrans*ScreenToCubeScale;

  DebugMainWindow(GameInput);

  r32 LineNumber = 0;
  if(DebugState->Compiling)
  {
    debug_process_state ProcessState = Platform.DEBUGGetProcessState(DebugState->Compiler);
    DebugState->Compiling = ProcessState.IsRunning;
    if(DebugState->Compiling)
    {
      DEBUGAddTextSTB("Compiling", LineNumber++);
    }
  }

  if(DebugState->Frames)
  {
    c8 StringBuffer[256] = {};
     debug_frame* Frame = GetActiveDebugFrame(DebugState);
    Platform.DEBUGFormatString(StringBuffer, sizeof(StringBuffer), sizeof(StringBuffer)-1,
  "%3.1f Hz, %4.2f ms", 1.f/Frame->WallSecondsElapsed, Frame->WallSecondsElapsed*1000);
    DEBUGAddTextSTB(StringBuffer, LineNumber++);
  }

  if(!DebugState->ChartVisible) return;

  rect2f Chart = GetRegion(window_regions::RightBody, DebugState->MainWindow);

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

      DEBUGPushQuad(Rect, Color);

      if((GameInput->MouseX >= Rect.X) && (GameInput->MouseX <= Rect.X+Rect.W) &&
         (GameInput->MouseY >= Rect.Y) && (GameInput->MouseY <= Rect.Y+Rect.H))
      {
        c8 StringBuffer[256] = {};
        Platform.DEBUGFormatString( StringBuffer, sizeof(StringBuffer), sizeof(StringBuffer)-1,
        "%s : %2.2f MCy", Region->Record->BlockName, (Region->MaxT-Region->MinT)/1000000.f);
        DEBUGTextOutAt(GameInput->MouseX, GameInput->MouseY+0.02f, StringBuffer);
        if(GameInput->MouseButton[PlatformMouseButton_Left].Pushed)
        {
          HotRecord = Region->Record;
        }
      }
    }
  }

  if(GameInput->MouseButton[PlatformMouseButton_Left].Pushed)
  {
    if((GameInput->MouseX >= 0) && (GameInput->MouseX <= AspectRatio) &&
       (GameInput->MouseY >= 0) && (GameInput->MouseY <= 1))
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


void DrawFunctionCount(){
  //Assert(DebugState->SnapShotIndex < SNAPSHOT_COUNT);

#if 0
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
        DEBUGPushQuad(GlobalDebugRenderGroup, Rect,Color);
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
#endif
}



rect2f ToGlobalSpace(rect2f Subregion, rect2f GlobalRegion)
{
  rect2f Result = {};
  Result.X = GlobalRegion.X + Subregion.X*GlobalRegion.H;
  Result.Y = GlobalRegion.Y + Subregion.Y*GlobalRegion.H;
  Result.W = Subregion.W*GlobalRegion.H;
  Result.H = Subregion.H*GlobalRegion.H;
  return Result;
}

void Draw(region_node* Root)
{
  game_window_size WindowSize = GameGetWindowSize();
  r32 AspectRatio = WindowSize.WidthPx / WindowSize.HeightPx;
  rect2f GlobalSpaceRect = ToGlobalSpace(Root->Region, Rect2f(0,0,AspectRatio,1));
  DEBUGPushQuad(GlobalSpaceRect, Root->Color);
  for (u32 BranchIndex = 0; BranchIndex < Root->BranchCount; ++BranchIndex)
  {
    region_node* Branch = &Root->Branches[BranchIndex];
    rect2f Region = ToGlobalSpace(Branch->Region, GlobalSpaceRect);
    DEBUGPushQuad(Region, Branch->Color);
    for (u32 BranchIndex2 = 0; BranchIndex2 < Branch->BranchCount; ++BranchIndex2)
    {
      region_node* Branch2 = &Branch->Branches[BranchIndex2];
      rect2f Region2 = ToGlobalSpace(Branch2->Region, Region);
      DEBUGPushQuad(Region2, Branch2->Color);
    }
  }
}

inline internal b32
Intersects(const rect2f & Rect, v2 P)
{
  b32 Result = (P.X>=Rect.X && (P.X<=Rect.X+Rect.W)) &&
               (P.Y>=Rect.Y && (P.Y<=Rect.Y+Rect.H));
  return Result;
}


region_node* GetMouseOverRegion(region_node* Root, v2 MousePosition)
{
  region_node* Result = 0;
  game_window_size WindowSize = GameGetWindowSize();
  r32 AspectRatio = WindowSize.WidthPx / WindowSize.HeightPx;
  rect2f GlobalSpaceRect = ToGlobalSpace(Root->Region, Rect2f(0,0,AspectRatio,1));

  v4 Red = V4(1,0,0,1);
  v4 Green = V4(0,1,0,1);

  if(Intersects(GlobalSpaceRect, MousePosition))
  {
    Result = Root;
  }
  for (u32 BranchIndex = 0; BranchIndex < Root->BranchCount; ++BranchIndex)
  {
    region_node* Branch = &Root->Branches[BranchIndex];
    rect2f Region = ToGlobalSpace(Branch->Region, GlobalSpaceRect);
    if(Intersects(Region, MousePosition))
    {
      Result = Branch;
    }
    for (u32 BranchIndex2 = 0; BranchIndex2 < Branch->BranchCount; ++BranchIndex2)
    {
      region_node* Branch2 = &Branch->Branches[BranchIndex2];
      rect2f Region2 = ToGlobalSpace(Branch2->Region, Region);
      DEBUGPushQuad(Region2, Branch2->Color);
      if(Intersects(Region2, MousePosition))
      {
        Result = Branch2;
      }
    }
  }
  return Result;
}


void SetMenuInput(game_input* GameInput, debug_state* DebugState, main_window* MainWindow)
{
  v2 MousePos = V2(GameInput->MouseX, GameInput->MouseY);

  Update(&MainWindow->MouseLeftButton, GameInput->MouseButton[PlatformMouseButton_Left].EndedDown);
  if( MainWindow->MouseLeftButton.Edge )
  {
    if(MainWindow->MouseLeftButton.Active )
    {
      Platform.DEBUGPrint("Mous Down\n");
      MainWindow->MouseLeftButtonPush = MousePos;
    }else{
      Platform.DEBUGPrint("Mous Up\n");
      MainWindow->MouseLeftButtonRelese = MousePos;
    }
  }
  MainWindow->MousePos = MousePos;

}

void MoveRegion(rect2f* SubRegion, rect2f* SubRegionStart, rect2f* Surroundings, v2 Delta)
{
  Assert((Surroundings->Y + Surroundings->H)<=1);

  r32 NewPosX = SubRegionStart->X + Delta.X;
  r32 NewPosY = SubRegionStart->Y + Delta.Y;
  if(NewPosX < Surroundings->X)
  {
    NewPosX = Surroundings->X;
  }
  if( (NewPosX + SubRegion->W) > (Surroundings->X + Surroundings->W))
  {
    NewPosX = (Surroundings->X + Surroundings->W) - SubRegion->W;
  }
  if(NewPosY < Surroundings->Y)
  {
    NewPosY = Surroundings->Y;
  }
  if(NewPosY + SubRegion->H > (Surroundings->Y + Surroundings->H))
  {
    NewPosY = (Surroundings->Y + Surroundings->H) - SubRegion->H;
  }

  SubRegion->X = NewPosX;
  SubRegion->Y = NewPosY;
}

void DragBorderRight(rect2f* SubRegion, rect2f* SubRegionStart, r32 DeltaX, r32 XMax, r32 MinWidth)
{
  r32 NewWidth = SubRegionStart->W + DeltaX;
  if(SubRegionStart->X + NewWidth > XMax )
  {
    NewWidth = XMax - SubRegionStart->X;
  }else if(NewWidth < MinWidth)
  {
    NewWidth = MinWidth;
  }
  SubRegion->W = NewWidth;
}

void DragBorderTop(rect2f* SubRegion, rect2f* SubRegionStart, r32 DeltaY, r32 YMax, r32 MinHeight)
{
  r32 NewHeight = SubRegionStart->H + DeltaY;
  if((SubRegionStart->Y + NewHeight) > YMax)
  {
    NewHeight = YMax - SubRegionStart->Y;
  }else if(NewHeight < MinHeight)
  {
    NewHeight = MinHeight;
  }
  SubRegion->H = NewHeight;
}

void DragBorderLeft(rect2f* SubRegion, rect2f* SubRegionStart, r32 DeltaX, r32 XMin, r32 MinWidth)
{
  r32 NewXPos   = SubRegionStart->X + DeltaX;
  r32 NewWidth  = SubRegionStart->W - DeltaX;
  if(NewXPos < XMin)
  {
    NewXPos = XMin;
    NewWidth = (SubRegionStart->X - XMin) + SubRegionStart->W;
  }else if(NewWidth < MinWidth)
  {
    NewXPos  = SubRegionStart->X + SubRegionStart->W - MinWidth;
    NewWidth = MinWidth;
  }
  SubRegion->X = NewXPos;
  SubRegion->W = NewWidth;
}

void DragBorderBot(rect2f* SubRegion, rect2f* SubRegionStart, r32 DeltaY, r32 YMin, r32 MinHeight)
{
  r32 NewYPos   = SubRegionStart->Y + DeltaY;
  r32 NewHeight = SubRegionStart->H - DeltaY;
  if(NewYPos < YMin)
  {
    NewYPos = YMin;
    NewHeight = (SubRegionStart->Y - YMin) + SubRegionStart->H;
  }else if(NewHeight < MinHeight)
  {
    NewYPos = SubRegionStart->Y + SubRegionStart->H - MinHeight;
    NewHeight = MinHeight;
  }
  SubRegion->Y = NewYPos;
  SubRegion->H = NewHeight;
}


rect2f GetRegion(window_regions Type, rect2f Region, r32 BorderSize, r32 HeaderSize, r32 SplitBorderPos)
{
  rect2f Result = {};
  r32 XStart = 0;
  r32 YStart = 0;
  r32 Width  = 0;
  r32 Height = 0;
  switch(Type)
  {
    case window_regions::WholeBody:
    {
      XStart = Region.X;                                // Left
      YStart = Region.Y;                                // Bot
      Width  = Region.W;                                // Whole
      Height = Region.H;                                // Bot
    }break;
    case window_regions::BotBody:
    {
      XStart = Region.X;                                // Left
      YStart = Region.Y;                                // Bot
      Width  = Region.W;                                // Whole
      Height = SplitBorderPos - 0.5f * BorderSize; // Bot
    }break;
    case window_regions::TopBody:
    {
      XStart = Region.X;                                              // Left
      YStart = Region.Y + SplitBorderPos + 0.5f * BorderSize;    // Top
      Width  = Region.W;                                              // Whole
      Height = Region.H - (SplitBorderPos + 0.5f * BorderSize);  // Top
    }break;
    case window_regions::LeftBody:
    {
      XStart = Region.X;                                              // Left
      YStart = Region.Y;                                              // Bot
      Width  = SplitBorderPos - 0.5f * BorderSize;                 // Left
      Height = Region.H;                                              // Whole
    }break;
    case window_regions::RightBody:
    {
      XStart = Region.X + SplitBorderPos + 0.5f * BorderSize;      // Right
      YStart = Region.Y;                                              // Bot
      Width  = Region.W - (SplitBorderPos + 0.5f * BorderSize);    // Right
      Height = Region.H;                                              // Whole
    }break;

    case window_regions::VerticalBorder:
    {
      XStart = Region.X + SplitBorderPos    - 0.5f * BorderSize;
      YStart = Region.Y;
      Width  = BorderSize;
      Height = Region.H;
    }break;
    case window_regions::HorizontalBorder:
    {
      XStart = Region.X;
      YStart = Region.Y + SplitBorderPos - 0.5f * BorderSize;
      Width  = Region.W;
      Height = BorderSize;
    }break;

    case window_regions::LeftBorder:
    {
      XStart = Region.X - BorderSize;
      YStart = Region.Y - BorderSize;
      Width  = BorderSize;
      Height = Region.H + 2*BorderSize + HeaderSize;
    }break;
    case window_regions::RightBorder:
    {
      XStart = Region.X + Region.W;
      YStart = Region.Y - BorderSize;
      Width  = BorderSize;
      Height = Region.H + 2*BorderSize + HeaderSize;
    }break;
    case window_regions::BotBorder:
    {
      XStart = Region.X - BorderSize;
      YStart = Region.Y - BorderSize;
      Width  = Region.W + 2*BorderSize;
      Height = BorderSize;
    }break;
    case window_regions::TopBorder:
    {
      XStart = Region.X - BorderSize;
      YStart = Region.Y + Region.H + HeaderSize;
      Width  = Region.W + 2*BorderSize;
      Height = BorderSize;
    }break;

    case window_regions::Header:
    {
      XStart = Region.X;
      YStart = Region.Y + Region.H;
      Width  = Region.W;
      Height = HeaderSize;
    }break;

    case window_regions::BotLeftCorner:
    {
      XStart = Region.X - BorderSize;
      YStart = Region.Y - BorderSize;
      Width  = BorderSize;
      Height = BorderSize;
    }break;
    case window_regions::BotRightCorner:
    {
      XStart = Region.X + Region.W;
      YStart = Region.Y - BorderSize;
      Width  = BorderSize;
      Height = BorderSize;
    }break;
    case window_regions::TopLeftCorner:
    {
      XStart = Region.X - BorderSize;
      YStart = Region.Y + Region.H + HeaderSize;
      Width  = BorderSize;
      Height = BorderSize;
    }break;
    case window_regions::TopRightCorner:
    {
      XStart = Region.X + Region.W;
      YStart = Region.Y + Region.H + HeaderSize;
      Width  = BorderSize;
      Height = BorderSize;
    }break;
  }

  Result = Rect2f(XStart, YStart, Width, Height);
  return Result;
}
inline rect2f GetRegion(window_regions Type, main_window* MainWindow)
{
  r32 SplitBorderPos =  MainWindow->VerticalSplit ? MainWindow->SplitBorderFraction * MainWindow->Region.W :  MainWindow->SplitBorderFraction * MainWindow->Region.H;
  return GetRegion(Type, MainWindow->Region, MainWindow->BorderSize, MainWindow->HeaderSize, SplitBorderPos);
}

void ActOnInput(debug_state* DebugState, main_window* MainWindow)
{
  rect2f Region = MainWindow->Region;
  r32 BorderSize = MainWindow->BorderSize;
  r32 HeaderSize = MainWindow->HeaderSize;
  rect2f SubRegion = {};

  v2 MousePos = MainWindow->MousePos;

  if(MainWindow->MouseLeftButton.Active)
  {
    // Mouse Clicked Event
    if(MainWindow->MouseLeftButton.Edge)
    {
      SubRegion = GetRegion(window_regions::Header, MainWindow);
      if(Intersects(SubRegion, MousePos))
      {
        MainWindow->WindowDrag = true;
        Platform.DEBUGPrint("Header Selected\n");
        MainWindow->WindowDraggingStart = Region;
      }
      SubRegion = GetRegion(window_regions::LeftBorder, MainWindow);
      if(Intersects(SubRegion, MousePos))
      {
        MainWindow->LeftBorderDrag = true;
        Platform.DEBUGPrint("Left Border Selected\n");
        MainWindow->WindowDraggingStart = Region;
      }

      SubRegion = MainWindow->VerticalSplit ? GetRegion(window_regions::VerticalBorder, MainWindow) :
                                              GetRegion(window_regions::HorizontalBorder, MainWindow);
      if(Intersects(SubRegion, MousePos))
      {
        MainWindow->SplitBorderDrag = true;
        Platform.DEBUGPrint("%s", MainWindow->VerticalSplit ? "Vertical Border Selected\n" : "Horizontal Border Selected\n");
        MainWindow->SplitBorderDraggingStart = MainWindow->SplitBorderFraction;
      }
      SubRegion = GetRegion(window_regions::RightBorder, MainWindow);
      if(Intersects(SubRegion, MousePos))
      {
        MainWindow->RightBorderDrag = true;
        Platform.DEBUGPrint("Right Border Selected\n");
        MainWindow->WindowDraggingStart = Region;
      }
      SubRegion = GetRegion(window_regions::BotBorder, MainWindow);
      if(Intersects(SubRegion, MousePos))
      {
        MainWindow->BotBorderDrag = true;
        Platform.DEBUGPrint("Bot Border Selected\n");
        MainWindow->WindowDraggingStart = Region;
      }
      SubRegion = GetRegion(window_regions::TopBorder, MainWindow);
      if(Intersects(SubRegion, MousePos))
      {
        MainWindow->TopBorderDrag = true;
        Platform.DEBUGPrint("Top Border Selected\n");
        MainWindow->WindowDraggingStart = Region;
      }

    // Mouse Down Movement State
    }else{

    }
  }else{
    // Mouse Released Event
    if(MainWindow->MouseLeftButton.Edge)
    {
      if(MainWindow->WindowDrag)
      {
        Platform.DEBUGPrint("Header Released\n");
      }
      MainWindow->WindowDrag = false;

      if(MainWindow->LeftBorderDrag)
      {
        Platform.DEBUGPrint("Left Border Released\n");
      }
      MainWindow->LeftBorderDrag = false;

      if(MainWindow->SplitBorderDrag)
      {
        Platform.DEBUGPrint("%s", MainWindow->VerticalSplit ? "Vertical Border Released\n" : "Horizontal Border Released\n");
      }
      MainWindow->SplitBorderDrag = false;

      if(MainWindow->RightBorderDrag)
      {
        Platform.DEBUGPrint("Right Border Released\n");
      }
      MainWindow->RightBorderDrag = false;

      if(MainWindow->BotBorderDrag)
      {
        Platform.DEBUGPrint("Bot Border Released\n");
      }
      MainWindow->BotBorderDrag = false;

      if(MainWindow->TopBorderDrag)
      {
        Platform.DEBUGPrint("Top Border Released\n");
      }
      MainWindow->TopBorderDrag = false;

      // Mouse Exploration State
    }else{

    }
  }

  game_window_size WindowSize = GameGetWindowSize();
  r32 Width = WindowSize.WidthPx/WindowSize.HeightPx;
  r32 MinWidth  = MainWindow->VerticalSplit ? 2 * MainWindow->MinWidth : MainWindow->MinWidth;
  r32 MinHeight = MainWindow->VerticalSplit ? MainWindow->MinHeight : 2 * MainWindow->MinHeight;
  v2 Delta = MainWindow->MousePos - MainWindow->MouseLeftButtonPush;
  rect2f Surroundings = Rect2f(0,0,Width,1);

  if(MainWindow->WindowDrag)
  {
    MoveRegion(&MainWindow->Region, &MainWindow->WindowDraggingStart, &Surroundings, Delta);
  }

  if(MainWindow->RightBorderDrag)
  {
    DragBorderRight(&MainWindow->Region, &MainWindow->WindowDraggingStart, Delta.X, Surroundings.X+Surroundings.W, MinWidth);
  }

  if(MainWindow->LeftBorderDrag)
  {
    DragBorderLeft(&MainWindow->Region, &MainWindow->WindowDraggingStart, Delta.X, Surroundings.X, MinWidth);
  }

  if(MainWindow->BotBorderDrag)
  {
    DragBorderBot(&MainWindow->Region, &MainWindow->WindowDraggingStart, Delta.Y, Surroundings.Y, MinHeight);
  }

  if(MainWindow->TopBorderDrag)
  {
    DragBorderTop(&MainWindow->Region, &MainWindow->WindowDraggingStart, Delta.Y, Surroundings.Y+Surroundings.H, MinHeight);
  }

  if(MainWindow->VerticalSplit)
  {
    if(MainWindow->SplitBorderDrag)
    {
      MainWindow->SplitBorderFraction = MainWindow->SplitBorderDraggingStart + Delta.X/Region.W;
    }
    if(MainWindow->SplitBorderFraction <= 0.5f*MinWidth/Region.W)
    {
      MainWindow->SplitBorderFraction = 0.5f*MinWidth/Region.W;
    }
    if(MainWindow->SplitBorderFraction > (Region.W - 0.5f*MinWidth)/Region.W)
    {
      MainWindow->SplitBorderFraction = (Region.W - 0.5f*MinWidth)/Region.W;
    }
  }else{
    if(MainWindow->SplitBorderDrag)
    {
      MainWindow->SplitBorderFraction = MainWindow->SplitBorderDraggingStart + Delta.Y/Region.H;
    }
    if(MainWindow->SplitBorderFraction <= (0.5f*MinHeight)/Region.H)
    {
      MainWindow->SplitBorderFraction = (0.5f*MinHeight)/Region.H;
    }
    if(MainWindow->SplitBorderFraction > (Region.H - 0.5f*MinHeight)/Region.H)
    {
      MainWindow->SplitBorderFraction = (Region.H - 0.5f*MinHeight)/Region.H;
    }
  }

}

void DrawWindow(main_window* MainWindow)
{
  rect2f Region  = MainWindow->Region;
  r32 BorderSize = MainWindow->BorderSize;
  r32 HeaderSize = MainWindow->HeaderSize;

  v4 CornerColor = V4(1,0,1,1);
  v4 BorderColor = V4(0,0,1,1);
  v4 BodyColor   = V4(0.2,0.1,0.1,1);
  v4 HeaderColor = V4(1,0,0,1);

  if(MainWindow->SubWindows[0] && MainWindow->SubWindows[1])
  {
    if(MainWindow->VerticalSplit)
    {
      DEBUGPushQuad(GetRegion(window_regions::VerticalBorder, MainWindow), BorderColor);
    }else{
      DEBUGPushQuad(GetRegion(window_regions::HorizontalBorder, MainWindow), BorderColor);
    }
  }

  DEBUGPushQuad(GetRegion(window_regions::LeftBorder,     MainWindow), BorderColor);
  DEBUGPushQuad(GetRegion(window_regions::RightBorder,    MainWindow), BorderColor);
  DEBUGPushQuad(GetRegion(window_regions::TopBorder,      MainWindow), BorderColor);
  DEBUGPushQuad(GetRegion(window_regions::BotBorder,      MainWindow), BorderColor);
  DEBUGPushQuad(GetRegion(window_regions::BotLeftCorner,  MainWindow), CornerColor);
  DEBUGPushQuad(GetRegion(window_regions::BotRightCorner, MainWindow), CornerColor);
  DEBUGPushQuad(GetRegion(window_regions::TopLeftCorner,  MainWindow), CornerColor);
  DEBUGPushQuad(GetRegion(window_regions::TopRightCorner, MainWindow), CornerColor);
  DEBUGPushQuad(GetRegion(window_regions::Header,         MainWindow), HeaderColor);

}

void PushBodyRegionQuad(rect2f Region, r32 HeaderSize, v4 BodyColor, v4 HeaderColor)
{
  DEBUGPushQuad(Region, BodyColor);
  rect2f Header = Region;
  Header.Y  += Header.H;
  Header.H  = HeaderSize;
  DEBUGPushQuad(Header, HeaderColor);
}

void DrawWidget(main_window* Widget)
{
  v4 CornerColor = V4(1,0,1,1);
  v4 BorderColor = V4(0,0,1,1);
  v4 BodyColor   = V4(0.2,0.2,0.2,1);
  v4 HeaderColor = V4(0.4,0.4,0.4,1);

  if(Widget->SubWindows[0] && Widget->SubWindows[1])
  {
    if(Widget->VerticalSplit)
    {
      PushBodyRegionQuad(GetRegion(window_regions::LeftBody, Widget), Widget->HeaderSize, BodyColor, HeaderColor);
      PushBodyRegionQuad(GetRegion(window_regions::RightBody, Widget), Widget->HeaderSize, BodyColor, HeaderColor);
      DEBUGPushQuad(GetRegion(window_regions::VerticalBorder, Widget), BorderColor);
    }else{
      PushBodyRegionQuad(GetRegion(window_regions::BotBody, Widget), Widget->HeaderSize, BodyColor, HeaderColor);
      PushBodyRegionQuad(GetRegion(window_regions::TopBody, Widget), Widget->HeaderSize, BodyColor, HeaderColor);
      DEBUGPushQuad(GetRegion(window_regions::HorizontalBorder, Widget), BorderColor);
    }
  }else{
    PushBodyRegionQuad(GetRegion(window_regions::WholeBody, Widget), Widget->HeaderSize, BodyColor, HeaderColor);
  }

}

void Draw(main_window* MainWindow)
{
  DrawWindow(MainWindow);

  game_window_size WindowSize = GameGetWindowSize();
  r32 AspectRatio = WindowSize.WidthPx / WindowSize.HeightPx;
  rect2f GlobalSpaceRect = ToGlobalSpace(MainWindow->Region, Rect2f(0,0,AspectRatio,1));

  rect2f SubRegion;
  if(MainWindow->SubWindows[0] && MainWindow->SubWindows[1])
  {
    if(MainWindow->VerticalSplit)
    {
      main_window* LeftChild = MainWindow->SubWindows[0];
      SubRegion = GetRegion(window_regions::LeftBody, MainWindow);
      SubRegion.H-=LeftChild->HeaderSize;
      LeftChild->Region = SubRegion;
      DrawWidget(LeftChild);

      main_window* RightChild = MainWindow->SubWindows[1];
      SubRegion = GetRegion(window_regions::RightBody, MainWindow);
      SubRegion.H-=RightChild->HeaderSize;
      RightChild->Region = SubRegion;
      DrawWidget(RightChild);
    }else{
      main_window* BotChild = MainWindow->SubWindows[0];
      SubRegion = GetRegion(window_regions::BotBody, MainWindow);
      SubRegion.H-=BotChild->HeaderSize;
      BotChild->Region = SubRegion;
      DrawWidget(BotChild);

      main_window* TopChild = MainWindow->SubWindows[1];
      SubRegion = GetRegion(window_regions::TopBody, MainWindow);
      SubRegion.H-=TopChild->HeaderSize;
      TopChild->Region = SubRegion;
      DrawWidget(TopChild);
    }
  }else{
    Assert(MainWindow->SubWindows[0] && !MainWindow->SubWindows[1]);
    main_window* Child = MainWindow->SubWindows[0];
    SubRegion = GetRegion(window_regions::WholeBody, MainWindow);
    SubRegion.H-=Child->HeaderSize;
    Child->Region = SubRegion;
    DrawWidget(Child);
  }

}

main_window* GetMenu(debug_state* DebugState, c8* WindowTitle, rect2f Region)
{
  main_window* MainWindow = DEBUGPushStruct(DebugState, main_window);
  MainWindow->Region = Region;
  MainWindow->BorderSize = 0.007;
  MainWindow->HeaderSize = 0.02;
  MainWindow->MinWidth  = 4*MainWindow->BorderSize;
  MainWindow->MinHeight = 4*MainWindow->BorderSize;
  MainWindow->SplitBorderFraction = 0.5f; // [0,1]
  MainWindow->VerticalSplit = false;
  Platform.DEBUGFormatString(MainWindow->Header, 64,53,"%s", WindowTitle);
  MainWindow->Color = V4(0,0,0,1);

  return MainWindow;
}
