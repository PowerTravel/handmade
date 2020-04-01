
void PushDebugOverlay(debug_state* DebugState)
{
  TIMED_FUNCTION();
  r32 Line = 0;
  r32 CornerPaddingPx = 30;

  v4 Yellow   = V4(1,1,0,1);
  v4 Green = V4(0,1,0,1);
  r32 Kx = 1.f / GlobalDebugRenderGroup->ScreenWidth;
  r32 Ky = 1.f / GlobalDebugRenderGroup->ScreenHeight;
  r32 AspectRatio = Kx/Ky;

  Assert(DebugState->SnapShotIndex < SNAPSHOT_COUNT);

  u32 TotalCounterStateCount = ArrayCount(DebugState->CounterStates);
  for(u32 i = 0; i<TotalCounterStateCount; ++i)
  {
    debug_counter_state* CounterState = &DebugState->CounterStates[i];
    if(!CounterState->FileName) continue;

    stb_font_map* FontMap = &GlobalDebugRenderGroup->Assets->STBFontMap;

    r32 GraphLeft = 1/4.f;
    r32 GraphRight = 4/8.f;
    r32 BarSegmentWidth = (GraphRight - GraphLeft)/SNAPSHOT_COUNT;

    r32 BaselinePixels = GlobalDebugRenderGroup->ScreenHeight - (Line+1) * FontMap->FontHeightPx;
    r32 GraphBot = Ky*BaselinePixels;
    r32 GraphTop = GraphBot + Ky*FontMap->Ascent;

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

    r32 xMin = GraphLeft;
    for(u32 j = 0; j<SNAPSHOT_COUNT; ++j)
    {
      debug_frame_snapshot* SnapShot = &CounterState->Snapshots[j];
      r32 xMax = xMin + BarSegmentWidth;

      if(HitCount->Avg)
      {
        r32 BarHeightScale = (GraphTop - GraphBot)/(2.f*SnapShot->CycleCountStat.Avg);
        r32 yMax = GraphBot + BarHeightScale*SnapShot->CycleCount;
        aabb2f Rect = AABB2f( V2(xMin,GraphBot), V2(xMax,yMax));
        v4 Color = Green + ((SnapShot->CycleCountStat.Avg) / (SnapShot->CycleCountStat.Max) ) * (Yellow - Green);
        DEBUGPushQuad(GlobalDebugRenderGroup, Rect,Color);
      }

      xMin = GraphLeft + j * BarSegmentWidth;
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

  const r32 GraphLeft = -7/8.f;
  const r32 GraphBot = -7/8.f;
  const r32 GraphWidth = 1.7;
  const r32 GraphHeight = 1/2.f;
  const r32 GraphRight = GraphLeft + GraphWidth;
  const r32 GraphTop = GraphBot + GraphHeight;
  const r32 BarHeightScale = (GraphTop - GraphBot)/(1/60.f);
  const r32 BarSegmentWidth = (GraphRight - GraphLeft)/(r32)SNAPSHOT_COUNT;

  v4 ColorTable[] = {V4(1,0,0,1),
                     V4(0,1,0,1),
                     V4(0,0,1,1),
                     V4(1,1,0,1),
                     V4(1,0,1,1),
                     V4(0,1,1,1),
                     V4(1,1,1,1),
                     V4(0,0,0,1)};


#if 0
  r32 xMin = GraphLeft;
  for(u32 i = 0; i<SNAPSHOT_COUNT; ++i)
  {
    debug_frame_end_info* TimeStamp = DebugState->FrameEndInfos + i;
    r32 xMax = xMin + BarSegmentWidth - BarSegmentWidth/2.f;
    r32 yMin = GraphBot;
    for(u32 j = 0; j<TimeStamp->TimestampCount; ++j)
    {
      debug_frame_timestamp* Time = TimeStamp->Timestamps + j;
      r32 Seconds = Time->Seconds;

      r32 yMax = GraphBot + BarHeightScale*Seconds;

      aabb2f Rect = AABB2f( V2(xMin, yMin), V2(xMax,yMax));
      v4 Color = ColorTable[(u32)(j%ArrayCount(ColorTable))];
      DEBUGPushQuad(GlobalDebugRenderGroup, Rect, Color);

      yMin = yMax;
    }
    xMin = GraphLeft + i * BarSegmentWidth;
  }

  r32 LineWidth = 0.005;
  r32 yMin = GraphBot + BarHeightScale*(1/60.f)+LineWidth/2.f;
  r32 yMax = GraphBot + BarHeightScale*(1/60.f)-LineWidth/2.f;
  aabb2f Rect = AABB2f( V2(GraphLeft,yMin), V2(GraphRight,yMax));

  DEBUGPushQuad(GlobalDebugRenderGroup, Rect, V4(0,0,0,1));
#endif

}

#define DebugRecords_Main_Count __COUNTER__
global_variable debug_table GlobalDebugTable_;
debug_table* GlobalDebugTable = &GlobalDebugTable_;


void CollateDebugRecords(debug_state* DebugState, u32 EventCount, u32 EventArrayIndex, debug_table* DebugTable)
{
  debug_counter_state* CounterArray[MAX_DEBUG_TRANSLATION_UNITS];
  debug_counter_state* CurrentCounter = DebugState->CounterStates;
  u32 TotalRecordCount = 0;
  for(u32 UnitIndex = 0;
      UnitIndex < MAX_DEBUG_TRANSLATION_UNITS;
      ++UnitIndex)
  {
    CounterArray[UnitIndex] = CurrentCounter;
    TotalRecordCount += DebugTable->RecordCount[UnitIndex];
    CurrentCounter += DebugTable->RecordCount[UnitIndex];
  }
  DebugState->CounterStateCount = TotalRecordCount;

  for(u32 StateIndex = 0; StateIndex < DebugState->CounterStateCount; ++StateIndex)
  {
    debug_counter_state* Dst = DebugState->CounterStates + StateIndex;
    Dst->Snapshots[DebugState->SnapShotIndex].HitCount = 0;
    Dst->Snapshots[DebugState->SnapShotIndex].CycleCount = 0;
  }

  debug_event* Events = DebugTable->Events[EventArrayIndex];
  for (u32 EventIndex = 0; EventIndex < EventCount; ++EventIndex)
  {
    const debug_event* Event = Events + EventIndex;
    const u32 DebugRecordIndex = Event->DebugRecordIndex;
    const u32 TranslationUnit = Event->TranslationUnit;
    debug_counter_state* Dst = CounterArray[TranslationUnit] + DebugRecordIndex;

    debug_record* Src = DebugTable->Records[TranslationUnit] + DebugRecordIndex;

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
}