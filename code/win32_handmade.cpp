/*
  TODO:
  - Saved game locations
  - Asset loading path
  - Raw Input (support for multiple keyboards)
  - Sleep/timeBeginPeriod
  - WM_SETCURSOR (control cursor visibility)
  - WM_ACTIVATEAPP (for when we are not the active application)
  - GetKeyboardLayout (for French keyboards, international WASD support)
  Just a partial list of stuff!!
*/

// Note(Jakob): If you have windows questions, look for answers given by Raymond Chen

#include "win32_handmade.h"
#include "win32_init_opengl.h"

platform_api Platform;

global_variable win32_state GlobalWin32State = {};
global_variable b32 GlobalRunning;
global_variable b32 GlobalPause;
//global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable game_render_commands* GlobalRenderCommands = {};
global_variable s64 GlobalPerfCounterFrequency;
global_variable b32 DEBUGGlobalShowCursor;
global_variable WINDOWPLACEMENT GlobalWindowPosition = {sizeof(GlobalWindowPosition)};
global_variable game_input GlobalInput[2] = {};

// Note:  We don't want to rely on the libraries needed to run the functions
//      XInputGetState and XInputSetState defined in <xinput.h> as they have
//      poor support on different windows versions. See handmadeHero ep 006;

// Note: XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name( DWORD dwUserIndex, XINPUT_STATE* pState )
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE( XInputGetStateStub )
{
  return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputSetState XInputSetState_


// Note: XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name( DWORD dwUserIndex, XINPUT_VIBRATION* pVibration )
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE( XInputSetStateStub )
{
  return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state* XInputSetState_ = XInputSetStateStub;
#define XInputGetState XInputGetState_


#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name( LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter )
typedef DIRECT_SOUND_CREATE(direct_sound_create );

#include "math/aabb.cpp"
#include "platform_opengl.cpp"

DEBUG_PLATFORM_EXECUTE_SYSTEM_COMMAND(DEBUGExecuteSystemCommand)
{
  debug_executing_process Result = {};
  
  STARTUPINFO StartupInfo = {};
  StartupInfo.cb = sizeof(StartupInfo);
  StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
  StartupInfo.wShowWindow = SW_HIDE;
  
  PROCESS_INFORMATION ProcessInfo = {};
  if(CreateProcess(Command,
                   CommandLine,
                   0,
                   0,
                   FALSE,
                   0,
                   0,
                   Path,
                   &StartupInfo,
                   &ProcessInfo))
  {
    Assert(sizeof(Result.OSHandle) >= sizeof(ProcessInfo.hProcess));
    *(HANDLE *)&Result.OSHandle = ProcessInfo.hProcess;
  }
  else
  {
    DWORD ErrorCode = GetLastError();
    *(HANDLE *)&Result.OSHandle = INVALID_HANDLE_VALUE;
  }

  return Result;
}

#include <stdarg.h>
DEBUG_PLATFORM_FORMAT_STRING(DEBUGFormatString)
{
  va_list args;
  va_start(args, FormatString);
  u32 Result = _vsnprintf_s(
    Buffer,
    SizeOfBuffer,
    CharCount,
    FormatString,
    args);
  va_end(args);
  return Result;
}

DEBUG_PLATFORM_PRINT(DEBUGPrint)
{
  va_list args;
  c8 Buffer[2048] = {};
  va_start(args, DebugString);
  u32 Result = _vsnprintf_s(
    Buffer,
    sizeof(Buffer),
    _TRUNCATE,
    DebugString,
    args);
  va_end(args);
  OutputDebugStringA(Buffer);
}

DEBUG_PLATFORM_GET_PROCESS_STATE(DEBUGGetProcessState)
{
  debug_process_state Result = {};

  HANDLE hProcess = *(HANDLE *)&Process.OSHandle;
  if(hProcess != INVALID_HANDLE_VALUE)
  {
    Result.StartedSuccessfully = true;
    if(WaitForSingleObject(hProcess, 0) == WAIT_OBJECT_0 )
    {
      DWORD ReturnCode = 0;
      GetExitCodeProcess(hProcess, &ReturnCode);
      Result.ReturnCode = ReturnCode;
      CloseHandle(hProcess);
    }
    else
    {
      Result.IsRunning = true;
    }
  }

  return(Result);
}

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
  if(Memory)
  {
    VirtualFree(Memory, 0 ,MEM_RELEASE);
  }
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
  debug_read_file_result Result = {};
  HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ,
                  NULL, OPEN_EXISTING, NULL, NULL);

  if(FileHandle != INVALID_HANDLE_VALUE)
  {
    LARGE_INTEGER FileSize;
    if(GetFileSizeEx( FileHandle, &FileSize))
    {
      Result.Contents = VirtualAlloc(NULL, FileSize.QuadPart,
              MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
      if(Result.Contents)
      {
        // TODO: DEFINES FOR MAX VALUES

        DWORD BytesRead;
        Result.ContentSize = SafeTruncateUInt64(FileSize.QuadPart);
        if(ReadFile( FileHandle, Result.Contents, Result.ContentSize, &BytesRead,0) &&
          (Result.ContentSize == BytesRead))
        {
          // Note: File read successfully
        }else{

          // TODO: Logging
          DEBUGPlatformFreeFileMemory(Thread, Result.Contents);
          Result.Contents = NULL;
        }

      }else{
        // TODO: Logging
      }
    }else{
      // TODO: Logging
    }

    CloseHandle( FileHandle);
  }else{
    // TODO: Logging
  }

  return Result;
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile)
{
  bool Result = 0;
  HANDLE FileHandle = CreateFileA(Filename,
                                  GENERIC_WRITE,
                                  NULL,
                                  NULL,
                                  CREATE_ALWAYS,
                                  NULL,
                                  NULL);

  if(FileHandle != INVALID_HANDLE_VALUE)
  {
    DWORD BytesWritten;
    if(WriteFile( FileHandle, Memory,  MemorySize, &BytesWritten, NULL))
    {
      Result = (BytesWritten  == MemorySize);
      // Note: File read successfully
    }else{
      // TODO: Logging
    }

    CloseHandle( FileHandle);
  }else{
    // TODO: Logging
  }

  return Result;
}

DEBUG_PLATFORM_APPEND_TO_FILE(DEBUGPlatformAppendToFile)
{
  bool Result = 0;
  HANDLE FileHandle = CreateFile(Filename, // open Two.txt
                        FILE_APPEND_DATA,         // open for writing
                        FILE_SHARE_READ,          // allow multiple readers
                        NULL,                     // no security
                        OPEN_ALWAYS,              // open or create
                        FILE_ATTRIBUTE_NORMAL,    // normal file
                        NULL);                    // no attr. template

  if(FileHandle != INVALID_HANDLE_VALUE)
  {
    DWORD BytesWritten, FilePosition;

    FilePosition = SetFilePointer(FileHandle, 0, NULL, FILE_END);
    LockFile(FileHandle, FilePosition, 0, MemorySize, 0);
    if(WriteFile( FileHandle, Memory, MemorySize, &BytesWritten, NULL))
    {
      Result = (BytesWritten  == MemorySize);
      // Note: File read successfully
    }else{
      // TODO: Logging
    }
    UnlockFile(FileHandle, FilePosition, 0, MemorySize, 0);
    CloseHandle(FileHandle);
  }else{
    // TODO: Logging
  }

  return Result;
}


internal void
Win32GetEXEFileName(win32_state* State)
{
  DWORD SizeofFilename = GetModuleFileName(0,State->EXEFileName, sizeof(State->EXEFileName));
  State->OnePastLastSlashEXEFileName  = State->EXEFileName;
  for(char* Scan  = State->EXEFileName; *Scan!='\0'; ++Scan)
  {
    if(*Scan == '\\')
    {
      State->OnePastLastSlashEXEFileName = Scan+1;
    }
  }

}


internal void
Win32BuildEXEPathFileName( win32_state* State,
               char* FileName, s32 DestCount, char* Dest )
{
  str::CatStrings( State->OnePastLastSlashEXEFileName - State->EXEFileName,
           State->EXEFileName,
           str::StringLength(FileName), FileName,
           DestCount, Dest);
}

inline FILETIME
Win32GetLastWriteTime(char* FileName)
{
  FILETIME LastWriteTime = {};
  WIN32_FILE_ATTRIBUTE_DATA Data;
  if(GetFileAttributesEx( FileName, GetFileExInfoStandard, &Data))
  {
    LastWriteTime = Data.ftLastWriteTime;
  }

  return LastWriteTime;
}

internal win32_game_code
Win32LoadGameCode(char* SourceDLLName, char* TempDLLName, char* LockFileName)
{
  win32_game_code Result = {};
  WIN32_FILE_ATTRIBUTE_DATA IgnoredResult;
  if(!GetFileAttributesEx(LockFileName, GetFileExInfoStandard, &IgnoredResult))
  {
    Result.LastDLLWriteTime = Win32GetLastWriteTime(SourceDLLName);
    CopyFile(SourceDLLName, TempDLLName, FALSE);
    Result.GameCodeDLL = LoadLibraryA(TempDLLName);
    if(Result.GameCodeDLL)
    {
      Result.UpdateAndRender = (game_update_and_render* )
        GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");

      Result.GetSoundSamples = (game_get_sound_samples* )
        GetProcAddress(Result.GameCodeDLL, "GameGetSoundSamples");

      Result.DEBUGGameFrameEnd = (debug_frame_end* )
        GetProcAddress(Result.GameCodeDLL, "DEBUGGameFrameEnd");

      Result.IsValid = (Result.UpdateAndRender && Result.GetSoundSamples);
    }
  }
  if(!Result.IsValid)
  {
    Result.UpdateAndRender = 0;
    Result.GetSoundSamples = 0;
    Result.DEBUGGameFrameEnd = 0;
  }

  return Result;
}

internal void
Win32UnloadGameCode(win32_game_code* aGameCode)
{
  if(aGameCode->GameCodeDLL)
  {
    FreeLibrary(aGameCode->GameCodeDLL);
    aGameCode->GameCodeDLL = 0;
  }

  aGameCode->IsValid = false;
  aGameCode->UpdateAndRender = 0;
  aGameCode->GetSoundSamples = 0;
  aGameCode->DEBUGGameFrameEnd = 0;

}

internal void
Win32LoadXInput(void)
{
  // Note:  There are three versions of XInput at the time of writing;
  //      XInput 1.4 is bundled with Windows 8,
  //      XInput 1.3 is bundled with Windows 7 and
  //      XInput 9.1.0 is a generalized version with some features added
  //      and some removed, which can be used across platforms.

  char *XInputDLLs[] = {"xinput1_4.dll","xinput1_3.dll","Xinput9_1_0.dll"};
  s32 XInputDLLCount = sizeof(XInputDLLs)/sizeof(XInputDLLs[0]);
  HMODULE XInputLibrary;
  for (s32 DLLIndex = 0; DLLIndex < XInputDLLCount; DLLIndex++)
  {
    XInputLibrary = LoadLibraryA(XInputDLLs[DLLIndex]);
    if (XInputLibrary)
    {
      XInputGetState = (x_input_get_state* )GetProcAddress(XInputLibrary, "XInputGetState");
      if(!XInputGetState)
      {
        XInputGetState = XInputGetStateStub;
      }

      XInputSetState = (x_input_set_state* )GetProcAddress(XInputLibrary,"XInputSetState");
      if(!XInputSetState)
      {
        XInputSetState = XInputSetStateStub;
      }

      // TODO: Diagnostic
      break;
    }else{
      // TODO: Diagnostic
    }

  }
}

internal void
Win32InitDSound( HWND aWindow, win32_sound_output* aSoundOutput)
{
  // Note: load the library
  HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
  if(DSoundLibrary)
  {
    // Note: Get a DirectSound object - cooperative mode
    direct_sound_create* DirectSoundCreate = (direct_sound_create* )
                GetProcAddress(DSoundLibrary, "DirectSoundCreate");

    // TODO Double-check that this works on XP - Directsound8 or 7?
    LPDIRECTSOUND DirectSound;
    if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)) )
    {
      WAVEFORMATEX WaveFormat = {};
      WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
      WaveFormat.nChannels = (WORD) aSoundOutput->ChannelCount;
      WaveFormat.nSamplesPerSec =(WORD) aSoundOutput->SamplesPerSecond;
      WaveFormat.wBitsPerSample =(WORD) aSoundOutput->BytesPerSample * 8; // cd-quality
      // EXP: Audio samples for 2 channels is stored as
      //    LEFT RIGHT LEFT RIGHT LEFT RIGHT etc.
      //    nBlockAlign wants the size of one [LEFT RIGHT]
      //    block in bytes. For us this becomes
      //    (2 channels) X (nr of bits per sample) / (bits in a byte) =
      //      2 * 16 /8 = 4 bytes per sample.
      WaveFormat.nBlockAlign =(WORD) (aSoundOutput->ChannelCount * aSoundOutput->BytesPerSample);
      WaveFormat.nAvgBytesPerSec =
        WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
      WaveFormat.cbSize = 0;

      if(SUCCEEDED(DirectSound->SetCooperativeLevel( aWindow, DSSCL_PRIORITY)) )
      {
        DSBUFFERDESC BufferDescription = {};
        BufferDescription.dwSize = sizeof(BufferDescription);
        BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

        // NOTE: "Create" a primary buffer
        // EXP:  The primary buffer is only for setting the format of our primary
        //     sound device to what we want. The secondary buffer is the actual
        //     buffer we will write.
        // TODO: DBCAPS_GLOBALFOCUS?
        LPDIRECTSOUNDBUFFER PrimaryBuffer;
        if(SUCCEEDED(DirectSound->CreateSoundBuffer
                      (&BufferDescription ,&PrimaryBuffer,0)))
        {
          HRESULT Error= PrimaryBuffer->SetFormat(&WaveFormat);
          if(SUCCEEDED(Error))
          {
            OutputDebugStringA("Primary buffer format was set.\n");
            // NOTE: We have finally set the format
          }else{
            // TODO: Diagnostics
          }
        }else{
          // TODO: diagnostics
        }
      }else{
        // TODO: diagnostics
      }


      // NOTE: "Create" a secondary buffer
      DSBUFFERDESC BufferDescription = {};
      BufferDescription.dwSize = sizeof(BufferDescription);
//      BufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
      BufferDescription.dwFlags = 0;
      BufferDescription.dwBufferBytes = aSoundOutput->BufferSizeInBytes;
      BufferDescription.lpwfxFormat = &WaveFormat;
      HRESULT Error =DirectSound->CreateSoundBuffer
                (&BufferDescription ,&aSoundOutput->SecondaryBuffer,0);
      if(SUCCEEDED(Error))
      {
        OutputDebugStringA("Secondary buffer created successfully.\n");
      }else{
        // TODO: Diagnostic
      }

    }else{
      // TODO: Diagnostic
    }
  }else{
    // TODO: Diagnostic
  }

}

