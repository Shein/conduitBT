/*******************************************************************\
 Library     :  Utils
 Filename    :  mutex.h
 Purpose     :  Task synchronization objects: Mutex, Event
\*******************************************************************/

#pragma once
#pragma managed(push, off)


enum { 
	WAIT_FOREVER = INFINITE		// Our platform independent INFINITE redefinition
};

typedef void* MxHandle;


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

    MxHandle GetWaitHandle() { return MxHandle(hWinEvent); }

  protected:
    HANDLE   hWinEvent;
};


class Semaph_os
{
  public:
    Semaph_os()  { hWinSemaph = CreateSemaphore (NULL, 0/*InitCount*/, LONG_MAX, NULL); }
    ~Semaph_os() { CloseHandle (hWinSemaph); }

    MxHandle GetWaitHandle() { return MxHandle(hWinSemaph); }

  protected:
    HANDLE  hWinSemaph;
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
    void Wait (unsigned timeout = WAIT_FOREVER);
	void Reset();
    MxHandle GetWaitHandle() { return Event_os::GetWaitHandle(); }
};


class Semaph : protected Semaph_os
{
  public:
    void Signal ();
    bool Take (unsigned timeout = WAIT_FOREVER);
    void Reset () { while (Take(0)); }
    MxHandle GetWaitHandle() { return Semaph_os::GetWaitHandle(); }
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


inline void Semaph::Signal ()
{
    ReleaseSemaphore (hWinSemaph, 1, NULL);     
}


inline bool Semaph::Take (unsigned timeout)
{
    int ret = WaitForSingleObject (hWinSemaph, timeout);
	return (ret == WAIT_OBJECT_0);
}


#pragma managed(pop)
