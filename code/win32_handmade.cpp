/*
  TODO:
  - Saved game locations
  - Getting a handle to our own executable file
  - Asset loading path
  - Threading (launch a thread)
  - Raw Input (support for multiple keyboards)
  - Sleep/timeBeginPeriod
  - ClipCursor() (for multimonitor support)
  - Fullscreen support
  - WM_SETCURSOR (control cursor visibility)
  - QueryCancelAutoplay
  - WM_ACTIVATEAPP (for when we are not the active application)
  - Blit speed improvements (BitBlt)
  - Hardware acceleration (OpenGL or Direct3D or BOTH??)
  - GetKeyboardLayout (for French keyboards, international WASD support)

  Just a partial list of stuff!!
*/

// Note(Jakob): If you have windows questions, look for answers given by Raymond Chen


//#include "handmade.h"

#include "handmade_platform.h"

#include <windows.h>
#include <xinput.h>
#include <dsound.h>

#include <stdio.h>

#include "win32_handmade.h"



// TODO: Clobal for now
global_variable bool32 GlobalRunning;
global_variable bool32 GlobalPause; 
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable int64 GlobalPerfCounterFrequency;
global_variable bool32 DEBUGGlobalShowCursor;
global_variable WINDOWPLACEMENT GlobalWindowPosition = {sizeof(GlobalWindowPosition)};


	// Note(Jakob): Caseys native screen resolution is 960 by 540 times two
	// Note(Jakob): My native screen resolution is 840 by 525 times two
#define MONITOR_HEIGHT 525
#define MONITOR_WIDTH 840


// Note: 	We don't want to rely on the libraries needed to run the functions
//			XInputGetState and XInputSetState defined in <xinput.h> as they have
//			poor support on different windows versions. See handmadeHero ep 006;


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
	HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE, NULL, NULL, 
											  CREATE_ALWAYS, NULL, NULL);

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

internal int
StringLength(char* String)
{
	// foo++ -> increment after
	// ++foo -> increment before
	int Count = 0;
	while(*String++)
	{
		++Count;
	}
	return Count;
}


internal void
CatStrings(	size_t SourceACount, char* SourceA,
			size_t SourceBCount, char* SourceB,
			size_t DestCount, char* Dest)
{

	for(int Index = 0; Index < SourceACount; ++Index)
	{
		*Dest++ = *SourceA++;
	}


	for(int Index = 0; Index < SourceBCount; ++Index)
	{
		*Dest++ = *SourceB++;
	}

	*Dest++ ='\0';
}


internal void
Win32BuildEXEPathFileName(win32_state* State, char* FileName,
						int DestCount, char* Dest )
{
	CatStrings(State->OnePastLastSlashEXEFileName - State->EXEFileName, State->EXEFileName, 
				StringLength(FileName), FileName,
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
		CopyFile(SourceDLLName,TempDLLName,FALSE);
		Result.GameCodeDLL = LoadLibraryA(TempDLLName);
		if(Result.GameCodeDLL)
		{
			Result.UpdateAndRender = (game_update_and_render* )
				GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");
			
			Result.GetSoundSamples = (game_get_sound_samples* )\
				GetProcAddress(Result.GameCodeDLL, "GameGetSoundSamples");
		
			Result.IsValid = (Result.UpdateAndRender && Result.GetSoundSamples);
		}
	}
	if(!Result.IsValid)
	{
		Result.UpdateAndRender = 0;
		Result.GetSoundSamples = 0;
	}

	return Result;
}

internal void
Win32UnloadGameCode(win32_game_code* GameCode)
{
	if(GameCode->GameCodeDLL)
	{
		FreeLibrary(GameCode->GameCodeDLL);
		GameCode->GameCodeDLL = 0;
	}

	GameCode->IsValid = false;
	GameCode->UpdateAndRender = 0;
	GameCode->GetSoundSamples = 0;

}