internal win32_window_dimension
Win32GetWindowDimension( HWND Window )
{
  win32_window_dimension Result;

  RECT ClientRect;
  GetClientRect(Window, &ClientRect);
  Result.Width  = ClientRect.right - ClientRect.left;
  Result.Height = ClientRect.bottom - ClientRect.top;

  return Result;
}


internal LRESULT CALLBACK
MainWindowCallback( HWND Window,
          UINT Message,
          WPARAM WParam,
          LPARAM LParam)
{
  LRESULT Result = 0;
  switch(Message)
  {
    // User x-out
    case WM_CLOSE:
    {
      // TODO: Handle with message to user?
      GlobalRunning = false;
    }break;

    case WM_SETCURSOR:
    {
      if(DEBUGGlobalShowCursor)
      {
        Result = DefWindowProc(Window, Message, WParam, LParam);
      }else{
        SetCursor(0);
      }

    }break;

    // User has clicked to make us active app
    // Or user has deactivated this app by tabbing or whatever
    case WM_ACTIVATEAPP:
    {
#if 0
      if( WParam == TRUE)
      {
        SetLayeredWindowwgl_create_context_attrib_arbutes(Window, RGB(0,0,0), 255, LWA_ALPHA);
      }else{
        SetLayeredWindowAttributes(Window, RGB(0,0,0), 64, LWA_ALPHA);
      }
#endif
    }break;

    // Windows deletes window
    case WM_DESTROY:
    {
      // TODO: Handle as error - recreate window?
      GlobalRunning = false;
    }break;

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
      Assert(!"Kewyboard input came in through a non-dispatch message!");
    }break;

    case WM_PAINT:
    {

      PAINTSTRUCT Paint;
      HDC DeviceContext = BeginPaint(Window, &Paint);
      win32_window_dimension Dimension = Win32GetWindowDimension(Window);
      GlobalRenderCommands->ScreenWidthPixels = Dimension.Width;
      GlobalRenderCommands->ScreenHeightPixels = Dimension.Height;
      EndPaint(Window, &Paint);
    }break;

    default:
    {
      //OutputDebugStringA("default\n");

      // Windows default window message handling.
      Result = DefWindowProc(Window, Message, WParam, LParam);
    }break;
  }

  return Result;
}


internal void
Win32ClearSoundBuffer(win32_sound_output* aSoundOutput)
{
  VOID* Region1;
  DWORD Region1Size;
  VOID* Region2;
  DWORD Region2Size;
  if( SUCCEEDED(aSoundOutput->SecondaryBuffer->Lock(0, aSoundOutput->BufferSizeInBytes,
                             &Region1, &Region1Size,
                             &Region2, &Region2Size,
                             0)))
  {
    u8* DestSample = ( u8* ) Region1;
    for(DWORD ByteIndex = 0;
      ByteIndex < Region1Size;
      ++ByteIndex)
    {
      *DestSample++ = 0;
    }

    DestSample = ( u8* ) Region2;
    for(DWORD ByteIndex = 0;
      ByteIndex < Region2Size;
      ++ByteIndex)
    {
      *DestSample++ = 0;
    }

    aSoundOutput->SecondaryBuffer->Unlock(Region1, Region1Size,
        Region2, Region2Size);
  }
};

internal void
Win32FillSoundBuffer(win32_sound_output* aSoundOutput, DWORD aByteToLock, DWORD aBytesToWrite, game_sound_output_buffer* aSourceBuffer)
{
  VOID* Region1;
  DWORD Region1Size;
  VOID* Region2;
  DWORD Region2Size;
  if( SUCCEEDED(aSoundOutput->SecondaryBuffer->Lock( aByteToLock, aBytesToWrite,
                             &Region1, &Region1Size,
                             &Region2, &Region2Size,
                             0)))
  {
    // TODO: assert that region1size / region2size is valid
    DWORD Region1SampleCount = Region1Size/aSoundOutput->BytesPerSampleTotal;
    s16* DestSample = ( s16* ) Region1;
    s16* SourceSample = aSourceBuffer->Samples;

    for(DWORD SampleIndex = 0;
      SampleIndex < Region1SampleCount;
      ++SampleIndex)
    {
      *DestSample++ = *SourceSample++; // Left Speker
      *DestSample++ = *SourceSample++; // Rright speaker
      aSoundOutput->RunningSampleIndex++;
    }

    DWORD Region2SampleCount = Region2Size/aSoundOutput->BytesPerSampleTotal;
    DestSample = (s16*) Region2;
    for(DWORD SampleIndex = 0;
      SampleIndex < Region2SampleCount;
      ++SampleIndex)
    {
      *DestSample++ = *SourceSample++;
      *DestSample++ = *SourceSample++;
      aSoundOutput->RunningSampleIndex++;
    }


    aSoundOutput->SecondaryBuffer->Unlock(Region1, Region1Size,Region2, Region2Size);
  }
}


internal void
Win32GetInputFileLocation(win32_state* aState, b32 aInputStream,
            s32 aSlotIndex, s32 aDestCount, char* aDest)
{
  char Temp[64];
  wsprintf(Temp, "loop_edit_%d_%s.dmi", aSlotIndex, aInputStream? "input" : "state" );
  Win32BuildEXEPathFileName(aState, Temp, aDestCount, aDest );
}

internal win32_replay_buffer*
Win32GetReplayBuffer(win32_state *aState, s32 Index)
{
  Assert(Index-1 < ArrayCount(aState->ReplayBuffer));
  Assert(Index-1 >= 0);
  win32_replay_buffer* Result = &aState->ReplayBuffer[Index-1];
  return Result;
}

