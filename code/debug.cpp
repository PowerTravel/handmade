#include "debug.h"

MENU_DRAW(DrawStatistics);

internal void RefreshCollation();
internal void RestartCollation();

MENU_EVENT_CALLBACK(DebugToggleButton)
{
  v4 InactiveColor = V4(0.3,0.1,0.1,1);
  v4 ActiveColor = V4(0.1,0.3,0.1,1);

  b32* ButtonFlag = (b32*) Data;
  *ButtonFlag = !*ButtonFlag;
  color_attribute* ColorAttr =  (color_attribute*) GetAttributePointer(CallerNode, ATTRIBUTE_COLOR);
  ColorAttr->Color = (*ButtonFlag) ?  ActiveColor : InactiveColor;
}

MENU_EVENT_CALLBACK(DebugRecompileButton)
{
  DebugRewriteConfigFile();
}

MENU_EVENT_CALLBACK(DebugPauseCollationButton)      
{
  debug_state* DebugState = DEBUGGetState();
  DebugState->Paused = !DebugState->Paused;
  if(!DebugState->Paused)
  {
    RefreshCollation();
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
    // Transient Memory Begin
    DebugState->CollateTemp = BeginTemporaryMemory(&DebugState->Arena);
    DebugState->Initialized = true;
    DebugState->Paused = false;

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
    container_node* SettingsPlugin = 0;
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

        RegisterMenuEvent(GlobalGameState->MenuInterface, menu_event_type::MouseDown, ButtonNode, ButtonFlag, DebugToggleButton, 0 );

        return ButtonNode;
      };

      ConnectNodeToBack(ButtonContainer, CreateButton(&DebugState->ConfigMultiThreaded, "Multithread"));
      ConnectNodeToBack(ButtonContainer, CreateButton(&DebugState->ConfigCollisionPoints, "CollisionPoints"));
      ConnectNodeToBack(ButtonContainer, CreateButton(&DebugState->ConfigCollider, "Colliders"));
      ConnectNodeToBack(ButtonContainer, CreateButton(&DebugState->ConfigAABBTree, "AABBTree"));
      container_node* RecompileButton = ConnectNodeToBack(ButtonContainer, NewContainer(GlobalGameState->MenuInterface));
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

        RegisterMenuEvent(GlobalGameState->MenuInterface, menu_event_type::MouseDown, RecompileButton, 0, DebugRecompileButton, 0 );
      }
      SettingsPlugin = CreatePlugin(GlobalGameState->MenuInterface, "Settings", V4(0.5,0.5,0.5,1), ButtonContainer);
    }

    // Create graph window
    container_node* FunctionPlugin = 0;
    {
      //RegisterMenuEvent(GlobalGameState->MenuInterface, menu_event_type::MouseDown, Button, 0, DebugPauseCollationButton, 0 );

      container_node* FunctionContainer = NewContainer(GlobalGameState->MenuInterface);
      FunctionContainer->Functions.Draw = DeclareFunction(menu_draw, DrawStatistics);

      FunctionPlugin = CreatePlugin(GlobalGameState->MenuInterface, "Functions", HexCodeToColorV4( 0xABF74F ), FunctionContainer);
    }

    // Create graph window
    container_node* GraphPlugin = 0;
    {
      container_node* GridContainer = NewContainer(GlobalGameState->MenuInterface, container_type::Grid);
      grid_node* Grid = GetGridNode(GridContainer);
      Grid->Col = 1;
      Grid->TotalMarginX = 0;
      Grid->TotalMarginY = 0;

      container_node* ChartContainer =  ConnectNodeToBack(GridContainer, NewContainer(GlobalGameState->MenuInterface));
      //color_attribute* ChartColor = (color_attribute*) PushAttribute(GlobalGameState->MenuInterface, ChartContainer, ATTRIBUTE_COLOR);
      //ChartColor->Color = V4(0.3,0.1,0.3,1);

      size_attribute* ChartSize = (size_attribute*) PushAttribute(GlobalGameState->MenuInterface, ChartContainer, ATTRIBUTE_SIZE);
      ChartSize->Width = ContainerSizeT(menu_size_type::RELATIVE, 1);
      ChartSize->Height = ContainerSizeT(menu_size_type::RELATIVE, 1);
      ChartSize->LeftOffset = ContainerSizeT(menu_size_type::RELATIVE, 0);
      ChartSize->TopOffset = ContainerSizeT(menu_size_type::RELATIVE, 0);
      ChartSize->XAlignment = menu_region_alignment::CENTER;
      ChartSize->YAlignment = menu_region_alignment::CENTER;


      ChartContainer->Functions.Draw = DeclareFunction(menu_draw, DrawFunctionTimeline);

      container_node* FrameContainer =  ConnectNodeToBack(GridContainer, NewContainer(GlobalGameState->MenuInterface));
      FrameContainer->Functions.Draw = DeclareFunction(menu_draw, DrawFrameFunctions);

      size_attribute* FrameSize = (size_attribute*) PushAttribute(GlobalGameState->MenuInterface, FrameContainer, ATTRIBUTE_SIZE);
      FrameSize->Width = ContainerSizeT(menu_size_type::RELATIVE, 1);
      FrameSize->Height = ContainerSizeT(menu_size_type::RELATIVE, 1);
      FrameSize->LeftOffset = ContainerSizeT(menu_size_type::RELATIVE, 0);
      FrameSize->TopOffset = ContainerSizeT(menu_size_type::RELATIVE, 0);
      FrameSize->XAlignment = menu_region_alignment::CENTER;
      FrameSize->YAlignment = menu_region_alignment::CENTER;

      container_node* Button = NewContainer(GlobalGameState->MenuInterface);
      color_attribute* ButtonColor = (color_attribute*) PushAttribute(GlobalGameState->MenuInterface, Button, ATTRIBUTE_COLOR);
      ButtonColor->Color = V4(0.3,0.1,0.3,1);

      size_attribute* ButtonSizeAttr = (size_attribute*) PushAttribute(GlobalGameState->MenuInterface, GridContainer, ATTRIBUTE_SIZE);
      ButtonSizeAttr->Width = ContainerSizeT(menu_size_type::RELATIVE, 1);
      ButtonSizeAttr->Height = ContainerSizeT(menu_size_type::RELATIVE, 1);
      ButtonSizeAttr->LeftOffset = ContainerSizeT(menu_size_type::RELATIVE, 0);
      ButtonSizeAttr->TopOffset = ContainerSizeT(menu_size_type::RELATIVE, 0);
      ButtonSizeAttr->XAlignment = menu_region_alignment::CENTER;
      ButtonSizeAttr->YAlignment = menu_region_alignment::CENTER;

      text_attribute* Text = (text_attribute*) PushAttribute(GlobalGameState->MenuInterface, Button, ATTRIBUTE_TEXT);
      str::CopyStringsUnchecked( "Pause", Text->Text );
      Text->FontSize = FontSize;
      Text->Color = TextColor;


      container_node* SplitNode  = NewContainer(GlobalGameState->MenuInterface, container_type::Split);
      color_attribute* BackgroundColor = (color_attribute* ) PushAttribute(GlobalGameState->MenuInterface, SplitNode, ATTRIBUTE_COLOR);
      BackgroundColor->Color = V4(0,0,0,0.7);

      container_node* BorderNode = CreateBorderNode(GlobalGameState->MenuInterface, false, 0.2);
      ConnectNodeToBack(SplitNode, BorderNode);
      ConnectNodeToBack(SplitNode, Button);
      ConnectNodeToBack(SplitNode, GridContainer);

      RegisterMenuEvent(GlobalGameState->MenuInterface, menu_event_type::MouseDown, Button, 0, DebugPauseCollationButton, 0 );


      GraphPlugin = CreatePlugin(GlobalGameState->MenuInterface, "Profiler", HexCodeToColorV4( 0xAB274F ), SplitNode);
    }

   // Create empty debug window
    container_node* DebugWindow1 = 0;
    {
      container_node* ColorNode  = NewContainer(GlobalGameState->MenuInterface, container_type::None);
      color_attribute* BackgroundColor = (color_attribute* ) PushAttribute(GlobalGameState->MenuInterface, ColorNode, ATTRIBUTE_COLOR);
      BackgroundColor->Color =  HexCodeToColorV4( 0xD187A2 );

      DebugWindow1 = CreatePlugin(GlobalGameState->MenuInterface, "Debug1",  HexCodeToColorV4( 0xF19CBB ), ColorNode);
    }
    // Create empty debug window
    container_node* DebugWindow2 = 0;
    {
      container_node* ColorNode  = NewContainer(GlobalGameState->MenuInterface, container_type::None);
      color_attribute* BackgroundColor = (color_attribute* ) PushAttribute(GlobalGameState->MenuInterface, ColorNode, ATTRIBUTE_COLOR);
      BackgroundColor->Color = HexCodeToColorV4( 0x9971A3 );

      DebugWindow2 = CreatePlugin(GlobalGameState->MenuInterface, "Debug2",  HexCodeToColorV4( 0xB284BE ), ColorNode);
    }

    menu_tree* WindowsDropDownMenu = RegisterMenu(GlobalGameState->MenuInterface, "Windows");
    RegisterWindow(GlobalGameState->MenuInterface, WindowsDropDownMenu, SettingsPlugin);
    RegisterWindow(GlobalGameState->MenuInterface, WindowsDropDownMenu, GraphPlugin);
    RegisterWindow(GlobalGameState->MenuInterface, WindowsDropDownMenu, DebugWindow1);
    RegisterWindow(GlobalGameState->MenuInterface, WindowsDropDownMenu, DebugWindow2);
    RegisterWindow(GlobalGameState->MenuInterface, WindowsDropDownMenu, FunctionPlugin);

    RestartCollation();
  }
  return DebugState;
}
#define MAX_DEBUG_COLLATED_FRAMES MAX_DEBUG_EVENT_ARRAY_COUNT*4