internal void 
Win32LoadXInput(void) 
{  
	// Note: 	There are three versions of XInput at the time of writing; 
	// 			XInput 1.4 is bundled with Windows 8, 
	//			XInput 1.3 is bundled with Windows 7 and 
	//			XInput 9.1.0 is a generalized version with some features added 
	// 			and some removed, which can be used across platforms. 

	char *XInputDLLs[] = {"xinput1_4.dll","xinput1_3.dll","Xinput9_1_0.dll"};   
	int XInputDLLCount = sizeof(XInputDLLs)/sizeof(XInputDLLs[0]);
	HMODULE XInputLibrary;   
	for (int DLLIndex = 0; DLLIndex < XInputDLLCount; DLLIndex++) 
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
Win32InitDSound(HWND Window, win32_sound_output* SoundOutput)
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
			WaveFormat.nChannels = (WORD) SoundOutput->ChannelCount;
			WaveFormat.nSamplesPerSec =(WORD) SoundOutput->SamplesPerSecond;
			WaveFormat.wBitsPerSample =(WORD) SoundOutput->BytesPerSample * 8; // cd-quality
			// EXP: Audio samples for 2 channels is stored as
			// 		LEFT RIGHT LEFT RIGHT LEFT RIGHT etc.
			//		nBlockAlign wants the size of one [LEFT RIGHT]
			//		block in bytes. For us this becomes
			//		(2 channels) X (nr of bits per sample) / (bits in a byte) = 
			//			2 * 16 /8 = 4 bytes per sample.
			WaveFormat.nBlockAlign =(WORD) (SoundOutput->ChannelCount * SoundOutput->BytesPerSample);
			WaveFormat.nAvgBytesPerSec = 
				WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
			WaveFormat.cbSize = 0;

			if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)) )
			{
				DSBUFFERDESC BufferDescription = {};
				BufferDescription.dwSize = sizeof(BufferDescription);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
					
				// NOTE: "Create" a primary buffer
				// EXP:	 The primary buffer is only for setting the format of our primary
				// 		 sound device to what we want. The secondary buffer is the actual
				// 		 buffer we will write.
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
//			BufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
			BufferDescription.dwFlags = 0;
			BufferDescription.dwBufferBytes = SoundOutput->BufferSizeInBytes;
			BufferDescription.lpwfxFormat = &WaveFormat;
			HRESULT Error =DirectSound->CreateSoundBuffer
								(&BufferDescription ,&SoundOutput->SecondaryBuffer,0);
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

internal win32_window_dimension Win32GetWindowDimension( HWND Window )
{
	win32_window_dimension Result;

	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height =  ClientRect.bottom - ClientRect.top;
	
	return Result;
}


internal void 
Win32ResizeDIBSection(win32_offscreen_buffer* Buffer, int Width, int Height)
{
	if(Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, NULL, MEM_RELEASE);
	}

	Buffer->Width = Width;
	Buffer->Height = Height;
	
	Buffer->BytesPerPixel = 4;
	// Note: When biHeight is set to negative it is the cue to the
	// compiler that we are to draw in a top down coordinate system
	// where our screen origin is in the top left corner.
	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;	
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;



	int BitmapMemorySize = (Buffer->Width*Buffer->Height)*Buffer->BytesPerPixel;
	Buffer->Memory = VirtualAlloc(NULL, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel;


	// TODO: Probably clear this to black

}

internal void
Win32DisplayBufferInWindow(win32_offscreen_buffer* Buffer,
					HDC DeviceContext, int WindowWidth, int WindowHeight )
{	

	//TODO(Jakob): Fix so that aspect ration always is kept when resizing

	if( (WindowWidth >= Buffer->Width*2) && 
		(WindowHeight >= Buffer->Height*2) )
	{
		// Note: No Stretching for prorotyping purposes. 
		StretchDIBits(	DeviceContext, 
						0,0, WindowWidth,WindowHeight,
						0,0, Buffer->Width, Buffer->Height,
						Buffer->Memory,
						&Buffer->Info,
					    DIB_RGB_COLORS, SRCCOPY);
	}else{
		int OffsetX = 10;
		int OffsetY = 10;
		PatBlt(DeviceContext,Buffer->Width+OffsetX, 0 , WindowWidth, WindowHeight, BLACKNESS);
		PatBlt(DeviceContext, 0, Buffer->Height+OffsetY, WindowWidth, WindowHeight, BLACKNESS);
		PatBlt(DeviceContext, 0, 0, OffsetX, WindowHeight, BLACKNESS);
		PatBlt(DeviceContext, 0, 0, WindowWidth, OffsetY, BLACKNESS);
		/*
		StretchDIBits(	DeviceContext, 
						0,0, WindowWidth, WindowHeight,
						0,0, Buffer->Width, Buffer->Height,
						Buffer->Memory,
						&Buffer->Info,
					    DIB_RGB_COLORS, SRCCOPY);
		*/
		// Note: No Stretching for prorotyping purposes. 
		StretchDIBits(	DeviceContext, 
						OffsetX,OffsetY, Buffer->Width, Buffer->Height,
						0,0, Buffer->Width, Buffer->Height,
						Buffer->Memory,
						&Buffer->Info,
					    DIB_RGB_COLORS, SRCCOPY);
	}

}


internal LRESULT CALLBACK 
MainWindowCallback(	HWND Window,
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
				SetLayeredWindowAttributes(Window, RGB(0,0,0), 255, LWA_ALPHA);
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
			Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, 
								Dimension.Width, Dimension.Height);

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
Win32ClearSoundBuffer(win32_sound_output* SoundOutput)
{
	VOID* Region1;
	DWORD Region1Size;
	VOID* Region2;
	DWORD Region2Size;
	if( SUCCEEDED(SoundOutput->SecondaryBuffer->Lock(0, SoundOutput->BufferSizeInBytes,
				        						 &Region1, &Region1Size,
				        						 &Region2, &Region2Size,
				        						 0)))
	{
		uint8* DestSample = ( uint8* ) Region1;
		for(DWORD ByteIndex = 0; 
			ByteIndex < Region1Size; 
			++ByteIndex)
		{
			*DestSample++ = 0;
		}
		
		DestSample = ( uint8* ) Region2;
		for(DWORD ByteIndex = 0; 
			ByteIndex < Region2Size; 
			++ByteIndex)
		{
			*DestSample++ = 0;
		}
	
		SoundOutput->SecondaryBuffer->Unlock(Region1, Region1Size,
				Region2, Region2Size);
	}
};

internal void
Win32FillSoundBuffer(win32_sound_output* SoundOutput, DWORD ByteToLock, DWORD BytesToWrite, game_sound_output_buffer* SourceBuffer)
{
	VOID* Region1;
	DWORD Region1Size;
	VOID* Region2;
	DWORD Region2Size;
	if( SUCCEEDED(SoundOutput->SecondaryBuffer->Lock( ByteToLock, BytesToWrite,
				        						 &Region1, &Region1Size,
				        						 &Region2, &Region2Size,
				        						 0)))
	{
		// TODO: assert that region1size / region2size is valid
		DWORD Region1SampleCount = Region1Size/SoundOutput->BytesPerSampleTotal;
		int16* DestSample = ( int16* ) Region1;
		int16* SourceSample = SourceBuffer->Samples;
		
		for(DWORD SampleIndex = 0; 
			SampleIndex < Region1SampleCount; 
			++SampleIndex)
		{
			*DestSample++ = *SourceSample++; // Left Speker
			*DestSample++ = *SourceSample++; // Rright speaker
			SoundOutput->RunningSampleIndex++;
		}
		
		DWORD Region2SampleCount = Region2Size/SoundOutput->BytesPerSampleTotal;
		DestSample = (int16*) Region2;
		for(DWORD SampleIndex = 0; 
			SampleIndex < Region2SampleCount;
			++SampleIndex)
		{
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			SoundOutput->RunningSampleIndex++;
		}
		

		SoundOutput->SecondaryBuffer->Unlock(Region1, Region1Size,Region2, Region2Size);
	}
}


internal void
Win32GetInputFileLocation(win32_state* State, bool32 InputStream, 
						int SlotIndex, int DestCount, char* Dest)
{
	char Temp[64];
	wsprintf(Temp, "loop_edit_%d_%s.dmi", SlotIndex, InputStream? "input" : "state" );
	Win32BuildEXEPathFileName(State, Temp, DestCount, Dest );
}

internal win32_replay_buffer*
Win32GetReplayBuffer(win32_state *State, int unsigned Index)
{
	Assert(Index-1 < ArrayCount(State->ReplayBuffer));
	Assert(Index-1 >= 0);
	win32_replay_buffer* Result = &State->ReplayBuffer[Index-1];
	return Result;
}

internal void
Win32BeginRecordingInput(win32_state* State, int RecordingIndex)
{
	win32_replay_buffer* ReplayBuffer  = Win32GetReplayBuffer(State, RecordingIndex);
	if(ReplayBuffer->MemoryBlock)
	{
		State->RecordingIndex = RecordingIndex;

		char FileName[WIN32_STATE_FILE_NAME_COUNT];
		Win32GetInputFileLocation(State, true, RecordingIndex,
 				sizeof(FileName), FileName);

		State->RecordingHandle = CreateFileA(FileName, GENERIC_WRITE, 
											0, 0, CREATE_ALWAYS,0,0);
#if 0
		LARGE_INTEGER FilePosition;
		FilePosition.QuadPart = State->TotalSize;
		SetFilePointerEx( State->RecordingHandle, FilePosition, 0, FILE_BEGIN);
#endif
		CopyMemory(ReplayBuffer->MemoryBlock, State->GameMemoryBlock, State->TotalSize);
	}
}

internal void
Win32EndRecordingInput(win32_state* State)
{
	CloseHandle(State->RecordingHandle);
	State->RecordingIndex = 0;
}
internal void
Win32BeginInputPlayback(win32_state* State, int PlayingIndex)
{
	win32_replay_buffer* ReplayBuffer  = Win32GetReplayBuffer(State, PlayingIndex);
	if(ReplayBuffer->MemoryBlock)
	{
		State->PlayingIndex = PlayingIndex;

		char FileName[WIN32_STATE_FILE_NAME_COUNT];
		Win32GetInputFileLocation(State, true, PlayingIndex,
 				sizeof(FileName), FileName);
		State->PlaybackHandle = CreateFileA(FileName, GENERIC_READ, 
											0, 0, OPEN_EXISTING,0,0);
#if 0
		LARGE_INTEGER FilePosition;
		FilePosition.QuadPart = State->TotalSize;
		SetFilePointerEx( State->PlaybackHandle, FilePosition, 0, FILE_BEGIN);
#endif
		CopyMemory(State->GameMemoryBlock, ReplayBuffer->MemoryBlock, State->TotalSize);
	}
}

internal void
Win32EndInputPlayback(win32_state* State)
{
	CloseHandle(State->PlaybackHandle);
	State->PlayingIndex=0;
}

internal void
Win32RecordInput(win32_state* State, game_input* NewInput)
{
	DWORD BytesWritten;
	WriteFile(State->RecordingHandle, NewInput, sizeof(*NewInput), &BytesWritten, 0);
}
 
internal void
Win32PlayBackInput(win32_state* State, game_input* NewInput)
{
	DWORD BytesRead;
	if(ReadFile(State->PlaybackHandle, NewInput, sizeof(*NewInput), &BytesRead, 0))
	{
		if(BytesRead==0)
		{
			int PlayingIndex = State->PlayingIndex;
			Win32EndInputPlayback(State);
			Win32BeginInputPlayback(State, PlayingIndex);
			ReadFile(State->PlaybackHandle, NewInput, sizeof(*NewInput), &BytesRead, 0);
		}
	}
}



internal void 
Win32ProcessXInputDigitalButton(DWORD XInputButtonState, game_button_state* OldState, DWORD ButtonBit, game_button_state* NewState )
{
	NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
	NewState->HalfTransitionCount = 
		(OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
						
};

internal void
Win32ProcessKeyboardMessage( game_button_state* NewState, bool32 IsDown)
{
	if(NewState->EndedDown != IsDown)
	{
		NewState->EndedDown = IsDown;
		++NewState->HalfTransitionCount;
	}
}

internal real32 
Win32ProcessXinputStickValue(SHORT Value, SHORT DeadZoneThershold)
{
	real32 Result = 0;

	if(Value < -DeadZoneThershold)
	{
		Result = (real32)(Value + DeadZoneThershold) / (32768.0f - DeadZoneThershold);
	}else if( Value > DeadZoneThershold){
		Result = (real32)(Value - DeadZoneThershold) / (32767.0f - DeadZoneThershold);
	}
	return Result;
}

internal void
Win32ProcessXinputStickValue(	real32* X, 
								real32* Y,
								SHORT Value_X,
								SHORT Value_Y,
								SHORT DeadZoneThershold)
{
	real32 normalizer = 1 / 32768.0f;
	real32 normalized_deadzone= 1.f / ( (real32) DeadZoneThershold );
	real32 normalized_deadzone_squared = normalized_deadzone*normalized_deadzone;

	real32 x_norm = Value_X * normalizer;
	real32 y_norm = Value_Y * normalizer;
	real32 stick_offset_squared = x_norm*x_norm + y_norm*y_norm;

	if(stick_offset_squared > normalized_deadzone_squared)
	{
		*X = (real32)(Value_X) * normalizer;
		*Y = (real32)(Value_Y) * normalizer;
	}else{
		*X = 0;
		*Y = 0;
	}
}

// Note(Jakob): Courtesy of Raymond Chen,
//				See: https://blogs.msdn.microsoft.com/oldnewthing/20100412-00/?p=14353
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


// TODO: Handle keyboard separately from gamepad as a key-map
// TODO: Handle Mouse input
internal void 
Win32ProcessKeyboard(win32_state* State,  game_controller_input*  OldKeyboardController, game_controller_input* KeyboardController)
{
	*KeyboardController = {};
	KeyboardController->IsConnected = true;
	for(int ButtonIndex = 0; 
		ButtonIndex  < ArrayCount(KeyboardController->Button);
		++ButtonIndex)
	{
		KeyboardController->Button[ButtonIndex].EndedDown =
			OldKeyboardController->Button[ButtonIndex].EndedDown;
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
				uint32 VKCode = (uint32)Message.wParam;
				#define KeyMessageWasDownBit (1<<30)
				#define KeyMessageIsDownBit (1<<31)
				bool WasDown = ((Message.lParam & KeyMessageWasDownBit) != 0);
				bool IsDown = ((Message.lParam &  KeyMessageIsDownBit) == 0);

				if(WasDown != IsDown)
				{

					if(VKCode == 'W')
					{
						Win32ProcessKeyboardMessage(
								&KeyboardController->LeftStickUp, IsDown);
//								&KeyboardController->LeftStickAverageY, 1.0);
					
					}else if(VKCode == 'A'){
						Win32ProcessKeyboardMessage(
								&KeyboardController->LeftStickLeft, IsDown);
//								&KeyboardController->LeftStickAverageX, -1.0);
						
					}else if(VKCode == 'S'){
						Win32ProcessKeyboardMessage(
								&KeyboardController->LeftStickDown, IsDown);
//								&KeyboardController->LeftStickAverageY, -1.0);
					}else if(VKCode == 'D'){
						Win32ProcessKeyboardMessage(
								&KeyboardController->LeftStickRight, IsDown);
//								&KeyboardController->LeftStickAverageX, 1.0);
					}else if(VKCode == 'Q'){
						Win32ProcessKeyboardMessage(
								&KeyboardController->LeftShoulder, IsDown);

					}else if(VKCode == 'E'){
						Win32ProcessKeyboardMessage(  
								&KeyboardController->RightShoulder, IsDown);

					}else if(VKCode == VK_UP){
						Win32ProcessKeyboardMessage(
								&KeyboardController->RightStickUp, IsDown);
//								&KeyboardController->RightStickAverageY, 1.0);
					}else if(VKCode == VK_LEFT){
						Win32ProcessKeyboardMessage(
								&KeyboardController->RightStickLeft, IsDown);
//								&KeyboardController->RightStickAverageX, -1.0);
					}else if(VKCode == VK_DOWN){
						Win32ProcessKeyboardMessage(
								&KeyboardController->RightStickDown, IsDown);
//								&KeyboardController->RightStickAverageY, -1.0);
					}else if(VKCode == VK_RIGHT){
						Win32ProcessKeyboardMessage(
								&KeyboardController->RightStickRight, IsDown);
//								&KeyboardController->RightStickAverageX, 1.0);
					}else if(VKCode == VK_ESCAPE){					
						Win32ProcessKeyboardMessage(
								&KeyboardController->Select, IsDown);
						GlobalRunning = false;
					}else if(VKCode == VK_SPACE){	
						Win32ProcessKeyboardMessage(
								&KeyboardController->Start, IsDown);
					}

#if 0
					// TODO: Map all the other buttons to the keyboard
					else if(VKCode == ){
						Win32ProcessKeyboardMessage(
								&KeyboardController->DPadUp, IsDown);
					}else if(VKCode == ){
						Win32ProcessKeyboardMessage(
								&KeyboardController->DPadLeft, IsDown);
					}else if(VKCode == ){
						Win32ProcessKeyboardMessage(
								&KeyboardController->DPadDown, IsDown);
					}else if(VKCode == ){
						Win32ProcessKeyboardMessage(
								&KeyboardController->DPadRight, IsDown);
					}else if(VKCode == ){
						Win32ProcessKeyboardMessage(
								&KeyboardController->RightTrigger, IsDown);
//								&KeyboardController->RightTriggerAverage, 1.0);
					}else if(VKCode == ){
						Win32ProcessKeyboardMessage(
								&KeyboardController->LeftTrigger, IsDown);
//								&KeyboardController->LeftTriggerAverage, 1.0);
					}else if(VKCode == ){
						Win32ProcessKeyboardMessage(
								&KeyboardController->A, IsDown);
					}else if(VKCode == ){
						Win32ProcessKeyboardMessage(
								&KeyboardController->B, IsDown);
					}else if(VKCode == ){
						Win32ProcessKeyboardMessage(
								&KeyboardController->X, IsDown);
					}else if(VKCode == ){
						Win32ProcessKeyboardMessage(
								&KeyboardController->Y, IsDown);
					}else if(VKCode == ){
						Win32ProcessKeyboardMessage(
								&KeyboardController->RightStick, IsDown);
					}else if(VKCode == ){
					
						Win32ProcessKeyboardMessage(
								&KeyboardController->LeftStick, IsDown);
					}
#endif
					
#if HANDMADE_INTERNAL
					else if(VKCode == 'P'){
						if(IsDown)
						{
							GlobalPause = !GlobalPause;
						}
					}else if(VKCode == 'L'){
						if(IsDown)
						{
							if(State->PlayingIndex == 0)
							{
							
								if(State->RecordingIndex == 0)
								{
									Win32BeginRecordingInput(State, 1);
								}else{
									Win32EndRecordingInput(State);
									Win32BeginInputPlayback(State,1);
								}
							}else{
								Win32EndInputPlayback(State);
								*KeyboardController ={};
							}
						}
					}
#endif
					if(IsDown)
					{
						bool AltKeyWasDown = ((Message.lParam & (1<<29)) != 0);
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
Win32ProcessControllerInput( game_input* OldInput,
							game_input* NewInput)
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
		game_controller_input* OldController = 
								GetController(OldInput, OurControllerIndex); 
		game_controller_input* NewController = 
								GetController(NewInput, OurControllerIndex); 
		// Note: Performancebug in XInputGetControllerstate.
		//		 See HH007 at 15 min. To be ironed out in optimization
		XINPUT_STATE ControllerState;
		if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
		{
			// NOTE: This controller is plugged in
			// TODO: See if ControllerState.dwPacketNumber increment too rapidly
			XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
		
			real32 StickThreshhold = 0.0f;
			real32 TriggerThreshhold = 0.5f;
	
			NewController->IsAnalog = true;
			NewController->IsConnected = true;
			
			// Todo: Check if controller deadzone is round or rectangular
			//		 Right now it is treated as rectangular inside 
			//		 Win32ProcessXinputStickValue
	
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
			Win32ProcessXinputStickValue( 	&(NewController->RightStickAverageX),
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
	
			NewController->LeftTriggerAverage = (real32)Pad->bLeftTrigger/255.0f;
			Win32ProcessXInputDigitalButton(
					(NewController->LeftTriggerAverage > TriggerThreshhold) ? 1 : 0, 
					&OldController->LeftTrigger, 1, 
					&NewController->LeftTrigger);
			
			NewController->RightTriggerAverage =(real32)Pad->bLeftTrigger/255.0f;
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
					&OldController->LeftShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER, 
					&NewController->LeftShoulder);
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

inline real32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
	real32 Result =  ((real32)(End.QuadPart - Start.QuadPart) / (real32)GlobalPerfCounterFrequency );
	return Result;
}

internal void
Win32DebugDrawVertical(win32_offscreen_buffer* BackBuffer, 
						int X, int Top, int Bottom, uint32 Color)
{
	if(Top < 0)
	{
		Top = 0;
	}

	if(Bottom > BackBuffer->Height)
	{
		Bottom = BackBuffer->Height; 
	}

	if(( X>=0 ) && ( X<BackBuffer->Width ))
	{
		uint8* Pixel =  ((uint8*) BackBuffer->Memory + 
						X*BackBuffer->BytesPerPixel + 
						Top*BackBuffer->Pitch);

		for(int Y = Top; Y<Bottom; ++Y)
		{
			*(uint32*) Pixel = Color;
			Pixel += BackBuffer->Pitch;
		}
	}
}

inline void 
Win32DebugDrawSoundBufferMarker(win32_offscreen_buffer* BackBuffer,
						win32_sound_output* SoundOutput, real32 C, 
								int PadX, int Top, int Bottom,
								uint32 Color, DWORD Value)
{
	real32 XReal32 = ( C * (real32) Value);\

	int X = PadX +  (int)  XReal32;

	Win32DebugDrawVertical(BackBuffer, X, Top, Bottom, Color);	
}
#if 0
internal void
Win32DebugSyncDisplay(win32_offscreen_buffer* BackBuffer, 
						int MarkerCount,win32_debug_time_marker* Markers, 
						int CurrentMarkerIndex, win32_sound_output* SoundOutput, real32 TargetSecondsPerFrame)
{
	int PadX = 16;
	int PadY = 16;

	int LineHeight = 64;
 
	real32 C = (real32) (BackBuffer->Width-2*PadX) / 
						(real32)SoundOutput-> BufferSizeInBytes;

	for(int MarkerIndex=0; MarkerIndex < MarkerCount; ++MarkerIndex )
	{

		win32_debug_time_marker* ThisMarker = &Markers[MarkerIndex]; 

		Assert(ThisMarker->OutputPlayCursor < SoundOutput->BufferSizeInBytes);
		Assert(ThisMarker->OutputWriteCursor < SoundOutput->BufferSizeInBytes);
		Assert(ThisMarker->OutputLocation < SoundOutput->BufferSizeInBytes);
		Assert(ThisMarker->OutputByteCount < SoundOutput->BufferSizeInBytes);
		Assert(ThisMarker->FlipPlayCursor < SoundOutput->BufferSizeInBytes);
		Assert(ThisMarker->FlipWriteCursor < SoundOutput->BufferSizeInBytes);
	
		int Top = PadY;
		int Bottom = PadY+LineHeight;

		DWORD PlayColor = 0xFFFFFFFF;
		DWORD WriteColor = 0xFFFF0000;
		DWORD ExpectedFlipColor = 0xFFFFFF00;
		DWORD PlayWindowColor = 0xFFFF00FF;

		if(MarkerIndex ==  CurrentMarkerIndex)
		{
			Top += LineHeight+PadY;
			Bottom += LineHeight+PadY;

			int FirstTop = Top;

			Win32DebugDrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom,
											PlayColor, ThisMarker->OutputPlayCursor);
			Win32DebugDrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom,
											WriteColor, ThisMarker->OutputWriteCursor);




			Top += LineHeight+PadY;
			Bottom += LineHeight+PadY;
			Win32DebugDrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom,
											PlayColor, ThisMarker->OutputLocation);
			Win32DebugDrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom,
											WriteColor, 
											ThisMarker->OutputLocation + 
											ThisMarker->OutputByteCount);


			Top += LineHeight+PadY;
			Bottom += LineHeight+PadY;
			
			Win32DebugDrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, FirstTop, Bottom,
											ExpectedFlipColor, 
											ThisMarker->ExpectedFlipPlayCursor);



		}
		Win32DebugDrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom,
										PlayColor, ThisMarker->FlipPlayCursor);
		Win32DebugDrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom,
										PlayWindowColor, ThisMarker->FlipPlayCursor+480*SoundOutput->BytesPerSample);
		Win32DebugDrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom,
										WriteColor, ThisMarker->FlipWriteCursor);
	}
}