internal void
Win32BeginRecordingInput(win32_state *aState, s32 aRecordingIndex)
{
    char FileName[WIN32_STATE_FILE_NAME_COUNT];
    Win32GetInputFileLocation(aState, true, aRecordingIndex, sizeof(FileName), FileName);
    aState->RecordingHandle = CreateFileA(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if(aState->RecordingHandle != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWritten;

        aState->RecordingIndex = aRecordingIndex;
        win32_memory_block* Sentinel = &aState->MemorySentinel;

        BeginTicketMutex(&GlobalWin32State.MemoryMutex);
        for(win32_memory_block *SourceBlock = Sentinel->Next;
            SourceBlock != Sentinel;
            SourceBlock = SourceBlock->Next)
        {
            if( !(SourceBlock->Block.Flags & PlatformMemory_NotRestored))
            {
                win32_saved_memory_block DestBlock;
                void* BasePointer = SourceBlock->Block.Base;
                DestBlock.BasePointer = (u64)BasePointer;
                DestBlock.Size = SourceBlock->Block.Size;
                WriteFile(aState->RecordingHandle, &DestBlock, sizeof(DestBlock), &BytesWritten, 0);
                Assert(DestBlock.Size <= U32Max);
                WriteFile(aState->RecordingHandle, BasePointer, (u32)DestBlock.Size, &BytesWritten, 0);
            }
        }
        EndTicketMutex(&GlobalWin32State.MemoryMutex);

        win32_saved_memory_block DestBlock = {};
        WriteFile(aState->RecordingHandle, &DestBlock, sizeof(DestBlock), &BytesWritten, 0);
    }
}

internal void
Win32EndRecordingInput(win32_state* aState)
{
  CloseHandle(aState->RecordingHandle);
  aState->RecordingIndex = 0;
}

internal void
Win32FreeMemoryBlock(win32_memory_block *aBlock)
{
    BeginTicketMutex(&GlobalWin32State.MemoryMutex);
    aBlock->Prev->Next = aBlock->Next;
    aBlock->Next->Prev = aBlock->Prev;
    EndTicketMutex(&GlobalWin32State.MemoryMutex);

    BOOL Result = VirtualFree(aBlock, 0, MEM_RELEASE);
    Assert(Result);
}

internal void
Win32ClearBlocksByMask(win32_state *aState, u64 aMask)
{
    for(win32_memory_block* BlockIter =   aState->MemorySentinel.Next;
                  BlockIter != &aState->MemorySentinel;)
    {
        win32_memory_block* Block = BlockIter;
        BlockIter = BlockIter->Next;

        if((Block->LoopingFlags & aMask) == aMask)
        {
            Win32FreeMemoryBlock(Block);
        }
        else
        {
            Block->LoopingFlags = 0;
        }
    }
}

internal void
Win32BeginInputPlayBack(win32_state *aState, s32 aPlayingIndex)
{
    Win32ClearBlocksByMask(aState, Win32Mem_AllocatedDuringLooping);

    char FileName[WIN32_STATE_FILE_NAME_COUNT];
    Win32GetInputFileLocation(aState, true, aPlayingIndex, sizeof(FileName), FileName);
    aState->PlaybackHandle = CreateFileA(FileName, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
    if(aState->PlaybackHandle != INVALID_HANDLE_VALUE)
    {
        aState->PlayingIndex = aPlayingIndex;

        for(;;)
        {
            win32_saved_memory_block Block = {};
            DWORD BytesRead;
            ReadFile(aState->PlaybackHandle, &Block, sizeof(Block), &BytesRead, 0);
            if(Block.BasePointer != 0)
            {
                void *BasePointer = (void *)Block.BasePointer;
                Assert(Block.Size <= U32Max);
                ReadFile(aState->PlaybackHandle, BasePointer, (u32)Block.Size, &BytesRead, 0);
            }
            else
            {
                break;
            }
        }
        // TODO(casey): Stream memory in from the file!
    }
}

internal void
Win32EndInputPlayback(win32_state* aState)
{
  Win32ClearBlocksByMask(aState, Win32Mem_FreedDuringLooping);
  CloseHandle(aState->PlaybackHandle);
  aState->PlayingIndex=0;
}

internal void
Win32RecordInput(win32_state* aState, game_input* NewInput)
{
  DWORD BytesWritten;
  WriteFile(aState->RecordingHandle, NewInput, sizeof(*NewInput), &BytesWritten, 0);
}

internal void
Win32PlayBackInput(win32_state*  aState, game_input* NewInput)
{
  DWORD BytesRead;
  if(ReadFile( aState->PlaybackHandle, NewInput, sizeof( *NewInput ), &BytesRead, 0 ) )
  {
    if(BytesRead==0)
    {
      s32 PlayingIndex =  aState->PlayingIndex;
      Win32EndInputPlayback( aState );
      Win32BeginInputPlayBack( aState, PlayingIndex );
      ReadFile( aState->PlaybackHandle, NewInput, sizeof( *NewInput ), &BytesRead, 0);
    }
  }
}

internal void
Win32ProcessXInputDigitalButton(DWORD aXInputButtonState, game_button_state* aOldState, DWORD aButtonBit, game_button_state* aNewState )
{
  aNewState->EndedDown = ((aXInputButtonState & aButtonBit) == aButtonBit);
  aNewState->HalfTransitionCount =
    (aOldState->EndedDown != aNewState->EndedDown) ? 1 : 0;

};

internal void
Win32ProcessKeyboardMessage( game_button_state* aNewState, b32 IsDown)
{
  aNewState->Pushed = false;
  aNewState->Released = false;
  if(aNewState->EndedDown != IsDown)
  {
    aNewState->EndedDown = IsDown;
    ++aNewState->HalfTransitionCount;
    if(IsDown)
    {
      aNewState->Pushed = true;
    }else{
      aNewState->Released = true;
    }
  }
  Assert( ( aNewState->Pushed   &&  aNewState->EndedDown)    || !aNewState->Pushed );
  Assert( ( aNewState->Released && !aNewState->EndedDown) || !aNewState->Released );
  Assert( ( aNewState->Pushed && !aNewState->Released) ||
          (!aNewState->Pushed &&  aNewState->Released) ||
          (!aNewState->Pushed && !aNewState->Released));
}

internal r32
Win32ProcessXinputStickValue(SHORT aValue, SHORT aDeadZoneThershold)
{
  r32 Result = 0;

  if(aValue < -aDeadZoneThershold)
  {
    Result = (r32)(aValue + aDeadZoneThershold) / (32768.0f - aDeadZoneThershold);
  }else if( aValue > aDeadZoneThershold){
    Result = (r32)(aValue - aDeadZoneThershold) / (32767.0f - aDeadZoneThershold);
  }
  return Result;
}

internal void
Win32ProcessXinputStickValue( r32* aX,
                r32* aY,
                SHORT Value_X,
                SHORT Value_Y,
                SHORT aDeadZoneThershold)
{
  r32 normalizer = 1 / 32768.0f;
  r32 normalized_deadzone= 1.f / ( (r32) aDeadZoneThershold );
  r32 normalized_deadzone_squared = normalized_deadzone*normalized_deadzone;

  r32 x_norm = Value_X * normalizer;
  r32 y_norm = Value_Y * normalizer;
  r32 stick_offset_squared = x_norm*x_norm + y_norm*y_norm;

  if(stick_offset_squared > normalized_deadzone_squared)
  {
    *aX = (r32)(Value_X) * normalizer;
    *aY = (r32)(Value_Y) * normalizer;
  }else{
    *aX = 0;
    *aY = 0;
  }
}

// Note(Jakob): Courtesy of Raymond Chen,
//        See: https://blogs.msdn.microsoft.com/oldnewthing/20100412-00/?p=14353
internal void
Win32ToggleFullscreen(HWND Window)
{
  DWORD Style = GetWindowLong(Window, GWL_STYLE);
    if (Style & WS_OVERLAPPEDWINDOW) {
      MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };
      if(GetWindowPlacement(Window, &GlobalWindowPosition) &&
         GetMonitorInfo(MonitorFromWindow(Window,MONITOR_DEFAULTTOPRIMARY), &MonitorInfo))
      {
        SetWindowLong(Window, GWL_STYLE, Style & ~WS_OVERLAPPEDWINDOW);
          SetWindowPos(Window, HWND_TOP,
                   MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
                   MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                   MonitorInfo.rcMonitor.bottom - MonitorInfo .rcMonitor.top,
                   SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
      }
    }else{
      SetWindowLong(Window, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
      SetWindowPlacement(Window, &GlobalWindowPosition);
      SetWindowPos(Window, NULL, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                 SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
  }
}

internal void
Win32ProcessKeyboard(win32_state* aState,  keyboard_input*  OldKeyboardController, keyboard_input* NewKeyboardController)
{
  *NewKeyboardController = {};
  Assert(ArrayCount(NewKeyboardController->Keys) == KeyboardButton_COUNT);
  for(s32 ButtonIndex = 0;
    ButtonIndex  < KeyboardButton_COUNT;
    ++ButtonIndex)
  {
    NewKeyboardController->Keys[ButtonIndex].EndedDown =
      OldKeyboardController->Keys[ButtonIndex].EndedDown;
  }

  MSG Message;
  while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
  {
    switch(Message.message)
    {
      case WM_QUIT:
      {
        GlobalRunning =  false;
      }break;

      case WM_SYSKEYDOWN:
      case WM_SYSKEYUP:
      case WM_KEYDOWN:
      case WM_KEYUP:
      {
        u32 VKCode = (u32)Message.wParam;
        #define KeyMessageWasDownBit (1<<30)
        #define KeyMessageIsDownBit (1<<31)
        bool WasDown = ((Message.lParam & KeyMessageWasDownBit) != 0);
        bool IsDown  = ((Message.lParam &  KeyMessageIsDownBit) == 0);

        b32 AltKeyWasDown   = ( Message.lParam & (1 << 29));
        b32 ShiftKeyWasDown = ( GetKeyState(VK_SHIFT) & (1 << 15));
        
        if(WasDown != IsDown)
        {
          switch(VKCode)
          {
            ////case VK_LBUTTON:    {}break; // 0x01 Left Button **
            ////case VK_RBUTTON:    {}break; // 0x02 Right Button **
            ////case VK_CANCEL:     {}break; // 0x03 Break
            ////case VK_MBUTTON:    {}break; // 0x04 Middle Button **
            ////case VK_XBUTTON1:   {}break; // 0x05 X Button 1 **
            ////case VK_XBUTTON2:   {}break; // 0x06 X Button 2 **
            case VK_BACK:       { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_BACK, IsDown); }break; // 0x08 Backspace
            case VK_TAB:        { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_TAB, IsDown); }break; // 0x09 Tab
            case 0x10:          { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_SHIFT, IsDown); }break; // 0x10 SHIFT
            case 0x11:          { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_CTRL, IsDown); }break; // 0x011 CTRL
            case 0x12:          { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_ALT, IsDown); }break; // 0x012 ALT
            case VK_CLEAR:      {Win32ProcessKeyboardMessage(&NewKeyboardController->Key_CLR, IsDown);}break; // 0x0C Clear
            case VK_RETURN:     { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_ENTER, IsDown); }break; // 0x0D Enter
            case VK_PAUSE:      { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_PAUSE, IsDown); }break; // 0x13 Pause
            case VK_CAPITAL:    { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_CPSLCK, IsDown); }break; // 0x14 Caps Lock
            ////case VK_KANA:       {}break; // 0x15 Kana
            ////case VK_JUNJA:      {}break; // 0x17 Junja
            ////case VK_FINAL:      {}break; // 0x18 Final
            ////case VK_KANJI:      {}break; // 0x19 Kanji
            case VK_ESCAPE:     { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_ESCAPE, IsDown); GlobalRunning = false; }break; // 0x1B Esc
            ////case VK_CONVERT:    {}break; // 0x1C Convert
            ////case VK_NONCONVERT: {}break; // 0x1D Non Convert
            ////case VK_ACCEPT:     {}break; // 0x1E Accept
            ////case VK_MODECHANGE: {}break; // 0x1F Mode Change
            case VK_SPACE:      { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_SPACE, IsDown);}break; // 0x20 Space
            case VK_PRIOR:      { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_PGUP, IsDown);}break;  // 0x21 Page Up
            case VK_NEXT:       { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_PDWN, IsDown);}break;  // 0x22 Page Down
            case VK_END:        { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_END, IsDown);}break;    //  0x23  End
            case VK_HOME:       { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_HOME, IsDown);}break;     // 0x24  Home
            case VK_LEFT:       { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_LEFT, IsDown);}break;     // 0x25  Arrow Left
            case VK_UP:         { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_UP, IsDown);}break;     // 0x26  Arrow Up
            case VK_RIGHT:      { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_RIGHT, IsDown);}break;    //  0x27  Arrow Right
            case VK_DOWN:       { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_DOWN, IsDown);}break;     // 0x28  Arrow Down
            ////case VK_SELECT:     {}break; // 0x29 Select
            ////case VK_PRINT:      {}break; // 0x2A Print
            ////case VK_EXECUTE:    {}break; // 0x2B Execute
            case VK_SNAPSHOT:   { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_PRTSC, IsDown);} break;     // 0x2C  Print Screen
            case VK_INSERT:     { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_INS, IsDown);} break;     // 0x2D  Insert
            case VK_DELETE:     { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_DEL, IsDown);} break;     // 0x2E  Delete
            ////case VK_HELP:       {}break; // 0x2F  Help
            case '0':     { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_0, IsDown);  } break; //  0x30 ('0')  0
            case '1':     { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_1, IsDown);  } break; //  0x31 ('1')  1
            case '2':     { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_2, IsDown);  } break; //  0x32 ('2')  2
            case '3':     { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_3, IsDown);  } break; //  0x33 ('3')  3
            case '4':     { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_4, IsDown);  } break; //  0x34 ('4')  4
            case '5':     { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_5, IsDown);  } break; //  0x35 ('5')  5
            case '6':     { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_6, IsDown);  } break; //  0x36 ('6')  6
            case '7':     { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_7, IsDown);  } break; //  0x37 ('7')  7
            case '8':     { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_8, IsDown);  } break; //  0x38 ('8')  8
            case '9':     { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_9, IsDown);  } break; //  0x39 ('9')  9
            case 'A':     { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_A, IsDown); } break; //  0x41 ('A')  A
            case 'B':     { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_B, IsDown); } break; //  0x42 ('B')  B
            case 'C':     { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_C, IsDown); } break; //  0x43 ('C')  C
            case 'D':     { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_D, IsDown); } break; //  0x44 ('D')  D
            case 'E':     { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_E, IsDown); } break; //  0x45 ('E')  E
            case 'F':     { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_F, IsDown); } break; //  0x46 ('F')  F
            case 'G':     { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_G, IsDown); } break; //  0x47 ('G')  G
            case 'H':     { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_H, IsDown); } break; //  0x48 ('H')  H
            case 'I':     { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_I, IsDown); } break; //  0x49 ('I')  I
            case 'J':     { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_J, IsDown); } break; //  0x4A ('J')  J
            case 'K':     { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_K, IsDown); } break; //  0x4B ('K')  K
            case 'L':     { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_L, IsDown); //  0x4C ('L')  L
