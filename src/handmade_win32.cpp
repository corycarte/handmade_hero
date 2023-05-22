#include <windows.h>

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
        } break;

        case WM_DESTROY: // Window is destroyed
        {
            OutputDebugStringA("WM_DESTROY\n");
        } break;
        
        case WM_CLOSE: // Window is closed
        {
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

            // Epilepsy window operations.
            static DWORD Operation = WHITENESS; // Lexical scoping to prevent calling outside context. Useful for debugging.
            PatBlt(DeviceContext, X, Y, Width, Height, Operation);
            Operation = Operation == WHITENESS ? BLACKNESS : WHITENESS;
            
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

            for (;;)// Loop forever unless break is called.
            {
                BOOL MessageResult = GetMessageA(&Message, 0, 0, 0);

                if (MessageResult > 0)
                {
                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }
                else
                {
                    break;
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
