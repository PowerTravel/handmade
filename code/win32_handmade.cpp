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
global_variable bool32 GlobalRunning;
global_variable bool32 GlobalPause; 
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;	
global_variable int64 GlobalPerfCounterFrequency;
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
			DSBUFFERDESC BufferDescription = {};
			BufferDescription.dwSize = sizeof(BufferDescription);
//			BufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
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
	Buffer->BytesPerPixel = BytesPerPixel;
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
	Assert(NewState->EndedDown != IsDown);

	NewState->EndedDown = IsDown;
	++NewState->HalfTransitionCount;
						
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

internal void 
Win32ProcessPendingMessages(game_controller_input* KeyboardController)
{
	MSG Message;
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
						Win32ProcessKeyboardMessage(
								&KeyboardController->LeftStickUp, IsDown);
					
					}else if(VKCode == 'A'){
						Win32ProcessKeyboardMessage(
								&KeyboardController->LeftStickLeft, IsDown);

					}else if(VKCode == 'S'){
						Win32ProcessKeyboardMessage(
								&KeyboardController->LeftStickDown, IsDown);

					}else if(VKCode == 'D'){
						Win32ProcessKeyboardMessage(
								&KeyboardController->LeftStickRight, IsDown);

					}else if(VKCode == 'Q'){
						Win32ProcessKeyboardMessage(
								&KeyboardController->LeftShoulder, IsDown);

					}else if(VKCode == 'E'){
						Win32ProcessKeyboardMessage(  
								&KeyboardController->RightShoulder, IsDown);

					}else if(VKCode == VK_UP){
						Win32ProcessKeyboardMessage(
								&KeyboardController->RightStickUp, IsDown);

					}else if(VKCode == VK_LEFT){
						Win32ProcessKeyboardMessage(
								&KeyboardController->RightStickLeft, IsDown);

					}else if(VKCode == VK_DOWN){
						Win32ProcessKeyboardMessage(
								&KeyboardController->RightStickDown, IsDown);

					}else if(VKCode == VK_RIGHT){
						Win32ProcessKeyboardMessage(
								&KeyboardController->RightStickRight, IsDown);
					
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
					}else if(VKCode == ){
						Win32ProcessKeyboardMessage(
								&KeyboardController->LeftTrigger, IsDown);
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
					if(VKCode == 'P')
					{
						if(IsDown)
						{
							GlobalPause = !GlobalPause;
						}
					}
#endif
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
	real32 XReal32 = ( C * (real32) Value);
	real32 X = PadX + (int)  XReal32;

	Win32DebugDrawVertical(BackBuffer, X, Top, Bottom, Color);	
}