#if HANDMADE_INTERNAL
              if(IsDown)
              {
                if(AltKeyWasDown){
                  Win32BeginInputPlayBack(aState,1);
                }else{
                  if(aState->PlayingIndex == 0)
                  {
                    if(aState->RecordingIndex == 0)
                    {
                      Win32BeginRecordingInput(aState, 1);
                    }else{
                      Win32EndRecordingInput(aState);
                      Win32BeginInputPlayBack(aState,1);
                    }
                  }else{
                    Win32EndInputPlayback(aState);
                    *NewKeyboardController ={};
                  }
                }
              }
#endif
            }break;
            case 'M':    { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_M, IsDown); }break; //  0x4D ('M')  M
            case 'N':    { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_N, IsDown); }break; //  0x4E ('N')  N
            case 'O':    { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_O, IsDown); }break; //  0x4F ('O')  O
            case 'P':    { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_P, IsDown); //  0x50 ('P')  P
#if HANDMADE_INTERNAL
              if(IsDown)
              {
                GlobalPause = !GlobalPause;
              }
#endif
            }break;
            case 'Q':      { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_Q, IsDown); }break; // 0x51 ('Q') Q
            case 'R':      { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_R, IsDown); }break; // 0x52 ('R') R
            case 'S':      { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_S, IsDown); }break; // 0x53 ('S') S
            case 'T':      { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_T, IsDown); }break; // 0x54 ('T') T
            case 'U':      { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_U, IsDown); }break; // 0x55 ('U') U
            case 'V':      { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_V, IsDown); }break; // 0x56 ('V') V
            case 'W':      { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_W, IsDown); }break; // 0x57 ('W') W
            case 'X':      { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_X, IsDown); }break; // 0x58 ('X') X
            case 'Y':      { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_Y, IsDown); }break; // 0x59 ('Y') Y
            case 'Z':      { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_Z, IsDown); }break; // 0x5A ('Z') Z
            case VK_LWIN:      { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_LWIN, IsDown); }break; // 0x5B  Left Win
            case VK_RWIN:      { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_RWIN, IsDown); }break; // 0x5C  Right Win
            ////case VK_APPS:      {}break; // 0x5D  Context Menu
            ////case VK_SLEEP:     {}break; //  0x5F  Sleep
            case VK_NUMPAD0:   { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_NP_0, IsDown); }break; //  0x60  Numpad 0
            case VK_NUMPAD1:   { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_NP_1, IsDown); }break; //  0x61  Numpad 1
            case VK_NUMPAD2:   { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_NP_2, IsDown); }break; //  0x62  Numpad 2
            case VK_NUMPAD3:   { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_NP_3, IsDown); }break; //  0x63  Numpad 3
            case VK_NUMPAD4:   { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_NP_4, IsDown); }break; //  0x64  Numpad 4
            case VK_NUMPAD5:   { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_NP_5, IsDown); }break; //  0x65  Numpad 5
            case VK_NUMPAD6:   { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_NP_6, IsDown); }break; //  0x66  Numpad 6
            case VK_NUMPAD7:   { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_NP_7, IsDown); }break; //  0x67  Numpad 7
            case VK_NUMPAD8:   { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_NP_8, IsDown); }break; //  0x68  Numpad 8
            case VK_NUMPAD9:   { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_NP_9, IsDown); }break; //  0x69  Numpad 9
            case VK_MULTIPLY:  { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_NP_STAR, IsDown); }break; // 0x6A  Numpad *
            case VK_ADD:       { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_NP_PLUS, IsDown); }break; //  0x6B  Numpad +
            //case VK_SEPARATOR: { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_, IsDown); }break; //  0x6C  Separator
            case VK_SUBTRACT:  { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_NP_DASH, IsDown); }break; // 0x6D  Num -
            case VK_DECIMAL:   { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_NP_DEL, IsDown); }break; //  0x6E  Numpad .
            case VK_DIVIDE:    { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_NP_SLASH, IsDown); }break; // 0x6F  Numpad /
            case VK_F1:        { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_F1, IsDown); }break; // 0x70  F1
            case VK_F2:        { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_F2, IsDown); }break; // 0x71  F2
            case VK_F3:        { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_F3, IsDown); }break; // 0x72  F3
            case VK_F4:        { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_F4, IsDown); }break; // 0x73  F4
            case VK_F5:        { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_F5, IsDown); }break; // 0x74  F5
            case VK_F6:        { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_F6, IsDown); }break; // 0x75  F6
            case VK_F7:        { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_F7, IsDown); }break; // 0x76  F7
            case VK_F8:        { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_F8, IsDown); }break; // 0x77  F8
            case VK_F9:        { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_F9, IsDown); }break; // 0x78  F9
            case VK_F10:       { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_F10, IsDown); }break; //  0x79  F10
            case VK_F11:       { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_F11, IsDown); }break; //  0x7A  F11
            case VK_F12:       { Win32ProcessKeyboardMessage(&NewKeyboardController->Key_F12, IsDown); }break; //  0x7B  F12
            ////case VK_F13:       {}break; //  0x7C  F13
            ////case VK_F14:       {}break; //  0x7D  F14
            ////case VK_F15:       {}break; //  0x7E  F15
            ////case VK_F16:       {}break; //  0x7F  F16
            ////case VK_F17:       {}break; //  0x80  F17
            ////case VK_F18:       {}break; //  0x81  F18
            ////case VK_F19:       {}break; //  0x82  F19
            ////case VK_F20:       {}break; //  0x83  F20
            ////case VK_F21:       {}break; //  0x84  F21
            ////case VK_F22:       {}break; //  0x85  F22
            ////case VK_F23:       {}break; //  0x86  F23
            ////case VK_F24:       {}break; //  0x87  F24
            case VK_NUMLOCK:   {Win32ProcessKeyboardMessage(&NewKeyboardController->Key_NP_NMLK, IsDown);}break; //  0x90  Num Lock
            case VK_SCROLL:    {Win32ProcessKeyboardMessage(&NewKeyboardController->Key_SCRLK, IsDown);}break; // 0x91  Scrol Lock
            ////case VK_OEM_FJ_JISHO:    {}break; // 0x92  Jisho
            ////case VK_OEM_FJ_MASSHOU:  {}break; // 0x93  Mashu
            ////case VK_OEM_FJ_TOUROKU:  {}break; // 0x94  Touroku
            ////case VK_OEM_FJ_LOYA:     {}break; //  0x95  Loya
            ////case VK_OEM_FJ_ROYA:     {}break; //  0x96  Roya
            case VK_LSHIFT:          {Win32ProcessKeyboardMessage(&NewKeyboardController->Key_LSHIFT, IsDown);}break; // 0xA0  Left Shift
            case VK_RSHIFT:          {Win32ProcessKeyboardMessage(&NewKeyboardController->Key_RSHIFT, IsDown);}break; // 0xA1  Right Shift
            case VK_LCONTROL:        {Win32ProcessKeyboardMessage(&NewKeyboardController->Key_LCTRL, IsDown);}break; // 0xA2  Left Ctrl
            case VK_RCONTROL:        {Win32ProcessKeyboardMessage(&NewKeyboardController->Key_RCTRL, IsDown);}break; // 0xA3  Right Ctrl
          //case VK_LMENU:           {Win32ProcessKeyboardMessage(&NewKeyboardController->Key_, IsDown);}break; //  0xA4  Left Alt
          //case VK_RMENU:           {Win32ProcessKeyboardMessage(&NewKeyboardController->Key_, IsDown);}break; //  0xA5  Right Alt
            ////case VK_BROWSER_BACK:    {}break; // 0xA6  Browser Back
            ////case VK_BROWSER_FORWARD: {}break; //  0xA7  Browser Forward
            ////case VK_BROWSER_REFRESH: {}break; //  0xA8  Browser Refresh
            ////case VK_BROWSER_STOP:      {}break; // 0xA9  Browser Stop
            ////case VK_BROWSER_SEARCH:    {}break; // 0xAA  Browser Search
            ////case VK_BROWSER_FAVORITES: {}break; //  0xAB  Browser Favorites
            ////case VK_BROWSER_HOME:      {}break; // 0xAC  Browser Home
            ////case VK_VOLUME_MUTE:       {}break; //  0xAD  Volume Mute
            ////case VK_VOLUME_DOWN:       {}break; //  0xAE  Volume Down
            ////case VK_VOLUME_UP:         {}break; //  0xAF  Volume Up
            ////case VK_MEDIA_NEXT_TRACK:  {}break; // 0xB0  Next Track
            ////case VK_MEDIA_PREV_TRACK:  {}break; // 0xB1  Previous Track
            ////case VK_MEDIA_STOP:        {}break; // 0xB2  Stop
            ////case VK_MEDIA_PLAY_PAUSE:  {}break; // 0xB3  Play / Pause
            ////case VK_LAUNCH_MAIL:       {}break; //  0xB4  Mail
            ////case VK_LAUNCH_MEDIA_SELECT: {}break; //  0xB5  Media
            ////case VK_LAUNCH_APP1:         {}break; //  0xB6  App1
            ////case VK_LAUNCH_APP2:         {}break; //  0xB7  App2
            case VK_OEM_1:       {Win32ProcessKeyboardMessage(&NewKeyboardController->Key_COLON, IsDown);}break; //  0xBA  OEM_1 (: ;)
            case VK_OEM_PLUS:    {Win32ProcessKeyboardMessage(&NewKeyboardController->Key_EQUAL, IsDown);}break; // 0xBB  OEM_PLUS (+ =)
            case VK_OEM_COMMA:   {Win32ProcessKeyboardMessage(&NewKeyboardController->Key_COMMA, IsDown);}break; //  0xBC  OEM_COMMA (< ,)
            case VK_OEM_MINUS:   {Win32ProcessKeyboardMessage(&NewKeyboardController->Key_DASH, IsDown);}break; //  0xBD  OEM_MINUS (_ -)
            case VK_OEM_PERIOD:  {Win32ProcessKeyboardMessage(&NewKeyboardController->Key_DOT, IsDown);}break; // 0xBE  OEM_PERIOD (> .)
            case VK_OEM_2:       {Win32ProcessKeyboardMessage(&NewKeyboardController->Key_FSLASH, IsDown);}break; //  0xBF  OEM_2 (? /)
            case VK_OEM_3:       {Win32ProcessKeyboardMessage(&NewKeyboardController->Key_TILDE, IsDown);}break; //  0xC0  OEM_3 (~ `)
