#include <stdint.h>
#include <windows.h>
#include <wingdi.h>
#include <winuser.h>

#define internal static
#define persistent_local static
#define global_variable static

// NOTE: This is global for now: https://www.youtube.com/watch?v=GAi_nTx1zG8&list=TLPQMjMwNTIwMjMXiLtHW48gLw&index=2
global_variable bool Running;

global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable int BitmapWidth;
global_variable int BitmapHeight;
global_variable int BytesPerPixel = 4;

// NOTE: Required to make custom renderer

internal void RenderWeirdGradient(int XOffset, int YOffset)
{
  int Width = BitmapWidth;
  int Height = BitmapHeight;

  int Pitch = Width*BytesPerPixel;
  uint8_t *Row = (uint8_t *)BitmapMemory;
  for (int Y = 0; Y < BitmapHeight; ++Y)
  {
    uint32_t *Pixel = (uint32_t *)Row;
    for (int X = 0; X < BitmapWidth; ++X)
    {
      /*
       *                   0  1  2  3
       * Pixel in memory: BB GG RR xx
       *
       * Little Endian Arch LOW -> HIGH
       *
       * Memory:   BB GG RR xx
       * Register: xx RR GG BB
       */
       uint8_t Blue = (X + XOffset);
       uint8_t Green = (Y + YOffset);

       // Shift Green value left by 8;
       *Pixel++ = ((Green << 8) | Blue);
    }
    Row += Pitch;
  }
}

internal void Win32ResizeDIBSection(int Width, int Height)
{
  // TODO: Make bulletproof
  if (BitmapMemory)
  {
    // TODO: MEM_DECOMMIT vs MEM_RELEASE
    VirtualFree(BitmapMemory, 0, MEM_RELEASE);
  }

  BitmapWidth = Width;
  BitmapHeight = Height;

  BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
  BitmapInfo.bmiHeader.biWidth = BitmapWidth;
  BitmapInfo.bmiHeader.biHeight = -BitmapHeight; // Setting biHeight to negative sets Top-Down painting
  BitmapInfo.bmiHeader.biPlanes = 1;
  BitmapInfo.bmiHeader.biBitCount = 32; // Keeping 32 bits to align on 4 byte boundaries
  BitmapInfo.bmiHeader.biCompression = BI_RGB;
  // BitmapInfo.bmiHeader.biSizeImage = 0; // NOTE: Global declaration removes requirement to init
  // BitmapInfo.bmiHeader.biXPelsPerMeter = 0;
  // BitmapInfo.bmiHeader.biYPelsPerMeter = 0;
  // BitmapInfo.bmiHeader.biClrUsed = 0;
  // BitmapInfo.bmiHeader.biClrImportant = 0;

  int BitmapMemorySize = (Width * Height) * BytesPerPixel;
  // HeapAlloc vs VirtualAlloc? VirtualAlloc gives an entire page
  // TODO: Get MSLearn Link for VirtualAlloc
  BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

  RenderWeirdGradient(0, 0);
}

internal void Win32UpdateWindow(HDC DeviceContext, RECT *WindowRect, int X, int Y, int Width, int Height)
{
  // How do we paint to the window? TopDown vs BottomUp?

  int WindowWidth = WindowRect->right - WindowRect->left;
  int WindowHeight = WindowRect->bottom - WindowRect->top;
  // NOTE: from HandmadeHero Day 004: Slower path (before 3d graphics cards)
  StretchDIBits(DeviceContext,
      /*
         X, Y, Width, Height, // Set Destination rect
         X, Y, Width, Height, // Source Rectangle
         */
      0, 0, BitmapWidth, BitmapHeight,
      0, 0, WindowWidth, WindowHeight,
      BitmapMemory,
      &BitmapInfo,
      DIB_RGB_COLORS, // NOTE: https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-stretchdibits#:~:text=Specifies%20whether%20the,literal%20RGB%20values.
      SRCCOPY); // NOTE: https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-bitblt#:~:text=The%20following%20list%20shows%20some%20common%20raster%20operation%20codes.
}