#endif

int CALLBACK 
WinMain(HINSTANCE Instance, 
			HINSTANCE PrevInstance, 
			LPSTR CommandLine, 
			int ShowCode )
{

	win32_state Win32State = {};

	Win32GetEXEFileName(&Win32State);
	
	char SourceGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
	Win32BuildEXEPathFileName(&Win32State, "handmade.dll",
						sizeof(SourceGameCodeDLLFullPath), SourceGameCodeDLLFullPath );
	
	char TempGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
	Win32BuildEXEPathFileName(&Win32State, "handmade_temp.dll",
						sizeof(TempGameCodeDLLFullPath), TempGameCodeDLLFullPath );
	

	char TempGameCodeLockFullPath[WIN32_STATE_FILE_NAME_COUNT];
	Win32BuildEXEPathFileName(&Win32State, "lock.tmp",
						sizeof(TempGameCodeLockFullPath), TempGameCodeLockFullPath );

	LARGE_INTEGER PerfCounterFrequencyResult;
	QueryPerformanceFrequency(&PerfCounterFrequencyResult);
	GlobalPerfCounterFrequency = PerfCounterFrequencyResult.QuadPart;

	// Note: 	sets the Windows scheduler granularity (The max time resolution of Sleep() 
	//			- function) to 1 ms.
	UINT DesiredSchedulerMS = 1;
	bool32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

	Win32LoadXInput();

#if HANDMADE_INTERNAL
	DEBUGGlobalShowCursor = true;
#endif
	WNDCLASSA WindowClass = {};
	
	Win32ResizeDIBSection(&GlobalBackBuffer, MONITOR_WIDTH, MONITOR_HEIGHT);

	WindowClass.style = CS_HREDRAW|CS_VREDRAW;//|CS_OWNDC;
	WindowClass.lpfnWndProc = MainWindowCallback;
	WindowClass.hInstance = PrevInstance; 
	WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
// 	WindowClass.hIcon;	// window icon (for later)
	WindowClass.lpszClassName = "HandmadeWindowClass";	// Name for the window class

	if(RegisterClass(&WindowClass))
	{
		HWND Window = CreateWindowEx(
				0,//WS_EX_TOPMOST|WS_EX_LAYERED,
				WindowClass.lpszClassName,
				"HandMade",
				WS_OVERLAPPEDWINDOW|WS_VISIBLE,
				CW_USEDEFAULT, 
				CW_USEDEFAULT, 
				CW_USEDEFAULT, 
				CW_USEDEFAULT, 
				0,
				0,
				Instance,
				NULL);


		if(Window != NULL)
		{

			// TODO: How do we reliably query this on windows?
			int MonitorRefreshHz = 60;
			HDC DC = GetDC(Window);
			int Win32RefreshRate = GetDeviceCaps(DC,VREFRESH);
			ReleaseDC(Window, DC);
			if(Win32RefreshRate > 1)
			{
				MonitorRefreshHz = Win32RefreshRate;
			}
			int GameUpdateHz = 60;
			real32 TargetSecondsPerFrame = 1.0f / (real32) GameUpdateHz;


			win32_sound_output SoundOutput = {};

			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.ChannelCount = 2;
			SoundOutput.BytesPerSample = sizeof(int16); // 2 bytes
			SoundOutput.BufferSizeInSeconds = 1;
			SoundOutput.TargetSecondsOfLatency = (real32) (1.0/60.0); // 2 frames at 60 fps = 0.033 seconds.

			SoundOutput.BytesPerSampleTotal = SoundOutput.ChannelCount * SoundOutput.BytesPerSample;
			SoundOutput.BytesPerSecond = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSampleTotal;
			SoundOutput.BufferSizeInBytes = SoundOutput.BufferSizeInSeconds * SoundOutput.BytesPerSecond;

			SoundOutput.BytesOfLatency = (DWORD) (SoundOutput.TargetSecondsOfLatency  * (real32) SoundOutput.BytesPerSecond );
			Win32InitDSound(Window, &SoundOutput);
			
			Win32ClearSoundBuffer(&SoundOutput);
			SoundOutput.SecondaryBuffer->Play(0,0, DSBPLAY_LOOPING);

			// TODO: Pool with Bitmap VirtualAlloc
			int16* SoundSamples = (int16*) VirtualAlloc(0, 
											SoundOutput.BufferSizeInBytes,
											MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

			
#if HANDMADE_INTERNAL
			LPVOID BaseAddress =(LPVOID) Terrabytes(2);
#else
			LPVOID BaseAddress = 0;
#endif

			game_memory GameMemory = {};
			GameMemory.PermanentStorageSize = Megabytes(64);
			GameMemory.TransientStorageSize = Gigabytes(1);
			GameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;
			GameMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
			GameMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;

			// TODO: Use MEM_LARGE_PAGES and call adjust token privelages when not on XP.
			Win32State.TotalSize = GameMemory.PermanentStorageSize +
								GameMemory.TransientStorageSize;
			Win32State.GameMemoryBlock = VirtualAlloc(BaseAddress, (size_t)Win32State.TotalSize,
											MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);	
			GameMemory.PermanentStorage = Win32State.GameMemoryBlock;
			GameMemory.TransientStorage = ((uint8*) GameMemory.PermanentStorage + 
								GameMemory.PermanentStorageSize);


			for(int ReplayIndex = 0;
				ReplayIndex < ArrayCount(Win32State.ReplayBuffer);
				++ReplayIndex)
			{
				
				/* Note (Jakob): For my own educational purposes:
					CreateFileMapping wants two 32 bit values which corresponds to the higher and lower 
					bits separately from Win32State->TotalSize which is a 64 bit. This is for compatibility
					reasons between 32 and 64 bit systems.
					In other words it wants us to split the 64 bit value into 2 32 bit values.
					The high bit is extracted by bitshifting the TotalSize down by 32 bits and then
					truncate it.
					The low bit is extracted by using bit-and operator with a 32 bit value set to
					[111111 .... 1111] == 0xFFFFFFFF. Clever. Example with 16bit int: [00101101 10010101]
					HIGH BIT: [00101101 10010101] >> 32 = [10101010 00101101] 
								=> (Cast to int8) => [00101101] 
					LOW BIT: [00101101 10010101] & [11111111] = [10010101]

					Question: Cant we just truncate lower bit?
					[00101101 10010101] => (Cast to int8) => [10010101] ????? 		
					Answer: Turns out it works just fine but using & is always more fancy.		
				*/
				win32_replay_buffer* ReplayBuffer = &Win32State.ReplayBuffer[ReplayIndex];

				// NOTE: We start indexing from 1 as 0 means no recording  / playing
				Win32GetInputFileLocation(&Win32State, false, ReplayIndex+1,
 							sizeof(ReplayBuffer->FileName), ReplayBuffer-> FileName);

				ReplayBuffer->FileHandle = CreateFileA(ReplayBuffer->FileName,
										GENERIC_WRITE|GENERIC_READ, 0, 0, CREATE_ALWAYS,0,0);
	
				LARGE_INTEGER MaxSize;
				MaxSize.QuadPart = Win32State.TotalSize;
				ReplayBuffer->MemoryMap = CreateFileMapping( 
										ReplayBuffer->FileHandle, 0, 
										PAGE_READWRITE,
				 						MaxSize.HighPart, 
				 						MaxSize.LowPart, 
				 						0);
				
				ReplayBuffer->MemoryBlock = MapViewOfFile( 
											ReplayBuffer->MemoryMap,  FILE_MAP_ALL_ACCESS, 
											0, 0, Win32State.TotalSize);
				
				if(ReplayBuffer->MemoryBlock)
				{
					// Success
				}else{
					// TODO: Diagnostics, remove assert
				}

			}


			if( SoundSamples && GameMemory.PermanentStorage && GameMemory.TransientStorage)
			{
				game_input Input[2] = {};
				game_input* NewInput = &Input[0];
				game_input* OldInput = &Input[1];
				

				LARGE_INTEGER LastCounter = Win32GetWallClock();
				LARGE_INTEGER FlipWallClock = Win32GetWallClock();
				
				int DebugTimeMarkerIndex = 0;
				win32_debug_time_marker DebugTimeMarkers[30] = {0};
				DWORD AudioLatencyBytes = 0;
				real32 AudioLatencySeconds = 0;

				GlobalRunning = true;
				bool32 SoundIsValid = false;

				win32_game_code Game = Win32LoadGameCode(SourceGameCodeDLLFullPath,
														 TempGameCodeDLLFullPath,
														 TempGameCodeLockFullPath);

				uint64 LastCycleCount = __rdtsc();
				while(GlobalRunning)
				{	
					NewInput->dt = TargetSecondsPerFrame;
					
					FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
					if(CompareFileTime(&NewDLLWriteTime, &Game.LastDLLWriteTime))
					{
						Win32UnloadGameCode(&Game);
						Game = Win32LoadGameCode(SourceGameCodeDLLFullPath,
													TempGameCodeDLLFullPath,
													TempGameCodeLockFullPath);					
					}

					game_controller_input* OldKeyboardController =GetController(OldInput,0); 
					game_controller_input* NewKeyboardController =GetController(NewInput,0); 

					Win32ProcessKeyboard(&Win32State, OldKeyboardController, NewKeyboardController);
					
					if(!GlobalPause)
					{
						Win32ProcessControllerInput( OldInput, NewInput);
						POINT MouseP;
						GetCursorPos(&MouseP);
						ScreenToClient(Window, &MouseP);
						NewInput->MouseX = MouseP.x;
						NewInput->MouseY = MouseP.y;
						NewInput->MouseZ = 0;
						Win32ProcessKeyboardMessage(&NewInput->MouseButton[0], GetKeyState(VK_LBUTTON) & (1<<15));
						Win32ProcessKeyboardMessage(&NewInput->MouseButton[1], GetKeyState(VK_MBUTTON) & (1<<15));
						Win32ProcessKeyboardMessage(&NewInput->MouseButton[2], GetKeyState(VK_RBUTTON) & (1<<15));
						Win32ProcessKeyboardMessage(&NewInput->MouseButton[3], GetKeyState(VK_XBUTTON1) & (1<<15));
						Win32ProcessKeyboardMessage(&NewInput->MouseButton[4], GetKeyState(VK_XBUTTON2) & (1<<15));

						game_offscreen_buffer Buffer= {};
						Buffer.Memory= GlobalBackBuffer.Memory;
						Buffer.Width=  GlobalBackBuffer.Width;
						Buffer.Height= GlobalBackBuffer.Height;
						Buffer.Pitch=  GlobalBackBuffer.Pitch;
						Buffer.BytesPerPixel =  GlobalBackBuffer.BytesPerPixel;
	
						thread_context Thread = {};

						if(Win32State.RecordingIndex)
						{
							Win32RecordInput(&Win32State, NewInput);
						}
	
						if(Win32State.PlayingIndex)
						{
							Win32PlayBackInput(&Win32State, NewInput);
						}
	
						if(Game.UpdateAndRender)
						{
							Game.UpdateAndRender(&Thread, &GameMemory, &Buffer, NewInput); 
						}
	
						LARGE_INTEGER AudioWallClock = Win32GetWallClock();
						real32 FromBeginToAudioSeconds = Win32GetSecondsElapsed(FlipWallClock, AudioWallClock);
						real32 SecondsLeftUntillFlip = TargetSecondsPerFrame - FromBeginToAudioSeconds;

						DWORD PlayCursor;
						DWORD WriteCursor;
						if(SoundOutput.SecondaryBuffer->
										GetCurrentPosition(&PlayCursor, &WriteCursor)==DS_OK )
						{
	
						// Note, Sound will sound crackly when running through Visual Studio.
						/*	Note:
							Here is how sound output computation works.
	
							We define a saftey value that is the number of samples we think our game update loop may vary by (let's say 2ms)
	
							When we wake up to write audio, we will look and see what the playcursor position is and we will forecast ahead where we think the play cursor will be on the next frame boundary.
	
							We will then look to see if the write cursor is before that by at least our saftey value. If it is, the target fill position is that frame boundary plus one frame. This gives us perfect audio sync in the case of a card that has low enough latency.  
							
							If the write cursor is _after_ that saftey margin, then we assume we can never sync the audio perfectly, so we will write one frame's worth of audio plus the saftey margin's worth of guard samples.
					*/

							if(!SoundIsValid)
							{
								SoundOutput.RunningSampleIndex = WriteCursor / 
																	SoundOutput.BytesPerSampleTotal ;
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
							bool32 AudioCardIsLowLatency = 
											(SafeWriteCursor < ExpectedFrameBoundaryByte);
	
							DWORD TargetCursor = 0;
							if(AudioCardIsLowLatency)
							{
								TargetCursor =	(ExpectedFrameBoundaryByte +
											  	ExpectedSoundBytesPerFrame); 
							}else{
								TargetCursor = (WriteCursor + ExpectedSoundBytesPerFrame + 
												SoundOutput.BytesOfLatency);
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
							win32_debug_time_marker* Marker = &DebugTimeMarkers[	DebugTimeMarkerIndex];
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
									((real32) AudioLatencyBytes /
									(real32) SoundOutput.BytesPerSample) / 
									(real32) SoundOutput.SamplesPerSecond);

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


						LARGE_INTEGER WorkCounter = Win32GetWallClock();
						real32 WorkSecondsElapsed = Win32GetSecondsElapsed(
															LastCounter, WorkCounter);
						DWORD SleepMS = 0;
						real32 SecondsElapsedForFrame = WorkSecondsElapsed;
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


							real32 TestSecondsElapsedForFrame =  Win32GetSecondsElapsed(
												 LastCounter, Win32GetWallClock() );

							if(TestSecondsElapsedForFrame < TargetSecondsPerFrame)
							{
								//TODO: Log missed framerate
							}
	
							while(SecondsElapsedForFrame<TargetSecondsPerFrame)
							{
								SecondsElapsedForFrame =  Win32GetSecondsElapsed( LastCounter,
															 Win32GetWallClock() );
							}
						}else{
							// TODO: MISSED FRAME RATE
							// TODO: Logging
						}

						LARGE_INTEGER EndCounter = Win32GetWallClock();
						real32 MSPerFrame = 1000.0f * 
											Win32GetSecondsElapsed (LastCounter, EndCounter );
						LastCounter = EndCounter;
	
						// Update Window
						win32_window_dimension Dimension = Win32GetWindowDimension( Window );
#if HANDMADE_INTERNAL
				
						// Note this is wrong on zeroth index
				// 		Win32DebugSyncDisplay(&GlobalBackBuffer, ArrayCount(DebugTimeMarkers), 	DebugTimeMarkers, DebugTimeMarkerIndex-1, &SoundOutput, 	TargetSecondsPerFrame);
#endif
						HDC DeviceContext = GetDC(Window);
						Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, 
										Dimension.Width, Dimension.Height);
						ReleaseDC(Window, DeviceContext);
	
						FlipWallClock = Win32GetWallClock();
					
#if HANDMADE_INTERNAL
						// Note: This is debug code
						{
							
							DWORD DebugPlayCursor;
							DWORD DebugWriteCursor;
							if(SoundOutput.SecondaryBuffer->GetCurrentPosition(&DebugPlayCursor, 	&DebugWriteCursor)==DS_OK )
							{
								Assert(DebugTimeMarkerIndex <  ArrayCount(DebugTimeMarkers));
								win32_debug_time_marker* Marker = & DebugTimeMarkers[	DebugTimeMarkerIndex];
								Marker->FlipPlayCursor = DebugPlayCursor;
								Marker->FlipWriteCursor = DebugWriteCursor; 
							}
						}
#endif

						// TODO: Should We clear hese?
						game_input* Temp = NewInput;
						NewInput = OldInput;
						OldInput = Temp;
	
#if 1
						uint64 EndCycleCount = __rdtsc();
						uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
						LastCycleCount = EndCycleCount;
	
	
						real64 FPS = 0.0f;
						real64 MCFP =  ((real32)CyclesElapsed /(1000.0f*1000.0f));
	
						char FPSBuffer[256];
						snprintf(FPSBuffer, sizeof(FPSBuffer), "%.02f ms/f, %.02f tmf/s, %.02f msSlept, %.02f	Mc/f \n", MSPerFrame, TargetSecondsPerFrame*1000,(real32) SleepMS, MCFP);
						OutputDebugString(FPSBuffer);
#endif
#if HANDMADE_INTERNAL
						++DebugTimeMarkerIndex;
						if(DebugTimeMarkerIndex == ArrayCount(DebugTimeMarkers))
						{
							DebugTimeMarkerIndex = 0;
						}
#endif
					} // Global Pause
				}
			}else{

				//	TODO: Logging	
			}
		}else{	
		//	TODO: Logging	
		}
	}else{
	 // TODO: Logging
	}

	return 0;
}

