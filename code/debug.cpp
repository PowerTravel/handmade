#include "debug.h"

#include "math/utils.h"
#include "color_table.h"

MENU_DRAW(DrawStatistics);

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

      container_node* FunctionContainer = NewContainer(GlobalGameState->MenuInterface);
      FunctionContainer->Functions.Draw = DeclareFunction(menu_draw, DrawStatistics);

      FunctionPlugin = CreatePlugin(GlobalGameState->MenuInterface, "Functions", HexCodeToColorV4( 0xABF74F ), FunctionContainer);
    }

    // Create graph window
    container_node* GraphPlugin = 0;
    {
      container_node* ChartContainer = NewContainer(GlobalGameState->MenuInterface);
      ChartContainer->Functions.Draw = DeclareFunction(menu_draw, DrawFunctionTimeline);

      container_node* FrameContainer = NewContainer(GlobalGameState->MenuInterface);
      FrameContainer->Functions.Draw = DeclareFunction(menu_draw, DrawFrameFunctions);

      container_node* SplitNode  = NewContainer(GlobalGameState->MenuInterface, container_type::Split);
      color_attribute* BackgroundColor = (color_attribute* ) PushAttribute(GlobalGameState->MenuInterface, SplitNode, ATTRIBUTE_COLOR);
      BackgroundColor->Color = V4(0,0,0,0.7);

      container_node* BorderNode = CreateBorderNode(GlobalGameState->MenuInterface, false, 0.7);
      ConnectNodeToBack(SplitNode, BorderNode);
      ConnectNodeToBack(SplitNode, FrameContainer);
      ConnectNodeToBack(SplitNode, ChartContainer);

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

    ToggleWindow(GlobalGameState->MenuInterface, "Profiler");

  }
  return DebugState;
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

internal debug_record*
GetRecordFrom(debug_block* OpenBlock)
{
  debug_record* Result = OpenBlock ? OpenBlock->Record : 0;
  return Result;
}

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

  u32 EventCount = GlobalDebugTable->EventCount[DebugTableFrame];
  for(u32 EventIndex = 0;
          EventIndex < EventCount;
          ++EventIndex)
  {
    debug_event* Event = GlobalDebugTable->Events[DebugTableFrame] + EventIndex;
    debug_record* Record = (GlobalDebugTable->Records[Event->TranslationUnit] + Event->DebugRecordIndex);

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
          if(Thread->OpenBlock && !Thread->OpenBlock->FirstChild)
          {
            Thread->OpenBlock->FirstChild = Block;
          }

          Block->Parent = Thread->OpenBlock;


          if(Thread->ClosedBlock && GetRecordFrom(Thread->ClosedBlock->Parent) == GetRecordFrom(Block->Parent))
          {
            Thread->ClosedBlock->Next = Block;
          }

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

          debug_event* OpeningEvent = &Block->OpeningEvent;
          Assert(OpeningEvent->TC.ThreadID      == Event->TC.ThreadID);
          Assert(OpeningEvent->DebugRecordIndex == Event->DebugRecordIndex);
          Assert(OpeningEvent->TranslationUnit  == Event->TranslationUnit);

          Thread->ClosedBlock = Block;
          
          Thread->OpenBlock = Block->Parent;
        }
      }break;
    }
  }
  Platform.DEBUGPrint("\n");
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

v4 GetColorForRecord(debug_record* Record)
{
  umm Index = (Record - GlobalDebugTable->Records[0]) << 2;
  v4 Result = GetColor(Index);
  return Result;
}

void PushHorizontalBlockRect( debug_block* Block, rect2f Region, u32 Lane, r32 StartY, r32 BarWidth, r32 Envelope, r32 CycleScaling )
{
  rect2f Rect{};
  Rect.X = Region.X+Envelope*0.5f;
  Rect.Y = StartY + Lane * BarWidth + Envelope * 0.5f;
  r32 CycleCount = (r32)(Block->EndClock - Block->BeginClock);
  Rect.W = CycleScaling * CycleCount - Envelope;
  Rect.H = BarWidth - Envelope;

  v4 Color = GetColorForRecord(Block->Record);
  PushOverlayQuad(Rect, Color);
}


