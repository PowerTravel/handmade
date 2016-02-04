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

#include <stdint.h>
#include <math.h>

#define internal		 static
#define local_persist    static
#define global_variable  static

#define Pi32 3.14159265359

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;


#include <windows.h>
#include <xinput.h>
#include <dsound.h>

#include <stdio.h>

#include "win32_handmade.hpp"



// TODO: Clobal for now
global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;	
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

internal debug_read_file_result 
DEBUGPlatformReadEntireFile(char* Filename)
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
					DEBUGFreeFileMemory(Result.Contents);
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
internal void
DEBUGFreeFileMemory(void* Memory)
{
	if(Memory)
	{
		VirtualFree(Memory, 0 ,MEM_RELEASE);
	}
}

internal bool32 
DEBUGPlatformWriteEntireFile(char* Filename, uint32 MemorySize, void* Memory)
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
Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
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
			WaveFormat.nChannels = 2;		// sterio sound
			WaveFormat.nSamplesPerSec = SamplesPerSecond;
			WaveFormat.wBitsPerSample = 16; // cd-quality
			// EXP: Audio samples for 2 channels is stored as
			// 		LEFT RIGHT LEFT RIGHT LEFT RIGHT etc.
			//		nBlockAlign wants the size of one [LEFT RIGHT]
			//		block in bytes. For us this becomes
			//		(2 channels) X (nr of bits per sample) / (bits in a byte) = 
			//			2 * 16 /8 = 4 bytes per sample.
			WaveFormat.nBlockAlign = 
					(WaveFormat.nChannels * WaveFormat.wBitsPerSample ) / 8 ;
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
			// TODO: DSBCAPS_GETCURRENTPOSITION2  ? 
			DSBUFFERDESC BufferDescription = {};
			BufferDescription.dwSize = sizeof(BufferDescription);
			BufferDescription.dwFlags = 0;
			BufferDescription.dwBufferBytes = BufferSize;
			BufferDescription.lpwfxFormat = &WaveFormat;
			HRESULT Error =DirectSound->CreateSoundBuffer
								(&BufferDescription ,&GlobalSecondaryBuffer,0);
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

	// TODO: Bulletproof this
	// Maybe don't free firs, free after, then free if that fails.

	if(Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, NULL, MEM_RELEASE);
	}

	Buffer->Width = Width;
	Buffer->Height = Height;
	
	int BytesPerPixel = 4;

	// Note: When biHeight is set to negative it is the cue to the
	// compiler that we are to draw in a top down coordinate system
	// where our screen origin is in the top left corner.
	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;	
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;



	int BitmapMemorySize = (Buffer->Width*Buffer->Height)*BytesPerPixel;
	Buffer->Memory = VirtualAlloc(NULL, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	Buffer->Pitch = Buffer->Width * BytesPerPixel;

	// TODO: Probably clear this to black
}

