/*******************************************************************\
 Library     :  Utils
 Filename    :  mutex.h
 Purpose     :  Task synchronization objects: Mutex, Event
\*******************************************************************/

#pragma once
#pragma managed(push, off)


class Mutex_os
{
  public:
    Mutex_os()   {  InitializeCriticalSection (&CritSection);  }
    ~Mutex_os()  {  DeleteCriticalSection (&CritSection);      }
      
  protected:
    CRITICAL_SECTION  CritSection;
};


class Event_os
{
  public:
    Event_os()   { hWinEvent = CreateEvent (0, FALSE, FALSE, 0); }
    ~Event_os()  { CloseHandle (hWinEvent);  }

    int GetWaitHandle() { return int(hWinEvent); }

  protected:
    HANDLE   hWinEvent;
};


class Mutex : protected Mutex_os
{
  public:
    void Lock ();
    void Unlock ();
};


class Event : protected Event_os
{
  public:
    void Signal();
    void Wait (unsigned timeout = INFINITE);
	void Reset();
    int  GetWaitHandle() { return Event_os::GetWaitHandle(); }
};



class MUTEXLOCK
{
  public:

    MUTEXLOCK (Mutex* pMutex) : pMutex(pMutex)  { pMutex->Lock();   }
    ~MUTEXLOCK ()                               { pMutex->Unlock(); }

  protected:
    Mutex* pMutex;
};


// *******************************************************
// Macro for automatic locking/unlocking a function body.
// Simply place it at the function beginning.
#define MUTEXLOCK(Mutex)  MUTEXLOCK  _alobj(&Mutex)



inline void Mutex::Lock()
{
   EnterCriticalSection (&CritSection);
}


inline void Mutex::Unlock() 
{
   LeaveCriticalSection (&CritSection);
}

inline void Event::Wait(unsigned timeout) 
{
    WaitForSingleObject (hWinEvent, timeout);
}


inline void Event::Signal() 
{
    SetEvent (hWinEvent);
}


inline void Event::Reset() 
{
    ResetEvent (hWinEvent);
}

#pragma managed(pop)
