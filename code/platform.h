#pragma once

#include "debug_config.h"

//  NOTE: Stuff that gets refferenced in the platform layer as well as
//      the game layer

/*
  NOTE:

  HANDMADE_INTERNAL
    0: build for public release
    1: build for developer only

  HANDMADE_SLOW
    0: no slow code allowed
    1: slow code welcome
*/

// Note(Jakob): Compilers

#ifndef COMPILER_MSVC
#define COMPILER_MSVC 0
#endif

#ifndef COMPILER_LLVM
#define COMPILER_LLVM 0
#endif

#if !COMPILER_MSVC && !COMPILER_LLVM

#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#include <intrin.h>
#else
// Todo(Jakob): More compilers
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#endif

#endif

#include <stddef.h> // size_t exists in this header on some platforms
#include "types.h"
#include "intrinsics.h"
#include "platform_opengl.h"


#if COMPILER_MSVC
inline void* AtomicExchangePointer(void** volatile Target, void* New)
{
  void* Result = _InterlockedExchangePointer(Target,New);
  return Result;
}
inline u32 AtomicCompareExchange(u32 volatile* Value, u32 New, u32 Expected)
{
  u32 Result = _InterlockedCompareExchange((long volatile *)Value, New, Expected);
  return(Result);
}
inline u32 AtomicExchangeu32( u32 volatile* Value, u32 New)
{
  u32 Result = _InterlockedExchange( (long volatile *)Value, New);
  return(Result);
}
inline u32 AtomicIncrementu32( u32 volatile* Value )
{
  u32 Result = _InterlockedIncrement( (long volatile *)Value);
  return(Result);
}
inline u64 AtomicExchangeu64( u64 volatile* Value, u64 New)
{
  u64 Result = _InterlockedExchange64( (__int64 volatile *)Value, New);
  return(Result);
}
inline u64 AtomicAddu64( u64 volatile* Value, u64 Added)
{
  u64 Result = _InterlockedExchangeAdd64( (__int64 volatile *)Value, Added);
  return(Result);
}
inline u32 AtomicAddu32( u32 volatile* Value, u32 Added)
{
  u32 Result = _InterlockedExchangeAdd( (long volatile *)Value, Added);
  return(Result);
}
#elif COMPILER_LLVM

#endif

struct ticket_mutex
{
  volatile u64 Ticket;
  volatile u64 Serving;
};

struct thread_context
{
  s32 placeholder;
};

#if HANDMADE_INTERNAL

struct debug_read_file_result
{
  u32 ContentSize;
  void* Contents;
};

struct debug_process_state
{
  b32 StartedSuccessfully;
  b32 IsRunning;
  s32 ReturnCode;
};

struct debug_executing_process
{
  u64 OSHandle;
};

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name( thread_context* Thread, void* Memory )
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY( debug_platfrom_free_file_memory );

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name( thread_context* Thread, c8* Filename )
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE( debug_platform_read_entire_file );

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) b32 name( thread_context* Thread, c8* Filename, u32 MemorySize, void* Memory )
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE( debug_platform_write_entire_file );

#define DEBUG_PLATFORM_APPEND_TO_FILE(name) b32 name( thread_context* Thread, c8* Filename, u32 MemorySize, void* Memory )
typedef DEBUG_PLATFORM_APPEND_TO_FILE( debug_platform_append_to_file );

#define DEBUG_PLATFORM_EXECUTE_SYSTEM_COMMAND(name) debug_executing_process name(c8* Path, c8* Command, c8* CommandLine)
typedef DEBUG_PLATFORM_EXECUTE_SYSTEM_COMMAND( debug_platform_execute_system_command );
 
#define DEBUG_PLATFORM_GET_PROCESS_STATE(name) debug_process_state name(debug_executing_process Process)
typedef DEBUG_PLATFORM_GET_PROCESS_STATE( debug_platform_get_process_state );

