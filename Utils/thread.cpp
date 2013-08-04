/**********************************************************************\
 Library     :  Utils
 Filename    :  thread.cpp
 Purpose     :  Class THREAD
 Platform    :  Windows.
\**********************************************************************/

#pragma managed(push, off)

#include "def.h"
#include "deblog.h"
#include "thread.h"


//static
int Thread_os::aPriorities [Thread::PRIORITY_CRITICAL + 1] =
{
    /* 00 */ THREAD_PRIORITY_IDLE,

    /* 01 */ THREAD_PRIORITY_LOWEST,
    /* 02 */ THREAD_PRIORITY_LOWEST,
    /* 03 */ THREAD_PRIORITY_LOWEST,
    /* 04 */ THREAD_PRIORITY_LOWEST,
    /* 05 */ THREAD_PRIORITY_LOWEST,
    /* 06 */ THREAD_PRIORITY_LOWEST,

    /* 07 */ THREAD_PRIORITY_BELOW_NORMAL,
    /* 08 */ THREAD_PRIORITY_BELOW_NORMAL,
    /* 09 */ THREAD_PRIORITY_BELOW_NORMAL,
    /* 10 */ THREAD_PRIORITY_BELOW_NORMAL,
    /* 11 */ THREAD_PRIORITY_BELOW_NORMAL,
    /* 12 */ THREAD_PRIORITY_BELOW_NORMAL,

    /* 13 */ THREAD_PRIORITY_NORMAL,
    /* 14 */ THREAD_PRIORITY_NORMAL,
    /* 15 */ THREAD_PRIORITY_NORMAL,
    /* 16 */ THREAD_PRIORITY_NORMAL,
    /* 17 */ THREAD_PRIORITY_NORMAL,
    /* 18 */ THREAD_PRIORITY_NORMAL,

    /* 19 */ THREAD_PRIORITY_ABOVE_NORMAL,
    /* 20 */ THREAD_PRIORITY_ABOVE_NORMAL,
    /* 21 */ THREAD_PRIORITY_ABOVE_NORMAL,
    /* 22 */ THREAD_PRIORITY_ABOVE_NORMAL,
    /* 23 */ THREAD_PRIORITY_ABOVE_NORMAL,
    /* 24 */ THREAD_PRIORITY_ABOVE_NORMAL,

    /* 25 */ THREAD_PRIORITY_HIGHEST,
    /* 26 */ THREAD_PRIORITY_HIGHEST,
    /* 27 */ THREAD_PRIORITY_HIGHEST,
    /* 28 */ THREAD_PRIORITY_HIGHEST,
    /* 29 */ THREAD_PRIORITY_HIGHEST,
    /* 30 */ THREAD_PRIORITY_HIGHEST,

    /* 31 */ THREAD_PRIORITY_TIME_CRITICAL
};


//static 
unsigned __stdcall Thread_os::staticThreadFunc (void *param)
{
    ((Thread*)param)->ThreadFunc ();
    return 0;
}


bool Thread_os::Construct (cchar *name, int priority, int stacksize)
{
    ThreadId = 0;

    ASSERT_f (priority <= Thread::PRIORITY_CRITICAL);

    hThread = CreateThread 
                (
                    0, 
                    0, 
                    (LPTHREAD_START_ROUTINE) Thread_os::staticThreadFunc,
                    (Thread*)this, 
                    CREATE_SUSPENDED,
                    (unsigned long*)&ThreadId
                );
    
    VERIFY_f (hThread);
    SetThreadPriority (hThread, aPriorities[priority]); // Convert to thread_os (Windows) priority number
    return true;
}



bool Thread_os::Execute ()
{
    ResumeThread (hThread);
    return true;
}


void Thread_os::Terminate ()
{
    if (hThread) {
        TerminateThread (hThread, 0);
        hThread = 0;
    }
}


void Thread::WaitEnding()
{
    DWORD dwWaitRet;

    while (true)
    {
        dwWaitRet = MsgWaitForMultipleObjects (1, &hThread, FALSE, INFINITE, QS_SENDMESSAGE);

        if (dwWaitRet >= WAIT_OBJECT_0 + 1)
        {   // message available in the queue
            MSG Msg;
            if (GetMessage (&Msg,0,0,0))
            {
               TranslateMessage (&Msg);
               DispatchMessage  (&Msg);
            }
        }
        else return;
    }
}


//static
bool Thread::Init ()
{
    return Thread_os::Init();
}


//static
bool Thread::End ()
{
    return Thread_os::End();
}


void Thread::ThreadFunc ()
{
    ExecMutex.Lock ();
    bRunning = true;
    Run();
    bRunning = false;
    ExecMutex.Unlock ();
}


#pragma managed(pop)