internal void
Win32DisplayBufferInWindow(win32_offscreen_buffer* Buffer,
					HDC DeviceContext, int WindowWidth, int WindowHeight )
{	
	//TODO: Aspect ratio correction
	//TODO: Play with stretch modes
	StretchDIBits(	DeviceContext, 
					0,0, WindowWidth, WindowHeight,
					0,0, Buffer->Width, Buffer->Height,
					Buffer->Memory,
					&Buffer->Info,
				    DIB_RGB_COLORS, SRCCOPY);
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

		// User has clicked to make us active app
		// Or user has deactivated this app by tabbing or whatever
		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVEAPP\n");
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
	if( SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize,
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
	
		GlobalSecondaryBuffer->Unlock(Region1, Region1Size,
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
	if( SUCCEEDED(GlobalSecondaryBuffer->Lock( ByteToLock, BytesToWrite,
				        						 &Region1, &Region1Size,
				        						 &Region2, &Region2Size,
				        						 0)))
	{
		// TODO: assert that region1size / region2size is valid
		DWORD Region1SampleCount = Region1Size/SoundOutput->BytesPerSample;
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
		
		DWORD Region2SampleCount = Region2Size/SoundOutput->BytesPerSample;
		DestSample = (int16*) Region2;
		for(DWORD SampleIndex = 0; 
			SampleIndex < Region2SampleCount;
			++SampleIndex)
		{
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			SoundOutput->RunningSampleIndex++;
		}
		

		GlobalSecondaryBuffer->Unlock(Region1, Region1Size,Region2, Region2Size);
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
Win32ProcessKeyboardMessage( game_button_state* NewState, 
							 bool32 IsDown )
{
	NewState->EndedDown = IsDown;
	++NewState->HalfTransitionCount;
						
}


int CALLBACK 
WinMain(HINSTANCE Instance, 
			HINSTANCE PrevInstance, 
			LPSTR CommandLine, 
			int ShowCode )
{

	LARGE_INTEGER PerfCounterFrequencyResult;
	QueryPerformanceFrequency(&PerfCounterFrequencyResult);

	int64 PerfCounterFrequency = PerfCounterFrequencyResult.QuadPart;


	Win32LoadXInput();

	WNDCLASSA WindowClass = {};
	

	Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

	WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
	WindowClass.lpfnWndProc = MainWindowCallback;
	WindowClass.hInstance = PrevInstance; 
// 	WindowClass.hIcon;	// window icon (for later)
	WindowClass.lpszClassName = "HandmadeWindowClass";	// Name for the window class
			
	
	if(RegisterClass(&WindowClass) != 0)
	{
		HWND Window = CreateWindowEx(
				0,
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

		// Handle window messages
		if(Window != NULL)
		{
			// NOTE: Since we are specifying CS_OWNDC in our WindowClass, we can just
			// get one device context and use it forever because we are not sharing it 
			// with anyone. 
			// The function used to realese is ReleaseDC(Window, DeviceContext);
			HDC DeviceContext = GetDC(Window);

			win32_sound_output SoundOutput = {};
			// NOTE: Sound test
			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.RunningSampleIndex = 0;
			SoundOutput.BytesPerSample = sizeof(int16)*2;
			SoundOutput.SecondaryBufferSize =SoundOutput.SamplesPerSecond*
												SoundOutput.BytesPerSample;
			SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond/15;
			Win32InitDSound(Window,SoundOutput.SamplesPerSecond,
									SoundOutput.SecondaryBufferSize);
			
			Win32ClearSoundBuffer(&SoundOutput);
			GlobalSecondaryBuffer->Play(0,0, DSBPLAY_LOOPING);


			// TODO: Pool with Bitmap VirtualAlloc
			int16* Samples = (int16*) VirtualAlloc(0, 
											2*SoundOutput.SamplesPerSecond*sizeof(int16),
											MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

			
#if HANDMADE_INTERNAL
			LPVOID BaseAddress =(LPVOID) Terrabytes(2);
#else
			LPVOID BaseAddress = 0;
#endif
			game_memory GameMemory = {};
			GameMemory.PermanentStorageSize =  Megabytes(64);
			GameMemory.TransientStorageSize = Gigabytes(4);
			uint64 TotalSize = GameMemory.PermanentStorageSize +
								GameMemory.TransientStorageSize;
			GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, (size_t)TotalSize,
											MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);	
			GameMemory.TransientStorage = ((uint8*) GameMemory.PermanentStorage + 
								GameMemory.PermanentStorageSize);

			if(GameMemory.PermanentStorage && Samples)
			{
				game_input Input[2] = {};
				game_input* NewInput = &Input[0];
				game_input* OldInput = &Input[1];

				LARGE_INTEGER LastCounter;
				QueryPerformanceCounter(&LastCounter);
				int64 LastCycleCount = __rdtsc();
			
				GlobalRunning = true;
				while(GlobalRunning)
				{	
				
					MSG Message;
					game_controller_input* KeyboardController = &NewInput->Controllers[0]; 
					// TODO Zeroing Macro
					game_controller_input ZeroController = {};
					*KeyboardController = ZeroController;
					
					while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
					{
						if(Message.message == WM_QUIT)
						{
							GlobalRunning =  false;
						}
						switch(Message.message)
						{
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
									
									}else if(VKCode == 'A'){

									}else if(VKCode == 'S'){

									}else if(VKCode == 'D'){

									}else if(VKCode == 'Q'){
										Win32ProcessKeyboardMessage(
												&KeyboardController->LeftShoulder, IsDown);

									}else if(VKCode == 'E'){
										Win32ProcessKeyboardMessage(  
												&KeyboardController->RightShoulder, IsDown);

									}else if(VKCode == VK_UP){
										Win32ProcessKeyboardMessage(
												&KeyboardController->Up, IsDown);

									}else if(VKCode == VK_LEFT){
										Win32ProcessKeyboardMessage(
												&KeyboardController->Left, IsDown);

									}else if(VKCode == VK_DOWN){
										Win32ProcessKeyboardMessage(
												&KeyboardController->Down, IsDown);

									}else if(VKCode == VK_RIGHT){
										Win32ProcessKeyboardMessage(
												&KeyboardController->Right, IsDown);
									
									}else if(VKCode == VK_ESCAPE){
									
										GlobalRunning = false;

									}else if(VKCode == VK_SPACE){
									
									}
								}
		
								bool AltKeyWasDown = ((Message.lParam & (1<<29)) != 0);
								if(VKCode==VK_F4 && AltKeyWasDown)
								{
									GlobalRunning=false;
								}	
							}break;

							default:
							{
								TranslateMessage(&Message);
								DispatchMessage(&Message);
							}break;
						}
					}
				
					// Input
					// TODO: Should we poll this more frequently?
					DWORD MaxControllerCount = XUSER_MAX_COUNT;

					if(MaxControllerCount > ArrayCount(NewInput->Controllers))
					{
						MaxControllerCount = ArrayCount(NewInput->Controllers);
					}

					for( DWORD ControllerIndex = 0; 
						ControllerIndex< MaxControllerCount; 
						++ControllerIndex )
					{
							
						game_controller_input* OldController = &OldInput->Controllers[ControllerIndex]; 
						game_controller_input* NewController = &NewInput->Controllers[ControllerIndex]; 

						// Note: Performancebug in XInputGetControllerstate.
						//		 See HH007 at 15 min. To be ironed out in optimization
						XINPUT_STATE ControllerState;
						if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
						{
							// NOTE: This controller is plugged in
							// TODO: See if ControllerState.dwPacketNumber increment too rapidly
							XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

							// TODO: Dpad
							//bool Up 	= ( Pad ->wButtons & XINPUT_GAMEPAD_DPAD_UP ); 
							//bool Left 	= ( Pad ->wButtons & XINPUT_GAMEPAD_DPAD_LEFT ); 
							//bool Right 	= ( Pad ->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT ); 
							//bool Down 	= ( Pad ->wButtons & XINPUT_GAMEPAD_DPAD_DOWN ); 
							//int16 LeftTrigger  = Pad->bLeftTrigger; 
							//int16 RightTrigger = Pad->bRightTrigger;
							//int16 RightStickX = Pad->sThumbRX; 
							//int16 RightStickY = Pad->sThumbRY; 

							
							NewController->IsAnalog = true;
							NewController->StartX = OldController->EndX;
							NewController->StartY = OldController->EndY;

							real32 X;
							if(Pad->sThumbLX < 0)
							{
								X = (real32)Pad->sThumbLX / 32768.0f;
							}else{
								X = (real32)Pad->sThumbLX / 32767.0f;
							}
							NewController->MinX = NewController->MaxX = NewController->EndX = X;

							real32 Y;
							if(Pad->sThumbLY < 0)
							{
								Y = (real32)Pad->sThumbLY / 32768.0f;
							}else{
								Y = (real32)Pad->sThumbLY / 32767.0f;
							}
							NewController->MinY = NewController->MaxY = NewController->EndY = Y;


							Win32ProcessXInputDigitalButton(
									Pad ->wButtons, 
									&OldController->Down, XINPUT_GAMEPAD_A, 
									&NewController->Down);

							Win32ProcessXInputDigitalButton(
									Pad ->wButtons, 
									&OldController->Right, XINPUT_GAMEPAD_B, 
									&NewController->Right);

							Win32ProcessXInputDigitalButton(
									Pad ->wButtons, 
									&OldController->Left, XINPUT_GAMEPAD_X, 
									&NewController->Left);

							Win32ProcessXInputDigitalButton(
									Pad ->wButtons, 
									&OldController->Up, XINPUT_GAMEPAD_Y, 
									&NewController->Up);
							
							Win32ProcessXInputDigitalButton(
									Pad ->wButtons, 
									&OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER, 
									&NewController->LeftShoulder);

							Win32ProcessXInputDigitalButton(
									Pad ->wButtons, 
									&OldController->LeftShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER, 
									&NewController->LeftShoulder);






//							bool Start	= ( Pad ->wButtons & XINPUT_GAMEPAD_START ); 
//							bool Back	= ( Pad ->wButtons & XINPUT_GAMEPAD_BACK ); 
//							bool LeftThumb	= ( Pad ->wButtons & XINPUT_GAMEPAD_LEFT_THUMB ); 
//							bool RightThumb = ( Pad ->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB ); 
//							bool LeftShoulder= ( Pad ->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER ); 
//							bool RightShoulder= ( Pad ->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER ); 


					
						}else{
							// NOTE: The controller is not available
						}
					}
					
					// Sound
					DWORD ByteToLock = 0;
					DWORD TargetCursor = 0;
					DWORD BytesToWrite = 0;
					DWORD PlayCursor = 0;
					DWORD WriteCursor = 0;
					bool32 SoundIsValid = false;
					// TODO Tighten up sound logic so that we know where we should be writing to
					//		and can anticipate the time spent in the game update
					if(SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition( &PlayCursor, &WriteCursor)))
					{
						ByteToLock = ((SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) 
									% SoundOutput.SecondaryBufferSize);
						TargetCursor = ((PlayCursor + 
							(SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample)) %
							SoundOutput.SecondaryBufferSize);
						
						if(ByteToLock > TargetCursor)
						{
							BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
							BytesToWrite += TargetCursor;
						}else{
							BytesToWrite = TargetCursor - ByteToLock;
						}

						SoundIsValid= true;
					}

					game_offscreen_buffer Buffer= {};
					Buffer.Memory= GlobalBackBuffer.Memory;
					Buffer.Width=  GlobalBackBuffer.Width;
					Buffer.Height= GlobalBackBuffer.Height;
					Buffer.Pitch=  GlobalBackBuffer.Pitch;
					
					
					game_sound_output_buffer SoundBuffer = {};
					SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
					SoundBuffer.SampleCount = BytesToWrite/SoundOutput.BytesPerSample;
					SoundBuffer.Samples = Samples;
					GameUpdateAndRender(&GameMemory, &Buffer, &SoundBuffer, NewInput);

					// Sound
					if(SoundIsValid)
					{
						Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
					}

					// Update Window
					win32_window_dimension Dimension = Win32GetWindowDimension( Window );
					Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, 
										Dimension.Width, Dimension.Height);
					
					int64 EndCycleCount = __rdtsc();

					LARGE_INTEGER EndCounter;
					QueryPerformanceCounter(&EndCounter);

					uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
					int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
					real64 MSPerFrame = ((1000.0f*(real64)CounterElapsed)/(real64)PerfCounterFrequency);
					real64 FPS = (real64)PerfCounterFrequency/(real64)CounterElapsed;
					real64 MCFP =  ((real64)CyclesElapsed /(1000.0f*1000.0f));
#if 0
					char txtBuffer[256];
					snprintf(txtBuffer, sizeof(txtBuffer), "%.02f ms/f, %.02f f/s, %.02f Mc/f \n", MSPerFrame, FPS, MCFP);
					OutputDebugString(Buffer);
#endif			
					LastCounter = EndCounter;
					LastCycleCount = EndCycleCount;


					// TODO: Should We clear hese?
					game_input* Temp = NewInput;
					NewInput = OldInput;
					OldInput = Temp;
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

