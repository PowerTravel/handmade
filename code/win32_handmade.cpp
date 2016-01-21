
#include <windows.h>
#include <stdint.h>

#define internal		 static
#define local_persist    static
#define global_variable  static



typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;



// Resize Device Independent Bitmap

struct win32_offscreen_buffer
{
	BITMAPINFO Info;
	void* Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;

};


// TODO: Clobal for now
global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackBuffer = {};

struct win32_window_dimension{
	int Width;
	int Height;
};

win32_window_dimension Win32GetWindowDimension( HWND Window )
{
	win32_window_dimension Result;

	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height =  ClientRect.bottom - ClientRect.top;
	
	return Result;
}

internal void 
RenderWeirdGradient(win32_offscreen_buffer Buffer, int XOffset, int YOffset)
{	
	uint8* Row = (uint8*)Buffer.Memory;


	for(int Y= 0; Y< Buffer.Height; ++Y)
	{
		uint32* Pixel = (uint32*)Row;
		for(int X= 0; X < Buffer.Width; ++X)
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

		Row += Buffer.Pitch;
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
	Buffer->BytesPerPixel = 4;

	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;	
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	int BitmapMemorySize = (Buffer->Width*Buffer->Height)*Buffer->BytesPerPixel;
	Buffer->Memory = VirtualAlloc(NULL, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	Buffer->Pitch = Width * Buffer->BytesPerPixel;
}

internal void
Win32DisplayInWindow( HDC DeviceContext, int WindowWidth, int WindowHeight, 
					win32_offscreen_buffer Buffer,
					int X, int Y, int Width, int Height)
{	
	//TODO: Aspect ratio correction
	StretchDIBits(	DeviceContext, 
					0,0, WindowWidth, WindowHeight,
					0,0, Buffer.Width, Buffer.Height,
					Buffer.Memory,
					&Buffer.Info,
				    DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK 
MainWindowCallback(	HWND Window,
					UINT Message,
					WPARAM WParam,
					LPARAM LParam)
{
	LRESULT Result = 0;
	switch(Message)
	{
		// Window changes size
		case WM_SIZE:
		{
		}break;

		// User x-out
		case WM_CLOSE:
		{
			// TODO: Handle with message to user?
			Running = false;
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
			Running = false;
		}break;

		case WM_PAINT:
		{

			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);
			int X = Paint.rcPaint.left;
			int Y = Paint.rcPaint.top;
			int Width = Paint.rcPaint.right - Paint.rcPaint.left;
			int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

			win32_window_dimension Dimension = Win32GetWindowDimension(Window);
			Win32DisplayInWindow(DeviceContext, Dimension.Width, Dimension.Height,
								GlobalBackBuffer, X, Y, Width, Height);

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
	WNDCLASS WindowClass = {};
		
	Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

	WindowClass.style = CS_HREDRAW|CS_VREDRAW;
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
				0);

		// Handle window messages
		if(Window != NULL)
		{
			int XOffset = 0;
			int YOffset = 0;
			Running = true;
			while(Running)
			{	
				MSG Message;
				while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
				{
					if(Message.message == WM_QUIT)
					{
						Running =  false;
					}
					
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}
				
				// Update or globally declared bitmap with new colors
				RenderWeirdGradient(GlobalBackBuffer, XOffset, YOffset);

				HDC DeviceContext = GetDC(Window);
				win32_window_dimension Dimension = Win32GetWindowDimension( Window );
				Win32DisplayInWindow( DeviceContext, Dimension.Width, Dimension.Height,
								GlobalBackBuffer,0, 0, Dimension.Width, Dimension.Height);
				ReleaseDC(Window, DeviceContext);

				++XOffset;
				YOffset+=2;
			}
		}else{	
		//	TODO: Logging	
		}
	}else{
	 // TODO: Logging
	}

	return 0;
}

