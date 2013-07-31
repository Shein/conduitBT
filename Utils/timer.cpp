/**********************************************************************\
 Filename    :  timer.cpp
 Purpose     :  Class TIMER
 Platform    :  Windows.
\**********************************************************************/

#include "def.h"
#include "deblog.h"
#include "timer.h"


#define WM_SET_TIMER   (WM_USER + 1)
#define WM_KILL_TIMER  (WM_USER + 2)


HWND    Timer_os::hTimerWnd;
HANDLE  Timer_os::hTimerThread;
HANDLE  Timer_os::hWinEvent;


void Timer::TimerProc ()
{
    if (bStop)
        Stop();

    CallBack(Context);
}


//static 
bool Timer_os::Init ()
{
    DWORD ThreadId;

    hWinEvent = CreateEvent (0, FALSE, FALSE, 0);
    hTimerThread  = (HANDLE) CreateThread (0, 0, LPTHREAD_START_ROUTINE(TimerThread), 0, 0, &ThreadId);
    WaitForSingleObject (hWinEvent, INFINITE);
    CloseHandle (hWinEvent);
    hWinEvent = 0;
    return true;
}

//static 
bool Timer_os::End ()
{
    if (hTimerWnd)
        SendMessage (hTimerWnd, WM_QUIT, 0, 0);

    return true;
}

extern HMODULE ThisDllHModule;

//static
DWORD CALLBACK Timer_os::TimerThread (void * param)
{
   LogMsg ("Timer_os::TimerThread started");

   WNDCLASS   wndclass;
   MSG        msg;

   wndclass.hIcon          = 0;
   wndclass.hbrBackground  = 0;
   wndclass.style          = 0;
   wndclass.lpfnWndProc    = (WNDPROC) TimerProc;
   wndclass.hCursor        = 0;
   wndclass.lpszMenuName   = 0;
   wndclass.hInstance      = GetModuleHandle(0);
   wndclass.lpszClassName  = "Timer_os";
   wndclass.cbClsExtra     = 0;
   wndclass.cbWndExtra     = 0;
   RegisterClass (&wndclass);

   hTimerWnd = CreateWindow
                (
                    "Timer_os",			// class name
                    "",                 // window name
                    WS_OVERLAPPEDWINDOW,
                    CW_USEDEFAULT, CW_USEDEFAULT,
                    1,1, 0, 0, 
                    wndclass.hInstance, 
                    0
                );
	
    SetEvent (hWinEvent);

    while (GetMessage(&msg,0,0,0))
    {
        // For more speed don't use TranslateMessage() & DispatchMessage() 
        // and call directly the window procedure.
        // This thread expects only messages for hTimerWnd.

        TimerProc (hTimerWnd, msg.message, msg.wParam, msg.lParam);
    }

    // K.S. 
    // The following code is unreachable: the GetMessage() doesn't wake up after sending
    // to it the WM_QUIT message over an unknown cause.
    UnregisterClass ("Ttimer_os", wndclass.hInstance);
    hTimerWnd = 0;
    return 0;
}


//static
LRESULT CALLBACK Timer_os::TimerProc (HWND hWnd, UINT msg, WPARAM pTimer, LPARAM dwTime)
{
    switch (msg)
    {
        case WM_SET_TIMER:
            // msg.wParam  - timer identifier 
            // msg.lParam  - interval in milliseconds
            ::SetTimer (hWnd, pTimer, (UINT)dwTime, 0);
            break;

        case WM_KILL_TIMER:
            ::KillTimer (hWnd, pTimer);
            break;

        case WM_TIMER:
            ((Timer*)pTimer)->TimerProc ();
            break;
    }

    //return 0;
    return DefWindowProc(hWnd, msg, (UINT32) pTimer, dwTime);
}


void Timer_os::Start (unsigned timeout)
{
    SendMessage (hTimerWnd, WM_SET_TIMER, (WPARAM)this, timeout);
}


void Timer_os::Stop ()
{
    SendMessage (hTimerWnd, WM_KILL_TIMER, (WPARAM)this, 0);
}


//static 
bool Timer::Init ()
{    
	return Timer_os::Init ();
}

//static 
bool Timer::End ()
{
    return Timer_os::End ();
}


bool Timer::Construct (TIMERCB callback, void* context)
{
    this->CallBack = callback;
    this->Context  = context;
    bStarted = false;
    Timer_os::Construct();
    return true;
}
