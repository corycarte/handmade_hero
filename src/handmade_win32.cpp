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
global_variable HBITMAP BitmapHandle;
global_variable HDC BitmapDeviceContext;

// NOTE: Required to make custom renderer
internal void Win32ResizeDIBSection(int, int);
internal void Win32UpdateWindow(HDC, int, int, int, int);


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

            Win32UpdateWindow(DeviceContext, X, Y, Width, Height);
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
            MSG Message;

            Running = true; // Set running true before while
            while (Running)
            {
                BOOL MessageResult = GetMessageA(&Message, 0, 0, 0);

                if (MessageResult > 0)
                {
                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }
                else {
                    // TODO:
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

internal void Win32ResizeDIBSection(int Width, int Height)
{
    // TODO: Make bulletproof
    
    // TODO: Free DIBSection
    if (BitmapHandle)
    {
        DeleteObject(BitmapHandle);
    }

    if (BitmapDeviceContext)
    {
        BitmapDeviceContext = CreateCompatibleDC(0);
    }


    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = Width;
    BitmapInfo.bmiHeader.biHeight = Height;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;
    // BitmapInfo.bmiHeader.biSizeImage = 0; // NOTE: Global declaration removes requirement to init
    // BitmapInfo.bmiHeader.biXPelsPerMeter = 0;
    // BitmapInfo.bmiHeader.biYPelsPerMeter = 0;
    // BitmapInfo.bmiHeader.biClrUsed = 0;
    // BitmapInfo.bmiHeader.biClrImportant = 0;
    
    
    BitmapHandle = CreateDIBSection(
            BitmapDeviceContext,
            &BitmapInfo,
            DIB_RGB_COLORS,
            &BitmapMemory,
            0,
            0);

}

internal void Win32UpdateWindow(HDC DeviceContext, int X, int Y, int Width, int Height)
{
   StretchDIBits(DeviceContext, 
                X, Y, Width, Height, // Set Destination rect
                X, Y, Width, Height, // Source Rectangle
                BitmapMemory,
                &BitmapInfo,
                DIB_RGB_COLORS, // NOTE: https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-stretchdibits#:~:text=Specifies%20whether%20the,literal%20RGB%20values.
                SRCCOPY); // NOTE: https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-bitblt#:~:text=The%20following%20list%20shows%20some%20common%20raster%20operation%20codes.
}
