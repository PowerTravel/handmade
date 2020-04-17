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


void PushDebugOverlay(debug_state* DebugState, game_input* GameInput)
{
  TIMED_FUNCTION();
  r32 Line = 0;
  r32 CornerPaddingPx = 30;

  v4 Yellow   = V4(1,1,0,1);
  v4 Green = V4(0,1,0,1);
  r32 Kx = 1.f / GlobalDebugRenderGroup->ScreenWidth;
  r32 Ky = 1.f / GlobalDebugRenderGroup->ScreenHeight;
  r32 AspectRatio = Kx/Ky;

  //Assert(DebugState->SnapShotIndex < SNAPSHOT_COUNT);
#if 0
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
        r32 BarHeightScale = (ChartTop - ChartBot)/(2.f*SnapShot->CycleCountStat.Avg);
        r32 yMax = ChartBot + BarHeightScale*SnapShot->CycleCount;
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

  r32 ChartLeft = -1.f;
  r32 ChartBot = -1.f;
  r32 ChartWidth = 2.f;
  r32 ChartHeight = 1.f;

  if((GameInput->MouseX >= ChartLeft) && (GameInput->MouseX <= ChartLeft+ChartWidth) &&
     (GameInput->MouseY >= ChartBot)  && (GameInput->MouseY <= ChartBot+ChartHeight))
  {
    DebugState->Paused = true;
  }else{
    DebugState->Paused = false;
  }

  r32 BarWidth = ChartWidth/(r32)DebugState->FrameCount;
  r32 LaneWidth = BarWidth/(r32)DebugState->FrameBarLaneCount;

  r32 BarHeightScale = ChartHeight/(r32)DebugState->FrameBarRange;


  v4 ColorTable[] = {V4(1,0,0,1),
                     V4(0,1,0,1),
                     V4(0,0,1,1),
                     V4(1,1,0,1),
                     V4(1,0,1,1),
                     V4(0,1,1,1),
                     V4(1,1,1,1),
                     V4(0,0,0,1)};

  if(DebugState->Frames)
  {
    // First frame is the current one.
    c8 StringBuffer[256] = {};
    _snprintf_s(StringBuffer, sizeof(StringBuffer), sizeof(StringBuffer)-1,
  "%2.0f Hz, %0.3f ms", 1.f/DebugState->Frames->WallSecondsElapsed, DebugState->Frames->WallSecondsElapsed);
    DEBUGAddTextSTB(GlobalDebugRenderGroup, StringBuffer, CornerPaddingPx, Line++);
  }


  for(u32 FrameIndex = 0; FrameIndex < DebugState->FrameCount; ++FrameIndex)
  {
    debug_frame* Frame = DebugState->Frames + FrameIndex;
    r32 StackX = ChartLeft + (r32)FrameIndex*BarWidth;
    r32 StackY = ChartBot;
    for(u32 RegionIndex = 0; RegionIndex < Frame->RegionCount; ++RegionIndex)
    {
      debug_frame_region* Region = Frame->Regions + RegionIndex;
      v4 Color = ColorTable[(u32)(RegionIndex%ArrayCount(ColorTable))];
      r32 MinY = StackY + BarHeightScale*Region->MinT;
      r32 MaxY = StackY + BarHeightScale*Region->MaxT;
      r32 MinX = StackX + LaneWidth*Region->LaneIndex;
      r32 MaxX = MinX + LaneWidth;
      aabb2f Rect = AABB2f( V2(MinX, MinY),
                            V2(MaxX, MaxY));
      DEBUGPushQuad(GlobalDebugRenderGroup, Rect, Color);

      if((GameInput->MouseX >= Rect.P0.X) && (GameInput->MouseX <= Rect.P1.X) &&
         (GameInput->MouseY >= Rect.P0.Y) && (GameInput->MouseY <= Rect.P1.Y))
      {
        c8 StringBuffer[256] = {};
    _snprintf_s( StringBuffer, sizeof(StringBuffer), sizeof(StringBuffer)-1,
  "%s : %2.2f MCy", Region->Record.BlockName, (Region->MaxT-Region->MinT)/1000000.f);

        DEBUGAddTextSTB(GlobalDebugRenderGroup, StringBuffer, CornerPaddingPx, Line++);
      }
    }
  }

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
AddRegion( debug_state* DebugState, debug_frame* CurrentFrame)
{
  debug_frame_region* Result = CurrentFrame->Regions + CurrentFrame->RegionCount;
  if(CurrentFrame->RegionCount < MAX_REGIONS_PER_FRAME-1)
  {
    CurrentFrame->RegionCount++;
  }
  return Result;
}

void CollateDebugRecords(debug_state* DebugState, u32 InvalidEventArrayIndex)
{
  DebugState->Frames = PushArray(&DebugState->Arena, MAX_DEBUG_EVENT_ARRAY_COUNT*4, debug_frame);
  DebugState->FrameBarLaneCount = 0;
  DebugState->FrameCount = 0;
  DebugState->FrameBarRange = 0.0f;
  debug_frame* CurrentFrame = 0;
  for(u32 EventArrayIndex = InvalidEventArrayIndex+1;
      ; // No break
      ++EventArrayIndex)
  {
    if(EventArrayIndex == MAX_DEBUG_EVENT_ARRAY_COUNT)
    {
      EventArrayIndex=0;
    }

    if(EventArrayIndex == InvalidEventArrayIndex)
    {
      break;
    }

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


        CurrentFrame = DebugState->Frames + DebugState->FrameCount++;
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

          DebugBlock->FrameIndex = FrameIndex;
          DebugBlock->OpeningEvent = Event;
          DebugBlock->Parent = Thread->FirstOpenBlock;
          Thread->FirstOpenBlock = DebugBlock;
          DebugBlock->NextFree = 0;
        }else if( Event->Type == DebugEvent_EndBlock){
          if(Thread->FirstOpenBlock && CurrentFrame->Regions)
          {
            open_debug_block* MatchingBlock = Thread->FirstOpenBlock;
            debug_event* OpeningEvent = MatchingBlock->OpeningEvent;
            if((OpeningEvent->TC.ThreadID         == Event->TC.ThreadID) &&
               (OpeningEvent->DebugRecordIndex == Event->DebugRecordIndex) &&
               (OpeningEvent->TranslationUnit  == Event->TranslationUnit))
            {
              if(MatchingBlock->FrameIndex == FrameIndex)
              {
                if(MatchingBlock->Parent == 0)
                {
                  r32 MinT = (r32)(OpeningEvent->Clock - CurrentFrame->BeginClock);
                  r32 MaxT = (r32)(Event->Clock -  CurrentFrame->BeginClock);
                  r32 ThresholdT = 2000;
                  if((MaxT-MinT) > ThresholdT )
                  {
                    debug_frame_region* Region = AddRegion(DebugState, CurrentFrame);
                    Region->LaneIndex = Thread->LaneIndex;
                    Region->MinT = (r32)(OpeningEvent->Clock - CurrentFrame->BeginClock);
                    Region->MaxT = (r32)(Event->Clock -  CurrentFrame->BeginClock);
                    Region->Record = *Source;
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
  #if 0
  debug_counter_state* CounterArray[MAX_DEBUG_TRANSLATION_UNITS];
  debug_counter_state* CurrentCounter = DebugState->CounterStates;
  u32 TotalRecordCount = 0;
  for(u32 UnitIndex = 0;
      UnitIndex < MAX_DEBUG_TRANSLATION_UNITS;
      ++UnitIndex)
  {
    CounterArray[UnitIndex] = CurrentCounter;
    TotalRecordCount += GlobalDebugTable->RecordCount[UnitIndex];
    CurrentCounter += GlobalDebugTable->RecordCount[UnitIndex];
  }
  DebugState->CounterStateCount = TotalRecordCount;

  for(u32 StateIndex = 0; StateIndex < DebugState->CounterStateCount; ++StateIndex)
  {
    debug_counter_state* Dst = DebugState->CounterStates + StateIndex;
    Dst->Snapshots[DebugState->SnapShotIndex].HitCount = 0;
    Dst->Snapshots[DebugState->SnapShotIndex].CycleCount = 0;
  }

  debug_event* Events = GlobalDebugTable->Events[EventArrayIndex];
  for (u32 EventIndex = 0; EventIndex < EventCount; ++EventIndex)
  {
    const debug_event* Event = Events + EventIndex;
    const u32 DebugRecordIndex = Event->DebugRecordIndex;
    const u32 TranslationUnit = Event->TranslationUnit;
    debug_counter_state* Dst = CounterArray[TranslationUnit] + DebugRecordIndex;

    debug_record* Src = GlobalDebugTable->Records[TranslationUnit] + DebugRecordIndex;

    if (!Dst->FileName)
    {
      Dst->FileName = (char*) PushCopy(&DebugState->Memory, str::StringLength(Src->FileName)+1, Src->FileName);
    }
    if (!Dst->FunctionName)
    {
      Dst->FunctionName = (char*) PushCopy(&DebugState->Memory, str::StringLength(Src->BlockName)+1, Src->BlockName);
    }
    Dst->LineNumber = Src->LineNumber;
    if (Event->Type == DebugEvent_BeginBlock)
    {
      ++Dst->Snapshots[DebugState->SnapShotIndex].HitCount;
      Dst->Snapshots[DebugState->SnapShotIndex].CycleCount -= Event->Clock;
    }else if (Event->Type == DebugEvent_EndBlock){
      Dst->Snapshots[DebugState->SnapShotIndex].CycleCount += Event->Clock;
    }
  }
  #endif
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
  debug_state* DebugState = Memory->DebugState;
  if(DebugState)
  {
    if(!DebugState->Initialized)
    {
      DebugState->CollateTemp = BeginTemporaryMemory(&DebugState->Arena);
      DebugState->Initialized = true;
    }

    if(!DebugState->Paused)
    {
      EndTemporaryMemory(DebugState->CollateTemp);
      DebugState->CollateTemp = BeginTemporaryMemory(&DebugState->Arena);

      DebugState->FirstThread = 0;
      DebugState->FirstFreeBlock = 0;

      CollateDebugRecords(DebugState, GlobalDebugTable->CurrentEventArrayIndex);
    }
  }
  return GlobalDebugTable;
}