// Main window callback
LRESULT CALLBACK MainWindowCallback(HWND Window,
                UINT Message,
                WPARAM WParam,
                LPARAM LParam)
{
    // Window: Handle to window
    // Message: System message passed into Wndproc. Required mainly for windows standard controls.
    // WParam: Extra message data
    // LParam: Extra message data

    LRESULT Result = 0; // Return result after handling message. https://learn.microsoft.com/en-us/windows/win32/winprog/windows-data-types#:~:text=typedef%20WCHAR%20*LPWSTR%3B-,LRESULT,-Signed%20result%20of
    switch(Message)
    {
        case WM_SIZE: // Window is resized
        {
            OutputDebugStringA("WM_SIZE\n"); // OutputDebugStringA will put info to the debug console.

            RECT ClientRect;
            GetClientRect(Window, &ClientRect);

            int Width = ClientRect.right - ClientRect.left;
            int Height = ClientRect.bottom - ClientRect.top;

            Win32ResizeDIBSection(Width, Height);
        } break;

        case WM_DESTROY: // Window is destroyed
        {
            // TODO: Handle as error? recreate window?
            Running = false;
            OutputDebugStringA("WM_DESTROY\n");
        } break;

        case WM_CLOSE: // Window is closed
        {
            // TODO: Prompt user for confirmation.
            Running = false;
            OutputDebugStringA("WM_CLOSE\n");
        } break;

        case WM_ACTIVATEAPP: // Window becomes active
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            // Begin and End paint required
            HDC DeviceContext = BeginPaint(Window, &Paint); // Start painting


            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

            RECT ClientRect;
            GetClientRect(Window, &ClientRect);
            Win32UpdateWindow(DeviceContext, &ClientRect, X, Y, Width, Height);
            // Epilepsy window operations.

            EndPaint(Window, &Paint); // End painting
        } break;

        default: // Handle all other message types
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }

    return Result;
}

int CALLBACK
WinMain (HINSTANCE Instance,
         HINSTANCE PrevInstance,
         LPSTR CmdLine,
         int ShowCmd)
{
    // NOTE from https://youtu.be/4ROiWonnWGk?list=TLPQMjIwNTIwMjM3jPmfw1hgbA&t=750: if no performance concern, no initialization code
    WNDCLASS WindowClass = {};

    // CS_CLASSDC and CS_OWNDC to get own Device Context. Maybe overkill with current computers
    WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW; // https://learn.microsoft.com/en-us/windows/win32/winmsg/window-class-styles


    WindowClass.lpfnWndProc = MainWindowCallback;
    WindowClass.hInstance = Instance; // If not known, use GetModuleHandle(0) for hInstance of current running program
    // WindowClass.hIcon = ;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass"; // Required to effectively create a window

    if (RegisterClassA(&WindowClass)) // & operator returns pointer to variable if not defined as a pointer. https://cplusplus.com/doc/tutorial/pointers/#:~:text=relative%20to%20it.-,Address%2Dof%20operator%20(%26),-The%20address%20of
    {
        HWND WindowHandle = CreateWindowEx( // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-createwindowexa
            0,
            WindowClass.lpszClassName,
            "HandmadeHero",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            Instance,
            0
        );

        if (WindowHandle)
        {
            // Start messsage handling loop after window is created.
            int XOffset = 0, YOffset = 0;
            Running = true; // Set running true before while
            while (Running)
            {
                MSG Message;
                while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                {
                  if (Message.message == WM_QUIT)
                  { 
                    Running = false;
                  }
                  TranslateMessage(&Message);
                  DispatchMessageA(&Message);
                }

                {
                  RenderWeirdGradient(XOffset, YOffset);

                  HDC DeviceContext = GetDC(WindowHandle);
                  RECT ClientRect;
                  GetClientRect(WindowHandle, &ClientRect);
                  int WindowWidth = ClientRect.right - ClientRect.left;
                  int WindowHeight = ClientRect.bottom - ClientRect.top;
                  Win32UpdateWindow(DeviceContext, &ClientRect, 0, 0, WindowWidth, WindowHeight);
                }

                ++XOffset;
            }
        }
        else
        {
            // TODO: Handle create window failures.
        }
    }
    else
    {
        // TODO: Log error if registering window class fails.
    }

    return(0);
}