#define DEBUG_PLATFORM_FORMAT_STRING(name) u32 name(c8* Buffer, midx SizeOfBuffer, midx CharCount, const c8* FormatString, ... )
typedef DEBUG_PLATFORM_FORMAT_STRING(debug_platform_format_string);

#define DEBUG_PLATFORM_PRINT(name) void name(const c8* DebugString, ...)
typedef DEBUG_PLATFORM_PRINT(debug_platform_print);


#endif // HANDMADE_INTERNAL


inline u32
SafeTruncateUInt64( u64 Value )
{
  Assert(Value <= 0xFFFFFFFF);
  u32 Result = (u32)Value;
  return Result;
}

inline r32
SafeTruncateReal32( u32 Value )
{
  Assert(Value <= 0xFFFFFFFF);
  Assert(Value >= 0xFFFFFFFF);
  r32 Result = (r32) Value;
  return Result;
}


/*
  NOTE: Services that the game provides to the platform layer.
*/

struct debug_record
{
  char* FileName;
  char* BlockName;
  u32 LineNumber;
  u32 Reserved;
};

enum debug_event_type
{
  DebugEvent_FrameMarker,
  DebugEvent_BeginBlock,
  DebugEvent_EndBlock,
};

struct threadid_coreindex
{
  u16 ThreadID;
  u16 CoreIndex;
};

struct debug_event
{
  u64 Clock;
  union
  {
    threadid_coreindex TC;
    r32 SecondsElapsed;
  };
  u16 DebugRecordIndex;
  u8 TranslationUnit;
  u8 Type;
};

#define MAX_DEBUG_EVENT_ARRAY_COUNT 8    // How many frames we are tracking
#define MAX_DEBUG_TRANSLATION_UNITS (2)  // How many translation units we have
#define MAX_DEBUG_EVENT_COUNT (4*65536)  // 
#define MAX_DEBUG_RECORD_COUNT (65536)

struct debug_table
{
  // TODO: Were never ensure that writes to the debugevents is complete
  //       before we swap them
  u64 volatile EventArrayIndex_EventIndex;    // Two numbers stored in one u64.
                                              // The high bits "EventArrayIndex" indexes into a frame [0, MAX_DEBUG_EVENT_ARRAY_COUNT]
                                              // The low bits "EventIndex" says how many events were recorded in that frame, [0,MAX_DEBUG_EVENT_COUNT]
  u32 CurrentEventArrayIndex;                 // Keep track on which event array we are writing to.

  // These are tracked on a per-translation-unit basis
  u32 RecordCount[MAX_DEBUG_TRANSLATION_UNITS];                              // How tracked functions records exist per translation unit
  debug_record Records[MAX_DEBUG_TRANSLATION_UNITS][MAX_DEBUG_RECORD_COUNT]; // Each tracked function or block has an entry here

  // These are tracked on a per-frame basis
  u32 EventCount[MAX_DEBUG_EVENT_ARRAY_COUNT];                               // How many events exists per frame
  debug_event Events[MAX_DEBUG_EVENT_ARRAY_COUNT][MAX_DEBUG_EVENT_COUNT];    // Here are actual instances of debug-functions.
};

extern debug_table* GlobalDebugTable;

#if HANDMADE_PROFILE

#define RecordDebugEventCommon(RecordIndex, EventType) \
  u64 ArrayIndex_EventIndex = AtomicAddu64(&GlobalDebugTable->EventArrayIndex_EventIndex, 1); \
  u32 EventIndex = ArrayIndex_EventIndex & 0xFFFFFFFF; \
  u32 ArrayIndex = ArrayIndex_EventIndex >> 32; \
  debug_event* Event = GlobalDebugTable->Events[ArrayIndex] + EventIndex; \
  Event->Clock = __rdtsc(); \
  Event->DebugRecordIndex = (u16) RecordIndex; \
  Event->TranslationUnit = TRANSLATION_UNIT_INDEX; \
  Event->Type = (u8) EventType;