b32 DrawLane(u32 ArrayMaxCount, u32 BlockCount, debug_block*** BlockArray, u32* BufferCount, debug_block*** Buffer, debug_block** SelectedBlock,
             r32 StartX, r32 StartY, u32 LaneIndex, r32 LaneWidth, r32 CycleScaling, u64 CycleOffset, r32 PixelSize, v2 MousePos)
{
  *BufferCount = 0;
  *SelectedBlock = 0;
  ZeroArray(ArrayMaxCount, *Buffer);
  b32 BlocksDrawn = false;
  for (u32 i = 0; i < BlockCount; ++i)
  {
    debug_block* Block = (*BlockArray)[i];
    while(Block)
    {
      if(Block->FirstChild)
      {
        (*Buffer)[(*BufferCount)++] = Block->FirstChild;
      }

      rect2f Rect{};
      Rect.X = StartX + CycleScaling * (r32)( Block->BeginClock - CycleOffset) + PixelSize*0.5f;
      Rect.Y = StartY + LaneWidth * LaneIndex + PixelSize*0.5f;
      r32 CycleCount = (r32)(Block->EndClock - Block->BeginClock);
      Rect.W = CycleScaling * CycleCount - PixelSize;
      Rect.H = LaneWidth - PixelSize;
      
      if(Rect.W > PixelSize)
      {
        v4 Color = GetColorForRecord(Block->Record);
        PushOverlayQuad(Rect, Color);
        BlocksDrawn = true;
        if(Intersects(Rect, MousePos))
        {
          c8 StringBuffer[1048] = {};
          Platform.DEBUGFormatString( StringBuffer, sizeof(StringBuffer), sizeof(StringBuffer)-1,
          "%s : %2.2f MCy", Block->Record->BlockName, CycleCount/1000000.f);
          PushTextAt(MousePos.X, MousePos.Y+0.02f, StringBuffer, 24, V4(1,1,1,1));
          *SelectedBlock = Block;
        }
      }
      Block = Block->Next;
    }
  }
  return BlocksDrawn;
}

u32 PushLaneToBuffer(debug_block* Block, u32 BufferCount, debug_block** Buffer)
{
  while(Block)
  {
    if(Block->FirstChild)
    {
      Buffer[BufferCount++] = Block->FirstChild;  
    }
    Block = Block->Next;
  }
  return BufferCount;
}

debug_block* DrawBlockChain(debug_block* Block, r32 StartX, r32 StartY, r32 LaneWidth, r32 PixelSize, r32 CycleScaling, u64 CycleOffset, v2 MousePos)
{
  debug_block* Result = 0;
  while(Block)
  {
    rect2f Rect{};
    Rect.X = StartX + CycleScaling * (r32)(Block->BeginClock - CycleOffset) + PixelSize*0.5f;
    Rect.Y = StartY + PixelSize*0.5f;
    r32 CycleCount = (r32)(Block->EndClock - Block->BeginClock);
    Rect.W = CycleScaling * CycleCount - PixelSize;
    Rect.H = LaneWidth - PixelSize;

    if(Rect.W > PixelSize)
    {
      v4 Color = GetColorForRecord(Block->Record);
      PushOverlayQuad(Rect, Color);

      if(Intersects(Rect, MousePos))
      {
        c8 StringBuffer[1048] = {};
        Platform.DEBUGFormatString( StringBuffer, sizeof(StringBuffer), sizeof(StringBuffer)-1,
        "%s : %2.2f MCy", Block->Record->BlockName, CycleCount/1000000.f);
        PushTextAt(MousePos.X, MousePos.Y+0.02f, StringBuffer, 24, V4(1,1,1,1));
        
        Result = Block;
      }
    }


    Block = Block->Next;
  }

  return Result;
}

u64 GetBlockChainCycleCount(debug_block* FirstBlock)
{
  debug_block* LastBlock = FirstBlock;
  while(LastBlock->Next)
  {
    LastBlock = LastBlock->Next;
  }
  u64 Result = LastBlock->EndClock - FirstBlock->BeginClock;
  return Result;
}