//            case VK_ABNT_C1:     {}break; //  0xC1  Abnt C1
//            case VK_ABNT_C2:     {}break; //  0xC2  Abnt C2
            case VK_OEM_4:       {Win32ProcessKeyboardMessage(&NewKeyboardController->Key_LBRACKET, IsDown);}break; //  0xDB  OEM_4 ({ [)
            case VK_OEM_5:       {Win32ProcessKeyboardMessage(&NewKeyboardController->Key_RBSLASH, IsDown);}break; //  0xDC  OEM_5 (| \)
            case VK_OEM_6:       {Win32ProcessKeyboardMessage(&NewKeyboardController->Key_RBRACKET, IsDown);}break; //  0xDD  OEM_6 (} ])
            case VK_OEM_7:       {Win32ProcessKeyboardMessage(&NewKeyboardController->Key_QUOTE, IsDown);}break; //  0xDE  OEM_7 (" ')
            //case VK_OEM_8:       {Win32ProcessKeyboardMessage(&NewKeyboardController->Key_, IsDown);}break; //  0xDF  OEM_8 (§ !)
            ////case VK_OEM_AX:      {}break; // 0xE1  Ax
            case VK_OEM_102:     {Win32ProcessKeyboardMessage(&NewKeyboardController->Key_LBSLASH, IsDown);}break; // 0xE2  OEM_102 (> <)
            ////case VK_ICO_HELP:    {}break; // 0xE3  IcoHlp
            ////case VK_ICO_00:      {}break; // 0xE4  Ico00 *
            ////case VK_PROCESSKEY:  {}break; // 0xE5  Process
            ////case VK_ICO_CLEAR:   {}break; // 0xE6  IcoClr
            ////case VK_PACKET:      {}break; // 0xE7  Packet
            ////case VK_OEM_RESET:   {}break; // 0xE9  Reset
            ////case VK_OEM_JUMP:    {}break; // 0xEA  Jump
            ////case VK_OEM_PA1:     {}break; // 0xEB  OemPa1
            ////case VK_OEM_PA2:     {}break; // 0xEC  OemPa2
            ////case VK_OEM_PA3:     {}break; // 0xED  OemPa3
            ////case VK_OEM_WSCTRL:  {}break; // 0xEE  WsCtrl
            ////case VK_OEM_CUSEL:   {}break; // 0xEF  Cu Sel
            ////case VK_OEM_ATTN:    {}break; // 0xF0  Oem Attn
            ////case VK_OEM_FINISH:  {}break; // 0xF1  Finish
            ////case VK_OEM_COPY:    {}break; // 0xF2  Copy
            ////case VK_OEM_AUTO:    {}break; // 0xF3  Auto
            ////case VK_OEM_ENLW:    {}break; // 0xF4  Enlw
            ////case VK_OEM_BACKTAB: {}break; // 0xF5  Back Tab
            ////case VK_ATTN:        {}break; // 0xF6  Attn
            ////case VK_CRSEL:       {}break; // 0xF7  Cr Sel
            ////case VK_EXSEL:       {}break; // 0xF8  Ex Sel
            ////case VK_EREOF:       {}break; // 0xF9  Er Eof
            ////case VK_PLAY:        {}break; // 0xFA  Play
            ////case VK_ZOOM:        {}break; // 0xFB  Zoom
            ////case VK_NONAME:      {}break; // 0xFC  NoName
            ////case VK_PA1:         {}break; // 0xFD  Pa1
            ////case VK_OEM_CLEAR:   {}break; // 0xFE  OemClr
//            case VK__none_:      {}break; // 0xFF  no VK mapping
            default: DEBUGPrint("INVALID %d\n", VKCode); break;
                
          }
        }
        if(IsDown)
        {
          if( (VKCode==VK_F4) && AltKeyWasDown)
          {
            GlobalRunning=false;
          }
          if( (VKCode==VK_RETURN) && AltKeyWasDown)
          {
            if(Message.hwnd)
            {
              Win32ToggleFullscreen(Message.hwnd);
            }
          }
        }
      }break;
      default:
      {
        TranslateMessage(&Message);
        DispatchMessage(&Message);
      }break;
    }
  }
}