#define RecordDebugEvent( RecordIndex, EventType) \
{\
  RecordDebugEventCommon( RecordIndex, EventType); \
  Event->TC.ThreadID = (u16)GetThreadID(); \
  Event->TC.CoreIndex = 0; \
}

#define FRAME_MARKER(SecondsElapsed) \
{ \
  u32 RecordIndex = __COUNTER__; \
  RecordDebugEventCommon(RecordIndex, DebugEvent_FrameMarker);\
  Event->SecondsElapsed = SecondsElapsed; \
  debug_record* Record = GlobalDebugTable->Records[TRANSLATION_UNIT_INDEX] + RecordIndex; \
  Record->FileName = __FILE__; \
  Record->BlockName = "Frame Marker"; \
  Record->LineNumber = __LINE__; \
}


#define TIMED_BLOCK__(BlockName, Number, ... ) timed_block TimedBlock_##Number(__COUNTER__, __FILE__, __LINE__, BlockName, ## __VA_ARGS__)
#define TIMED_BLOCK_(BlockName, Number, ... ) TIMED_BLOCK__(BlockName, Number, ## __VA_ARGS__)
// Used to time a {CodeBlock} with a specific name
#define TIMED_BLOCK(BlockName, ...) TIMED_BLOCK_(#BlockName, __LINE__, ## __VA_ARGS__)
// Used same as TIMED_BLOCK but automatically gives the function name.
#define TIMED_FUNCTION() TIMED_BLOCK_(__FUNCTION__, __LINE__, ## __VA_ARGS__)


#define BEGIN_BLOCK_(RecordIndex, FileNameArg, LineNumberArg, BlockNameArg) \
{\
  debug_record* Record = GlobalDebugTable->Records[TRANSLATION_UNIT_INDEX] + RecordIndex; \
  Record->FileName = (char*) FileNameArg; \
  Record->BlockName = (char*) BlockNameArg; \
  Record->LineNumber = LineNumberArg; \
  RecordDebugEvent(RecordIndex, DebugEvent_BeginBlock); \
}

#define END_BLOCK_(RecordIndex) \
  RecordDebugEvent(RecordIndex, DebugEvent_EndBlock);