MENU_DRAW(DrawFrameFunctions)
{
  r32 ThreadBreak = 0.005;

  rect2f Chart = Node->Region;
  Chart.H -= ThreadBreak;

  debug_state* DebugState = DEBUGGetState();
  v2 MousePos = Interface->MousePos;

  debug_frame* Frame = DebugState->SelectedFrame;
  if(!Frame)
  {
    u32 FrameCount = ArrayCount(DebugState->Frames);
    u32 FrameIndex = (DebugState->CurrentFrameIndex-1) % FrameCount;
    Frame = DebugState->Frames + FrameIndex;
  };

  r32 ThreadCount = (r32)Frame->FrameBarLaneCount;
  r32 MaxLaneCountPerThread = 8;

  game_window_size WindowSize = GameGetWindowSize();
  r32 PixelSize = 1.f / WindowSize.HeightPx;

  r32 LaneWidth = Chart.H/(MaxLaneCountPerThread + ThreadCount);
  r32 dt = GlobalGameState->Input->dt;

  debug_block* SelectedBlock = 0;
  r32 ThreadHeight = 0;
  r32 StartY = Chart.Y;
  
  u32 SelectedThreadIndex = 0;

  r32 ThreadStartY = Chart.Y;
  

  debug_block* HotBlock = 0;
  debug_thread* HotThread = 0;

  debug_block* SwapBuffer[2][1048] = {};
  debug_block** ConsumeBuffer = SwapBuffer[0];
  debug_block** ProduceBuffer = SwapBuffer[1];
  u32 ConsumeCount = 0;
  u32 ProducedCount = 0;
  r32 LaneIndex = 0;
  for (u32 ThreadIndex = 0; ThreadIndex < ThreadCount; ++ThreadIndex)
  {
    r32 CycleScaling = Chart.W / (r32) (Frame->EndClock - Frame->BeginClock);
    u64 CycleOffset = 0;

    debug_thread* Thread = Frame->Threads + ThreadIndex;
  
    Assert(Thread->FirstBlock);

    if(Thread->SelectedBlock)
    {
      debug_block* BaseBlock = Thread->SelectedBlock;

      CycleScaling = Chart.W / (r32) (BaseBlock->EndClock - BaseBlock->BeginClock);
      CycleOffset = BaseBlock->BeginClock;

      rect2f FirstBlockRect = {};
      FirstBlockRect.X = Chart.X;
      FirstBlockRect.Y = ThreadStartY+ ThreadBreak *0.5f;
      FirstBlockRect.W = Chart.W;
      FirstBlockRect.H = LaneWidth;

      v4 Color = GetColorForRecord(BaseBlock->Record);
      if(Intersects(FirstBlockRect, MousePos))
      {
        c8 StringBuffer[1048] = {};
        Platform.DEBUGFormatString( StringBuffer, sizeof(StringBuffer), sizeof(StringBuffer)-1,
        "%s : %2.2f MCy", BaseBlock->Record->BlockName, (u32)(BaseBlock->EndClock - BaseBlock->BeginClock)/1000000.f);
        PushTextAt(MousePos.X, MousePos.Y+0.02f, StringBuffer, 24, V4(1,1,1,1));
        HotBlock = Thread->SelectedBlock;
      }
      PushOverlayQuad(FirstBlockRect, Color);
      ConsumeBuffer[ConsumeCount++] = Thread->SelectedBlock->FirstChild;
      LaneIndex = 1;
    }else{
      debug_block* MousedOverBlock = DrawBlockChain(Thread->FirstBlock, Chart.X, ThreadStartY + ThreadBreak *0.5f, LaneWidth, PixelSize, CycleScaling, CycleOffset, MousePos);
      if(!HotBlock)
      {
        HotBlock = MousedOverBlock;
      }
      LaneIndex = 1;
      if(DebugState->ThreadSelected)
      {
        if(ThreadIndex == DebugState->SelectedThreadIndex)
        {
          ConsumeCount = PushLaneToBuffer(Thread->FirstBlock, 0, ConsumeBuffer);
        }
      }
    }
    
    while(ConsumeCount && LaneIndex < MaxLaneCountPerThread )
    {
      for (u32 BlockIndex = 0; BlockIndex < ConsumeCount; ++BlockIndex)
      {
        debug_block* Block = ConsumeBuffer[BlockIndex];
        r32 YOffset = LaneWidth * LaneIndex + ThreadBreak*0.5f;
        debug_block* MousedOverBlock = DrawBlockChain(Block, Chart.X, ThreadStartY + YOffset, LaneWidth, PixelSize, CycleScaling, CycleOffset, MousePos);
        if(MousedOverBlock)
        {
          HotBlock = MousedOverBlock;
        }
        ProducedCount = PushLaneToBuffer(Block, ProducedCount, ProduceBuffer);
      }
      ++LaneIndex;

      ZeroArray(ConsumeCount, ConsumeBuffer);
      ConsumeCount = ProducedCount;
      ProducedCount = 0;

      debug_block** TmpBuffer = ConsumeBuffer;
      ConsumeBuffer = ProduceBuffer;
      ProduceBuffer = TmpBuffer;
    }

    rect2f ThreadRegion = {};
    ThreadRegion.X = Chart.X;
    ThreadRegion.Y = ThreadStartY;
    ThreadRegion.W = Chart.W;

    r32 ThreadRegionHeight = LaneWidth + ThreadBreak;
    if(DebugState->ThreadSelected && (ThreadIndex == DebugState->SelectedThreadIndex))
    {
      ThreadRegionHeight = LaneWidth*MaxLaneCountPerThread;
    }

    ThreadRegion.H = ThreadRegionHeight;
    ThreadStartY += ThreadRegionHeight;
    if(!HotThread && Intersects(ThreadRegion, MousePos))
    {
      HotThread = Thread;
    }

    if(ThreadIndex < ThreadCount)
    {
      rect2f ThreadBreakRect = {};
      ThreadBreakRect.X = Chart.X;
      ThreadBreakRect.Y = ThreadStartY - ThreadBreak * 0.5f;
      ThreadBreakRect.W = Chart.W;
      ThreadBreakRect.H = ThreadBreak;
      PushOverlayQuad(ThreadBreakRect, V4(0,0,1,1));
    }
  }

  // Click-logic!
  if(Interface->MouseLeftButton.Edge && Interface->MouseLeftButton.Active && Intersects(Chart,MousePos))
  {
    // We clicked inside a thread-region
    if(HotThread)
    {
      // We klicked inside an already active thread-region
      if(DebugState->ThreadSelected && DebugState->SelectedThreadIndex == HotThread->LaneIndex)
      {
        // We klicked on a block inside an already active thread-region
        if(HotBlock)
        {
          // We klicked on an already selected block inside an already active thread-region
          if(HotThread->SelectedBlock == HotBlock)
          {
            // We set the selected block to it's parent
            HotThread->SelectedBlock = HotThread->SelectedBlock->Parent;
          }else{
            // We set the threads selected block
            HotThread->SelectedBlock = HotBlock;  
          }

        // We did not click on a block inside the already selected thread-region
        // And no block was selected
        }else if(!HotThread->SelectedBlock)
        {
          // Collapse the selected thread region
          DebugState->ThreadSelected = false;

        // We did not click on a block inside the already selected thread-region
        // But a block was selected
        }else if(HotThread->SelectedBlock)
        {
          // Reset the selected block
          HotThread->SelectedBlock = 0;
        }

      // We klicked inside a thread-region but it's not an already selected region
      }else{
        // Another region was already selected
        if(DebugState->ThreadSelected)
        {
          // Set it's selected block to 0
          debug_thread* PreviouslySelectedThread = Frame->Threads + DebugState->SelectedThreadIndex;
          PreviouslySelectedThread->SelectedBlock = 0;
        }
        DebugState->ThreadSelected = true;
      }
      DebugState->SelectedThreadIndex = HotThread->LaneIndex;

      //Frame->Threads[SelectedThreadIndex].SelectedBlock = SelectedBlock;
    }else{
      DebugState->ThreadSelected = false;
    }
  }
}