internal void
Win32DebugSyncDisplay(win32_offscreen_buffer* BackBuffer, 
						int MarkerCount,win32_debug_time_marker* Markers, 
						int CurrentMarkerIndex, win32_sound_output* SoundOutput, real32 TargetSecondsPerFrame)
{
	int PadX = 16;
	int PadY = 16;

	int LineHeight = 64;
 
	real32 C = (real32) (BackBuffer->Width-2*PadX) / 
						(real32)SoundOutput-> SecondaryBufferSize;

	for(int MarkerIndex=0; MarkerIndex < MarkerCount; ++MarkerIndex )
	{

		win32_debug_time_marker* ThisMarker = &Markers[MarkerIndex]; 

		Assert(ThisMarker->OutputPlayCursor < SoundOutput->SecondaryBufferSize);
		Assert(ThisMarker->OutputWriteCursor < SoundOutput->SecondaryBufferSize);
		Assert(ThisMarker->OutputLocation < SoundOutput->SecondaryBufferSize);
		Assert(ThisMarker->OutputByteCount < SoundOutput->SecondaryBufferSize);
		Assert(ThisMarker->FlipPlayCursor < SoundOutput->SecondaryBufferSize);
		Assert(ThisMarker->FlipWriteCursor < SoundOutput->SecondaryBufferSize);
	
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
int CALLBACK 
WinMain(HINSTANCE Instance, 
			HINSTANCE PrevInstance, 
			LPSTR CommandLine, 
			int ShowCode )
{

	LARGE_INTEGER PerfCounterFrequencyResult;
	QueryPerformanceFrequency(&PerfCounterFrequencyResult);
	GlobalPerfCounterFrequency = PerfCounterFrequencyResult.QuadPart;

	// Note: 	sets the Windows scheduler granularity (The max time resolution of Sleep() 
	//			- function) to 1 ms.
	UINT DesiredSchedulerMS = 1;
	bool32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

	Win32LoadXInput();

	WNDCLASSA WindowClass = {};
	
	Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

	WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
	WindowClass.lpfnWndProc = MainWindowCallback;
	WindowClass.hInstance = PrevInstance; 
// 	WindowClass.hIcon;	// window icon (for later)
	WindowClass.lpszClassName = "HandmadeWindowClass";	// Name for the window class

	// TODO: How do we reliably query this on windows?

#define MonitorRefreshHz 60
#define GameUpdateHz (MonitorRefreshHz/2)
	real32 TargetSecondsPerFrame = 1.0f / (real32) GameUpdateHz;

	if(RegisterClass(&WindowClass))
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


		if(Window != NULL)
		{
			// NOTE: Since we are specifying CS_OWNDC in our WindowClass, we can just
			// get one device context and use it forever because we are not sharing it 
			// with anyone. 
			// The function used to realese is ReleaseDC(Window, DeviceContext);
			HDC DeviceContext = GetDC(Window);

			win32_sound_output SoundOutput = {};

			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.BytesPerSample = sizeof(int16)*2;

			
		//	SoundOutput.RunningSampleIndex = 0;

			SoundOutput.SecondaryBufferSize =SoundOutput.SamplesPerSecond*
												SoundOutput.BytesPerSample;
		    // TODO: Get rid of LatencySampleCount
			// TODO: Actually compute this variance and see what the lowest reasonable value is.
			SoundOutput.LatencySampleCount = 3*(SoundOutput.SamplesPerSecond/GameUpdateHz);
			
			SoundOutput.SafteyBytes = (SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample /GameUpdateHz)/3;
			Win32InitDSound(Window,SoundOutput.SamplesPerSecond,
									SoundOutput.SecondaryBufferSize);
			
			Win32ClearSoundBuffer(&SoundOutput);
			GlobalSecondaryBuffer->Play(0,0, DSBPLAY_LOOPING);


#if 0
			bool apa = true;
			// Note: This tests the playcursor write cruesor update frequency. 
			// It was 480 samples or 1920 bytes
			DWORD TMPPC = 0;
			DWORD TMPWC = 0;
			while(apa==true)
			{
				DWORD PlayCursor = 0;
				DWORD WriteCursor = 0;
				GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);
				
				if(TMPPC != PlayCursor||
				 TMPWC != WriteCursor)
				{
					char txtBuffer[256];
					snprintf(txtBuffer, sizeof(txtBuffer), 
									"PC: %u, WC: %u, \n", PlayCursor,WriteCursor);
					OutputDebugString(txtBuffer);
					TMPPC = PlayCursor;
					TMPWC = WriteCursor;
				}
				
			}
#endif
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
			GameMemory.PermanentStorageSize = Megabytes(64);
			GameMemory.TransientStorageSize = Gigabytes(4);
			uint64 TotalSize = GameMemory.PermanentStorageSize +
								GameMemory.TransientStorageSize;
			GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, (size_t)TotalSize,
											MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);	
			GameMemory.TransientStorage = ((uint8*) GameMemory.PermanentStorage + 
								GameMemory.PermanentStorageSize);

			if( Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage)
			{
				game_input Input[2] = {};
				game_input* NewInput = &Input[0];
				game_input* OldInput = &Input[1];

				LARGE_INTEGER LastCounter = Win32GetWallClock();
				LARGE_INTEGER FlipWallClock = Win32GetWallClock();
				
				int DebugTimeMarkerIndex = 0;
				win32_debug_time_marker DebugTimeMarkers[GameUpdateHz/2] = {0};


				DWORD AudioLatencyBytes = 0;
				real32 AudioLatencySeconds = 0;

				GlobalRunning = true;
				bool32 SoundIsValid = false;

				uint64 LastCycleCount = __rdtsc();
				while(GlobalRunning)
				{	
					game_controller_input* OldKeyboardController =GetController(OldInput,0); 
					game_controller_input* NewKeyboardController =GetController(NewInput,0); 
					*NewKeyboardController = {};
					NewKeyboardController->IsConnected = true;
					for(int ButtonIndex = 0; 
						ButtonIndex  < ArrayCount(NewKeyboardController->Button);
						++ButtonIndex)
					{
						NewKeyboardController->Button[ButtonIndex].EndedDown =
							OldKeyboardController->Button[ButtonIndex].EndedDown;
					}


					Win32ProcessPendingMessages(NewKeyboardController);
					
					if(!GlobalPause)
					{

					Win32ProcessControllerInput( OldInput, NewInput);

					game_offscreen_buffer Buffer= {};
					Buffer.Memory= GlobalBackBuffer.Memory;
					Buffer.Width=  GlobalBackBuffer.Width;
					Buffer.Height= GlobalBackBuffer.Height;
					Buffer.Pitch=  GlobalBackBuffer.Pitch;
					Buffer.BytesPerPixel =  GlobalBackBuffer.BytesPerPixel;
					GameUpdateAndRender(&GameMemory, &Buffer, NewInput); 

					LARGE_INTEGER AudioWallClock = Win32GetWallClock();
					real32 FromBeginToAudioSeconds = Win32GetSecondsElapsed(FlipWallClock, AudioWallClock);

					DWORD PlayCursor;
					DWORD WriteCursor;
					if(GlobalSecondaryBuffer->
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
																SoundOutput.BytesPerSample ;
							SoundIsValid=true;
						}
						
						
						DWORD ByteToLock = ((SoundOutput.RunningSampleIndex * 
									  		 SoundOutput.BytesPerSample) % 
											 SoundOutput.SecondaryBufferSize);
						
						DWORD ExpectedSoundBytesPerFrame = 
							(SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample) /  GameUpdateHz;
						real32 SecondsLeftUntillFlip = TargetSecondsPerFrame - FromBeginToAudioSeconds;
						DWORD ExpectedBytesUntillFlip  = (DWORD) ((SecondsLeftUntillFlip /TargetSecondsPerFrame) * (real32)ExpectedSoundBytesPerFrame); 
						DWORD ExpectedFrameBoundaryByte = PlayCursor +
														 ExpectedSoundBytesPerFrame;



						DWORD SafeWriteCursor = WriteCursor;
						if(SafeWriteCursor < PlayCursor)
						{	
							SafeWriteCursor += SoundOutput.SecondaryBufferSize;
						}
						Assert(SafeWriteCursor >= PlayCursor);
						SafeWriteCursor += SoundOutput.SafteyBytes;
						bool32 AudioCardIsLowLatency = 
										(SafeWriteCursor < ExpectedFrameBoundaryByte);

						DWORD TargetCursor = 0;
						if(AudioCardIsLowLatency)
						{
							TargetCursor =	(ExpectedFrameBoundaryByte +
										  	ExpectedSoundBytesPerFrame); 
						}else{
							TargetCursor = (WriteCursor + ExpectedSoundBytesPerFrame + 
											SoundOutput.SafteyBytes);
						}
						TargetCursor = TargetCursor % SoundOutput.SecondaryBufferSize;
						
						DWORD BytesToWrite = 0;
						if(ByteToLock > TargetCursor)
						{
							BytesToWrite=(SoundOutput.SecondaryBufferSize - ByteToLock);
							BytesToWrite += TargetCursor;
					 	}else{
							BytesToWrite = TargetCursor - ByteToLock;
						}


						game_sound_output_buffer SoundBuffer = {};
						SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
						SoundBuffer.SampleCount = BytesToWrite/SoundOutput.BytesPerSample;
						SoundBuffer.Samples = Samples;
						GameGetSoundSamples(&GameMemory, &SoundBuffer);

						
#if HANDMADE_INTERNAL
						win32_debug_time_marker* Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
						Marker->OutputPlayCursor = PlayCursor;
						Marker->OutputWriteCursor = WriteCursor;
						Marker->OutputLocation = ByteToLock;
						Marker->OutputByteCount = BytesToWrite;
						Marker->ExpectedFlipPlayCursor = ExpectedFrameBoundaryByte;

						DWORD UnwrappedWriteCursor = WriteCursor;
						if(UnwrappedWriteCursor < PlayCursor)
						{
							UnwrappedWriteCursor += SoundOutput.SecondaryBufferSize;
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
						Win32FillSoundBuffer(&SoundOutput, ByteToLock,
												 BytesToWrite, &SoundBuffer);
						
					}else{
						SoundIsValid = false;
					}


					LARGE_INTEGER WorkCounter = Win32GetWallClock();
					real32 WorkSecondsElapsed = Win32GetSecondsElapsed(
															LastCounter, WorkCounter);

					real32 SecondsElapsedForFrame = WorkSecondsElapsed;
					if(SecondsElapsedForFrame<TargetSecondsPerFrame)
					{
						if(SleepIsGranular)
						{
							DWORD SleepMS = (DWORD) ( 1000.f *
								   (TargetSecondsPerFrame - SecondsElapsedForFrame));
							if(SleepMS>0)
							{
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
					Win32DebugSyncDisplay(&GlobalBackBuffer, ArrayCount(DebugTimeMarkers), DebugTimeMarkers, DebugTimeMarkerIndex-1, &SoundOutput, TargetSecondsPerFrame);
#endif
					Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, 
										Dimension.Width, Dimension.Height);
					
	
					FlipWallClock = Win32GetWallClock();
					
#if HANDMADE_INTERNAL
					// Note: This is debug code
					{
						
						DWORD DebugPlayCursor;
						DWORD DebugWriteCursor;
						if(GlobalSecondaryBuffer->GetCurrentPosition(&DebugPlayCursor, &DebugWriteCursor)==DS_OK )
						{
							Assert(DebugTimeMarkerIndex <  ArrayCount(DebugTimeMarkers));
							win32_debug_time_marker* Marker = & DebugTimeMarkers[DebugTimeMarkerIndex];
							Marker->FlipPlayCursor = DebugPlayCursor;
							Marker->FlipWriteCursor = DebugWriteCursor; 
						}
					}
#endif

					// TODO: Should We clear hese?
					game_input* Temp = NewInput;
					NewInput = OldInput;
					OldInput = Temp;

					uint64 EndCycleCount = __rdtsc();
					uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
					LastCycleCount = EndCycleCount;


					real64 FPS = 0.0f;
					real64 MCFP =  ((real32)CyclesElapsed /(1000.0f*1000.0f));

					char FPSBuffer[256];
					snprintf(FPSBuffer, sizeof(FPSBuffer), "%.02f ms/f, %.02f f/s, %.02f Mc/f \n", MSPerFrame, FPS, MCFP);
					OutputDebugString(FPSBuffer);
		
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