#define BEGIN_BLOCK(Name) \
  int RecordIndex_##Name = __COUNTER__; \
  BEGIN_BLOCK_(RecordIndex_##Name, __FILE__, __LINE__, #Name);

#define END_BLOCK(Name) \
  END_BLOCK_(RecordIndex_##Name);

struct timed_block
{
  u32 RecordIndex;
  timed_block(const u32 RecordIndexInit, const char* FileName, const u32 LineNumber, const char* BlockName)
  : RecordIndex(RecordIndexInit)
  {
    BEGIN_BLOCK_(RecordIndex, FileName, LineNumber, BlockName);
  };

  ~timed_block()
  {
    END_BLOCK_(RecordIndex);
  };
};


#else
#define BEGIN_BLOCK(Name)
#define END_BLOCK(Name)
#define TIMED_BLOCK(BlockName, ...)
#define TIMED_FUNCTION()
#define FRAME_MARKER(SecondsElapsed)

#endif


inline void
BeginTicketMutex(ticket_mutex* Mutex)
{
  TIMED_FUNCTION();
  u64 Ticket = AtomicAddu64(&Mutex->Ticket, 1);
  while(Ticket != Mutex->Serving) {_mm_pause();}
}

inline void
EndTicketMutex(ticket_mutex* Mutex)
{
  AtomicAddu64(&Mutex->Serving, 1);
}

struct game_render_commands
{
  s32 ScreenWidthPixels;
  s32 ScreenHeightPixels;

  open_gl OpenGL;

  struct game_asset_manager* AssetManager;

  // 3D Lights Render Group
  struct render_group* LightsGroup;

  // 3D Scene Render Group
  struct render_group* RenderGroup;

  // 2D Overlay Render Group
  struct render_group* OverlayGroup;
};

struct game_sound_output_buffer
{
  s32 SamplesPerSecond;
  s32 SampleCount;
  s32 channels;
  s16* Samples;
};

struct game_button_state
{
  s32 HalfTransitionCount;
  b32 EndedDown;
  b32 Pushed;
  b32 Released;
};

struct keyboard_input
{
  union
  {
    game_button_state Keys[105];
    struct
    {
      game_button_state Key_BACK;
      game_button_state Key_TAB;
      game_button_state Key_SHIFT;
      game_button_state Key_CTRL;
      game_button_state Key_CLR;
      game_button_state Key_ALT;
      game_button_state Key_ENTER;
      game_button_state Key_PAUSE;
      game_button_state Key_CPSLCK;
      game_button_state Key_ESCAPE;
      game_button_state Key_SPACE;
      game_button_state Key_PGUP;
      game_button_state Key_PDWN;
      game_button_state Key_END;
      game_button_state Key_HOME;
      game_button_state Key_LEFT;
      game_button_state Key_UP;
      game_button_state Key_RIGHT;
      game_button_state Key_DOWN;
      game_button_state Key_PRTSC;
      game_button_state Key_INS;
      game_button_state Key_DEL;
      game_button_state Key_0;
      game_button_state Key_1;
      game_button_state Key_2;
      game_button_state Key_3;
      game_button_state Key_4;
      game_button_state Key_5;
      game_button_state Key_6;
      game_button_state Key_7;
      game_button_state Key_8;
      game_button_state Key_9;
      game_button_state Key_A;
      game_button_state Key_B;
      game_button_state Key_C;
      game_button_state Key_D;
      game_button_state Key_E;
      game_button_state Key_F;
      game_button_state Key_G;
      game_button_state Key_H;
      game_button_state Key_I;
      game_button_state Key_J;
      game_button_state Key_K;
      game_button_state Key_L;
      game_button_state Key_M;
      game_button_state Key_N;
      game_button_state Key_O;
      game_button_state Key_P;
      game_button_state Key_Q;
      game_button_state Key_R;
      game_button_state Key_S;
      game_button_state Key_T;
      game_button_state Key_U;
      game_button_state Key_V;
      game_button_state Key_W;
      game_button_state Key_X;
      game_button_state Key_Y;
      game_button_state Key_Z;
      game_button_state Key_LWIN;
      game_button_state Key_RWIN;
      game_button_state Key_NP_0;
      game_button_state Key_NP_1;
      game_button_state Key_NP_2;
      game_button_state Key_NP_3;
      game_button_state Key_NP_4;
      game_button_state Key_NP_5;
      game_button_state Key_NP_6;
      game_button_state Key_NP_7;
      game_button_state Key_NP_8;
      game_button_state Key_NP_9;
      game_button_state Key_NP_STAR;
      game_button_state Key_NP_PLUS;
      game_button_state Key_NP_DASH;
      game_button_state Key_NP_DEL;
      game_button_state Key_NP_SLASH;
      game_button_state Key_F1;
      game_button_state Key_F2;
      game_button_state Key_F3;
      game_button_state Key_F4;
      game_button_state Key_F5;
      game_button_state Key_F6;
      game_button_state Key_F7;
      game_button_state Key_F8;
      game_button_state Key_F9;
      game_button_state Key_F10;
      game_button_state Key_F11;
      game_button_state Key_F12;
      game_button_state Key_NP_NMLK;
      game_button_state Key_SCRLK;
      game_button_state Key_LSHIFT;
      game_button_state Key_RSHIFT;
      game_button_state Key_LCTRL;
      game_button_state Key_RCTRL;
      game_button_state Key_COLON;
      game_button_state Key_EQUAL;
      game_button_state Key_COMMA;
      game_button_state Key_DASH;
      game_button_state Key_DOT;
      game_button_state Key_FSLASH;
      game_button_state Key_TILDE;
      game_button_state Key_LBRACKET;
      game_button_state Key_RBSLASH;
      game_button_state Key_RBRACKET;
      game_button_state Key_QUOTE;
      game_button_state Key_LBSLASH;
    };
  };
};

struct game_controller_input
{
  b32 IsAnalog;
  b32 IsConnected;

  union
  {
    r32 Averages[6];
    struct
    {
      r32 LeftStickAverageX;
      r32 LeftStickAverageY;
      r32 RightStickAverageX;
      r32 RightStickAverageY;
      r32 LeftTriggerAverage;
      r32 RightTriggerAverage;
    };
  };

  union
  {
    game_button_state Button[24];
    struct
    {
      game_button_state DPadUp;
      game_button_state DPadDown;
      game_button_state DPadLeft;
      game_button_state DPadRight;

      game_button_state Start;
      game_button_state Select;

      game_button_state LeftShoulder;
      game_button_state RightShoulder;

      game_button_state LeftTrigger;
      game_button_state RightTrigger;

      game_button_state LeftStick;
      game_button_state LeftStickUp;
      game_button_state LeftStickDown;
      game_button_state LeftStickLeft;
      game_button_state LeftStickRight;

      game_button_state RightStick;
      game_button_state RightStickUp;
      game_button_state RightStickDown;
      game_button_state RightStickLeft;
      game_button_state RightStickRight;

      game_button_state A;
      game_button_state B;
      game_button_state X;
      game_button_state Y;


      // Note: Fake Button, All new buttons must be added above this one
      game_button_state Terminator;
    };
  };
};

enum mouse_button{
  PlatformMouseButton_Left,
  PlatformMouseButton_Middle,
  PlatformMouseButton_Right,
  PlatformMouseButton_ExtraOne,
  PlatformMouseButton_ExtraTwo,
  PlatformMouseButton_Count
};

enum keyboard_button
{
  KeyboardButton_BACK,
  KeyboardButton_TAB,
  KeyboardButton_SHIFT,
  KeyboardButton_CTRL,
  KeyboardButton_CLR,
  KeyboardButton_ALT,
  KeyboardButton_ENTER,
  KeyboardButton_PAUSE,
  KeyboardButton_CPSLCK,
  KeyboardButton_ESCAPE,
  KeyboardButton_SPACE,
  KeyboardButton_PGUP,
  KeyboardButton_PDWN,
  KeyboardButton_END,
  KeyboardButton_HOME,
  KeyboardButton_LEFT,
  KeyboardButton_UP,
  KeyboardButton_RIGHT,
  KeyboardButton_DOWN,
  KeyboardButton_PRTSC,
  KeyboardButton_INS,
  KeyboardButton_DEL,
  KeyboardButton_0,
  KeyboardButton_1,
  KeyboardButton_2,
  KeyboardButton_3,
  KeyboardButton_4,
  KeyboardButton_5,
  KeyboardButton_6,
  KeyboardButton_7,
  KeyboardButton_8,
  KeyboardButton_9,
  KeyboardButton_A,
  KeyboardButton_B,
  KeyboardButton_C,
  KeyboardButton_D,
  KeyboardButton_E,
  KeyboardButton_F,
  KeyboardButton_G,
  KeyboardButton_H,
  KeyboardButton_I,
  KeyboardButton_J,
  KeyboardButton_K,
  KeyboardButton_L,
  KeyboardButton_M,
  KeyboardButton_N,
  KeyboardButton_O,
  KeyboardButton_P,
  KeyboardButton_Q,
  KeyboardButton_R,
  KeyboardButton_S,
  KeyboardButton_T,
  KeyboardButton_U,
  KeyboardButton_V,
  KeyboardButton_W,
  KeyboardButton_X,
  KeyboardButton_Y,
  KeyboardButton_Z,
  KeyboardButton_LWIN,
  KeyboardButton_RWIN,
  KeyboardButton_NP_0,
  KeyboardButton_NP_1,
  KeyboardButton_NP_2,
  KeyboardButton_NP_3,
  KeyboardButton_NP_4,
  KeyboardButton_NP_5,
  KeyboardButton_NP_6,
  KeyboardButton_NP_7,
  KeyboardButton_NP_8,
  KeyboardButton_NP_9,
  KeyboardButton_NP_STAR,
  KeyboardButton_NP_PLUS,
  KeyboardButton_NP_DASH,
  KeyboardButton_NP_DEL,
  KeyboardButton_NP_SLASH,
  KeyboardButton_F1,
  KeyboardButton_F2,
  KeyboardButton_F3,
  KeyboardButton_F4,
  KeyboardButton_F5,
  KeyboardButton_F6,
  KeyboardButton_F7,
  KeyboardButton_F8,
  KeyboardButton_F9,
  KeyboardButton_F10,
  KeyboardButton_F11,
  KeyboardButton_F12,
  KeyboardButton_NP_NMLK,
  KeyboardButton_SCRLK,
  KeyboardButton_LSHIFT,
  KeyboardButton_RSHIFT,
  KeyboardButton_LCTRL,
  KeyboardButton_RCTRL,
  KeyboardButton_COLON,
  KeyboardButton_EQUAL,
  KeyboardButton_COMMA,
  KeyboardButton_DASH,
  KeyboardButton_DOT,
  KeyboardButton_FSLASH,
  KeyboardButton_TILDE,
  KeyboardButton_LBRACKET,
  KeyboardButton_RBSLASH,
  KeyboardButton_RBRACKET,
  KeyboardButton_QUOTE,
  KeyboardButton_LBSLASH,
  KeyboardButton_COUNT
};


struct game_input
{
  // GameUpdateTime
  r32 dt;
  b32 ExecutableReloaded;

  game_button_state MouseButton[5];
  r32 MouseDX, MouseDY, MouseDZ;
  r32 MouseX, MouseY, MouseZ;

  keyboard_input Keyboard;
  game_controller_input Controllers[4];
};

inline game_controller_input* GetController(game_input* Input, s32 ControllerIndex)
{
  Assert( ControllerIndex < ArrayCount( Input->Controllers ) );
  game_controller_input* Result = &Input->Controllers[ControllerIndex];
  return Result;
}

struct platform_file_handle
{
  b32 NoErrors;
  void *Platform;
};
/*
struct platform_file_info
{
    platform_file_info *Next;
    u64 FileDate; // NOTE(casey): This is a 64-bit number that _means_ the date to the platform, but doesn't have to be understood by the app as any particular date.
    u64 FileSize;
    char *BaseName; // NOTE(casey): Doesn't include a path or an extension
    void *Platform;
};
struct platform_file_group
{
    u32 FileCount;
    platform_file_info *FirstFileInfo;
    void *Platform;
};
enum platform_file_type
{
    PlatformFileType_AssetFile,
    PlatformFileType_SavedGameFile,
    PlatformFileType_PNG,
    PlatformFileType_WAV,

    PlatformFileType_Count,
};
*/

enum platform_memory_block_flags
{
  PlatformMemory_NotRestored = 0x1,
  PlatformMemory_OverflowCheck = 0x2,
  PlatformMemory_UnderflowCheck = 0x4,
};

/*
 *  Platform Memory Block (PMB)
 *       01234567
 *  Base -> |xxxxxxxx|   0
 *      |xxxxxxxx|   1
 *      |xxxxxxxx|   2
 *      ...
 *      |xxxxxxxx|   Used-1
 *  Used->  |--------|   Used
 *      ...
 *      |--------|   Size-2
 *      |--------|   Size-1
 *      |--------|   Size
 */
struct platform_memory_block
{
  u64 Flags;
  u64 Size;
  u8* Base;           // Pointer to the beginning of the memory block
  uintptr_t Used;           // Pointer to the end of the used data
  platform_memory_block *ArenaPrev;
};

enum platform_open_file_mode_flags
{
  OpenFile_Read = 0x1,
  OpenFile_Write = 0x2,
};

//#define PLATFORM_OPEN_FILE(name) platform_file_handle name(platform_file_group *FileGroup, platform_file_info *Info, u32 ModeFlags)
//typedef PLATFORM_OPEN_FILE(platform_open_file);
//
//#define PLATFORM_READ_DATA_FROM_FILE(name) void name(platform_file_handle *Handle, u64 Offset, u64 Size, void *Dest)
//typedef PLATFORM_READ_DATA_FROM_FILE(platform_read_data_from_file);
//
//#define PLATFORM_WRITE_DATA_TO_FILE(name) void name(platform_file_handle *Handle, u64 Offset, u64 Size, void *Source)
//typedef PLATFORM_WRITE_DATA_TO_FILE(platform_write_data_to_file);
//
//#define PLATFORM_FILE_ERROR(name) void name(platform_file_handle *Handle, char *Message)
//typedef PLATFORM_FILE_ERROR(platform_file_error);
//
//#define PLATFORM_CLOSE_FILE(name) void name(platform_file_handle *Handle)
//typedef PLATFORM_CLOSE_FILE(platform_close_file);

#define PlatformNoFileErrors(Handle) ((Handle)->NoErrors)

#define PLATFORM_ALLOCATE_MEMORY(name) platform_memory_block* name(memory_index aSize, u64 aFlags)
typedef PLATFORM_ALLOCATE_MEMORY(platform_allocate_memory);

#define PLATFORM_DEALLOCATE_MEMORY(name) void name(platform_memory_block *aBlock)
typedef PLATFORM_DEALLOCATE_MEMORY(platform_deallocate_memory);


struct platform_work_queue;

#define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(platform_work_queue* Queue, void* Data)
typedef PLATFORM_WORK_QUEUE_CALLBACK(platform_work_queue_callback);

typedef void platform_add_entry(platform_work_queue* Queue, platform_work_queue_callback* Callback, void* Data);
typedef void platform_complete_all_work(platform_work_queue* Queue);

struct platform_api
{
//    platform_open_file *OpenFile;
//    platform_read_data_from_file *ReadDataFromFile;
//    platform_write_data_to_file *WriteDataToFile;
//    platform_file_error *FileError;
//    platform_close_file *CloseFile;

    platform_allocate_memory*   AllocateMemory;
    platform_deallocate_memory* DeallocateMemory;

    platform_work_queue* HighPriorityQueue;

    platform_add_entry* PlatformAddEntry;
    platform_complete_all_work* PlatformCompleteWorkQueue;

//     {
      debug_platform_read_entire_file*       DEBUGPlatformReadEntireFile;
      debug_platfrom_free_file_memory*       DEBUGPlatformFreeFileMemory;
      debug_platform_write_entire_file*      DEBUGPlatformWriteEntireFile;
      debug_platform_append_to_file*         DEBUGPlatformAppendToFile;
      debug_platform_execute_system_command* DEBUGExecuteSystemCommand;
      debug_platform_get_process_state*      DEBUGGetProcessState;
      debug_platform_format_string*          DEBUGFormatString;
      debug_platform_print*                  DEBUGPrint;

//     }

//    debug_platform_get_memory_stats *DEBUGGetMemoryStats;

};

extern platform_api Platform;

struct debug_frame_end_info;

struct game_memory
{
  struct game_state* GameState;
  struct debug_state* DebugState;
  platform_api PlatformAPI;
  u32 ThreadID[4];
};


#define DEBUG_GAME_FRAME_END(name) debug_table* name(game_memory* Memory)
typedef DEBUG_GAME_FRAME_END(debug_frame_end);

#define GAME_UPDATE_AND_RENDER(name) void name(thread_context* Thread, game_memory* Memory, game_render_commands* RenderCommands, game_input* Input )
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

// Note: At the moment this has to be a very fast function it cannot be more than a millisecond or so.
// TODO: Reduce the pressure on this  function's preformance by measuring it  or asking about it, etc
#define GAME_GET_SOUND_SAMPLES(name) void name(thread_context* Thread, game_memory* Memory, game_sound_output_buffer* SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);