MENU_DRAW(DrawFunctionTimeline)
{
  rect2f Chart = Node->Region;

  r32 dt = GlobalGameState->Input->dt;

  r32 FrameTargetHeight = Chart.H * 0.7f;
  r32 HeightScaling = FrameTargetHeight/dt;

  debug_state* DebugState = DEBUGGetState();
  u32 MaxFramesToDisplay = ArrayCount(DebugState->Frames)-1;

  game_window_size WindowSize = GameGetWindowSize();
  r32 PixelSize = 1.f / WindowSize.HeightPx;

  r32 BarGap = PixelSize;

  v4 RedColor = V4(1,0,0,1);
  r32 FullRed = 1.2;
  r32 FullBlue = 1;
  v4 BlueColor =  V4(0,0,1,1);

  r32 MouseX = Interface->MousePos.X;
  r32 MouseY = Interface->MousePos.Y;

  u32 FrameCount =  ArrayCount(DebugState->Frames);
  u32 Count = 0;
  debug_frame* Frame = DebugState->Frames + DebugState->CurrentFrameIndex+1;
  debug_frame* SelectedFrame = 0;
  debug_thread* SelectedThread = 0;
  r32 BarWidth = Chart.W/(MaxFramesToDisplay);
  for(u32 BarIndex = 0; BarIndex < MaxFramesToDisplay; ++BarIndex)
  {
    u32 FrameIndex = (u32) (Frame-DebugState->Frames);
    if(FrameIndex >= FrameCount)
    {
      Frame = DebugState->Frames; 
    }
    
    debug_block* Block = Frame->Threads[0].FirstBlock;

    r32 FrameY = Chart.Y;

    r32 FrameHitRatio = Frame->WallSecondsElapsed / dt;
    r32 FrameCycleScaling = FrameTargetHeight * FrameHitRatio/(Frame->EndClock - Frame->BeginClock);

    r32 FrameX = Chart.X + (r32)BarIndex*BarWidth;

    if(DebugState->SelectedFrame && DebugState->SelectedFrame == Frame)
    {
      u64 LaneCycleCount = GetBlockChainCycleCount(Block);
      rect2f Rect = {};
      Rect.X = FrameX - 0.5f*PixelSize;
      Rect.Y = FrameY - 0.5f*PixelSize;
      Rect.W = BarWidth + PixelSize;
      Rect.H = FrameCycleScaling*LaneCycleCount + 2*PixelSize;
      PushOverlayQuad(Rect, V4(1,1,0,1));
    }

    while(Block)
    {
      rect2f Rect = {};
      Rect.X = FrameX + BarGap * 0.5f;
      Rect.Y = FrameY+FrameCycleScaling*Block->BeginClock;
      Rect.W = BarWidth - BarGap;
      Rect.H = FrameCycleScaling*(Block->EndClock - Block->BeginClock);

      v4 Color = GetColorForRecord(Block->Record);
      PushOverlayQuad(Rect, Color);
      
      if(Intersects(Rect, V2(MouseX,MouseY)))
      {
        c8 StringBuffer[2048] = {};
        Platform.DEBUGFormatString( StringBuffer, sizeof(StringBuffer), sizeof(StringBuffer),
        "Frame %d: %2.2f Sec (%s)", FrameIndex, Frame->WallSecondsElapsed, Block->Record->BlockName);

        PushTextAt(MouseX, MouseY+0.02f, StringBuffer, 24, V4(1,1,1,1));
      }
      Block = Block->Next;
    }  

    rect2f FrameRect = {};
    FrameRect.X = FrameX;
    FrameRect.Y = FrameY;
    FrameRect.W = BarWidth;
    FrameRect.H = Frame->WallSecondsElapsed * HeightScaling;

    if(Intersects(FrameRect, V2(MouseX,MouseY)))
    {
      SelectedFrame = Frame;
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
  Rect.Y = Chart.Y + FrameTargetHeight - 0.001f*0.5f;
  Rect.W = Chart.W;
  Rect.H = 0.001;
  PushOverlayQuad(Rect, V4(0,0,0,1));
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
