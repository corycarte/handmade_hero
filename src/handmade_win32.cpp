#include <cstdint>
#include <stdint.h>
#include <windows.h>
#include <winuser.h>
#include <xinput.h>

#pragma comment(lib, "XInput.lib")

#define internal static
#define persistent_local static
#define global_variable static
#define COLOR_INCREMENT 1

struct win32_offscreen_buffer {
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int BytesPerPixel;
    int Pitch;
};

// NOTE: This is global for now: https://www.youtube.com/watch?v=GAi_nTx1zG8&list=TLPQMjMwNTIwMjMXiLtHW48gLw&index=2
global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackBuffer;

global_variable uint8_t GlobalRed;
global_variable uint8_t GlobalGreen;
global_variable uint8_t GlobalBlue;

// NOTE: Dynamically load the XInputGetState and XInputSetState functions. Doesn't work for me in Windows 11
// Leaving Code here for reference

//#define XINPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
//typedef XINPUT_GET_STATE(x_input_get_state);
//XINPUT_GET_STATE(XInputGetStateStub) { return (0); }
//global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
//
//#define XINPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
//typedef XINPUT_SET_STATE(x_input_set_state);
//XINPUT_SET_STATE(XInputSetStateStub) { return (0); }
//global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
//
//#define XINPUT_ENABLE(name) void name(BOOL enable)
//typedef XINPUT_ENABLE(x_input_enable);
//XINPUT_ENABLE(XInputEnableStub) { }
//global_variable x_input_enable *XInputEnable_ = XInputEnableStub;
//
//#define XInputGetState XInputGetState_;
//#define XInputSetState XInputSetState_;
//#define XInputEnable XInputEnable_;

internal void
Win32LoadXInput(void)
{
    //    HMODULE XInputLibrary = LoadLibrary("xinput1_4.dll");
    //    if (XInputLibrary) {
    //        XInputGetState_ = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
    //        XInputSetState_ = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
    //        XInputEnable_ = (x_input_enable *)GetProcAddress(XInputLibrary, "XInputEnable");
    //        XInputEnable(true);
    //    }
}

internal struct 
win32_window_dimension {
    int Width;
    int Height;
};


internal win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

  return Result;
}

internal void
RenderWeirdGradient(win32_offscreen_buffer *Buffer, int XOffset, int YOffset)
{
    // TODO: What happens?
  uint8_t *Row = (uint8_t *)Buffer->Memory;
  for (int Y = 0; Y < Buffer->Height; ++Y)
  {
    uint32_t *Pixel = (uint32_t *)Row;
    for (int X = 0; X < Buffer->Width; ++X)
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
       uint8_t Blue = GlobalBlue + X + XOffset;
       uint8_t Green = GlobalGreen + Y + YOffset;
       uint8_t Red = GlobalRed;

       // Shift Green value left by 8;
       *Pixel++ = ((Red << 16) | (Green << 8) | Blue);
    }
    Row += Buffer->Pitch;
  }
}

internal void 
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
  // TODO: Make bulletproof
  if (Buffer->Memory)
  {
    // TODO: MEM_DECOMMIT vs MEM_RELEASE
    VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
  }

  Buffer->Width = Width;
  Buffer->Height = Height;
  Buffer->BytesPerPixel = 4;

  Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
  Buffer->Info.bmiHeader.biWidth  =  Buffer->Width;
  Buffer->Info.bmiHeader.biHeight = -Buffer->Height; // Setting biHeight to negative sets Top-Down painting
  Buffer->Info.bmiHeader.biPlanes = 1;
  Buffer->Info.bmiHeader.biBitCount = 32; // Keeping 32 bits to align on 4 byte boundaries
  Buffer->Info.bmiHeader.biCompression = BI_RGB;

  int BitmapMemorySize = (Width * Height) * Buffer->BytesPerPixel;
  // HeapAlloc vs VirtualAlloc? VirtualAlloc gives an entire page
  // https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc
  Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
  Buffer->Pitch = Width*Buffer->BytesPerPixel;
}