internal void
RestartCollation()
{
    debug_state* DebugState = DEBUGGetState();
    // We can store MAX_DEBUG_COLLATED_FRAMES frames of collated debug records
    // However, when we change Scope we clear ALL the collated memory.
    // So when we recollate we only have MAX_DEBUG_EVENT_ARRAY_COUNT frames worth of data
    // Thus we loose the 3 * MAX_DEBUG_EVENT_ARRAY_COUNT worth of collated data.
    // One effect of this is that we can display 10 frames, but MAX_DEBUG_EVENT_ARRAY_COUNT is atm 8;
    // Thus when we klick a function to inspect we suddenly only display 7 frames
    EndTemporaryMemory(DebugState->CollateTemp);

    DebugState->CollateTemp = BeginTemporaryMemory(&DebugState->Arena);
#if 0
    //DebugState->FirstThread = 0;
    //DebugState->FirstFreeBlock = 0;
    // TODO: Make this into a rolling buffer. IE never clear it.
    //DebugState->Frames = PushArray(&DebugState->Arena, MAX_DEBUG_COLLATED_FRAMES, debug_frame);
    DebugState->FrameBarLaneCount = 0;
    DebugState->FrameCount = 0;
    DebugState->FrameBarRange = 0;//60000000.0f;
    DebugState->CollationArrayIndex = GlobalDebugTable->CurrentEventArrayIndex+1;
    DebugState->CollationFrame = 0;

    // TODO: Make this into a rolling buffer. IE never clear it.
    DebugState->StatisticsCounts = PushArray(&DebugState->Arena, MAX_DEBUG_COLLATED_FRAMES, u32);
    DebugState->Statistics = PushArray(&DebugState->Arena, MAX_DEBUG_COLLATED_FRAMES, debug_statistics*);
#endif
}

