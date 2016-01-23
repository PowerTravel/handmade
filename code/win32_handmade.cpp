#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>

#define internal		 static
#define local_persist    static
#define global_variable  static

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;



struct win32_offscreen_buffer
{
	// Note: Pixels are always 32-bits wide: Memory Order BB GG RR XX
	BITMAPINFO Info;
	void* Memory;
	int Width;
	int Height;
	int Pitch;
};


struct win32_window_dimension{
	int Width;
	int Height;
};

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

// TODO: Clobal for now
global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;	

internal void 
Win32_LoadXInput(void) 
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
RenderWeirdGradient(win32_offscreen_buffer* Buffer, int XOffset, int YOffset)
{	
	uint8* Row = (uint8*)Buffer->Memory;

	for(int Y= 0; Y< Buffer->Height; ++Y)
	{
		uint32* Pixel = (uint32*)Row;
		for(int X= 0; X < Buffer->Width; ++X)
		{

			uint8 Blue = (X+XOffset);			
			uint8 Green = (Y+YOffset);
			uint8 Red = 0;
	
			/*
						  8b 8b 8b 8b = 36bit
				Memmory   BB GG RR xx
				Registry  xx RR GG BB
			*/

			*Pixel++ = ( (Red << 16 )  | ( Green << 8) | Blue);
		}

		Row += Buffer->Pitch;
	}
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
			uint32 VKCode = WParam;
			#define KeyMessageWasDownBit (1<<30)
			#define KeyMessageIsDownBit (1<<31)
			bool WasDown = ((LParam & KeyMessageWasDownBit) != 0);
			bool IsDown = ((LParam &  KeyMessageIsDownBit) == 0);

			if(WasDown != IsDown)
			{
				if(VKCode == 'W')
				{
				
				}else if(VKCode == 'A'){

				}else if(VKCode == 'S'){

				}else if(VKCode == 'D'){

				}else if(VKCode == 'Q'){

				}else if(VKCode == 'E'){

				}else if(VKCode == VK_UP){

				}else if(VKCode == VK_LEFT){

				}else if(VKCode == VK_DOWN){

				}else if(VKCode == VK_RIGHT){
				
				}else if(VKCode == VK_ESCAPE){
					OutputDebugString("ESCAPE");
					if(IsDown){
						OutputDebugString(" IsDown ");	
					}
					if(WasDown){
						OutputDebugString(" WasDown");
					}
					OutputDebugString("\n");
				}else if(VKCode == VK_SPACE){
				
				}
			}
		
			bool AltKeyWasDown = ((LParam & (1<<29)) != 0);
			if(VKCode==VK_F4 && AltKeyWasDown)
			{
				GlobalRunning=false;
			}	
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

int CALLBACK 
WinMain(HINSTANCE Instance, 
			HINSTANCE PrevInstance, 
			LPSTR CommandLine, 
			int ShowCode )
{
	WNDCLASSA WindowClass = {};
	
	Win32_LoadXInput();

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

			// NOTE: Graphics test
			int XOffset = 0;
			int YOffset = 0;

			// NOTE: Sound test
			local_persist int frame =0;
			frame++;
			int SamplesPerSecond = 48000;
			int ToneHz = 556;
			int16 ToneVolume = 500;
			uint32 RunningSampleIndex = 0;
			int SquareWavePeriod = SamplesPerSecond/ToneHz;
			int HalfSquareWavePeriod = SquareWavePeriod/2;
			int BytesPerSample = sizeof(int16)*2;
			int SecondaryBufferSize =SamplesPerSecond*BytesPerSample;

			Win32InitDSound(Window,SamplesPerSecond,SecondaryBufferSize);
			HRESULT SoundError = GlobalSecondaryBuffer->Play(0,0, DSBPLAY_LOOPING);

			GlobalRunning = true;
			while(GlobalRunning)
			{	
				MSG Message;
				while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
				{
					if(Message.message == WM_QUIT)
					{
						GlobalRunning =  false;
					}
					
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}
			
				// Input
				// TODO: Should we poll this more frequently

				for( DWORD ControllerIndex = 0; 
					ControllerIndex< XUSER_MAX_COUNT; 
					++ControllerIndex )
				{
					// Note: Performancebug in XInputGetControllerstate.
					//		 See HH007 at 15 min. To be ironed out in optimization
					XINPUT_STATE ControllerState;
					if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
					{
						// NOTE: This controller is plugged in
						// TODO: See if ControllerState.dwPacketNumber increment too rapidly
						XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
						bool Up 	= ( Pad ->wButtons & XINPUT_GAMEPAD_DPAD_UP ); 
						bool Left 	= ( Pad ->wButtons & XINPUT_GAMEPAD_DPAD_LEFT ); 
						bool Right 	= ( Pad ->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT ); 
						bool Down 	= ( Pad ->wButtons & XINPUT_GAMEPAD_DPAD_DOWN ); 
						bool Start	= ( Pad ->wButtons & XINPUT_GAMEPAD_START ); 
						bool Back	= ( Pad ->wButtons & XINPUT_GAMEPAD_BACK ); 
						bool LeftThumb	= ( Pad ->wButtons & XINPUT_GAMEPAD_LEFT_THUMB ); 
						bool RightThumb = ( Pad ->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB ); 
						bool LeftShoulder= ( Pad ->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER ); 
						bool RightShoulder= ( Pad ->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER ); 
						bool AButton = ( Pad ->wButtons & XINPUT_GAMEPAD_A); 
						bool BButton = ( Pad ->wButtons & XINPUT_GAMEPAD_B); 
						bool CButton = ( Pad ->wButtons & XINPUT_GAMEPAD_X); 
						bool DButton = ( Pad ->wButtons & XINPUT_GAMEPAD_Y); 

						int16 LeftTrigger  = Pad->bLeftTrigger; 
						int16 RightTrigger = Pad->bRightTrigger;

						// Note: 	In HandmadeHero tutorial the left sticks are 
						// 			simply named StickX and StickY.
						int16 LeftStickX =  Pad->sThumbLX; 
						int16 LeftStickY =  Pad->sThumbLY;  
						int16 RightStickX = Pad->sThumbRX; 
						int16 RightStickY = Pad->sThumbRY; 
						
						if(AButton)
						{
							XOffset = LeftStickX >> 7;
							YOffset = -LeftStickY >> 7;
						}else{
							XOffset += LeftStickX >> 12;
							YOffset -= LeftStickY >> 12;
						}
					}else{
						// NOTE: The controller is not available
					}
				}


				// Render
				RenderWeirdGradient(&GlobalBackBuffer, XOffset, YOffset);

				// Note: DirectSound output test
				DWORD PlayCursor;
				DWORD WriteCursor;
				if( SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition( &PlayCursor, &WriteCursor)))
				{

					DWORD ByteToLock = (RunningSampleIndex * BytesPerSample) % SecondaryBufferSize;
					DWORD BytesToWrite;
					// BUG: Somehow we get BytesToWrite == 0 which causes Lock() to fail.
					if(ByteToLock == PlayCursor)
					{
						BytesToWrite = SecondaryBufferSize;
					}
					else if(ByteToLock > PlayCursor)
					{
						BytesToWrite = (SecondaryBufferSize - ByteToLock);
						BytesToWrite += PlayCursor;
					}else{
						BytesToWrite = PlayCursor - ByteToLock;
					}
					VOID* Region1;
					DWORD Region1Size;
					VOID* Region2;
					DWORD Region2Size;

					SoundError = GlobalSecondaryBuffer->Lock( ByteToLock, BytesToWrite,
								        						 &Region1, &Region1Size,
								        						 &Region2, &Region2Size,
								        						 0);
					if( SUCCEEDED(SoundError) )
					{
						// TODO: assert that region1size / region2size is valid

						DWORD Region1SampleCount = Region1Size/BytesPerSample;
						int16* SampleOut = ( int16* ) Region1;
						for(DWORD SampleIndex = 0; 
							SampleIndex < Region1SampleCount; 
							++SampleIndex)
						{
							int16 SampleValue = 
							((RunningSampleIndex++/ HalfSquareWavePeriod ) %2 ) ? ToneVolume : -ToneVolume;
							*SampleOut++ = SampleValue;
							*SampleOut++ = SampleValue;
						}
						
						DWORD Region2SampleCount = Region2Size/BytesPerSample;
						SampleOut = (int16*) Region2;
						for(DWORD SampleIndex = 0; 
							SampleIndex < Region2SampleCount;
							++SampleIndex)
						{
							int16 SampleValue = 
							((RunningSampleIndex++/ HalfSquareWavePeriod ) %2) ? ToneVolume : -ToneVolume;
							*SampleOut++ = SampleValue;
							*SampleOut++ = SampleValue;
						}

						GlobalSecondaryBuffer->Unlock(Region1, Region1Size,
								Region2, Region2Size);
					}


				}

				win32_window_dimension Dimension = Win32GetWindowDimension( Window );
				Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, 
									Dimension.Width, Dimension.Height);
			}
		}else{	
		//	TODO: Logging	
		}
	}else{
	 // TODO: Logging
	}

	return 0;
}

