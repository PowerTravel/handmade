
#include <windows.h>
#include <stdint.h>

#define internal		 static
#define local_persist    static
#define global_variable  static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;


// TODO: Clobal for now
global_variable bool Running;

global_variable BITMAPINFO BitmapInfo;
global_variable void* BitmapMemory;
global_variable int BitmapWidth;
global_variable int BitmapHeight;
// Resize Device Independent Bitmap
internal void 
Win32ResizeDIBSection(int Width, int Height)
{

	// TODO: Bulletproof this
	// Maybe don't free firs, free after, then free if that fails.

	if(BitmapMemory)
	{
		VirtualFree(BitmapMemory, NULL, MEM_RELEASE);
	}

	BitmapWidth = Width;
	BitmapHeight = Height;

	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
	BitmapInfo.bmiHeader.biWidth = BitmapWidth;
	BitmapInfo.bmiHeader.biHeight = -BitmapHeight;	
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;

	int BytesPerPixel = 4;
	int BitmapMemorySize = BytesPerPixel*BitmapWidth*BitmapHeight;
	BitmapMemory = VirtualAlloc(NULL, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	int Pitch = BitmapWidth * BytesPerPixel;
	uint8* Row = (uint8*)BitmapMemory;
	for(int Y= 0; Y< BitmapHeight; ++Y)
	{
		uint8* Pixel = (uint8*)Row;
		for(int X= 0; X< BitmapWidth; ++X)
		{
			// Blue					
			*Pixel = (uint8)X;
			++Pixel;

			// Green
			*Pixel = (uint8)Y;			
			++Pixel;

			// Red
			*Pixel = 0;			
			++Pixel;
			
			// Padding
			*Pixel = 0;			
			++Pixel;
		}

		Row += Pitch;
	}

}

internal void
Win32UpdateWindow(HDC DeviceContext,RECT* WindowRect, int X, int Y, int Width, int Height)
{	
	int WindowWidth = WindowRect->right - WindowRect->left;
	int WindowHeight = WindowRect->bottom - WindowRect->top;
	StretchDIBits(	DeviceContext, 
					/*
					X, Y, Width, Height,
					X, Y, Width, Height,
					*/
					0,0, BitmapWidth, BitmapHeight,
					0,0, WindowWidth, WindowHeight,
					BitmapMemory,
					&BitmapInfo,
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
			RECT ClientRect;
			GetClientRect(Window, &ClientRect);
			int Width = ClientRect.right - ClientRect.left;
			int Height =  ClientRect.bottom - ClientRect.top;
			Win32ResizeDIBSection(Width, Height);
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

			RECT ClientRect;
			GetClientRect(Window, &ClientRect);

			Win32UpdateWindow(DeviceContext, &ClientRect, X, Y, Width, Height);
			//PatBlt(DeviceContext,X,Y,Width,Height, WHITENESS );
		//	EndPaint(Window, &Paint);
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
	WindowClass.style =CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
	WindowClass.lpfnWndProc = MainWindowCallback;
	// Passed to main but GetModuleHandle() also gets this pointer
	WindowClass.hInstance = PrevInstance; 
// 	WindowClass.hIcon;	// window icon (for later)
	WindowClass.lpszClassName = "HandmadeWindowClass";	// Name for the window class
	
	if(RegisterClass(&WindowClass) != 0)
	{
		HWND WindowHandle = CreateWindowEx(
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
		if(WindowHandle != NULL)
		{
			Running = true;
			while(Running)
			{
				MSG Message;
				BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
				if( MessageResult > 0)
				{
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}else{
					break;
				}
			}
		}else{	
		//	TODO: Logging	
		}
	}else{
	 // TODO: Logging
	}

	return 0;
}