internal void
Win32ProcessControllerInput( game_input* OldInput, game_input* NewInput)
{

  // Input
  // TODO: Should we poll this more frequently?
  DWORD MaxControllerCount = XUSER_MAX_COUNT;
  if(MaxControllerCount > (ArrayCount(NewInput->Controllers)-1))
  {
    MaxControllerCount = (ArrayCount(NewInput->Controllers)-1);
  }

  for( DWORD ControllerIndex = 0;
    ControllerIndex < MaxControllerCount;
    ++ControllerIndex )
  {
    DWORD OurControllerIndex = ControllerIndex+1;

    game_controller_input* OldController = GetController(OldInput, OurControllerIndex);
    game_controller_input* NewController = GetController(NewInput, OurControllerIndex);
    // Note: Performancebug in XInputGetControllerstate.
    //     See HH007 at 15 min. To be ironed out in optimization
    XINPUT_STATE ControllerState;
    if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
    {
      // NOTE: This controller is plugged in
      // TODO: See if ControllerState.dwPacketNumber increment too rapidly
      XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

      r32 StickThreshhold = 0.0f;
      r32 TriggerThreshhold = 0.5f;

      NewController->IsAnalog = true;
      NewController->IsConnected = true;

      // Todo: Check if controller deadzone is round or rectangular
      //     Right now it is treated as rectangular inside
      //     Win32ProcessXinputStickValue

      // Rectangular done deadzone
      NewController->LeftStickAverageX =
                Win32ProcessXinputStickValue(Pad->sThumbLX,
                XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
      NewController->LeftStickAverageY =
                Win32ProcessXinputStickValue(Pad->sThumbLY,
                XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
      NewController->RightStickAverageX =
                Win32ProcessXinputStickValue(Pad->sThumbRX,
                XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
      NewController->RightStickAverageY =
                Win32ProcessXinputStickValue(Pad->sThumbRY,
                XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);

#if 0
      // Rectangular Naivly done circular deadzone
      // Not working correctly, wait untill casey does it
      // on stream
      Win32ProcessXinputStickValue(
            &NewController->LeftStickAverageX,
            &NewController->LeftStickAverageY,
            Pad->sThumbLX,
            Pad->sThumbLY,
            XINPUT_GAMEPAD_LEFT_THUMB_DEADZO
      Win32ProcessXinputStickValue(   &(NewController->RightStickAverageX),
                      &(NewController->RightStickAverageY),
                      Pad->sThumbRX,
                      Pad->sThumbRY,
                      XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
#endif
      // Left stick
      Win32ProcessXInputDigitalButton(
        (NewController->LeftStickAverageX<-StickThreshhold) ? 1 : 0,
          &OldController->LeftStickLeft, 1,
          &NewController->LeftStickLeft);
      Win32ProcessXInputDigitalButton(
        (NewController->LeftStickAverageX >StickThreshhold) ? 1 : 0,
          &OldController->LeftStickRight, 1,
          &NewController->LeftStickRight);
      Win32ProcessXInputDigitalButton(
        (NewController->LeftStickAverageY <-StickThreshhold) ? 1 : 0,
          &OldController->LeftStickDown, 1,
          &NewController->LeftStickDown);
      Win32ProcessXInputDigitalButton(
        (NewController->LeftStickAverageY > StickThreshhold) ? 1 : 0,
          &OldController->LeftStickUp, 1,
          &NewController->LeftStickUp);
      // Right stick
      Win32ProcessXInputDigitalButton(
        (NewController->RightStickAverageX<-StickThreshhold) ? 1 : 0,
          &OldController->RightStickLeft, 1,
          &NewController->RightStickLeft);
      Win32ProcessXInputDigitalButton(
        (NewController->RightStickAverageX >StickThreshhold) ? 1 : 0,
          &OldController->RightStickRight, 1,
          &NewController->RightStickRight);
      Win32ProcessXInputDigitalButton(
        (NewController->RightStickAverageY<-StickThreshhold) ? 1 : 0,
          &OldController->RightStickDown, 1,
          &NewController->RightStickDown);
      Win32ProcessXInputDigitalButton(
        (NewController->RightStickAverageY >StickThreshhold) ? 1 : 0,
          &OldController->RightStickUp, 1,
          &NewController->RightStickUp);

      NewController->LeftTriggerAverage = (r32)Pad->bLeftTrigger/255.0f;
      Win32ProcessXInputDigitalButton(
          (NewController->LeftTriggerAverage > TriggerThreshhold) ? 1 : 0,
          &OldController->LeftTrigger, 1,
          &NewController->LeftTrigger);

      NewController->RightTriggerAverage =(r32)Pad->bRightTrigger/255.0f;
      Win32ProcessXInputDigitalButton(
          (NewController->RightTriggerAverage > TriggerThreshhold) ? 1 : 0,
          &OldController->RightTrigger, 1,
          &NewController->RightTrigger);

      // Button handling
      Win32ProcessXInputDigitalButton(
          Pad ->wButtons,
          &OldController->DPadUp, XINPUT_GAMEPAD_DPAD_UP,
          &NewController->DPadUp);
      Win32ProcessXInputDigitalButton(
          Pad ->wButtons,
          &OldController->DPadDown, XINPUT_GAMEPAD_DPAD_DOWN,
          &NewController->DPadDown);
      Win32ProcessXInputDigitalButton(
          Pad ->wButtons,
          &OldController->DPadLeft, XINPUT_GAMEPAD_DPAD_LEFT,
          &NewController->DPadLeft);
      Win32ProcessXInputDigitalButton(
          Pad ->wButtons,
          &OldController->DPadRight, XINPUT_GAMEPAD_DPAD_RIGHT,
          &NewController->DPadRight);
      Win32ProcessXInputDigitalButton(
          Pad ->wButtons,
          &OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER,
          &NewController->LeftShoulder);
      Win32ProcessXInputDigitalButton(
          Pad ->wButtons,
          &OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER,
          &NewController->RightShoulder);
      Win32ProcessXInputDigitalButton(
          Pad ->wButtons,
          &OldController->Start, XINPUT_GAMEPAD_START,
          &NewController->Start);
      Win32ProcessXInputDigitalButton(
          Pad ->wButtons,
          &OldController->Select, XINPUT_GAMEPAD_BACK,
          &NewController->Select);
      Win32ProcessXInputDigitalButton(
          Pad ->wButtons,
          &OldController->A, XINPUT_GAMEPAD_A,
          &NewController->A);
      Win32ProcessXInputDigitalButton(
          Pad ->wButtons,
          &OldController->B, XINPUT_GAMEPAD_B,
          &NewController->B);
      Win32ProcessXInputDigitalButton(
          Pad ->wButtons,
          &OldController->X, XINPUT_GAMEPAD_X,
          &NewController->X);
      Win32ProcessXInputDigitalButton(
          Pad ->wButtons,
          &OldController->Y, XINPUT_GAMEPAD_Y,
          &NewController->Y);

    }else{
      // NOTE: The controller is not available
      NewController->IsConnected = false;
    }
  }
}


inline LARGE_INTEGER
Win32GetWallClock(void)
{
  LARGE_INTEGER Result;
  QueryPerformanceCounter(&Result);
  return Result;
}

inline r32
Win32GetWallClockSeconds()
{
  r32 Result = (r32) ((r64) Win32GetWallClock().QuadPart / (r64)GlobalPerfCounterFrequency);
  return Result;
}

inline r32
Win32GetSecondsElapsed(LARGE_INTEGER aStart, LARGE_INTEGER aEnd)
{
  r32 Result =  ((r32)(aEnd.QuadPart - aStart.QuadPart) / (r32)GlobalPerfCounterFrequency );
  return Result;
}

internal void
Win32DebugDrawVertical(win32_offscreen_buffer* aBackBuffer,
            s32 aX, s32 aTop, s32 aBottom, u32 aColor)
{
  if(aTop < 0)
  {
    aTop = 0;
  }

  if(aBottom > aBackBuffer->Height)
  {
    aBottom = aBackBuffer->Height;
  }

  if(( aX>=0 ) && ( aX<aBackBuffer->Width ))
  {
    u8* Pixel =  ((u8*) aBackBuffer->Memory +
            aX*aBackBuffer->BytesPerPixel +
            aTop*aBackBuffer->Pitch);

    for(s32 Y = aTop; Y<aBottom; ++Y)
    {
      *(u32*) Pixel = aColor;
      Pixel += aBackBuffer->Pitch;
    }
  }
}

inline void
Win32DebugDrawSoundBufferMarker(win32_offscreen_buffer* aBackBuffer,
                win32_sound_output*   SoundOutput,
                r32 aC, s32 aPadX, s32 aTop, s32 aBottom,
                u32 Color, DWORD Value)
{
  r32 XReal32 = ( aC * (r32) Value);\

  s32 X = aPadX +  (s32)  XReal32;

  Win32DebugDrawVertical(aBackBuffer, X, aTop, aBottom, Color);
}

#if 0 // TODO: Implement later
internal PLATFORM_OPEN_FILE(Win32OpenFile)
{
    platform_file_handle Result = {};
    Assert(sizeof(HANDLE) <= sizeof(Result.Platform));

    DWORD HandlePermissions = 0;
    DWORD HandleCreation = 0;
    if(ModeFlags & OpenFile_Read)
    {
        HandlePermissions |= GENERIC_READ;
        HandleCreation = OPEN_EXISTING;
    }

    if(ModeFlags & OpenFile_Write)
    {
        HandlePermissions |= GENERIC_WRITE;
        HandleCreation = OPEN_ALWAYS;
    }

    wchar_t *FileName = (wchar_t *)Info->Platform;
    HANDLE Win32Handle = CreateFileW(FileName, HandlePermissions,
                                     FILE_SHARE_READ, 0, HandleCreation, 0, 0);
    Result.NoErrors = (Win32Handle != INVALID_HANDLE_VALUE);
    *(HANDLE *)&Result.Platform = Win32Handle;

    return(Result);
}

internal PLATFORM_FILE_ERROR(Win32FileError)
{
#if HANDMADE_INTERNAL
    OutputDebugString("WIN32 FILE ERROR: ");
    OutputDebugString(Message);
    OutputDebugString("\n");
#endif

    Handle->NoErrors = false;
}

internal PLATFORM_READ_DATA_FROM_FILE(Win32ReadDataFromFile)
{
    if(PlatformNoFileErrors(Handle))
    {
        HANDLE Win32Handle = *(HANDLE *)&Handle->Platform;

        OVERLAPPED Overlapped = {};
        Overlapped.Offset = (u32)((Offset >> 0) & 0xFFFFFFFF);
        Overlapped.OffsetHigh = (u32)((Offset >> 32) & 0xFFFFFFFF);

        u32 FileSize32 = SafeTruncateToU32(Size);

        DWORD BytesRead;
        if(ReadFile(Win32Handle, Dest, FileSize32, &BytesRead, &Overlapped) &&
           (FileSize32 == BytesRead))
        {
            // NOTE(casey): File read succeeded!
        }
        else
        {
            Win32FileError(Handle, "Read file failed.");
        }
    }
}

internal PLATFORM_WRITE_DATA_TO_FILE(Win32WriteDataToFile)
{
    if(PlatformNoFileErrors(Handle))
    {
        HANDLE Win32Handle = *(HANDLE *)&Handle->Platform;

        OVERLAPPED Overlapped = {};
        Overlapped.Offset = (u32)((Offset >> 0) & 0xFFFFFFFF);
        Overlapped.OffsetHigh = (u32)((Offset >> 32) & 0xFFFFFFFF);

        u32 FileSize32 = SafeTruncateToU32(Size);

        DWORD BytesWritten;
        if(WriteFile(Win32Handle, Source, FileSize32, &BytesWritten, &Overlapped) &&
           (FileSize32 == BytesWritten))
        {
            // NOTE(casey): File read succeeded!
        }
        else
        {
            Win32FileError(Handle, "Write file failed.");
        }
    }
}

internal PLATFORM_CLOSE_FILE(Win32CloseFile)
{
    HANDLE Win32Handle = *(HANDLE *)&Handle->Platform;
    if(Win32Handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(Win32Handle);
    }
}

#endif

inline b32
Win32IsInLoop(win32_state *aState)
{
    b32 Result = ((aState->RecordingIndex != 0) ||
                   (aState->PlayingIndex));
    return(Result);
}


// Signature: platform_memory_block* PLATFORM_ALLOCATE_MEMORY(memory_index aSize, u64 aFlags)

PLATFORM_ALLOCATE_MEMORY(Win32AllocateMemory)
{
    // NOTE(casey): We require memory block headers not to change the cache
    // line alignment of an allocation
    Assert(sizeof(win32_memory_block) == 64);

    uintptr_t PageSize = 4096; // TODO(casey): Query from system?
    uintptr_t TotalSize = aSize + sizeof(win32_memory_block);
    uintptr_t BaseOffset = sizeof(win32_memory_block);
    uintptr_t ProtectOffset = 0;
    if(aFlags & PlatformMemory_UnderflowCheck)
    {
        TotalSize = aSize + 2*PageSize;
        BaseOffset = 2*PageSize;
        ProtectOffset = PageSize;
    }
    else if(aFlags & PlatformMemory_OverflowCheck)
    {
        uintptr_t SizeRoundedUp = AlignPow2(aSize, PageSize);
        TotalSize = SizeRoundedUp + 2*PageSize;
        BaseOffset = PageSize + SizeRoundedUp - aSize;
        ProtectOffset = PageSize + SizeRoundedUp;
    }

    win32_memory_block* Block = (win32_memory_block*)
        VirtualAlloc(0, TotalSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

    Assert(Block);
    Block->Block.Base = (u8*) Block + BaseOffset;
    Assert(Block->Block.Used == 0);
    Assert(Block->Block.ArenaPrev == 0);

    if(aFlags & (PlatformMemory_UnderflowCheck|PlatformMemory_OverflowCheck))
    {
        DWORD OldProtect = 0;
        BOOL Protected = VirtualProtect((u8 *)Block + ProtectOffset, PageSize, PAGE_NOACCESS, &OldProtect);
        Assert(Protected);
    }

    win32_memory_block* Sentinel = &GlobalWin32State.MemorySentinel;

    Block->Next     = Sentinel;
    Block->Block.Size   = aSize;
    Block->Block.Flags  = aFlags;
    Block->LoopingFlags = 0;
    if(Win32IsInLoop(&GlobalWin32State) && !(aFlags & PlatformMemory_NotRestored))
    {
        Block->LoopingFlags = Win32Mem_AllocatedDuringLooping;
    }

    BeginTicketMutex(&GlobalWin32State.MemoryMutex);
    Block->Prev     = Sentinel->Prev;
    Block->Prev->Next   = Block;
    Block->Next->Prev   = Block;
    EndTicketMutex(&GlobalWin32State.MemoryMutex);

    platform_memory_block *PlatBlock = &Block->Block;
    return(PlatBlock);
}


// Signature void PLATFORM_DEALLOCATE_MEMORY(platform_memory_block *aBlock)

PLATFORM_DEALLOCATE_MEMORY(Win32DeallocateMemory)
{
  if( aBlock)
  {
    win32_memory_block* Win32Block = ( (win32_memory_block*) aBlock);
    if(Win32IsInLoop(&GlobalWin32State))
    {
      Win32Block->LoopingFlags = Win32Mem_FreedDuringLooping;
    }
    else
    {
      Win32FreeMemoryBlock(Win32Block);
    }
  }
}

global_variable debug_table GlobalDebugTable_;
debug_table* GlobalDebugTable = &GlobalDebugTable_;


struct platform_work_queue_entry
{
  platform_work_queue_callback* Callback;
  void* Data;
};

struct platform_work_queue
{
  u32 volatile CompletionGoal;
  u32 volatile CompletionCount;

  u32 volatile NextEntryToWrite;
  u32 volatile NextEntryToRead;
  HANDLE SemaphoreHandle;

  platform_work_queue_entry Entries[1024];
};

internal void
Win32AddEntry(platform_work_queue* Queue, platform_work_queue_callback* Callback, void* Data)
{
  // Todo Switch to Interlocked compare exchange to make it a multiple producer / multiple consumer queue
  u32 NewNextEntryToWrite = (Queue->NextEntryToWrite + 1) % ArrayCount(Queue->Entries);
  Assert( NewNextEntryToWrite != Queue->NextEntryToRead);
  platform_work_queue_entry* Entry = Queue->Entries + Queue->NextEntryToWrite;
  Entry->Callback = Callback;
  Entry->Data = Data;
  ++Queue->CompletionGoal;
  _WriteBarrier();
  Queue->NextEntryToWrite = NewNextEntryToWrite;
  ReleaseSemaphore(Queue->SemaphoreHandle, 1, 0);
}

internal b32
Win32DoNextWorkQueueEntry(platform_work_queue* Queue)
{
  b32 GoBackToSleep = false;
  u32 OriginalNextEntryToRead = Queue->NextEntryToRead;
  u32 NewNextEntryToRead =  (OriginalNextEntryToRead + 1) % ArrayCount(Queue->Entries);
  if(OriginalNextEntryToRead != Queue->NextEntryToWrite)
  {
    u32 Index = InterlockedCompareExchange((LONG volatile *)&Queue->NextEntryToRead,
                                      NewNextEntryToRead,
                                      OriginalNextEntryToRead);
    if(Index == OriginalNextEntryToRead)
    {
      platform_work_queue_entry* Entry = Queue->Entries + Index;
      Entry->Callback(Queue, Entry->Data);
      InterlockedIncrement( (long volatile *)&Queue->CompletionCount );
    }
  }else{
    // Go back to sleep
    GoBackToSleep = true;
  }
  // Go back to sleep
  return(GoBackToSleep);
}

internal void
Win32CompleteAllWork(platform_work_queue* Queue)
{
  platform_work_queue_entry Entry = {};
  while (Queue->CompletionGoal != Queue->CompletionCount)
  {
    Win32DoNextWorkQueueEntry(Queue);
  }

  Queue->CompletionCount = 0;
  Queue->CompletionGoal = 0;
}

struct win32_thread_info
{
  platform_work_queue* Queue;
  int LogicalThreadIndex;
};

DWORD WINAPI
ThreadProc(LPVOID lpParameter)
{
  win32_thread_info* ThreadInfo = (win32_thread_info*) lpParameter;
  for (;;)
  {
    if(Win32DoNextWorkQueueEntry(ThreadInfo->Queue))
    {
      WaitForSingleObjectEx( ThreadInfo->Queue->SemaphoreHandle, INFINITE, FALSE);
    }
  }
  return 0;
}

// operating on data
PLATFORM_WORK_QUEUE_CALLBACK(DoWorkerWork)
{
  char Buffer[256];;
  wsprintf(Buffer, "Thread %u: %s\n", GetCurrentThreadId(), (char*) Data);
  OutputDebugStringA(Buffer);
}

struct test_list
{
  u32 Num;
  test_list* Next;
};
struct test_struct
{
  memory_arena* Arena;
  u32 Sum;
  test_list* Head;
};

PLATFORM_WORK_QUEUE_CALLBACK(testFun)
{
  #if 1
  test_struct* Struct = (test_struct*) Data;
  temporary_memory TempMem = BeginTemporaryMemory(Struct->Arena);
  u32 memsize = 50;
  test_list* Arr = (test_list*) PushArray(Struct->Arena, memsize, test_list);
  test_list** a = &(Struct->Head);
  for (u32 i = 0; i < memsize; ++i)
  {
    *a = Arr+i;
    test_list* entry = *a;
    entry->Num  = sizeof(test_list);
    a = &(entry->Next);
  }

  u32 sum = 0;
  test_list* entry = Struct->Head;
  while(entry)
  {
    sum += entry->Num;
    entry = entry->Next;
  }
  char* Buffer = PushArray(Struct->Arena, 256, char);
  wsprintf(Buffer, "MemUsed %u kb\n", sum/(1028));
  OutputDebugStringA(Buffer);
  EndTemporaryMemory(TempMem);
#else
  u32 sum = 0;
  for (int i = 1; i < 2000; ++i)
  {
    sum+=i;
  }
  char Buffer[256] = {};
  wsprintf(Buffer, "Sum %u \n", sum);
  OutputDebugStringA(Buffer);
#endif

};

s32 CALLBACK
WinMain(  HINSTANCE Instance,
          HINSTANCE PrevInstance,
          LPSTR CommandLine,
          s32 ShowCode )
{
  win32_thread_info Threads[3] = {};
  u32 ThreadCount = 3;
  u32 InitialCount = 0;
  platform_work_queue HighPriorityQueue = {};
  HighPriorityQueue.SemaphoreHandle = CreateSemaphoreEx(0, InitialCount, ThreadCount, 0, 0, SEMAPHORE_ALL_ACCESS);
  for (u32 ThreadIndex = 0; ThreadIndex < ArrayCount(Threads); ++ThreadIndex)
  {
    win32_thread_info* Info = Threads + ThreadIndex;
    Info->LogicalThreadIndex = ThreadIndex;
    Info->Queue = &HighPriorityQueue;
    DWORD ThreadID;
    HANDLE ThreadHandle = CreateThread( 0, 0, ThreadProc, Info, 0, &ThreadID);
    CloseHandle(ThreadHandle);
  }

#if 0
  Win32AddEntry(&HighPriorityQueue, DoWorkerWork, "A0");
  Win32AddEntry(&HighPriorityQueue, DoWorkerWork, "A1");
  Win32AddEntry(&HighPriorityQueue, DoWorkerWork, "A2");
  Win32AddEntry(&HighPriorityQueue, DoWorkerWork, "A3");
  Win32AddEntry(&HighPriorityQueue, DoWorkerWork, "A4");
  Win32AddEntry(&HighPriorityQueue, DoWorkerWork, "A5");
  Win32AddEntry(&HighPriorityQueue, DoWorkerWork, "A6");
  Win32CompleteAllWork(&HighPriorityQueue);
#endif

  Win32GetEXEFileName(&GlobalWin32State);

  char SourceGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
  Win32BuildEXEPathFileName(&GlobalWin32State, "handmade.dll",
            sizeof(SourceGameCodeDLLFullPath), SourceGameCodeDLLFullPath );

  char TempGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
  Win32BuildEXEPathFileName(&GlobalWin32State, "handmade_temp.dll",
            sizeof(TempGameCodeDLLFullPath), TempGameCodeDLLFullPath );


  char TempGameCodeLockFullPath[WIN32_STATE_FILE_NAME_COUNT];
  Win32BuildEXEPathFileName(&GlobalWin32State, "lock.tmp",
            sizeof(TempGameCodeLockFullPath), TempGameCodeLockFullPath );

  LARGE_INTEGER PerfCounterFrequencyResult;
  QueryPerformanceFrequency(&PerfCounterFrequencyResult);
  GlobalPerfCounterFrequency = PerfCounterFrequencyResult.QuadPart;

  // Note:  sets the Windows scheduler granularity (The max time resolution of Sleep()
  //      - function) to 1 ms.
  UINT DesiredSchedulerMS = 1;
  b32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

  Win32LoadXInput();

#if HANDMADE_INTERNAL
  DEBUGGlobalShowCursor = true;
#endif
  
  WNDCLASSA WindowClass = {};
  WindowClass.style = CS_HREDRAW|CS_VREDRAW;//|CS_OWNDC;
  WindowClass.lpfnWndProc = MainWindowCallback;
  WindowClass.hInstance = PrevInstance;
  WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
//  WindowClass.hIcon;  // window icon (for later)
  WindowClass.lpszClassName = "HandmadeWindowClass";  // Name for the window class

  if(RegisterClass(&WindowClass))
  {
     HWND WindowHandle = CreateWindowEx(
        0,//WS_EX_TOPMOST|WS_EX_LAYERED,
        WindowClass.lpszClassName,
        "HandMade",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE,
        CW_USEDEFAULT, // X  CW_USEDEFAULT means the system decides for us.
        CW_USEDEFAULT, // Y
        CW_USEDEFAULT, // W
        CW_USEDEFAULT, // H
        0,
        0,
        Instance,
        NULL);


    if (WindowHandle != NULL)
    {
      game_render_commands RenderCommands = {};
      GlobalRenderCommands = &RenderCommands;
      HDC DeviceContext = GetDC(WindowHandle);
      win32_window_dimension Dimension = Win32GetWindowDimension( WindowHandle );
      RenderCommands.ScreenWidthPixels = Dimension.Width;
      RenderCommands.ScreenHeightPixels = Dimension.Height;
      ReleaseDC( WindowHandle, DeviceContext);
      RenderCommands.OpenGL = Win32InitOpenGL(WindowHandle);

      // TODO: How do we reliably query this on windows?
      s32 MonitorRefreshHz = 60;
      HDC DC = GetDC(WindowHandle);
      s32 Win32RefreshRate = GetDeviceCaps(DC,VREFRESH);
      s32 Win32MonitorWidthPx = GetDeviceCaps(DC,HORZRES);
      s32 Win32MonitorHeightPx = GetDeviceCaps(DC,VERTRES);
      ReleaseDC(WindowHandle, DC);
      if (Win32RefreshRate > 1)
      {
        MonitorRefreshHz = Win32RefreshRate;
      }
      s32 GameUpdateHz = 60;
      r32 TargetSecondsPerFrame = 1.0f / (r32) GameUpdateHz;

      win32_sound_output SoundOutput = {};

      SoundOutput.SamplesPerSecond = 48000;
      SoundOutput.ChannelCount = 2;
      SoundOutput.BytesPerSample = sizeof(s16); // 2 bytes
      SoundOutput.BufferSizeInSeconds = 1;
      SoundOutput.TargetSecondsOfLatency = (s32) (1.0/60.0); // 2 frames at 60 fps = 0.033 seconds.
      SoundOutput.BytesPerSampleTotal = SoundOutput.ChannelCount * SoundOutput.BytesPerSample;
      SoundOutput.BytesPerSecond = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSampleTotal;
      SoundOutput.BufferSizeInBytes = SoundOutput.BufferSizeInSeconds * SoundOutput.BytesPerSecond;
      SoundOutput.BytesOfLatency = (DWORD) (SoundOutput.TargetSecondsOfLatency  * (r32) SoundOutput.BytesPerSecond );

      Win32InitDSound(WindowHandle, &SoundOutput);

      Win32ClearSoundBuffer(&SoundOutput);
      SoundOutput.SecondaryBuffer->Play(0,0, DSBPLAY_LOOPING);

      // TODO: Merge with memory_arena somehow?
      s16* SoundSamples = (s16*) VirtualAlloc(0,
                      SoundOutput.BufferSizeInBytes,
                      MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

      // TODO: Implement these functions
      //GameMemory.PlatformAPI.OpenFile = Win32OpenFile;
      //GameMemory.PlatformAPI.ReadDataFromFile = Win32ReadDataFromFile;
      //GameMemory.PlatformAPI.WriteDataToFile = Win32WriteDataToFile;
      //GameMemory.PlatformAPI.FileError = Win32FileError;
      //GameMemory.PlatformAPI.CloseFile = Win32CloseFile;

      game_memory GameMemory = {};

      GameMemory.PlatformAPI.AllocateMemory   = Win32AllocateMemory;
      GameMemory.PlatformAPI.DeallocateMemory = Win32DeallocateMemory;

      GameMemory.PlatformAPI.DEBUGPlatformFreeFileMemory  = DEBUGPlatformFreeFileMemory;
      GameMemory.PlatformAPI.DEBUGPlatformReadEntireFile  = DEBUGPlatformReadEntireFile;
      GameMemory.PlatformAPI.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;
      GameMemory.PlatformAPI.DEBUGPlatformAppendToFile    = DEBUGPlatformAppendToFile;
      GameMemory.PlatformAPI.DEBUGExecuteSystemCommand    = DEBUGExecuteSystemCommand;
      GameMemory.PlatformAPI.DEBUGGetProcessState         = DEBUGGetProcessState;
      GameMemory.PlatformAPI.DEBUGFormatString            = DEBUGFormatString;
      GameMemory.PlatformAPI.DEBUGPrint                   = DEBUGPrint;

      GameMemory.PlatformAPI.HighPriorityQueue = &HighPriorityQueue;

      GameMemory.PlatformAPI.PlatformAddEntry = Win32AddEntry;
      GameMemory.PlatformAPI.PlatformCompleteWorkQueue = Win32CompleteAllWork;

      GlobalWin32State.MemorySentinel.Prev = &GlobalWin32State.MemorySentinel;
      GlobalWin32State.MemorySentinel.Next = &GlobalWin32State.MemorySentinel;

      GlobalWin32State.TotalSize = Gigabytes(1);

      Platform = GameMemory.PlatformAPI;

      for(s32 ReplayIndex = 0;
        ReplayIndex < ArrayCount(GlobalWin32State.ReplayBuffer);
        ++ReplayIndex)
      {

        /* Note (Jakob): For my own educational purposes:
          CreateFileMapping wants two 32 bit values which corresponds to the higher and lower
          bits separately from GlobalWin32State->TotalSize which is a 64 bit. This is for compatibility
          reasons between 32 and 64 bit systems.
          In other words it wants us to split the 64 bit value into 2 32 bit values.
          The high bit is extracted by bitshifting the TotalSize down by 32 bits and then
          truncate it.
          The low bit is extracted by using bit-and operator with a 32 bit value set to
          [111111 .... 1111] == 0xFFFFFFFF. Clever. Example with 16bit s32: [00101101 10010101]
          HIGH BIT: [00101101 10010101] >> 32 = [10101010 00101101]
                => (Cast to int8) => [00101101]
          LOW BIT: [00101101 10010101] & [11111111] = [10010101]

          Question: Cant we just truncate lower bit?
          [00101101 10010101] => (Cast to int8) => [10010101] ?????
          Answer: Turns out it works just fine but using & is always more fancy.
        */
        win32_replay_buffer* ReplayBuffer = &GlobalWin32State.ReplayBuffer[ReplayIndex];

        // NOTE: We start indexing from 1 as 0 means no recording  / playing
        Win32GetInputFileLocation(&GlobalWin32State, false, ReplayIndex+1,
              sizeof(ReplayBuffer->FileName), ReplayBuffer->FileName);

        ReplayBuffer->FileHandle = CreateFileA(ReplayBuffer->FileName,
                    GENERIC_WRITE|GENERIC_READ, 0, 0, CREATE_ALWAYS,0,0);

        LARGE_INTEGER MaxSize;
        MaxSize.QuadPart = GlobalWin32State.TotalSize;
        ReplayBuffer->MemoryMap = CreateFileMapping(
                    ReplayBuffer->FileHandle, 0,
                    PAGE_READWRITE,
                    MaxSize.HighPart,
                    MaxSize.LowPart,
                    0);

        ReplayBuffer->MemoryBlock = MapViewOfFile(
                      ReplayBuffer->MemoryMap,  FILE_MAP_ALL_ACCESS,
                      0, 0, GlobalWin32State.TotalSize);

        if(ReplayBuffer->MemoryBlock)
        {
          // Success
        }else{
          // TODO: Diagnostics, remove assert
          Assert(false);
        }

      }


      if( SoundSamples )
      {
        game_input* NewInput = &GlobalInput[0];
        game_input* OldInput = &GlobalInput[1];

        LARGE_INTEGER LastCounter = Win32GetWallClock();
        LARGE_INTEGER FlipWallClock = Win32GetWallClock();

        s32 DebugTimeMarkerIndex = 0;
        win32_debug_time_marker DebugTimeMarkers[30] = {0};
        DWORD AudioLatencyBytes = 0;
        r32 AudioLatencySeconds = 0;

        GlobalRunning = true;

        b32 SoundIsValid = false;

        win32_game_code Game = Win32LoadGameCode(SourceGameCodeDLLFullPath,
                                                 TempGameCodeDLLFullPath,
                                                 TempGameCodeLockFullPath);

        LARGE_INTEGER FrameMarkerClock = {};
        while(GlobalRunning)
        {
          NewInput->ExecutableReloaded = false;

          BEGIN_BLOCK(LoadGameCode);
          //TODO: Find out why we need a 2-3 sec wait for loop live code editing to work
          //      Are we not properly waiting to see if the lock file dissapears
          FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
          if (CompareFileTime(&NewDLLWriteTime, &Game.LastDLLWriteTime))
          {
            Win32UnloadGameCode(&Game);
            GlobalDebugTable = &GlobalDebugTable_;
            Game = Win32LoadGameCode(SourceGameCodeDLLFullPath,
                          TempGameCodeDLLFullPath,
                          TempGameCodeLockFullPath);
            NewInput->ExecutableReloaded = true;
          }

          END_BLOCK(LoadGameCode);

          //
          //
          //

          BEGIN_BLOCK(ProcessInput);

          // Todo: Unify controller and keyboad input into something that makes a bit more sense

          keyboard_input* OldKeyboardController = &OldInput->Keyboard;
          keyboard_input* NewKeyboardController = &NewInput->Keyboard;

          Win32ProcessKeyboard(&GlobalWin32State, OldKeyboardController, NewKeyboardController);
          Win32ProcessControllerInput(OldInput, NewInput);
          POINT MouseP;
          GetCursorPos(&MouseP);
          ScreenToClient( WindowHandle, &MouseP);
          // Transforms from Pixel Space [0,Height: Width,0] to OpenGL Viewport Space [-1,-1 : 1,1]
          NewInput->MouseX = (r32) Floor(MouseP.x+0.5f) / (r32) RenderCommands.ScreenHeightPixels;
          NewInput->MouseY = (r32) 1.f-(Floor(MouseP.y+0.5f) / (r32) RenderCommands.ScreenHeightPixels);
          NewInput->MouseZ = 0;

          DWORD WinButtonID[PlatformMouseButton_Count] =
          {
              VK_LBUTTON,
              VK_MBUTTON,
              VK_RBUTTON,
              VK_XBUTTON1,
              VK_XBUTTON2,
          };

          for(s32 ButtonIndex = 0;
            ButtonIndex < PlatformMouseButton_Count;
            ++ButtonIndex)
          {
            NewInput->MouseButton[ButtonIndex].HalfTransitionCount = OldInput->MouseButton[ButtonIndex].HalfTransitionCount;
            NewInput->MouseButton[ButtonIndex].EndedDown = OldInput->MouseButton[ButtonIndex].EndedDown;
            Win32ProcessKeyboardMessage(&NewInput->MouseButton[ButtonIndex], GetKeyState(WinButtonID[ButtonIndex])  & (1<<15));
          }

          END_BLOCK(ProcessInput);

          //
          //
          //

          thread_context Thread = {};
          if (!GlobalPause)
          {

            BEGIN_BLOCK(GameMainLoop);

            if (GlobalWin32State.RecordingIndex)
            {
              Win32RecordInput(&GlobalWin32State, NewInput);
            }

            if (GlobalWin32State.PlayingIndex)
            {
              Win32PlayBackInput(&GlobalWin32State, NewInput);
            }
            if (Game.UpdateAndRender)
            {
              Game.UpdateAndRender(&Thread, &GameMemory, &RenderCommands, NewInput);
            }

            END_BLOCK(GameMainLoop);

#if 0
            static u32 kk = 0;
            for (int i = 0; i < 6; ++i)
            {
              ++kk;
              Win32AddEntry(&HighPriorityQueue, testFun,  BootstrapPushStruct(test_struct, Arena));
              /* code */
            }

            Win32CompleteAllWork(&HighPriorityQueue);
#endif
            //
            //
            //

            BEGIN_BLOCK(ProcessSound);

            LARGE_INTEGER AudioWallClock = Win32GetWallClock();
            r32 FromBeginToAudioSeconds = Win32GetSecondsElapsed(FlipWallClock, AudioWallClock);
            r32 SecondsLeftUntillFlip = TargetSecondsPerFrame - FromBeginToAudioSeconds;

            DWORD PlayCursor;
            DWORD WriteCursor;
            if( SoundOutput.SecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)==DS_OK)
            {

            // Note, Sound will sound crackly when running through Visual Studio.
            /*  Note:
              Here is how sound output computation works.

              We define a saftey value that is the number of samples we think our game update loop may vary by (let's say 2ms)

              When we wake up to write audio, we will look and see what the playcursor position is and we will forecast ahead where we think the play cursor will be on the next frame boundary.

              We will then look to see if the write cursor is before that by at least our saftey value. If it is, the target fill position is that frame boundary plus one frame. This gives us perfect audio sync in the case of a card that has low enough latency.

              If the write cursor is _after_ that saftey margin, then we assume we can never sync the audio perfectly, so we will write one frame's worth of audio plus the saftey margin's worth of guard samples.
          */

              if (!SoundIsValid)
              {
                SoundOutput.RunningSampleIndex = WriteCursor / SoundOutput.BytesPerSampleTotal;
                SoundIsValid=true;
              }

              DWORD ExpectedSoundBytesPerFrame =
              (SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSampleTotal) / GameUpdateHz;

              DWORD ExpectedBytesUntillFlip  = (DWORD) (SecondsLeftUntillFlip * SoundOutput.BytesPerSecond);
              DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedBytesUntillFlip;

              DWORD SafeWriteCursor = WriteCursor;
              if(SafeWriteCursor < PlayCursor)
              {
                SafeWriteCursor += SoundOutput.BufferSizeInBytes;
              }
              Assert(SafeWriteCursor >= PlayCursor);
              SafeWriteCursor += SoundOutput.BytesOfLatency;
              b32 AudioCardIsLowLatency = (SafeWriteCursor < ExpectedFrameBoundaryByte);

              DWORD TargetCursor = 0;
              if(AudioCardIsLowLatency)
              {
                TargetCursor = (ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame);
              }else{
                TargetCursor = (WriteCursor + ExpectedSoundBytesPerFrame + SoundOutput.BytesOfLatency);
              }
              TargetCursor = TargetCursor % SoundOutput.BufferSizeInBytes;

              DWORD ByteToLock = ((SoundOutput.RunningSampleIndex *
                           SoundOutput.BytesPerSampleTotal) %
                         SoundOutput.BufferSizeInBytes);

              DWORD BytesToWrite = 0;
              if(ByteToLock > TargetCursor)
              {
                BytesToWrite=(SoundOutput.BufferSizeInBytes - ByteToLock);
                BytesToWrite += TargetCursor;
              }else{
                BytesToWrite = TargetCursor - ByteToLock;
              }

              game_sound_output_buffer SoundBuffer = {};
              SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
              SoundBuffer.SampleCount = BytesToWrite/SoundOutput.BytesPerSampleTotal;
              SoundBuffer.Samples = SoundSamples;
              if(Game.GetSoundSamples)
              {
                Game.GetSoundSamples(&Thread, &GameMemory, &SoundBuffer);
              }

#if HANDMADE_INTERNAL
#if 0
              win32_debug_time_marker* Marker = &DebugTimeMarkers[  DebugTimeMarkerIndex];
              Marker->OutputPlayCursor = PlayCursor;
              Marker->OutputWriteCursor = WriteCursor;
              Marker->OutputLocation = ByteToLock;
              Marker->OutputByteCount = BytesToWrite;
              Marker->ExpectedFlipPlayCursor = ExpectedFrameBoundaryByte;

              DWORD UnwrappedWriteCursor = WriteCursor;
              if(UnwrappedWriteCursor < PlayCursor)
              {
                UnwrappedWriteCursor += SoundOutput.BufferSizeInBytes;
              }

              AudioLatencyBytes = UnwrappedWriteCursor - PlayCursor;

              AudioLatencySeconds =(
                  ((r32) AudioLatencyBytes /
                  (r32) SoundOutput.BytesPerSample) /
                  (r32) SoundOutput.SamplesPerSecond);

              char txtBuffer[256];
              snprintf(txtBuffer, sizeof(txtBuffer),
              " BTL: %u, TC: %u, BTW: %u - PC: %u, WC %u, DELTA %u (%fs) \n",
               ByteToLock, TargetCursor, BytesToWrite,PlayCursor,
               WriteCursor, AudioLatencyBytes, AudioLatencySeconds);

              OutputDebugString(txtBuffer);
#endif
#endif
              Win32FillSoundBuffer(&SoundOutput, ByteToLock,
                           BytesToWrite, &SoundBuffer);

            }else{
              SoundIsValid = false;
            }

            END_BLOCK(ProcessSound);

            //
            //
            //

            BEGIN_BLOCK(ProcessRenderQueue);

            OpenGLRenderGroupToOutput( &RenderCommands );

            END_BLOCK(ProcessRenderQueue);

            //
            //
            //

            BEGIN_BLOCK(FrameWait);

            LARGE_INTEGER WorkCounter = Win32GetWallClock();
            r32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);
            DWORD SleepMS = 0;
            r32 SecondsElapsedForFrame = WorkSecondsElapsed;
            if(SecondsElapsedForFrame<TargetSecondsPerFrame)
            {
              if(SleepIsGranular)
              {
                SleepMS = (DWORD) ( 1000.f *
                     (TargetSecondsPerFrame - SecondsElapsedForFrame));
                if(SleepMS>0)
                {
                  /*
                    NOTE: Sleep() is unaccurate which causes us to skipp frames.
                      For now we use the while loop below to hit our target
                      frame rate. Need to tighten up audio buffering to avoid
                      skips, see:
                      https://hero.handmadedev.org/forum/code-discussion/137-sleep-problem-causing-audio-pops
                  */
                  Sleep(SleepMS);
                }
              }

              r32 TestSecondsElapsedForFrame =  Win32GetSecondsElapsed(LastCounter, Win32GetWallClock() );

              if(TestSecondsElapsedForFrame < TargetSecondsPerFrame)
              {
                //TODO: Log missed framerate
              }

              while(SecondsElapsedForFrame<TargetSecondsPerFrame)
              {
                SecondsElapsedForFrame =  Win32GetSecondsElapsed( LastCounter,Win32GetWallClock() );
              }
            }else{
              // TODO: MISSED FRAME RATE
              // TODO: Logging
            }

            END_BLOCK(FrameWait);

            //
            //
            //

            BEGIN_BLOCK(SwapBuffers);
            // Update Window
            DeviceContext = GetDC(WindowHandle);
            SwapBuffers(DeviceContext);
            ReleaseDC(WindowHandle, DeviceContext);
            END_BLOCK(SwapBuffers);


            FlipWallClock = Win32GetWallClock();

            // TODO: Should We clear hese?
            game_input* Temp = NewInput;
            NewInput = OldInput;
            OldInput = Temp;

#if 0
#if HANDMADE_INTERNAL
            // Note: This is debug code
            {
            // Note this is wrong on zeroth index
            // Win32DebugSyncDisplay(&GlobalBackBuffer, ArrayCount(DebugTimeMarkers),  DebugTimeMarkers, DebugTimeMarkerIndex-1, &SoundOutput,   TargetSecondsPerFrame);
              DWORD DebugPlayCursor;
              DWORD DebugWriteCursor;
              if (SoundOutput.SecondaryBuffer->GetCurrentPosition(&DebugPlayCursor,  &DebugWriteCursor)==DS_OK)
              {
                Assert(DebugTimeMarkerIndex <  ArrayCount(DebugTimeMarkers));
                win32_debug_time_marker* Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
                Marker->FlipPlayCursor = DebugPlayCursor;
                Marker->FlipWriteCursor = DebugWriteCursor;
              }
            }
#endif
#endif

            r32 FrameMarkerSecondsElapsed = 0;
            NewInput->dt = TargetSecondsPerFrame;

#if HANDMADE_INTERNAL
            BEGIN_BLOCK(DebugCollation);
            if(Game.DEBUGGameFrameEnd)
            {
              GlobalDebugTable = Game.DEBUGGameFrameEnd(&GameMemory);
            }
            GlobalDebugTable_.EventArrayIndex_EventIndex = 0;
            END_BLOCK(DebugCollation);
#endif
            LARGE_INTEGER EndCounter = Win32GetWallClock();
            r32 SecondsElapsed = Win32GetSecondsElapsed(LastCounter, EndCounter);
            FRAME_MARKER(SecondsElapsed);
            LastCounter = EndCounter;
            if(GlobalDebugTable)
            {
              GlobalDebugTable->RecordCount[TRANSLATION_UNIT_INDEX] = __COUNTER__;
            }
          } // Global Pause
        } // Global Running
      }else{

        //  TODO: Logging
      }
    }else{
    //  TODO: Logging
    }
  }else{
   // TODO: Logging
  }

  return 0;
}