void BeginDebugStatistics(debug_statistics* Statistic, debug_record* Record)
{
  Statistic->HitCount = 0;
  Statistic->Min =  R32Max;
  Statistic->Max = -R32Max;
  Statistic->Avg = 0;
  Statistic->Record = Record;
}

void EndDebugStatistics(debug_statistics* Statistic)
{
  if(Statistic->HitCount != 0)
  {
    Statistic->Avg /= Statistic->HitCount;
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
  ++Statistic->HitCount;
}


#define DebugRecords_Main_Count __COUNTER__

#if HANDMADE_PROFILE
global_variable debug_table GlobalDebugTable_;
debug_table* GlobalDebugTable = &GlobalDebugTable_;
#else
debug_table* GlobalDebugTable = 0;
#endif
internal debug_thread* GetDebugThread(game_memory* Memory, debug_frame* Frame, u16 ThreadID)
{
  Assert(ThreadID);
  if(Memory->ThreadID[0] != ThreadID &&
     Memory->ThreadID[1] != ThreadID &&
     Memory->ThreadID[2] != ThreadID &&
     Memory->ThreadID[3] != ThreadID)
  {
    int a = 10;
  }
  debug_thread* Result = 0;
  for(u32 ThreadArrayIndex = 0;
      ThreadArrayIndex < MAX_THREAD_COUNT;
      ++ThreadArrayIndex)
  {
    debug_thread* Thread = Frame->Threads + ThreadArrayIndex;
    if(!Thread->ID)
    {
      Result = Thread;
      *Result = {};
      Result->ID = ThreadID;
      Result->LaneIndex = Frame->FrameBarLaneCount++;
      Platform.DEBUGPrint("(%d,%d) ", ThreadArrayIndex, ThreadID);
      break;
    }else if((Thread->ID == ThreadID))
    {
      Result = Thread;
      break;
    }
  }
  Assert(Result);
  return Result;
}

#if 1
internal debug_frame_region* AddRegion(debug_frame* CurrentFrame){return 0;}
internal debug_record*
GetRecordFrom(debug_block* OpenBlock)
{
  debug_record* Result = OpenBlock ? OpenBlock->Record : 0;
  return Result;
}
#else
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
#endif
u32 GetBlockHashedIndex(u32 ArrayMaxCount, debug_statistics* StatisticsArray, debug_record* Record)
{
  u32 BlockNameHash = utils::djb2_hash(Record->BlockName);
  u32 Index = BlockNameHash % ArrayMaxCount;
  
  // Handle Collisions
  while(StatisticsArray[Index].Record && 
        !str::ExactlyEquals(StatisticsArray[Index].Record->BlockName, Record->BlockName) )
  {
    Index++;
  }
  return Index;
}

#if 1

debug_statistics* GetStatistics(debug_state* DebugState, debug_record* Record)
{
  u32 Index =  GetBlockHashedIndex(ArrayCount(DebugState->Statistics), DebugState->Statistics, Record);
  debug_statistics* Result = DebugState->Statistics + Index;
  return Result;  
}
void CollateDebugRecords(game_memory* Memory)
{
  debug_state* DebugState = DEBUGGetState();
  ScopedMemory ScopedMemory(GlobalGameState->TransientArena);

  // Start on the frame after the one we are writing to
  u32 DebugTableFrame = GlobalDebugTable->CurrentEventArrayIndex - 1;
  if(GlobalDebugTable->CurrentEventArrayIndex == 0)
  {
    DebugTableFrame = MAX_DEBUG_EVENT_ARRAY_COUNT-1;
  }

  debug_frame* Frame = DebugState->Frames + DebugState->CurrentFrameIndex;

  
  // Loop through all the events of a given frame
  u32 EventCount = GlobalDebugTable->EventCount[DebugTableFrame];
  //Platform.DEBUGPrint("Debug Table Frame %d  DebugStateFrame %d  EventCount %d: ", DebugTableFrame,   DebugState->CurrentFrameIndex, EventCount);
  for(u32 EventIndex = 0;
          EventIndex < EventCount;
          ++EventIndex)
  {
    // A debug event has information about a specific function being run a specific time. An event so to speak.
    // From it we can extract function-name, what thread accessed it and how long it took to execute
    debug_event* Event = GlobalDebugTable->Events[DebugTableFrame] + EventIndex;
    // The debug record holds information about block-name (function name), Line number and translation unit. 
    // It's the function that the event is pointing to.
    debug_record* Record = (GlobalDebugTable->Records[Event->TranslationUnit] + Event->DebugRecordIndex);
    //debug_statistics* Statistics = GetStatistics(DebugState, Record);

#if 1
    switch(Event->Type)
    {
      case DebugEvent_FrameMarker:
      {
        if(Frame->FirstFreeBlock)
        {
          Frame->EndClock = Event->Clock;
          Frame->WallSecondsElapsed = Event->SecondsElapsed;
        }

        if(DebugState->CurrentFrameIndex >= ArrayCount(DebugState->Frames))
        {
          DebugState->CurrentFrameIndex = 0;
          Frame = DebugState->Frames;
        }else{
          ++Frame;
          ++DebugState->CurrentFrameIndex;
        }

        ZeroArray(ArrayCount(Frame->Threads), Frame->Threads);
        
        Frame->BeginClock = Event->Clock;
        Frame->EndClock = 0;
        Frame->FrameBarLaneCount = 0;
        Frame->WallSecondsElapsed = 0;
        ZeroArray(ArrayCount(Frame->Blocks), Frame->Blocks);
        Frame->FirstFreeBlock = Frame->Blocks;
        
      }break;
      case DebugEvent_BeginBlock:
      {
        if(Frame->FirstFreeBlock)
        {
          debug_thread* Thread = GetDebugThread(Memory, Frame, Event->TC.ThreadID);
          debug_block* Block = Frame->FirstFreeBlock++;
          Block->Record = Record;
          Block->ThreadIndex = Thread->ID;
          Block->BeginClock = Event->Clock - Frame->BeginClock;
          Block->EndClock = 0;
          Block->OpeningEvent = *Event;

          // Set the opening block for this thread
          if(!Thread->FirstBlock)
          {
            Thread->FirstBlock = Block;
          }

          // If we have an open block it means this event was called inside it -> we are going deeper into the stack
          if(Thread->OpenBlock)
          {
            // If the first child for this block is not set we set it
            // (This event event is the first block going deeper into the stack) 
            if(!Thread->OpenBlock->FirstChild)
            {
              Thread->OpenBlock->FirstChild = Block;
            }
          }
          
          if(str::ExactlyEquals(Block->Record->BlockName, "GameUpdateAndRender"))
          {
            int a = 10;
          }
          if(str::ExactlyEquals(Block->Record->BlockName, "GameMainLoop"))
          {
            int a = 10;
          }

          // Set the open block as a parent
          // (It's ok for Thread->OpenBlock to be null)
          // It means that we are on the top of the stack
          Block->Parent = Thread->OpenBlock;

          if(Thread->ClosedBlock)
          {
            if(GetRecordFrom(Thread->ClosedBlock->Parent) == GetRecordFrom(Block->Parent))
            {
              Thread->ClosedBlock->Next = Block;
            }
          }

          // This is our new open block
          Thread->OpenBlock = Block;
        }

      }break;
      case DebugEvent_EndBlock:
      {
        if(Frame->FirstFreeBlock)
        {
          debug_thread* Thread = GetDebugThread(Memory, Frame, Event->TC.ThreadID);
          Assert(Thread->OpenBlock);
          debug_block* Block = Thread->OpenBlock;
          Block->EndClock = Event->Clock - Frame->BeginClock;
          
          if(str::ExactlyEquals(Block->Record->BlockName, "GameUpdateAndRender"))
          {
            int a = 10;
          }
          if(str::ExactlyEquals(Block->Record->BlockName, "GameMainLoop"))
          {
            int a = 10;
          }

#if 0
          debug_event* OpeningEvent = &Block->OpeningEvent;
          Assert(OpeningEvent->TC.ThreadID      == Event->TC.ThreadID);
          Assert(OpeningEvent->DebugRecordIndex == Event->DebugRecordIndex);
          Assert(OpeningEvent->TranslationUnit  == Event->TranslationUnit);
#endif

          Thread->ClosedBlock = Block;

          
          Thread->OpenBlock = Block->Parent;
        }
      }break;
    } 
    #endif
  }
  Platform.DEBUGPrint("\n");
}
#else
void CollateDebugRecords()
{
  debug_state* DebugState = DEBUGGetState();
  ScopedMemory Memory(GlobalGameState->TransientArena);

  // Start on the frame after the one we are writing to
  u32 FrameIndexToCollate = GlobalDebugTable->CurrentEventArrayIndex - 1;
  if(GlobalDebugTable->CurrentEventArrayIndex == 0)
  {
    FrameIndexToCollate = MAX_DEBUG_EVENT_ARRAY_COUNT-1;
  }

  u32 TempStatsMaxCount = MAX_DEBUG_TRANSLATION_UNITS * MAX_DEBUG_RECORD_COUNT;
  debug_statistics* TemporaryDebugStats = PushArray(GlobalGameState->TransientArena, TempStatsMaxCount, debug_statistics);
  u32 RecordsInFrame = 0;
  // The debug_frame keeps track on time spent in a current frame
  debug_frame* CurrentFrame = DebugState->CollationFrame;
  
  // Loop through all the events of a given frame
  u32 EventCount = GlobalDebugTable->EventCount[FrameIndexToCollate];
  for(u32 EventIndex = 0;
          EventIndex < EventCount;
          ++EventIndex)
  {
    // A debug event has information about a specific function being run a specific time. An event so to speak.
    // From it we can extract function-name, what thread accessed it and how long it took to execute
    debug_event* Event = GlobalDebugTable->Events[FrameIndexToCollate] + EventIndex;
    // The debug record holds information about block-name (function name), Line number and translation unit. 
    // It's the function that the event is pointing to.
    debug_record* Source = (GlobalDebugTable->Records[Event->TranslationUnit] + Event->DebugRecordIndex);

    // Handle the event type frame marker. Keeps track of time spent on the frame as a whole.
    // First time we enter here after a ResetCollation CurrentFrame will be null. 
    // We basically ignore all events untill we find a frame marker and only then initiate CurrentFrame
    if(Event->Type == DebugEvent_FrameMarker)
    {
      // Finish up the current frame
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

      // Start the next frame (or initiate the first frame)
      // Each time DebugState->FrameCount reaches the max number of frames we are tracking we restart collation
      // Has to be handled in a rolling buffer
      DebugState->CollationFrame = DebugState->Frames + DebugState->FrameIndex++;
      if(DebugState->FrameIndex >= MAX_DEBUG_COLLATED_FRAMES)
      {
        DebugState->FrameIndex = 0;
      }
      CurrentFrame = DebugState->CollationFrame;
      CurrentFrame->BeginClock = Event->Clock;
      CurrentFrame->EndClock = 0;
      CurrentFrame->RegionCount = 0;
      CurrentFrame->WallSecondsElapsed = 0;

      // A source of memory leaks if we don't reset the collation-memory from time to time.
      // TODO: Use a rolling buffer
      CurrentFrame->Regions = PushArray(&DebugState->Arena, MAX_REGIONS_PER_FRAME, debug_frame_region);

    }else if(CurrentFrame)
    {
      // If we have found a frame marker we start collation events for the frame.
      u32 FrameIndex = DebugState->FrameIndex - 1;
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

        // Push the latest event-block onto the stack
        DebugBlock->Parent = Thread->FirstOpenBlock;
        Thread->FirstOpenBlock = DebugBlock;

        DebugBlock->NextFree = 0;
      }else if(Event->Type == DebugEvent_EndBlock)
      {
        // Since any time we encounter a 'DebugEvent_EndBlock' it should refer to the Thread->FirstOpenBlock
        // Which means we should have gotten to a 'DebugEvent_BeginBlock' before it and thus have a valid
        // Thread->FirstOpenBlock.
        // Maybe this is not true for events that span 'DebugEvent_FrameMarker' boundaries, but we can handle that 
        // if it comes up.
        Assert(Thread->FirstOpenBlock);

        Assert(CurrentFrame->Regions);
        open_debug_block* MatchingBlock = Thread->FirstOpenBlock;
        debug_event* OpeningEvent = &MatchingBlock->OpeningEvent;

        if((OpeningEvent->TC.ThreadID      == Event->TC.ThreadID) &&
           (OpeningEvent->DebugRecordIndex == Event->DebugRecordIndex) &&
           (OpeningEvent->TranslationUnit  == Event->TranslationUnit))
        {
          if(MatchingBlock->StartingFrameIndex == FrameIndex)
          {
            r32 MinT = (r32)(OpeningEvent->Clock - CurrentFrame->BeginClock);
            r32 MaxT = (r32)(Event->Clock -  CurrentFrame->BeginClock);
            r32 Time = MaxT-MinT;

            u32 StatsIndex = GetBlockHashedIndex(TempStatsMaxCount, TemporaryDebugStats, Source);
            if(!TemporaryDebugStats[StatsIndex].Record)
            {
              ++RecordsInFrame;
              BeginDebugStatistics(TemporaryDebugStats + StatsIndex, Source);
            }

            AccumulateStatistic(TemporaryDebugStats + StatsIndex, Time);

            // We only collate records that has the same parent as DebugState->ScopeToRecord
            if(GetRecordFrom(MatchingBlock->Parent) == DebugState->ScopeToRecord)
            {
              r32 ThresholdT = 2000;
              if(Time > ThresholdT )
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
            Assert(0); // Just for alerting us if this case ever showes up so we can deal with it then

          }

          Thread->FirstOpenBlock->NextFree = DebugState->FirstFreeBlock;
          DebugState->FirstFreeBlock = Thread->FirstOpenBlock;
          Thread->FirstOpenBlock = MatchingBlock->Parent;
        }else{
          // Record span that goes to the beginning of the frame
          Assert(0); // Just for alerting us if this case ever showes up so we can deal with it then
        }
      }else{
        Assert(!"Invalid event type");
      }
    }
  }  
  if(RecordsInFrame)
  {
    u32 FrameIndex = DebugState->FrameIndex - 1;
    debug_statistics** StatisticsArray = DebugState->Statistics + FrameIndex;
    Assert(!(*StatisticsArray));
    *StatisticsArray = PushArray(&DebugState->Arena, RecordsInFrame, debug_statistics);
    DebugState->StatisticsCounts[FrameIndex] = RecordsInFrame;
    u32 Index = 0;
    for (u32 TempIndex = 0; TempIndex < TempStatsMaxCount; ++TempIndex)
    {
      debug_statistics* TempStatistic = TemporaryDebugStats + TempIndex;
      if(TempStatistic->Count)
      {
        EndDebugStatistics(TempStatistic);
        debug_statistics* Statistic = (*StatisticsArray) + Index++;
        *Statistic = *TempStatistic;
      }
    }
  }
}
#endif
internal void
RefreshCollation()
{
  RestartCollation();
  //CollateDebugRecords();
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
  GlobalDebugTable->RecordCount[0] = DebugRecords_Main_Count; // This stores the amount of records found is the first translation unit, (The Game)

  // Increment which event-array we should  be writing to. (Each frame writes into it's own array)
  ++GlobalDebugTable->CurrentEventArrayIndex;
  if(GlobalDebugTable->CurrentEventArrayIndex >= ArrayCount(GlobalDebugTable->Events))
  {
    // Wrap if we reached the final array
    GlobalDebugTable->CurrentEventArrayIndex=0;
  }

  // Shift CurrentEventArrayIndex to the high bits of ArrayIndex_EventIndex.
  //  The old value of ArrayIndex_EventIndex is returned by AtomicExchangeu64
  // Note the low bits "EventIndex" is zeroed out since we wanna start writing from the top of the array next frame.
  // EventIndex is incremented each time we run through a TIMED_FUNCTION macro
  u64 ArrayIndex_EventIndex = AtomicExchangeu64(&GlobalDebugTable->EventArrayIndex_EventIndex,          // The new value is stored in this variable
                                               ((u64)GlobalDebugTable->CurrentEventArrayIndex << 32));  // This is the new value

  u32 EventArrayIndex = (ArrayIndex_EventIndex >> 32);         // The event array index we just finished writing to
  u32 EventCount = (ArrayIndex_EventIndex & 0xFFFFFFFF);       // The number of events encountered last frame
  GlobalDebugTable->EventCount[EventArrayIndex] = EventCount;  // The frame "EventArrayIndex" saw "EventCount" Recorded Events

  debug_state* DebugState = DEBUGGetState();
  if(DebugState)
  {
    if(!DebugState->Paused)
    {
      // In our debug-local-storage we keep information of MAX_DEBUG_COLLATED_FRAMES frames
      #if 0
      if(DebugState->FrameCount >= MAX_DEBUG_COLLATED_FRAMES)
      {
        // Restart collation zeores out all our debug-local-storage and signals a restar of collation
        // Todo: Maybe we never should zero out this memory and just keep a rolling buffer going
        //       I don't know why we zero out every MAX_DEBUG_COLLATED_FRAMES frames
        RestartCollation();
      }
      #endif
      CollateDebugRecords(Memory);
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

//inline internal debug_frame*
//GetActiveDebugFrame(debug_state* DebugState)
//{
//  debug_frame* Result = DebugState->Frames + DebugState->FrameCount-2;
//  return Result;
//}

void DEBUGAddTextSTB(const c8* String, r32 LineNumber, u32 FontSize)
{
  TIMED_FUNCTION();
  game_window_size WindowSize = GameGetWindowSize();
  stb_font_map* FontMap = GetFontMap(GlobalGameState->AssetManager, FontSize);
  r32 CanPosX = 1/100.f;
  r32 CanPosY = 1 - ((LineNumber+1) * FontMap->Ascent - LineNumber*FontMap->Descent)/WindowSize.HeightPx;
  PushTextAt(CanPosX, CanPosY, String, FontSize, V4(1,1,1,1));
}


MENU_DRAW(DrawStatistics)
{
  #if 0
  rect2f Chart = Node->Region;

  debug_state* DebugState = DEBUGGetState();
  r32 Line = 3;
  TIMED_FUNCTION();

  u32 FrameIndex = DebugState->CurrentFrameIndex;
  debug_statistics* StatsArray = DebugState->Statistics + FrameIndex;
  u32 StatsCount = DebugState->StatisticsCounts[FrameIndex];
  for (u32 StatisticIndex = 0; StatisticIndex < StatsCount; ++StatisticIndex)
  {
    debug_statistics* Statistic = StatsArray + StatisticIndex;
    if(Statistic->Count)
    {
      c8 StringBuffer[256] = {};
      Platform.DEBUGFormatString(StringBuffer, sizeof(StringBuffer), sizeof(StringBuffer)-1,
    "(%5d)%-25s:%10dCy:%25dh:%10dcy/h",
        Statistic->Record->LineNumber, Statistic->Record->BlockName, (u32) Statistic->Avg, Statistic->Count,
        (u32) (Statistic->Avg/(r32)Statistic->Count));

//      rect2f TextBox = GetTextSize(0, 0, Text->Text, Text->FontSize);
//      PushTextAt(Parent->Region.X + Parent->Region.W/2.f - TextBox.W/2.f,
//                     Parent->Region.Y + Parent->Region.H/2.f - TextBox.H/3.f, Text->Text, Text->FontSize, Text->Color);
//
      DEBUGAddTextSTB(StringBuffer, Line, 18);
      Line++;  
    }
  }
  #endif
}

MENU_DRAW(DrawFrameFunctions)
{
  rect2f Chart = Node->Region;
  debug_state* DebugState = DEBUGGetState();

  debug_frame* Frame = DebugState->SelectedFrame;
  if(!Frame) return;

  u32 ThreadCount = ArrayCount(Frame->Threads);

  r32 BarWidth = Chart.H/(r32)ThreadCount;
  r32 dt = GlobalGameState->Input->dt;
  r32 FrameTargetWidth = Chart.W;
  r32 ChartWidthPerFrameSec = FrameTargetWidth/dt;

  r32 SecPerCycle = Frame->WallSecondsElapsed / (Frame->EndClock - Frame->BeginClock);

  r32 ChartWidtPerCycle = ChartWidthPerFrameSec * SecPerCycle;


  v4 ColorTable[] = {V4(1,0,0,1),
                     V4(0,1,0,1),
                     V4(0,0,1,1),
                     V4(1,1,0,1),
                     V4(1,0,1,1),
                     V4(0,1,1,1),
                     V4(1,1,1,1),
                     V4(0,0,0,1)};

  r32 MouseX = Interface->MousePos.X;
  r32 MouseY = Interface->MousePos.Y;
  debug_block* SelectedBlock = 0;
  //for (u32 ThreadIndex = 0; ThreadIndex < ThreadCount; ++ThreadIndex)
  for (u32 ThreadIndex = 0; ThreadIndex < 1; ++ThreadIndex)
  {
    debug_thread* Thread = Frame->Threads + ThreadIndex;

    if(!Thread->FirstBlock) continue;

    debug_block* BaseBlock = Thread->FirstBlock;
    if(Thread->SelectedBlock)
    {
      BaseBlock = Thread->SelectedBlock->FirstChild;
    }

    debug_block* ParentBlock = BaseBlock;
    u32 Depth = 0;
    while(ParentBlock)
    {
      ++Depth;
      ParentBlock = ParentBlock->Parent;
    }

    u32 LaneIndex = 0;
    while(BaseBlock)
    {
      debug_block* Block = BaseBlock;
      u32 BlockIndex = 0;
      while(Block)
      {
        rect2f Rect{};
        Rect.X = Chart.X + ChartWidtPerCycle * (r32)Block->BeginClock;
        r32 LineHeight = (r32) (Depth - LaneIndex);
        Rect.Y = Chart.Y + BarWidth * LineHeight;
        r32 CycleCount = (r32)(Block->EndClock - Block->BeginClock);
        Rect.W = ChartWidtPerCycle * CycleCount;
        Rect.H = BarWidth;
        
        v4 Color = ColorTable[(u32)(BlockIndex % ArrayCount(ColorTable))];
        BlockIndex++;

        PushOverlayQuad(Rect, Color);

        if(Intersects(Rect, V2(MouseX,MouseY)))
        {
          c8 StringBuffer[1048] = {};
          Platform.DEBUGFormatString( StringBuffer, sizeof(StringBuffer), sizeof(StringBuffer)-1,
          "%s : %2.2f MCy", Block->Record->BlockName, CycleCount/1000000.f);
          PushTextAt(MouseX, MouseY+0.02f, StringBuffer, 24, V4(1,1,1,1));
          SelectedBlock = Block;
        }

        Block = Block->Next;
      }  
      BaseBlock = BaseBlock->Parent;
      LaneIndex++;
    }
    if(Interface->MouseLeftButton.Edge && Interface->MouseLeftButton.Active)
    {
      Thread->SelectedBlock = SelectedBlock;
    }
  }
  

}

MENU_DRAW(DrawFunctionTimeline)
{
  #if 1
  rect2f Chart = Node->Region;

  r32 dt = GlobalGameState->Input->dt;

  r32 FrameTargetHeight = Chart.H * 0.7f;
  r32 HeightScaling = FrameTargetHeight/dt;

  debug_state* DebugState = DEBUGGetState();
  u32 MaxFramesToDisplay = ArrayCount(DebugState->Frames);
  r32 BarWidth = Chart.W/MaxFramesToDisplay;

  v4 ColorTable[] = {V4(1,0,0,1),
                     V4(0,1,0,1),
                     V4(0,0,1,1),
                     V4(1,1,0,1),
                     V4(1,0,1,1),
                     V4(0,1,1,1),
                     V4(1,1,1,1),
                     V4(0,0,0,1)};

  r32 MouseX = Interface->MousePos.X;
  r32 MouseY = Interface->MousePos.Y;

  u32 FrameCount =  ArrayCount(DebugState->Frames);
  u32 Count = 0;
  debug_frame* Frame = DebugState->Frames + DebugState->CurrentFrameIndex+1;
  debug_frame* SelectedFrame = 0;
  for(u32 BarIndex = 0; BarIndex < MaxFramesToDisplay-1; ++BarIndex)
  {
    u32 FrameIndex = (u32) (Frame-DebugState->Frames);
    if(FrameIndex >= FrameCount)
    {
      Frame = DebugState->Frames; 
    }
    r32 FrameX = Chart.X + (r32)(BarIndex)*BarWidth;
    r32 FrameY = Chart.Y;

    rect2f Rect = {};
    Rect.X = FrameX;
    Rect.Y = FrameY;
    Rect.W = BarWidth;
    Rect.H = Frame->WallSecondsElapsed * HeightScaling;
    v4 Color = ColorTable[(u32)(BarIndex%ArrayCount(ColorTable))];
    PushOverlayQuad(Rect, Color);

    if(Intersects(Rect, V2(MouseX,MouseY)))
    {
      SelectedFrame = Frame;
      c8 StringBuffer[1048] = {};
      Platform.DEBUGFormatString( StringBuffer, sizeof(StringBuffer), sizeof(StringBuffer)-1,
      "Frame %d:  %2.2f Sec", FrameIndex, Frame->WallSecondsElapsed);
      PushTextAt(MouseX, MouseY+0.02f, StringBuffer, 24, V4(1,1,1,1));
    }

    ++Frame;
  }

  if(Interface->MouseLeftButton.Edge && Interface->MouseLeftButton.Active)
  {
    if(SelectedFrame)
    {
      DebugState->SelectedFrame = SelectedFrame;
      DebugState->Paused = true;
    }else if(Intersects(Chart, V2(MouseX,MouseY))){
      DebugState->SelectedFrame = 0;
      DebugState->Paused = false;
    }
  }
  

  rect2f Rect = {};
  Rect.X = Chart.X;
  Rect.Y = Chart.Y + 0.7f*Chart.H;
  Rect.W = Chart.W;
  Rect.H = 0.01;
  PushOverlayQuad(Rect, V4(0,0,0,1));

  #endif
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

  //if(DebugState->Frames)
  //{
  //  c8 StringBuffer[256] = {};
  //   debug_frame* Frame = GetActiveDebugFrame(DebugState);
  //  Platform.DEBUGFormatString(StringBuffer, sizeof(StringBuffer), sizeof(StringBuffer)-1,
  //"%3.1f Hz, %4.2f ms", 1.f/Frame->WallSecondsElapsed, Frame->WallSecondsElapsed*1000);
  //  DEBUGAddTextSTB(StringBuffer, LineNumber++, 24);
  //}
}


#if 0
void DrawFunctionCount(){
  Assert(DebugState->SnapShotIndex < SNAPSHOT_COUNT);

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