internal void 
Win32DisplayBufferInWindow(HDC DeviceContext, int WindowWidth, int WindowHeight, 
                          win32_offscreen_buffer *Buffer,
                          int X, int Y, int Width, int Height)
{
  // How do we paint to the window? TopDown vs BottomUp?
  // TODO: Aspect Ratio correction required

  // NOTE: from HandmadeHero Day 004: Slower path (before 3d graphics cards)
  StretchDIBits(DeviceContext,
          0, 0, WindowWidth, WindowHeight, // Destination First
          0, 0, Buffer->Width, Buffer->Height,
          Buffer->Memory,
          &Buffer->Info,
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
            // Windows will send WM_PAINT when it thinks a region should be repainted
            PAINTSTRUCT Paint;
            // Begin and End paint required
            HDC DeviceContext = BeginPaint(Window, &Paint); // Start painting


            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

            win32_window_dimension Dimensions = Win32GetWindowDimension(Window);
            Win32DisplayBufferInWindow(DeviceContext, Dimensions.Width, Dimensions.Height, &GlobalBackBuffer, X, Y, Width, Height);

            EndPaint(Window, &Paint); // End painting
        } break;

        case WM_SYSKEYUP:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
        {
            // WPARAM is the virtual key code
            uint32_t VKCode = WParam;
            bool wasDown = ((LParam & (1 << 30)) != 0); // Check previous state with bit 30
            bool isDown = ((LParam & (1 << 31)) == 0); // 0 with WM_KEYDOWN and 1 with WM_KEYUP
            
            if (isDown != wasDown) { break; } 

            switch (VKCode) {
                case 'W': {} break;
                case 'A': {} break;
                case 'S': {} break;
                case 'D': {} break;
                case 'Q': {} break;
                case 'E': {} break;
                case  VK_DOWN: {} break;
                case  VK_UP: {} break;
                case  VK_LEFT: {} break;
                case  VK_RIGHT: {} break;
                case  VK_ESCAPE: {} break;
                case  VK_SPACE: {} break;
                default: {} break;
            }
        } break;

        default: // Handle all other message types
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }

    return Result;
}

// WinMain is the primary entry point for Windows Programs
int CALLBACK
WinMain (HINSTANCE Instance,
         HINSTANCE PrevInstance,
         LPSTR CmdLine,
         int ShowCmd)
{
    Win32LoadXInput();
    // NOTE from https://youtu.be/4ROiWonnWGk?list=TLPQMjIwNTIwMjM3jPmfw1hgbA&t=750: if no performance concern, no initialization code
    WNDCLASSA WindowClass = {};

    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

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

                // TODO: Should we poll more frequently?
                for(DWORD controller = 0; controller < XUSER_MAX_COUNT; ++controller) {
                    XINPUT_STATE ControllerState;
                    auto connection = XInputGetState(controller, &ControllerState);
                    if(XInputGetState(controller, &ControllerState) == ERROR_SUCCESS) { 
                        // NOTE: Controller is plugged in.
                        // TODO: Check polling rate.
                         XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad; // XINPUT_GAMEPAD maps to an xbox controller

                         bool dPadUp = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                         bool dPadDown = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                         bool dPadLeft = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                         bool dPadRight = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                         bool ButtonStart = (Pad->wButtons & XINPUT_GAMEPAD_START);
                         bool ButtonBack = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
                         bool ButtonA = (Pad->wButtons & XINPUT_GAMEPAD_A);
                         bool ButtonB = (Pad->wButtons & XINPUT_GAMEPAD_B);
                         bool ButtonX = (Pad->wButtons & XINPUT_GAMEPAD_X);
                         bool ButtonY = (Pad->wButtons & XINPUT_GAMEPAD_Y);
                         bool ButtonLShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                         bool ButtonRShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);

                         int16_t StickX = Pad->sThumbLX;
                         int16_t StickY = Pad->sThumbLY;
                        
                         if (ButtonY) { // RESET 
                             GlobalRed = 0;
                             GlobalBlue = 0;
                             GlobalGreen = 0;
                         }
                         if (ButtonB && ButtonRShoulder) { GlobalRed   += COLOR_INCREMENT; }
                         if (ButtonB && ButtonLShoulder) { GlobalRed   -= COLOR_INCREMENT; }

                         if (dPadUp)    { ++YOffset; }
                         if (dPadDown)  { --YOffset; }
                         if (dPadRight) { ++XOffset; }
                         if (dPadLeft)  { --XOffset; }
                    } else { // NOTE: Controller is unplugged.
                        OutputDebugStringA("No controller found\n");
                    }

                }

                XINPUT_VIBRATION Vibe;
                Vibe.wLeftMotorSpeed = 600000;
                Vibe.wRightMotorSpeed = 600000;
                XInputSetState(0, &Vibe);
                {
                  RenderWeirdGradient(&GlobalBackBuffer, XOffset, YOffset);
                  HDC DeviceContext = GetDC(WindowHandle);
                  win32_window_dimension Dimensions = Win32GetWindowDimension(WindowHandle);
                  Win32DisplayBufferInWindow(DeviceContext, Dimensions.Width, Dimensions.Height, &GlobalBackBuffer, 0, 0, Dimensions.Width, Dimensions.Height);
                }

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

