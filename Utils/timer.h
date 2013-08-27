/**********************************************************************\
 Library     :  Utils
 Filename    :  timer.h
 Purpose     :  Class TIMER
 Platform    :  Windows.
\**********************************************************************/

#pragma once

#include "def.h"


typedef void(*TIMERCB) (void* context);


class Timer_os
{
  public:

    static bool Init ();
    static bool End ();

    bool Construct () { return true; };
    bool Destruct ()  { return true; };

    void Start (unsigned timeout);
    void Stop();

  protected:
    static HWND     hTimerWnd;
    static HANDLE   hTimerThread;
    static HANDLE   hWinEvent;   // Windows event: signals when Timer_os::TimerThread() has started and hTimerWnd is created


    static LRESULT CALLBACK  TimerProc	  (HWND hWnd, UINT msg, WPARAM pTimer, LPARAM dwTime);
    static DWORD   CALLBACK  TimerThread  (void* param);
};


class Timer : protected Timer_os
{
  friend class Timer_os;

  // General Time functions
  public:
    static unsigned GetCurSec   ();
    static unsigned GetCurMilli ();

  public:
    static bool Init ();
    static bool End ();
	 
    Timer () {};
    Timer (TIMERCB callBack, void* context = 0) { Construct(callBack, context); }
    ~Timer ();

    bool Construct (TIMERCB callback, void* context = 0);
    void Destruct ();

	bool IsConstructed()					{ return CallBack!= 0;	  }
	void UpdateContext(void* context)		{ Context = context; }

    void Start (unsigned timeout, bool single = false);
    void Stop  ();

  protected:
    void TimerProc ();

    unsigned   Timeout;
    bool       bStarted, bStop;
    TIMERCB    CallBack;
    void     * Context;
};




inline Timer::~Timer ()
{
    Destruct ();
}


inline void Timer::Destruct ()
{
    if (bStarted)
        Stop();

    Timer_os::Destruct ();
}


inline void Timer::Start (unsigned timeout, bool single)
{
    Timeout  = timeout;
    bStop    = single;
    bStarted = true;
    Timer_os::Start (timeout);
}


inline void Timer::Stop ()
{
    bStarted = false;
    Timer_os::Stop ();
}

//static 
inline unsigned Timer::GetCurSec ()
{
    return GetTickCount() / 1000;
}


//static 
inline unsigned Timer::GetCurMilli ()
{
    return GetTickCount();
}