#include "debug.h"

internal void
RestartCollation();
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
    DebugState->CollateTemp = BeginTemporaryMemory(&DebugState->Arena);
    DebugState->Initialized = true;
    DebugState->Paused = false;
    DebugState->Resumed = false;
    DebugState->ScopeToRecord = 0;
    RestartCollation();
  }
  return DebugState;
}

internal void
RestartCollation()
{
    debug_state* DebugState = DEBUGGetState();
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

    DebugState->ChartLeft = -0.5f;
    DebugState->ChartBot = -0.5f;
    DebugState->ChartWidth = 1.f;
    DebugState->ChartHeight = 1.f;
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

internal void
Pause(debug_state* DebugState)
{
  if(DebugState->Paused) return;

  DebugState->Paused = true;
  DebugState->Resumed = false;
}

internal void
UnPause(debug_state* DebugState)
{
  if(!DebugState->Paused) return;
  if(DebugState->Resumed)
  {
    DebugState->Resumed = false;
  }

  if(DebugState->Paused)
  {
    DebugState->Resumed = true;
  }

  DebugState->Paused = false;
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
       DebugState->CollationArrayIndex =0;
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
    if(DebugState->Resumed)
    {
      DebugState->Resumed = false;
      RefreshCollation();
    }

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

void DebugMainWindow(game_input* GameInput)
{
  char* MenuItems[] =
  {
    "Functions",
    "PauseCollation"
    "Frames",
    "Memory",
    "Physics"
  };

  r32 AspectRatio = GlobalDebugRenderGroup->ScreenWidth/GlobalDebugRenderGroup->ScreenHeight;

  r32 ScreenWidth  = AspectRatio;
  r32 ScreenHeight = 1;

  debug_state* DebugState = DEBUGGetState();
  r32 Radius = 1/4.f;

  if(GameInput->MouseButton[PlatformMouseButton_Right].Pushed)
  {
    DebugState->RadialMenuX = GameInput->MouseX / AspectRatio;
    DebugState->RadialMenuY = GameInput->MouseY;
    if(DebugState->RadialMenuX > 1-Radius)
    {
      DebugState->RadialMenuX = 1-Radius;
    }else if(DebugState->RadialMenuX < Radius-1){
      DebugState->RadialMenuX = Radius-1;
    }

    if(DebugState->RadialMenuY > 1-Radius-0.05f)
    {
      DebugState->RadialMenuY = 1-Radius-0.05f;
    }else if(DebugState->RadialMenuY < Radius - 1){
      DebugState->RadialMenuY = Radius - 1;
    }
  }

  if(GameInput->MouseButton[PlatformMouseButton_Right].EndedDown)
  {
    DebugState->MainMenu = true;
  }else{
    DebugState->MainMenu = false;
  }


  r32 ClosestMenuItem = R32Max;
  u32 HotMenuItem = ArrayCount(MenuItems);
  for(u32 MenuItemIdx = 0; MenuItemIdx < ArrayCount(MenuItems); ++MenuItemIdx)
  {
    r32 Angle = MenuItemIdx * 2*Pi32/(r32)ArrayCount(MenuItems) + Pi32/2.f;

    v2 MouseButtonPos = V2(GameInput->MouseX/AspectRatio, GameInput->MouseY);
    v2 MenuItemPos = V2(DebugState->RadialMenuX + Radius*Cos(Angle) / AspectRatio,
                        DebugState->RadialMenuY + Radius*Sin(Angle));
    v2 MenuOrigin = V2(DebugState->RadialMenuX/AspectRatio, DebugState->RadialMenuY);

    r32 LenSqMenuItem   = NormSq(MouseButtonPos - MenuItemPos);
    // Todo: I think we want aspect ratio baked into the "canonical coordinate system somehow"
    // Checking distances without it is checking against an ellipse, not a circle
    r32 LenSqMenuOrigin = NormSq(MouseButtonPos - MenuOrigin);
    if(LenSqMenuItem < ClosestMenuItem)// &&
       //LenSqMenuOrigin > Radius * Radius / 12.f )
    {
      ClosestMenuItem = LenSqMenuItem;
      HotMenuItem = MenuItemIdx;
    }
  }


  if(DebugState->MainMenu)
  {
    for(u32 MenuItemIdx = 0; MenuItemIdx < ArrayCount(MenuItems); ++MenuItemIdx)
    {
      rect2f TextSize = DEBUGTextSize(GlobalDebugRenderGroup, MenuItems[MenuItemIdx]);

      r32 Angle = MenuItemIdx * 2*Pi32/(r32)ArrayCount(MenuItems) + Pi32/2.f;
      r32 xPos = DebugState->RadialMenuX + Radius*Cos(Angle) / AspectRatio;
      r32 yPos = DebugState->RadialMenuY + Radius*Sin(Angle);
      aabb2f TextBox = AABB2f(V2(xPos-TextSize.W/2.f, yPos + TextSize.Y),
                              V2(xPos+TextSize.W/2.f, yPos + TextSize.H + TextSize.Y));

      v4 Color = V4(0,0,0.5f,1);
      if(MenuItemIdx == HotMenuItem)
      {
        Color = V4(0,0.5f,0,1);
      }
      DEBUGPushQuad(GlobalDebugRenderGroup, TextBox, Color);
      DEBUGTextOutAt(TextBox.P0.X, yPos, GlobalDebugRenderGroup, MenuItems[MenuItemIdx]);
    }
  }

  if(GameInput->MouseButton[PlatformMouseButton_Right].Released)
  {
    DebugState->HotMenuItem = HotMenuItem;
  }

  if(DebugState->HotMenuItem == 1)
  {
    DebugState->IsVisible = true;
  }else{
    DebugState->IsVisible = false;
  }
}

void PushDebugOverlay(game_input* GameInput)
{
  TIMED_FUNCTION();
  debug_state* DebugState = DEBUGGetState();
  ResetRenderGroup(GlobalDebugRenderGroup);
  DebugMainWindow(GameInput);

  r32 Line = 0;
  r32 CornerPaddingPx = 30;
  if(DebugState->Frames)
  {
    // First frame is the current one.
    c8 StringBuffer[256] = {};
    debug_frame* Frame = DebugState->Frames + DebugState->FrameCount-2;
    _snprintf_s(StringBuffer, sizeof(StringBuffer), sizeof(StringBuffer)-1,
  "%3.1f Hz, %4.2f ms", 1.f/Frame->WallSecondsElapsed, Frame->WallSecondsElapsed*1000);
    DEBUGAddTextSTB(GlobalDebugRenderGroup, StringBuffer, CornerPaddingPx, Line++);
  }

  if(GameInput->MouseButton[PlatformMouseButton_Right].Pushed)
  {
    #if 1

    #else
    if(!DebugState->Paused)
    {
      Pause(DebugState);
    }else{
      UnPause(DebugState);
    }
    #endif
  }

  if(!DebugState->IsVisible) return;

  u32 MaxFramesToDisplay = DebugState->FrameCount < 10 ? DebugState->FrameCount : 10;
  r32 BarWidth = DebugState->ChartHeight/MaxFramesToDisplay;
  r32 LaneWidth = BarWidth/(r32)DebugState->FrameBarLaneCount;

  r32 LaneScale = DebugState->ChartWidth/(r32)DebugState->FrameBarRange;


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
    r32 StackX = DebugState->ChartLeft;
    r32 StackY = DebugState->ChartBot+DebugState->ChartHeight - (r32)(FrameIndex+1)*BarWidth;
    for(u32 RegionIndex = 0; RegionIndex < Frame->RegionCount; ++RegionIndex)
    {
      debug_frame_region* Region = Frame->Regions + RegionIndex;
      v4 Color = ColorTable[(u32)(RegionIndex%ArrayCount(ColorTable))];
      r32 MinX = StackX + LaneScale*Region->MinT;
      r32 MaxX = StackX + LaneScale*Region->MaxT;
      r32 MinY = StackY + LaneWidth*Region->LaneIndex;
      r32 MaxY = MinY + LaneWidth;
      aabb2f Rect = AABB2f( V2(MinX, MinY),
                            V2(MaxX, MaxY));
      DEBUGPushQuad(GlobalDebugRenderGroup, Rect, Color);


      if((GameInput->MouseX >= Rect.P0.X) && (GameInput->MouseX <= Rect.P1.X) &&
         (GameInput->MouseY >= Rect.P0.Y) && (GameInput->MouseY <= Rect.P1.Y))
      {
        c8 StringBuffer[256] = {};
        _snprintf_s( StringBuffer, sizeof(StringBuffer), sizeof(StringBuffer)-1,
        "%s : %2.2f MCy, (%f,%f) -> (%f,%f)", Region->Record->BlockName, (Region->MaxT-Region->MinT)/1000000.f,
        GameInput->MouseX,GameInput->MouseY, Rect.P0.X, Rect.P0.Y);
        DEBUGTextOutAt(GameInput->MouseX, GameInput->MouseY, GlobalDebugRenderGroup, StringBuffer);
        if(GameInput->MouseButton[PlatformMouseButton_Left].Pushed)
        {
          HotRecord = Region->Record;
        }
      }
    }
  }
  if(GameInput->MouseButton[PlatformMouseButton_Left].Pushed)
  {
    if((GameInput->MouseX >= -1) && (GameInput->MouseX <= 1) &&
       (GameInput->MouseY >= -1) && (GameInput->MouseY <= 1))
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


  //Assert(DebugState->SnapShotIndex < SNAPSHOT_COUNT);
#if 0
  v4 Yellow = V4(1,1,0,1);
  v4 Green  = V4(0,1,0,1);

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
