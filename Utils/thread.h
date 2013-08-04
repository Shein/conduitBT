/**********************************************************************\
 Library     :  Utils
 Filename    :  thread.h
 Purpose     :  Class THREAD
 Platform    :  Windows.
\**********************************************************************/

#pragma once
#pragma managed(push, off)

#include "def.h"
#include "mutex.h"


class Thread_os
{
  public:
    static bool Init();
    static bool End();
    static unsigned GetCurThreadId();

  public:
    bool Construct (cchar *name, int priority, int stacksize);
    bool Destruct() { return true; }

    bool Execute();
    void Terminate();
    void Sleep (unsigned millisec);
    void Wait (unsigned handle);

  protected:
    static int aPriorities [];

    static unsigned __stdcall staticThreadFunc (void *param);

    unsigned ThreadId;
    HANDLE   hThread;
};


/*
 *********************************************************************
 Abstract Thread class derived from OS-dependent THREAD_OS class.
 *********************************************************************
*/
class Thread : public Thread_os, public Mutex
{
  friend class Thread_os;

  public:
    // Thread priorities 0..31
    enum {
        PRIORITY_IDLE           =  0,
        PRIORITY_LOW            =  1,
        PRIORITY_BELOWNORMAL    =  7,
        PRIORITY_NORMAL         = 15,
        PRIORITY_ABOVENORMAL    = 19,
        PRIORITY_HIGH           = 25,
        PRIORITY_CRITICAL       = 31
    };

    // Thread stack size
    enum {
        STACK_SIZE_DEFAULT = 20000
    };

    static bool Init();
    static bool End();

    static unsigned GetCurThreadId() {  return Thread_os::GetCurThreadId(); }


  public:
    Thread (cchar *name = 0, int priority = PRIORITY_NORMAL, int stacksize = STACK_SIZE_DEFAULT) :
        Name(name), Priority(priority), StackSize(stacksize)
    {
    }

    ~Thread ()
    {
        Destruct ();
    }

    bool Construct ();
    bool Destruct ();

    bool Execute()                  { return Thread_os::Execute(); }
    void Terminate()                { Thread_os::Terminate(); }
    bool IsRunning()                { return bRunning; }
    int  GetPriority()              { return Priority; }
    void Sleep (unsigned millisec)  { Thread_os::Sleep(millisec); }
    unsigned GetThreadId()          { return Thread_os::ThreadId; }
    void Wait (unsigned handle)     { return Thread_os::Wait(handle); }
    void WaitEnding();

    virtual void Run() = 0;

  protected:
    /* This func should be called from the corresponding THREAD_OS function */
    void ThreadFunc();

  public:
    cchar	* Name;

  protected:
    int       Priority;
    int       StackSize;
    bool      bRunning;
    Mutex     ExecMutex;
};



//static
inline bool Thread_os::Init()
{
    return true;
}

//static
inline bool Thread_os::End()
{
    return true;
}

//static
inline unsigned Thread_os::GetCurThreadId()
{
    return ::GetCurrentThreadId();  // Win32 API
}


inline void Thread_os::Sleep (unsigned millisec)
{
    ::Sleep (millisec);
}


inline void Thread_os::Wait (unsigned handle)
{
    WaitForSingleObject (HANDLE(handle), INFINITE);
}


inline bool Thread::Construct ()
{
    bRunning = false;
    return Thread_os::Construct (Name, Priority, StackSize);
}


inline bool Thread::Destruct ()
{
    if (bRunning)
        Terminate ();
    return Thread_os::Destruct ();
}



#pragma managed(pop)
